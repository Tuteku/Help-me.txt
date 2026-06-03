# Informe TP4 — Módulos de Kernel de Linux

### Help-me.txt
Integrantes:
- Mauro Cabero
- Nicolas de la Mata
- Mateo Quispe

Enlace al repositorio en github: https://github.com/Tuteku/Help-me.txt
## Introduccion
Este trabajo práctico consiste en el diseño e implementación de un Character Device Driver (CDD) para el kernel de Linux que permita sensar dos señales externas con un período de muestreo de un segundo. Complementariamente, se desarrolla una aplicación de espacio de usuario capaz de leer una de las dos señales a través del CDD y graficarla en función del tiempo.
El objetivo es recorrer todo el camino que va desde el hardware (pines GPIO de una Raspberry Pi) hasta una representación visual en pantalla, pasando por las capas de abstracción que provee el kernel. En ese recorrido se ponen en práctica los conceptos de módulos del kernel, registración de dispositivos, file_operations, y la comunicación entre kernel space y user space.
## Marco Teórico
### Driver, Device Controller y Bus Driver
Estos tres conceptos suelen confundirse, pero ocupan lugares distintos en la jerarquía de hardware y software:
- Device Driver: es software puro que corre dentro del kernel. Sabe cómo controlar un tipo específico de dispositivo y expone una interfaz al espacio de usuario. Es lo que nosotros escribimos en este TP.
- Device Controller: es un componente de hardware (un chip o circuito integrado) que gestiona físicamente la comunicación con los dispositivos. Por ejemplo, el controlador USB de la Raspberry Pi es un chip que maneja el protocolo eléctrico USB. En un microcontrolador como los que usa la Raspberry Pi, varios controllers vienen integrados en el mismo chip (I2C, SPI, UART, GPIO).
- Bus Driver: es el software que le enseña al kernel cómo comunicarse con un device controller particular. Implementa el protocolo del bus (USB, I2C, SPI, PCI) y ofrece funciones que los device drivers de más arriba pueden usar. En nuestro caso, el bus driver del GPIO ya existe en el kernel de la Raspberry Pi; nosotros lo usamos pero no lo escribimos.

La jerarquía de capas queda así:

graph TD
    A[Aplicación de usuario]
    B[Device Driver <br><i>nuestro CDD</i>]
    C[Bus Driver <br><i>ya existente en el kernel</i>]
    D[Device Controller <br><i>chip GPIO del SoC</i>]
    E[Dispositivo físico <br><i>sensor</i>]

    A <--> |system calls| B
    B <--> |funciones del bus| C
    C <--> |registros de hardware| D
    D <--> |señales eléctricas| E
### Clasificación de drivers en Linux
Linux clasifica los dispositivos en tres verticales según cómo se transfieren los datos:
Network (orientado a paquetes): tarjetas de red, wifi, bluetooth. Transmiten datos como paquetes con cabeceras y protocolos.
Storage/Block (orientado a bloques): discos duros, pendrives, tarjetas SD. Los datos se leen y escriben en bloques de tamaño fijo (típicamente 512 bytes o 4 KB).
Character (orientado a bytes): todo lo demás. Puertos serie, teclados, ratones, sensores, cámaras, audio. Los datos se transfieren byte a byte, sin estructura de bloque. Este es el grupo mayoritario, y es donde se ubica nuestro driver.
### Character Device Driver

Un CDD en Linux se compone de los siguientes elementos:
- Módulo del kernel (.ko): el driver se empaqueta como un módulo cargable. Tiene un constructor (module_init) que se ejecuta al hacer insmod y un destructor (module_exit) que se ejecuta al hacer rmmod.
- Major y Minor numbers: el major number identifica qué driver maneja un dispositivo. El minor number distingue entre múltiples instancias del mismo driver. Juntos forman el par <major, minor> que el kernel usa para vincular un archivo de /dev/ con su driver correspondiente.
- Character Device File (CDF): es el archivo especial que aparece en /dev/ (por ejemplo /dev/sensor_cdd). Las aplicaciones de usuario interactúan con este archivo usando las llamadas estándar de archivos.
- file_operations: es la estructura central del driver. Es una tabla que le dice al kernel qué función ejecutar cuando el usuario hace open(), read(), write() o close() sobre el archivo del dispositivo.
- Transferencia de datos: dado que el kernel y el espacio de usuario tienen espacios de memoria separados, se usan las funciones copy_to_user() (para read) y copy_from_user() (para write) para mover datos de forma segura entre ambos mundos.
### Compilación cruzada
La compilación cruzada (cross-compilation) consiste en compilar código en una máquina (el host) para que se ejecute en otra con arquitectura diferente (el target). En nuestro caso el host es una PC x86_64 y el target es la Raspberry Pi con procesador ARM.
Para esto se necesitan dos elementos: un cross-compiler (gcc para ARM, como arm-linux-gnueabihf-gcc) y los headers del kernel que está corriendo en la Raspberry Pi. El Makefile invoca al sistema de build del kernel (kbuild) pasándole las variables ARCH y CROSS_COMPILE para que genere un .ko compatible con ARM.

El flujo de trabajo es:

[PC Host x86_64]                          [Raspberry Pi ARM]
                                          
 Código fuente (.c)                       
      |                                   
 Cross-compilación (make)                 
      |                                   
 sensor_drv.ko (ARM)                      
      |                                   
 scp via SSH ─────────────────────────>  sensor_drv.ko
                                               |
                                          sudo insmod
                                               |
                                          /dev/sensor_cdd
                                          
### GPIO y señales digitales
Los pines GPIO de la Raspberry Pi operan con lógica de 3.3V. Cuando se configuran como entrada, leen HIGH (1) si la tensión supera ~1.8V, o LOW (0) si está por debajo.
Al conectar un generador de señales, la señal analógica queda digitalizada por este umbral. Dado que nuestro período de muestreo es de 1 segundo, la frecuencia de la señal debe ser menor a 0.5 Hz (criterio de Nyquist) para capturar correctamente las transiciones.
Los GPIO toleran un máximo de 3.3V. Se recomienda intercalar una resistencia de 1kΩ en serie como protección.

## Desarrollo 
