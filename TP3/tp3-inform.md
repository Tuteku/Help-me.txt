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

