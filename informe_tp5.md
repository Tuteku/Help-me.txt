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
Device Driver: es software puro que corre dentro del kernel. Sabe cómo controlar un tipo específico de dispositivo y expone una interfaz al espacio de usuario. Es lo que nosotros escribimos en este TP.
Device Controller: es un componente de hardware (un chip o circuito integrado) que gestiona físicamente la comunicación con los dispositivos. Por ejemplo, el controlador USB de la Raspberry Pi es un chip que maneja el protocolo eléctrico USB. En un microcontrolador como los que usa la Raspberry Pi, varios controllers vienen integrados en el mismo chip (I2C, SPI, UART, GPIO).
Bus Driver: es el software que le enseña al kernel cómo comunicarse con un device controller particular. Implementa el protocolo del bus (USB, I2C, SPI, PCI) y ofrece funciones que los device drivers de más arriba pueden usar. En nuestro caso, el bus driver del GPIO ya existe en el kernel de la Raspberry Pi; nosotros lo usamos pero no lo escribimos.
La jerarquía de capas queda así:
Aplicación de usuario
    ↕ (system calls)
Device Driver (nuestro CDD)
    ↕ (funciones del bus)
Bus Driver (ya existente en el kernel)
    ↕ (registros de hardware)
Device Controller (chip GPIO del SoC)
    ↕ (señales eléctricas)
Dispositivo físico (sensor)
### Clasificación de drivers en Linux
Linux clasifica los dispositivos en tres verticales según cómo se transfieren los datos:
Network (orientado a paquetes): tarjetas de red, wifi, bluetooth. Transmiten datos como paquetes con cabeceras y protocolos.
Storage/Block (orientado a bloques): discos duros, pendrives, tarjetas SD. Los datos se leen y escriben en bloques de tamaño fijo (típicamente 512 bytes o 4 KB).
Character (orientado a bytes): todo lo demás. Puertos serie, teclados, ratones, sensores, cámaras, audio. Los datos se transfieren byte a byte, sin estructura de bloque. Este es el grupo mayoritario, y es donde se ubica nuestro driver.
### Character Device Driver
Un CDD en Linux se compone de los siguientes elementos:
Módulo del kernel (.ko): el driver se empaqueta como un módulo cargable. Tiene un constructor (module_init) que se ejecuta al hacer insmod y un destructor (module_exit) que se ejecuta al hacer rmmod.
Major y Minor numbers: el major number identifica qué driver maneja un dispositivo. El minor number distingue entre múltiples instancias del mismo driver. Juntos forman el par <major, minor> que el kernel usa para vincular un archivo de /dev/ con su driver correspondiente.
Character Device File (CDF): es el archivo especial que aparece en /dev/ (por ejemplo /dev/sensor_cdd). Las aplicaciones de usuario interactúan con este archivo usando las llamadas estándar de archivos.
file_operations: es la estructura central del driver. Es una tabla que le dice al kernel qué función ejecutar cuando el usuario hace open(), read(), write() o close() sobre el archivo del dispositivo.
Transferencia de datos: dado que el kernel y el espacio de usuario tienen espacios de memoria separados, se usan las funciones copy_to_user() (para read) y copy_from_user() (para write) para mover datos de forma segura entre ambos mundos.
### El modelo de capas del CDD
El flujo de datos en nuestro sistema sigue cuatro capas:
| Capa / Componente | Descripción y Funciones Clave |
| :--- | :--- |
| **Application**<br>`(sensor_app.py)` | <ul><li>`open("/dev/sensor_cdd")`</li><li>`read()` para obtener valor</li><li>`write("0" o "1")` para elegir sensor</li></ul> |
| **Character Device File**<br>`(/dev/sensor_cdd)` | <ul><li>Creado automáticamente por `udev`</li><li>Vinculado al driver por `<major,minor>`</li></ul> |
| **Character Device Driver**<br>`(sensor_drv.ko)` | <ul><li>`sensor_read()`: lee GPIO, `copy_to_user`</li><li>`sensor_write()`: selecciona canal</li><li>Timer: muestrea cada 1 segundo</li></ul> |
| **Hardware**<br>`GPIO pins de Raspberry Pi` | <ul><li>GPIO 17: sensor de temperatura</li><li>GPIO 27: sensor de luminosidad</li></ul> |
## Desarrollo 
