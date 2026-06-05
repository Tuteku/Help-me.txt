# Makefile out-of-tree para el CDD signar_cdd
#
# Compilador cross instalado en el host:  aarch64-linux-gnu-gcc
#   (paquete gcc-aarch64-linux-gnu -> binario /usr/bin/aarch64-linux-gnu-gcc)
# El kernel arma el nombre como $(CROSS_COMPILE)gcc, por eso el prefijo
# es "aarch64-linux-gnu-" (con guion final, SIN el "gcc").
#
# Uso:
#   Cross-compilacion (desde la PC x86_64 para la Raspberry Pi 5):
#       make KDIR=/ruta/al/kernel-rpi
#
#   Compilacion nativa (ejecutando directamente en la Raspberry Pi):
#       make ARCH= CROSS_COMPILE= KDIR=/lib/modules/$(uname -r)/build
#
#   Limpiar:
#       make clean

# Nombre del modulo = nombre del archivo fuente (signar_cdd.c -> signar_cdd.ko)
obj-m := signar_cdd.o

# Arquitectura y toolchain de cross-compilacion (sobreescribibles por linea de comandos).
ARCH          ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-

# Directorio del arbol del kernel contra el que se compila.
# - Cross-compile: apuntar al arbol/headers del kernel de la Pi (pasar KDIR=...).
# - Nativo en la Pi: usa los headers del kernel en ejecucion.
KDIR ?= /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) clean

.PHONY: all clean
