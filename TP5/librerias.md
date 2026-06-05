# Estudio de headers y APIs usadas en `signar_cdd.c`

Análisis de cada `struct`, función y macro del kernel que el driver invoca pero **no define**. Para cada símbolo: a qué header pertenece, qué hace, su firma real en el kernel, y cómo está implementado por dentro. Las citas son del kernel **v6.6 mainline** (la base del Raspberry Pi OS Bookworm de 64-bit que correrá la Pi 5).

> **Sobre la Raspberry Pi 5 — leer primero.** El SoC de la Pi 5 (BCM2712) **no maneja directamente los GPIO del header de 40 pines**. Los pines de usuario cuelgan de un chip aparte llamado **RP1**, conectado por PCIe, que actúa como southbridge. Esto cambia el bus driver de abajo (`pinctrl-rp1` en lugar de `pinctrl-bcm2835`), pero **no cambia los headers que incluimos ni los nombres de las funciones**: la API legacy `<linux/gpio.h>` (`gpio_request`, `gpio_get_value`, etc.) sigue funcionando porque internamente delega en gpiolib, y gpiolib en la Pi 5 está respaldado por el driver del RP1. La numeración BCM (GPIO 17, GPIO 27) **se mantiene** porque desde julio 2024 los kernels de Raspberry Pi reordenan los `gpiochip*` para que `gpiochip0` siga siendo el banco de pines de usuario. **Caveat:** `<linux/gpio.h>` está marcado oficialmente como **LEGACY** en el kernel — la cita textual del header v6.6 es _"This is the LEGACY GPIO bulk include file... should not be included in new code"_. Funciona y la cátedra lo acepta, pero conviene saberlo y mencionarlo en el informe.

---

## 1. `<linux/timer.h>` — Timers del kernel

### `struct timer_list`

Estructura que representa un temporizador del kernel. Es lo que se programa con `mod_timer()` para que el kernel invoque un callback en el futuro.

```c
struct timer_list {
    /*
     * All fields that change during normal runtime grouped to the
     * same cacheline
     */
    struct hlist_node   entry;
    unsigned long       expires;
    void                (*function)(struct timer_list *);
    u32                 flags;

#ifdef CONFIG_LOCKDEP
    struct lockdep_map  lockdep_map;
#endif
};
```

- `entry`: nodo de la lista enlazada interna donde el subsistema de timers lo encola.
- `expires`: momento en jiffies en el que debe dispararse.
- `function`: puntero al callback que el kernel ejecutará al expirar. Por contrato recibe un puntero al propio `timer_list` (de ahí el parámetro `struct timer_list *t` en nuestro `signal_timer_callback`).
- `flags`: combinación de banderas (`TIMER_DEFERRABLE`, `TIMER_PINNED`, `TIMER_IRQSAFE`). Nosotros pasamos `0` ⇒ timer ordinario, no diferible, sin pinning a CPU.

Para acceder a nuestra `struct signal_data` desde el callback usamos el pattern estándar: `container_of(t, struct signal_data, timer_muestreo)` — aunque en nuestro código lo evitamos porque `sdata` es global, así que dereferenciamos directamente.

### `timer_setup(timer, callback, flags)` — macro

Inicializa un `struct timer_list` y le asigna su callback y flags. **No** lo arranca; solo lo prepara para que después `mod_timer()` lo encole.

```c
#define timer_setup(timer, callback, flags)         \
    __init_timer((timer), (callback), (flags))

#define __init_timer(_timer, _fn, _flags)               \
    do {                                \
        static struct lock_class_key __key;         \
        init_timer_key((_timer), (_fn), (_flags), #_timer, &__key); \
    } while (0)
```

Reemplazó al antiguo `setup_timer()` / `init_timer()` + asignar campos a mano. La diferencia clave es que la firma del callback cambió: ahora recibe `struct timer_list *` en vez de `unsigned long data` (cambio del kernel 4.15, año 2018).

Llamada en nuestro código (línea 245):
```c
timer_setup(&sdata->timer_muestreo, signal_timer_callback, 0);
```

### `mod_timer(timer, expires)` — función

```c
extern int mod_timer(struct timer_list *timer, unsigned long expires);
```

