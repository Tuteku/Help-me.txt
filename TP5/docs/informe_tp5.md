# Informe TP5 — Módulos de Kernel de Linux

### Help-me.txt
Integrantes:
- Mauro Cabero
- Nicolas de la Mata
- Mateo Quispe

Enlace al repositorio en github: https://github.com/Tuteku/Help-me.txt
## Introduccion
Este trabajo práctico consiste en el diseño e implementación de un Character Device Driver (CDD) para el kernel de Linux que sensa dos señales externas con un período de muestreo de un segundo. Como complemento, se desarrolló una aplicación de espacio de usuario que lee una de las dos señales a través del CDD y la grafica en tiempo real en una interfaz web servida desde la propia Raspberry Pi.

Para compilar el módulo se usó compilación cruzada (cross-compilation): el código se escribe y compila en una PC host (x86_64) apuntando a la arquitectura ARM de la Raspberry Pi, y los binarios resultantes se transfieren vía SSH. Desde el navegador de la PC host se accede al gráfico en tiempo real.
## Marco Teórico
### Driver, Device Controller y Bus Driver
Estos tres conceptos suelen confundirse, pero ocupan lugares distintos en la jerarquía de hardware y software:
- Device Driver: es software que corre dentro del kernel. Sabe cómo controlar un tipo específico de dispositivo y expone una interfaz al espacio de usuario.
- Device Controller: es un componente de hardware (un chip o circuito integrado) que gestiona físicamente la comunicación con los dispositivos. Por ejemplo, el controlador USB de la Raspberry Pi es un chip que maneja el protocolo eléctrico USB. En un microcontrolador como los que usa la Raspberry Pi, varios controllers vienen integrados en el mismo chip (I2C, SPI, UART, GPIO).
- Bus Driver: es el software que le enseña al kernel cómo comunicarse con un device controller particular. Implementa el protocolo del bus (USB, I2C, SPI, PCI) y ofrece funciones que los device drivers de más arriba pueden usar. En nuestro caso, el bus driver del GPIO ya existe en el kernel de la Raspberry Pi; nosotros lo usamos pero no lo escribimos.

La jerarquía de capas queda así:

![jerarquia](./assets/linux_driver_stack.svg)
### Clasificación de drivers en Linux
Linux clasifica los dispositivos en tres verticales según cómo se transfieren los datos:
- Network (orientado a paquetes): tarjetas de red, wifi, bluetooth. Transmiten datos como paquetes con cabeceras y protocolos.
- Storage/Block (orientado a bloques): discos duros, pendrives, tarjetas SD. Los datos se leen y escriben en bloques de tamaño fijo (típicamente 512 bytes o 4 KB).
- Character (orientado a bytes): puertos serie, teclados, ratones, sensores, cámaras, audio. Los datos se transfieren byte a byte, sin estructura de bloque. Este es el grupo mayoritario, y es donde se ubica nuestro driver.
  
### Character Device Driver
Un CDD en Linux se compone de los siguientes elementos:
- Módulo del kernel (.ko): el driver se empaqueta como un módulo cargable. Tiene un constructor (module_init) que se ejecuta al hacer insmod y un destructor (module_exit) que se ejecuta al hacer rmmod.
- Major y Minor numbers: el major number identifica qué driver maneja un dispositivo. El minor number distingue entre múltiples instancias del mismo driver. Juntos forman el par <major, minor> que el kernel usa para vincular un archivo de /dev/ con su driver correspondiente.
- Character Device File (CDF): es el archivo especial que aparece en /dev/ (por ejemplo /dev/signal_cdd). Las aplicaciones de usuario interactúan con este archivo usando las llamadas estándar de archivos.
- file_operations: es la estructura central del driver. Es una tabla que le dice al kernel qué función ejecutar cuando el usuario hace open(), read(), write() o close() sobre el archivo del dispositivo.
- Transferencia de datos: dado que el kernel y el espacio de usuario tienen espacios de memoria separados, se usan las funciones copy_to_user() (para read) y copy_from_user() (para write) para mover datos de forma segura entre ambos mundos.
  
