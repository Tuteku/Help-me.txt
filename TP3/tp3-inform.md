# Informe TP3 — Sistemas de Computación

### Help-me.txt
Integrantes: 
- Mauro Cabero
- Nicolas de la Mata
- Mateo Quispe

Enlace al repositorio en github: https://github.com/Tuteku/Help-me.txt
---

- Crear imagen booteable: 

![imagen booteable](screens/1.png)

- Codificacion de instruccion: 

![](screens/2.png)

- Correr la imagen:

![](screens/3.png)

![](screens/4.png)


## 1. Firmware moderno: UEFI, CSME, MEBx y coreboot

### 1.1 ¿Qué es UEFI?

UEFI (*Unified Extensible Firmware Interface*) es la interfaz de firmware moderna que reemplazó al BIOS clásico que veníamos arrastrando desde los años 80. En la práctica, es el primer software que corre cuando uno enciende la computadora: se encarga de inicializar el hardware (procesador, memoria, controladoras de disco, USB, red), elegir un dispositivo de arranque y entregarle el control al sistema operativo.

A diferencia del BIOS tradicional, UEFI:

- Funciona en modo protegido o de 64 bits desde el arranque, no en modo real de 16 bits.
- Tiene un cargador de arranque por archivo (`.efi`) en una partición FAT especial llamada **EFI System Partition (ESP)**, en lugar de depender del MBR.
- Soporta discos GPT, lo que permite particiones mayores a 2 TB y más de 4 particiones primarias.
- Incluye **Secure Boot**, que verifica firmas digitales de los bootloaders antes de ejecutarlos.
- Expone una API estandarizada (servicios de arranque y servicios de runtime) que el sistema operativo puede usar.

### 1.2 ¿Cómo puedo usarlo?

UEFI define dos tipos de servicios accesibles mediante una tabla llamada `EFI_SYSTEM_TABLE`, que el firmware le pasa al programa al arrancar:

- **Boot Services:** disponibles solo hasta que el SO toma el control (`ExitBootServices`).
- **Runtime Services:** siguen disponibles incluso después de que el sistema operativo arrancó (por ejemplo, para leer/escribir variables NVRAM o reiniciar la máquina).

Para usarlo como desarrollador, lo habitual es escribir una **aplicación EFI** en C usando un toolkit como **GNU-EFI** o **EDK II**. Ejemplo codigo c:

```c
#include <efi.h>
#include <efilib.h>

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    Print(L"Hola desde UEFI!\n");
    return EFI_SUCCESS;
}
```

Implementación en Hardware Real:

- Formatear una memoria USB en FAT32 ya que el firmware UEFI solo puede leer sistemas de archivos FAT32.
- Crear la estructura de directorios /EFI/BOOT/.
- Renombrar el ejecutable a bootx64.efi y ponerlo en esa carpeta.
- Entrar a la BIOS/UEFI del equipo y seleccionar USB como primera opción de arranque.

### 1.3 Una función llamable bajo esa dinámica

Un ejemplo concreto: la función `OutputString` del protocolo `EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL`, que permite imprimir texto por consola sin tener todavía un sistema operativo:

```c
SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Texto en pantalla\n");
```

### 1.4 Casos de bugs de UEFI explotables

UEFI corre con privilegios máximos y persiste en flash, así que un bug acá es prácticamente un *rootkit* permanente. Algunos casos famosos:

- **LoJax (2018):** primer rootkit UEFI detectado *in the wild*, atribuido al grupo APT28. Modificaba el módulo SPI flash para sobrevivir reinstalaciones del SO y reemplazos de disco.
- **MosaicRegressor (2020):** implante UEFI descubierto por Kaspersky, basado en código filtrado de Hacking Team.
- **BootHole (CVE-2020-10713):** vulnerabilidad de buffer overflow en el parser de `grub.cfg` de GRUB2 que permitía saltar Secure Boot.
- **Logofail (2023):** familia de bugs en los parsers de imágenes (BMP, PNG, GIF) usados por el firmware para mostrar el logo del fabricante. Un logo malicioso embebido en la ESP ejecutaba código durante el arranque, antes incluso del SO.
- **PixieFail (2024):** nueve vulnerabilidades en la pila de red TCP/IPv6 de EDK II que permitían RCE durante un PXE boot.
- **BlackLotus (2023):** primer bootkit UEFI capaz de saltar Secure Boot incluso en Windows 11 actualizado, vendido en foros clandestinos.

