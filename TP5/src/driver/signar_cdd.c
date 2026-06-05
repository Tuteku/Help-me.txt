/**
 * sensor_drv.c - Character Device Driver para sensado de dos señales digitales
 * 
 * Este modulo implementa un CDD que lee dos señales externas provenientes
 * de un generador de señales, conectadas a pines GPIO de una Raspberry Pi.
 * El muestreo se realiza con un periodo de 1 segundo mediante un timer
 * del kernel.
 * 
 * Las señales son digitales: el GPIO lee HIGH (1) o LOW (0) segun el
 * nivel de tension en el pin (umbral ~1.8V para logica de 3.3V).
 * 
 * La aplicacion de usuario puede:
 *   - Leer el valor actual del canal seleccionado (read)
 *   - Seleccionar cual de las dos señales leer (write: '0' o '1')
 * 
 * Materia: Sistemas de Computacion - FCEFyN - UNC
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/slab.h>

#define DEVICE_NAME     "signal_cdd"
#define CLASS_NAME      "signal_class"
#define NUM_DEVICES     1

/* --- Pines GPIO ---
 * En la Raspberry Pi 5 los pines del header de 40 pines cuelgan del chip RP1
 * (gpiochip0, pinctrl-rp1), cuya base global NO es 0. La API legacy
 * gpio_request() usa el numero GLOBAL = base_del_chip + offset_BCM.
 * En este kernel la base es 569 (verificar con: sudo cat /sys/kernel/debug/gpio).
 * OJO: la base puede cambiar entre kernels/boots; si gpio_request vuelve a
 * fallar con -EPROBE_DEFER (-517), reverificarla. */
#define GPIO_RP1_BASE   569
#define GPIO_SIGNAL_0   (GPIO_RP1_BASE + 17)   /* BCM17 -> global 586 (canal 1) */
#define GPIO_SIGNAL_1   (GPIO_RP1_BASE + 27)   /* BCM27 -> global 596 (canal 2) */

/* --- Buffer circular para almacenar muestras --- */
#define BUFFER_SIZE     64

/* Estructura que almacena el estado del driver */
struct signal_data {
    int signal_activa;                  /* 0 o 1: cual señal se lee */
    int valores[2];                     /* ultimo valor leido de cada canal */
    struct timer_list timer_muestreo;   /* timer para muestreo periodico */
    int buffer[BUFFER_SIZE];            /* buffer circular de lecturas */
    int buf_head;                       /* indice de escritura */
    int buf_tail;                       /* indice de lectura */
    int buf_count;                      /* cantidad de datos disponibles */
    spinlock_t lock;                    /* proteccion de acceso concurrente */
};

static dev_t dev_num;                   /* par <major, minor> */
static struct cdev signal_cdev;         /* estructura cdev del kernel */
static struct class *signal_class;      /* clase del dispositivo */
static struct device *signal_device;    /* dispositivo */
static struct signal_data *sdata;       /* datos del driver */

/* ================================================================
 * Funcion de muestreo - se ejecuta cada 1 segundo via timer
 * ================================================================ */
static void signal_timer_callback(struct timer_list *t)
{
    unsigned long flags;

    spin_lock_irqsave(&sdata->lock, flags);

    /* Leer el nivel logico de ambos canales */
    sdata->valores[0] = gpio_get_value(GPIO_SIGNAL_0);
    sdata->valores[1] = gpio_get_value(GPIO_SIGNAL_1);

    /* Guardar el valor de la señal activa en el buffer circular */
    sdata->buffer[sdata->buf_head] = sdata->valores[sdata->signal_activa];
    sdata->buf_head = (sdata->buf_head + 1) % BUFFER_SIZE;

    if (sdata->buf_count < BUFFER_SIZE)
        sdata->buf_count++;
    else
        sdata->buf_tail = (sdata->buf_tail + 1) % BUFFER_SIZE;

    spin_unlock_irqrestore(&sdata->lock, flags);

    /* Reprogramar el timer para dentro de 1 segundo */
    mod_timer(&sdata->timer_muestreo, jiffies + HZ);
}

/* ================================================================
 * file_operations: las funciones que el kernel invoca
 * ================================================================ */

/* open() - se llama cuando la app hace open("/dev/signal_cdd") */
static int signal_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "signal_cdd: dispositivo abierto\n");
    return 0;
}

/* read() - la app pide leer el valor actual de la señal seleccionada.
 *          Devuelve "0\n" o "1\n" (nivel logico del GPIO) */
static ssize_t signal_read(struct file *filep, char __user *buffer,
                           size_t len, loff_t *offset)
{
    char msg[32];
    int valor;
    int msg_len;
    unsigned long flags;

    /* Evitar lecturas repetidas (EOF en la segunda llamada) */
    if (*offset > 0)
        return 0;

    spin_lock_irqsave(&sdata->lock, flags);
    valor = sdata->valores[sdata->signal_activa];
    spin_unlock_irqrestore(&sdata->lock, flags);

    /* Formatear el valor como string */
    msg_len = snprintf(msg, sizeof(msg), "%d\n", valor);

    if (msg_len > len)
        msg_len = len;

    /* Copiar datos del kernel al espacio de usuario */
    if (copy_to_user(buffer, msg, msg_len))
        return -EFAULT;

    *offset += msg_len;
    return msg_len;
}

/* write() - la app indica cual señal activar: '0' o '1'
 *           Ejemplo: echo "0" > /dev/signal_cdd */