Programa el timer para dispararse en el jiffy absoluto `expires`. Si el timer ya estaba encolado, lo reprograma (de ahí el "mod" de modify). Es la forma idiomática de tanto **arrancar por primera vez** un timer como **re-armarlo** desde dentro del propio callback, que es exactamente lo que hacemos:

```c
mod_timer(&sdata->timer_muestreo, jiffies + HZ);   // en init: arranca
mod_timer(&sdata->timer_muestreo, jiffies + HZ);   // en callback: se re-arma a sí mismo
```

Retorna 0 si el timer no estaba activo, 1 si sí lo estaba. Nosotros ignoramos el retorno porque ambos casos son válidos.

### `del_timer_sync(timer)` (alias actual: `timer_delete_sync`)

```c
extern int timer_delete_sync(struct timer_list *timer);
```

Desencola el timer **y espera** a que termine cualquier ejecución en curso del callback en otra CPU antes de retornar. Imprescindible llamarlo en `signal_exit()` antes de liberar `sdata`, porque si liberamos la memoria mientras el callback está corriendo en otro core ⇒ use-after-free del kernel. En kernels recientes el nombre canónico es `timer_delete_sync` pero `del_timer_sync` sigue siendo un alias válido.

---

## 2. `<linux/spinlock.h>` y `<linux/spinlock_types.h>` — Sincronización

### `spinlock_t`

```c
typedef struct spinlock {
    union {
        struct raw_spinlock rlock;
#ifdef CONFIG_DEBUG_LOCK_ALLOC
        struct {
            u8 __padding[LOCK_PADSIZE];
            struct lockdep_map dep_map;
        };
#endif
    };
} spinlock_t;
```

Un *busy-wait lock*: el thread que no logra adquirirlo gira en un loop hasta que el dueño libera. Es lo correcto para regiones cortas; un mutex (que duerme) **no se puede usar dentro del callback del timer** porque el callback corre en contexto de softirq y no puede dormir. Esa es la razón por la que elegimos spinlock y no `struct mutex`.

### `spin_lock_init(lock)`

```c
# define spin_lock_init(_lock)          \
do {                        \
    spinlock_check(_lock);          \
    *(_lock) = __SPIN_LOCK_UNLOCKED(_lock); \
} while (0)
```

Inicializa el spinlock en estado *unlocked*. Lo invocamos en `signal_init` después de `kzalloc` (línea 194): la memoria viene a cero pero el lockdep igual necesita esta inicialización formal.

### `spin_lock_irqsave(lock, flags)` y `spin_unlock_irqrestore(lock, flags)`

```c
#define spin_lock_irqsave(lock, flags)              \
do {                                \
    raw_spin_lock_irqsave(spinlock_check(lock), flags); \
} while (0)

#define raw_spin_lock_irqsave(lock, flags)          \
    do {                        \
        typecheck(unsigned long, flags);    \
        flags = _raw_spin_lock_irqsave(lock);   \
    } while (0)

static __always_inline void spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags)
{
    raw_spin_unlock_irqrestore(&lock->rlock, flags);
}
```

Hace tres cosas atómicamente: (1) guarda el estado actual de IRQs en `flags`, (2) deshabilita IRQs en esta CPU, (3) toma el spinlock. El `_irqrestore` hace lo inverso.

**¿Por qué la variante `_irqsave` y no `spin_lock()` a secas?** Porque nuestro callback corre en softirq y `read`/`write` corren en contexto de proceso. Si un proceso adquiere el lock con `spin_lock()` simple y llega una IRQ → softirq que intenta el mismo lock ⇒ deadlock. `spin_lock_irqsave` deshabilita IRQs locales mientras se tiene el lock, eliminando esa ventana.

El parámetro `flags` debe ser una variable local `unsigned long`, no un puntero — el macro la modifica por nombre. Por eso en cada función vemos `unsigned long flags;` antes del lock.

---

## 3. `<linux/cdev.h>` — Character device

### `struct cdev`

```c
struct cdev {
    struct kobject          kobj;
    struct module           *owner;
    const struct file_operations *ops;
    struct list_head        list;
    dev_t                   dev;
    unsigned int            count;
} __randomize_layout;
```

Es la representación interna del kernel de un character device. Conecta el par `<major, minor>` con la tabla `file_operations` que define qué hacer cuando alguien hace `open`/`read`/`write` sobre el archivo en `/dev/`.