El patrón común es que muchos de estos bugs son errores de validación, donde el firmware confía en datos que en realidad pueden venir de un atacante.

### 1.5 CSME e Intel MEBx

**Converged Security and Management Engine (CSME)** es un subsistema autónomo dentro de los chipsets Intel modernos. Es básicamente una computadora dentro de la computadora: tiene su propia CPU (un núcleo Quark/Minute IA), su propia RAM, su propio firmware, y corre incluso cuando la máquina está apagada (mientras tenga corriente). Se encarga de:

- Funciones de seguridad de plataforma (raíz de confianza, fTPM, EPID para DRM).
- **Intel AMT (Active Management Technology)**, que permite a un administrador acceder remotamente a la máquina aunque el SO esté caído.
- Boot Guard, BIOS Guard y verificación de firmware.

**Intel MEBx (Management Engine BIOS Extension)** es una pantalla de configuración accesible desde el firmware UEFI/BIOS que permite configurar AMT/CSME: contraseña del ME, redes administrativas, certificados, modo de aprovisionamiento, etc. Es la "puerta de entrada" desde el firmware para administrar el motor de gestión.

### 1.6 ¿Qué es coreboot?

**coreboot** es un firmware libre y open source que reemplaza al BIOS/UEFI. Su filosofía es: hacer **lo mínimo indispensable** para inicializar el hardware (memoria, CPU, buses) y luego saltar lo más rápido posible a un *payload* — que puede ser un kernel Linux, GRUB, etc.

#### Productos que lo incorporan

- **Chromebooks / ChromeOS devices:** prácticamente todos los Chromebook usan coreboot desde fábrica.
- **System76:** sus laptops Galago Pro, Darter Pro, Lemur Pro vienen con coreboot.
- **Servidores OCP (Open Compute Project)** de Facebook/Meta y otros hyperscalers.

#### Ventajas de su utilización

- **Tiempo de arranque mucho menor:** al hacer solo lo necesario, puede arrancar a Linux en menos de 1 segundo.
- **Código auditable:** al ser open source, se puede inspeccionar y verificar — clave en escenarios de seguridad y soberanía.
- **Personalización:** uno puede elegir el payload, sacar funciones que no usa, ajustar el comportamiento del arranque.
- **Reproducibilidad:** las builds son deterministas, lo que ayuda a verificar que el firmware instalado corresponde al código fuente.
- **Soporte prolongado:** placas que el fabricante ya abandonó pueden seguir recibiendo actualizaciones de la comunidad.

---

## 2. El linker y la generación de imágenes binarias

### 2.1 ¿Qué es un linker? ¿Qué hace?

El linker (o enlazador) es la herramienta que agarra uno o varios archivos objeto (`.o`) que vienen del ensamblador o del compilador y los junta en un solo ejecutable, o en una imagen binaria. Lo que hace, básicamente:

- Resuelve los símbolos. Si en un archivo se llama a `imprimir()` y la definición está en otro `.o`, el linker conecta la referencia con la definición.
- Decide en qué dirección de memoria queda cada sección (`.text`, `.data`, `.bss`, etc).
- Ajusta las direcciones absolutas (los saltos, las llamadas, los accesos a variables) para que apunten al lugar correcto una vez ubicado el código.
- Junta todas las secciones del mismo tipo: las `.text` de los distintos `.o` quedan en una sola `.text` del ejecutable, y lo mismo con `.data` y el resto.
- Arma el archivo de salida en el formato pedido (ELF, PE, Mach-O o un binario plano).