static ssize_t signal_write(struct file *filep, const char __user *buffer,
                            size_t len, loff_t *offset)
{
    char comando;
    unsigned long flags;

    if (len < 1)
        return -EINVAL;

    /* Copiar UN byte del espacio de usuario al kernel */
    if (copy_from_user(&comando, buffer, 1))
        return -EFAULT;

    if (comando == '0' || comando == '1') {
        spin_lock_irqsave(&sdata->lock, flags);
        sdata->signal_activa = comando - '0';
        /* Resetear el buffer al cambiar de señal */
        sdata->buf_head = 0;
        sdata->buf_tail = 0;
        sdata->buf_count = 0;
        spin_unlock_irqrestore(&sdata->lock, flags);
        printk(KERN_INFO "sensor_cdd: señal activa = %d\n",
               sdata->signal_activa);
    } else {
        printk(KERN_WARNING "sensor_cdd: comando invalido '%c'\n", comando);
        return -EINVAL;
    }

    return len;
}

/* close() - se llama cuando la app cierra el file descriptor */
static int signal_close(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "signal_cdd: dispositivo cerrado\n");
    return 0;
}

/* Tabla de operaciones: conecta syscalls con nuestras funciones */
static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = signal_open,
    .read    = signal_read,
    .write   = signal_write,
    .release = signal_close,
};

/* ================================================================
 * Constructor del modulo (insmod)
 * ================================================================ */
static int __init signal_init(void)
{
    int ret;

    printk(KERN_INFO "signal_cdd: inicializando modulo\n");

    /* 1. Asignar memoria para la estructura de datos */
    sdata = kzalloc(sizeof(struct signal_data), GFP_KERNEL);
    if (!sdata)
        return -ENOMEM;

    sdata->signal_activa = 0;
    spin_lock_init(&sdata->lock);

    /* 2. Registrar rango <major, minor> dinamicamente */
    ret = alloc_chrdev_region(&dev_num, 0, NUM_DEVICES, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ALERT "signal_cdd: fallo alloc_chrdev_region\n");
        goto fail_alloc;
    }
    printk(KERN_INFO "signal_cdd: major=%d, minor=%d\n",
           MAJOR(dev_num), MINOR(dev_num));

    /* 3. Inicializar y agregar cdev (vincular fops) */
    cdev_init(&signal_cdev, &fops);
    signal_cdev.owner = THIS_MODULE;
    ret = cdev_add(&signal_cdev, dev_num, NUM_DEVICES);
    if (ret < 0) {
        printk(KERN_ALERT "signal_cdd: fallo cdev_add\n");
        goto fail_cdev;
    }

    /* 4. Crear clase y dispositivo (creacion automatica en /dev/) */
    signal_class = class_create(CLASS_NAME);
    if (IS_ERR(signal_class)) {
        ret = PTR_ERR(signal_class);
        goto fail_class;
    }

    signal_device = device_create(signal_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(signal_device)) {
        ret = PTR_ERR(signal_device);
        goto fail_device;
    }

    /* 5. Solicitar los pines GPIO */
    ret = gpio_request(GPIO_SIGNAL_0, "signal_gpio_0");
    if (ret) {
        printk(KERN_ALERT "sensor_cdd: no se pudo obtener GPIO %d\n",
               GPIO_SIGNAL_0);
        goto fail_gpio0;
    }
    gpio_direction_input(GPIO_SIGNAL_0);

    ret = gpio_request(GPIO_SIGNAL_1, "signal_gpio_1");
    if (ret) {
        printk(KERN_ALERT "sensor_cdd: no se pudo obtener GPIO %d\n",
               GPIO_SIGNAL_1);
        goto fail_gpio1;
    }
    gpio_direction_input(GPIO_SIGNAL_1);

    /* 6. Configurar el timer de muestreo (1 segundo) */
    timer_setup(&sdata->timer_muestreo, signal_timer_callback, 0);
    mod_timer(&sdata->timer_muestreo, jiffies + HZ);

    printk(KERN_INFO "sensor_cdd: modulo cargado correctamente\n");
    return 0;

/* Manejo de errores en orden inverso (goto cleanup) */
fail_gpio1:
    gpio_free(GPIO_SIGNAL_0);
fail_gpio0:
    device_destroy(signal_class, dev_num);
fail_device:
    class_destroy(signal_class);
fail_class:
    cdev_del(&signal_cdev);
fail_cdev:
    unregister_chrdev_region(dev_num, NUM_DEVICES);
fail_alloc:
    kfree(sdata);
    return ret;
}

/* ================================================================
 * Destructor del modulo (rmmod)
 * ================================================================ */
static void __exit signal_exit(void)
{
    /* Desmontar todo en orden inverso al montaje */
    del_timer_sync(&sdata->timer_muestreo);
    gpio_free(GPIO_SIGNAL_1);
    gpio_free(GPIO_SIGNAL_0);
    device_destroy(signal_class, dev_num);
    class_destroy(signal_class);
    cdev_del(&signal_cdev);
    unregister_chrdev_region(dev_num, NUM_DEVICES);
    kfree(sdata);
    printk(KERN_INFO "sensor_cdd: modulo removido\n");
}

module_init(signal_init);
module_exit(signal_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Help-me.txt");
MODULE_DESCRIPTION("CDD para sensado de dos senales digitales via GPIO");
MODULE_VERSION("1.0");