- `kobj`: integración con sysfs (refcounting, jerarquía).
- `owner`: el módulo dueño — `THIS_MODULE`, sirve para que el kernel no permita `rmmod` mientras haya FDs abiertos.
- `ops`: nuestra tabla `fops` (línea 207 del driver).
- `dev`: el par `<major, minor>` que devolvió `alloc_chrdev_region`.
- `count`: cuántos minor numbers consecutivos cubre este cdev (en nuestro caso 1).

### Funciones que usamos

```c
void cdev_init(struct cdev *, const struct file_operations *);
int  cdev_add(struct cdev *, dev_t, unsigned);
void cdev_del(struct cdev *);
```

- `cdev_init(&signal_cdev, &fops)` — inicializa el cdev y le pega la tabla de operaciones. El campo `owner` queda en NULL y por eso la línea siguiente del driver hace `signal_cdev.owner = THIS_MODULE;`.
- `cdev_add(&signal_cdev, dev_num, NUM_DEVICES)` — **publica** el cdev en el kernel: a partir de este momento un `open()` sobre el archivo asociado a `dev_num` realmente entra a nuestro `signal_open`. Antes de esto la entrada `/dev/signal_cdd` aún no existe — la crea `device_create` (ver §6).
- `cdev_del(&signal_cdev)` — lo retira. Hay que llamarlo en `exit` antes de liberar el rango con `unregister_chrdev_region`.

---

## 4. `<linux/jiffies.h>` y `<asm/param.h>` — Tiempo del kernel

### `jiffies` y `HZ`

```c
extern u64 __cacheline_aligned_in_smp jiffies_64;
extern unsigned long volatile __cacheline_aligned_in_smp __jiffy_arch_data jiffies;

#include <asm/param.h>          /* for HZ */
```

- `jiffies` es un contador global de "ticks" del kernel. Cada vez que dispara la IRQ del timer del sistema, este contador se incrementa.
- `HZ` es la frecuencia de tick, en hertz. En las builds de Raspberry Pi OS suele ser `HZ=250`, así que `1 segundo = 250 jiffies`.

La expresión `jiffies + HZ` calcula "el valor de jiffies dentro de exactamente 1 segundo", que es la base de nuestro período de muestreo. Si en el futuro queremos cambiar a 500 ms haríamos `jiffies + HZ/2`, y para milisegundos arbitrarios usaríamos `msecs_to_jiffies(N)`.

---

## 5. `<linux/gpio.h>` — Acceso a GPIO (API legacy)

> **Header marcado LEGACY.** Cita textual del v6.6: _"This is the LEGACY GPIO bulk include file, including legacy APIs. It is used for GPIO drivers still referencing the global GPIO numberspace, and should not be included in new code."_ Sigue funcionando en la Pi 5 vía gpiolib + `pinctrl-rp1`, pero la API "moderna" es la basada en descriptores (`<linux/gpio/consumer.h>`, funciones `gpiod_*`). Para este TP la legacy es la opción razonable: la cátedra trabaja con números BCM y `gpio_get_value` es directo.

### Funciones que usamos

```c
int  gpio_request(unsigned gpio, const char *label);
void gpio_free(unsigned gpio);
static inline int gpio_direction_input(unsigned gpio);
static inline int gpio_get_value(unsigned gpio)
{
    return gpiod_get_raw_value(gpio_to_desc(gpio));
}
```

- `gpio_request(17, "signal_gpio_0")` — pide al kernel el GPIO 17 en exclusiva. El segundo argumento es un label informativo que aparece en `/sys/kernel/debug/gpio` para diagnóstico. Falla si otro driver ya lo tomó.
- `gpio_direction_input(17)` — configura el pin como entrada (alta impedancia, ideal para leer una señal externa).
- `gpio_get_value(17)` — lee el nivel lógico. Es un `static inline` que termina llamando a `gpiod_get_raw_value(gpio_to_desc(17))`. En la Pi 5 ese descriptor lo maneja `pinctrl-rp1`, que internamente lee un registro del chip RP1 vía PCIe. Devuelve `0` o `1`.
- `gpio_free(17)` — libera el pin. Obligatorio en `exit` y en los `goto fail_*`.

### Numeración BCM en la Pi 5