### Compilación cruzada
La compilación cruzada (cross-compilation) consiste en compilar código en una máquina para que se ejecute en otra con arquitectura diferente. En nuestro caso el host es una PC x86_64 y el target es la Raspberry Pi con procesador ARM.

Para esto se necesitan dos elementos: un cross-compiler (gcc para ARM de 64 bits, aarch64-linux-gnu-gcc) y los headers del kernel que está corriendo en la Raspberry Pi. El Makefile invoca al sistema de build del kernel (kbuild) pasándole las variables ARCH y CROSS_COMPILE para que genere un .ko compatible con ARM.

El flujo de trabajo es:

![Diagrama cross-compilación](./assets/cross_compilation_deploy_flow_v5.svg)
                                          
### GPIO y señales digitales
Los pines GPIO de la Raspberry Pi operan con lógica de 3.3V. Cuando se configuran como entrada, leen HIGH (1) si la tensión supera ~1.8V, o LOW (0) si está por debajo.

Al conectar un generador de señales, la señal analógica queda digitalizada por este umbral. Dado que nuestro período de muestreo es de 1 segundo, la frecuencia de la señal debe ser menor a 0.5 Hz (criterio de Nyquist) para capturar correctamente las transiciones.

Los GPIO toleran un máximo de 3.3V. Se recomienda intercalar una resistencia de 1kΩ en serie como protección.

## Desarrollo

El trabajo se dividió en dos etapas: la construcción y carga del CDD mediante compilación cruzada, y la aplicación de usuario con visualización web. Lo que sigue documenta el flujo completo con las capturas de cada paso.

### 1. Entorno de compilación cruzada

El host es una PC x86_64 con Ubuntu 24.04 y el target una Raspberry Pi 5 corriendo Raspberry Pi OS con kernel `6.12.25+rpt-rpi-2712` (aarch64).

Para cross-compilar un módulo de kernel no alcanza con un compilador y los headers sueltos: se necesita el **árbol del kernel exacto** del target. Los pasos de preparación fueron:

- Instalación del cross-compiler `aarch64-linux-gnu-gcc-12`.
- Instalación en la Pi de los headers (`linux-headers-rpi-2712`) y copia de ese árbol a la PC, replicando la ruta `/usr/src/linux-headers-6.12.25+rpt-rpi-2712`.
- Los paquetes de headers de Debian/RPi traen sus herramientas internas de build compiladas para ARM64 y sin código fuente para recompilarlas en x86. Para ejecutarlas se usó `qemu-user-static`, con la variable `QEMU_LD_PREFIX` apuntando al sysroot ARM64 del toolchain para resolver el cargador dinámico y la libc.

### 2. Compilación cruzada del módulo

Con el entorno listo, se compila el módulo con un Makefile *out-of-tree* que invoca a kbuild con `ARCH=arm64` y `CROSS_COMPILE=aarch64-linux-gnu-`:

![Compilación cruzada del módulo](assets/tp5_1.png)

Se observan las etapas de kbuild: `CC [M]` compila `signar_cdd.o` (con el cross-gcc, nativo x86), `MODPOST` procesa los símbolos y el *version magic* (corriendo emulado bajo QEMU), y `LD [M]` enlaza el módulo final `signar_cdd.ko`.

El build genera el conjunto de artefactos en el directorio del driver:

![Archivos generados por el build](assets/tp5_2_files_make.png)

### 3. Transferencia a la Raspberry Pi

El módulo se envía a la Pi por SSH con `scp`:

![Transferencia del módulo vía scp](assets/tp5_3_modulo_in_rpi5.png)

### 4. Carga del módulo en el kernel

En la Pi se inserta el módulo con `insmod` y se verifica en el log del kernel (`dmesg`):

![Carga del módulo y dmesg](assets/tp5_4_carga_modulo.png)

El `dmesg` muestra `modulo cargado correctamente`. En ese momento el driver registró dinámicamente el par `<major, minor>` — en nuestro caso `509, 0` — y el demonio `udev` leyó esa información desde sysfs y creó automáticamente el archivo `/dev/signal_cdd`. Esto se puede verificar con `ls -l /dev/signal_cdd`, que muestra la `c` de *character device* seguida del par `509, 0`.