### 2.2 ¿Qué es la dirección que aparece en el script del linker? ¿Por qué es necesaria?

En un linker script aparece:

```ld
SECTIONS {
    . = 0x7C00;
    .text : { *(.text) }
    .data : { *(.data) }
    .bss  : { *(.bss)  }
}
```

El `. = 0x7C00;` es el location counter, y le avisa al linker que el código va a correr a partir de esa dirección. Importa porque, cuando el linker resuelve direcciones absolutas (saltos, llamadas, accesos a variables), las arma asumiendo que el binario va a estar cargado ahí.

¿Por qué hace falta? Para un bootloader, la BIOS siempre carga el sector de arranque en `0x0000:0x7C00`. Si el linker asume otra base, los `jmp` y los `mov` a etiquetas terminan apuntando a lugares equivocados y el programa no anda. O sea, la dirección del linker script tiene que ser la misma dirección donde el firmware va a dejar el binario al cargarlo.

### 2.3 Comparación entre `objdump` y `hd`

`objdump -D` desensambla el binario interpretando las secciones según el formato (ELF u otro) y usa las direcciones del linker script. `hd` (hexdump) directamente vuelca los bytes del archivo, sin interpretar nada.

Para comparar:

```bash
# Desensamblado con direcciones lógicas
objdump -D -b binary -m i8086 boot.bin

# Volcado hexadecimal del archivo
hd boot.bin
```

En las dos salidas aparecen los mismos bytes, pero `objdump` los muestra etiquetados con la dirección que les puso el linker (`0x7C00` por ejemplo), y `hd` los muestra desde el offset 0 del archivo. De ahí se ve que el programa quedó al principio de la imagen, ocupando los primeros 510 bytes, y que los bytes 510-511 son la firma `0x55 0xAA`.

### 2.4 Probar la imagen con QEMU

En vez de pasar la imagen a un pendrive y bootear de ahí, optamos por correrla directamente en QEMU, que para esta etapa nos resulta más cómodo (no hay que reiniciar la máquina, se puede enganchar GDB y si algo sale mal no se rompe nada).

Los pasos que terminamos haciendo, que son los que se ven en las capturas del principio del informe (`screens/1.png` a `screens/4.png`):

```bash
# 1) Armamos la imagen booteable: hlt (0xF4), padding hasta el byte 510 y la firma 0x55 0xAA
printf '\364%509s\125\252' > main.img

# 2) Para confirmar la codificación de hlt
echo hlt > a.S
as -o a.o a.S
objdump -S a.o    # muestra que hlt se codifica como f4

# 3) Verificamos los bytes de la imagen
hd main.img       # f4 al inicio, 55 aa al final del sector

# 4) Corremos la imagen en QEMU
qemu-system-x86_64 --drive file=main.img,format=raw,index=0,media=disk
```

Al levantar QEMU se ve el SeaBIOS y el "Booting from Hard Disk...", y como la única instrucción del sector es `hlt`, la CPU queda colgada ahí, que es justo lo que esperábamos.

### 2.5 ¿Para qué se utiliza la opción `--oformat binary` en el linker?

Por defecto `ld` genera ejecutables en formato ELF, con cabeceras, tabla de secciones, tabla de símbolos, info de relocations y demás. Todo eso le sirve a un sistema operativo, pero a un bootloader no le sirve y encima molesta: la BIOS levanta los 512 bytes del primer sector y los toma como código x86, no sabe nada de ELF.

Con `--oformat binary` (o `-O binary` en `objcopy`) se le pide al linker que tire toda esa información de formato y deje solamente los bytes de las secciones (`.text`, `.data`, etc.) en el orden y las posiciones que dice el linker script. Lo que queda es lo que el firmware va a ejecutar tal cual. Es la opción que se usa para bootloaders, para kernels que se cargan en bare metal y para firmware embebido en general.