Usamos `#define GPIO_SIGNAL_0 17` y `#define GPIO_SIGNAL_1 27`. Esos números **son los BCM**, que coinciden con los rótulos clásicos del header de 40 pines (GPIO 17 = pin físico 11, GPIO 27 = pin físico 13). Esto sigue siendo válido en Pi 5 porque el kernel de Raspberry Pi reordena los `gpiochip*` para que `gpiochip0` siga siendo el banco de pines del header, y por tanto el offset global "17" lo resuelve a la línea GPIO17 del RP1. No hace falta cambiar nada en el código fuente al migrar de Pi 4 a Pi 5.

---

## 6. Otras dependencias notables (ya cubiertas pero conviene mencionar)

Para cerrar el mapa de dependencias del driver, estas vienen de headers ya incluidos y son referenciadas pero no estaban en tu lista original:

| Símbolo | Header | Rol |
|---|---|---|
| `dev_t` | `<linux/types.h>` (vía `<linux/fs.h>`) | Tipo opaco que codifica `<major:minor>` en 32 bits. |
| `MAJOR(dev)` / `MINOR(dev)` | `<linux/kdev_t.h>` | Macros que extraen el major/minor de un `dev_t`. |
| `alloc_chrdev_region` | `<linux/fs.h>` | Reserva un rango de `<major, minor>` dinámicamente. |
| `class_create` / `device_create` | `<linux/device.h>` | Lo que hace aparecer la entrada en `/dev/signal_cdd` automáticamente vía udev. Sin esto, habría que `mknod` a mano. |
| `kzalloc` / `kfree` | `<linux/slab.h>` | Asignador del kernel (zero-initialized). |
| `copy_to_user` / `copy_from_user` | `<linux/uaccess.h>` | Cruzan la frontera kernel↔usuario validando que el puntero sea legal. |
| `printk` / `KERN_INFO` | `<linux/kernel.h>` | Log al kernel ring buffer (`dmesg`). |

---

## Resumen del flujo de ejecución

1. **`insmod`** → `signal_init`: aloca `sdata`, init spinlock, reserva `<major,minor>`, registra `cdev`, crea clase y device (esto crea `/dev/signal_cdd`), pide GPIOs 17 y 27 como entrada, configura el timer y lo arranca con `mod_timer(... jiffies + HZ)`.
2. **Cada segundo** el kernel invoca `signal_timer_callback` en contexto softirq: bajo `spin_lock_irqsave` lee ambos GPIOs, guarda el de la señal activa en el buffer circular, y se re-arma con otro `mod_timer`.
3. **`open("/dev/signal_cdd")` desde Python** → `signal_open` (no-op informativo).
4. **`read()`** → `signal_read`: bajo spinlock copia el último valor de la señal activa, lo formatea y lo manda a userland con `copy_to_user`.
5. **`write("0")` o `write("1")`** → `signal_write`: bajo spinlock cambia `signal_activa` y resetea el buffer.
6. **`rmmod`** → `signal_exit`: `del_timer_sync` (espera a que termine cualquier callback), libera GPIOs, destruye device/class, `cdev_del`, libera el rango, `kfree(sdata)`.

Fuentes (todas verificadas contra el árbol v6.6 del kernel, que es la base de Raspberry Pi OS Bookworm 64-bit que usará la Pi 5):
- [include/linux/timer.h](https://raw.githubusercontent.com/torvalds/linux/v6.6/include/linux/timer.h)
- [include/linux/spinlock.h](https://raw.githubusercontent.com/torvalds/linux/v6.6/include/linux/spinlock.h)
- [include/linux/spinlock_types.h](https://raw.githubusercontent.com/torvalds/linux/v6.6/include/linux/spinlock_types.h)
- [include/linux/cdev.h](https://raw.githubusercontent.com/torvalds/linux/v6.6/include/linux/cdev.h)
- [include/linux/jiffies.h](https://raw.githubusercontent.com/torvalds/linux/v6.6/include/linux/jiffies.h)
- [include/linux/gpio.h](https://raw.githubusercontent.com/torvalds/linux/v6.6/include/linux/gpio.h)
- [Linux Drivers – Getting started with GPIO on Raspberry Pi 5 (Emlogic)](https://emlogic.no/2024/09/linux-drivers-getting-started-with-gpio-on-raspberry-pi-5/)
- [RPi5 and RP1 GPIO ALT function – Raspberry Pi Forums](https://forums.raspberrypi.com/viewtopic.php?t=373820)