### 5. Aplicación de usuario y servidor web

La aplicación `sensor_app.py` corre **en la Pi**, lee `/dev/signal_cdd` una vez por segundo y expone los datos por HTTP en el puerto 8080. Internamente, cada lectura del device devuelve `"0\n"` o `"1\n"` según el estado del GPIO; escribir `'0'` o `'1'` en el mismo archivo cambia el canal activo. Como el device pertenece a root, se le dan permisos de lectura/escritura (`chmod 666`) antes de lanzar el servidor:

![Lanzamiento del servidor Python](assets/tp5_5_levantar_py.png)


### 6. Visualización web

Desde el navegador de la PC host se accede a `http://172.26.88.185:8080`. Al inicio, sin señal aplicada, el gráfico muestra el pin en 0 V (LOW):

![Interfaz web inicial](assets/tp5_6_web.png)

Conectando el generador al canal 1, se observa la señal cuadrada sensada en tiempo real (con el eje en niveles 0 V / 3.3 V):

![Canal 1 sensando la señal](assets/tp5_7_ch1.png)

Al presionar el botón **Canal 2 (CH2)** —que dispara un POST al servidor y este escribe el carácter correspondiente al CDD— se cambia la señal activa del driver y se pasa a visualizar el segundo generador, con su propia cuadrada:

![Canal 2 sensando la señal](assets/tp5_8_ch2.png)

El driver, al recibir el cambio, actualiza la señal activa y resetea su buffer; la gráfica refleja el cambio de canal de inmediato.

### 7. Conexionado electrónico

Se utilizaron dos generadores de señales, uno por canal. Consideraciones de la conexión:

- **Pines:** las señales van a **GPIO17** y **GPIO27**.
- **Niveles:** los GPIO son entradas **digitales** de **3.3 V**. Cada generador se configuró como onda cuadrada entre 0 V y ~3.3 V .
- **Frecuencia:** como el muestreo es de 1 Hz, por Nyquist la señal debe ser < 0.5 Hz; se usaron frecuencias del orden de 0.1–0.5 Hz para observar las transiciones con claridad.

Dado que la entrada es digital, el GPIO solo distingue dos estados (0/1) según el umbral (~1.8 V): el eje "Tensión [V]" de la web es una reconstrucción a nivel de usuario (`valor × 3.3`), no una medición analógica.

### 8. Descarga del módulo

Una vez terminadas las pruebas, el módulo se descarga con `rmmod`. El destructor del driver (`module_exit`) desmonta todo en orden inverso: libera los GPIO, destruye el device y la clase en sysfs, elimina el cdev y libera el major. El `dmesg` lo confirma:

![Carga y descarga del módulo](assets/tp5_10_bajar_modulo.png)

Tras la descarga, `/dev/signal_cdd` desaparece y el major 509 se libera de `/proc/devices`.
## Conclusiones

Se implementó con éxito un Character Device Driver para Linux que sensa dos señales digitales por GPIO con período de muestreo de 1 segundo, junto con una aplicación de usuario que las sirve por una interfaz web. El driver se construyó por compilación cruzada desde una PC x86_64 hacia la arquitectura ARM64 de la Raspberry Pi 5 y se cargó vía SSH, validando todo el flujo de trabajo.

Los puntos que más trabajo dieron fueron tres. Primero, la compilación cruzada de un módulo requiere el árbol del kernel exacto del target, no solo los headers sueltos, y el *version magic* tiene que coincidir al byte. Segundo, los paquetes de headers de Raspberry Pi OS traen sus herramientas internas compiladas solo para ARM64, así que hubo que emularlas con `qemu-user-static` desde la PC x86. Tercero, la Raspberry Pi 5 gestiona los GPIO a través del chip RP1, lo que desplaza la base del *numberspace* global y rompe los números de pin de la API legacy; ajustando esos números se resolvió el `-EPROBE_DEFER` que aparecía al cargar el módulo.
