.code16
.global _start

_start:
    cli                          # deshabilitamos interrupciones
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $0x7C00, %sp             # stack justo debajo del bootloader

    # Habilitamos la línea A20 (método rápido por puerto 0x92)
    in $0x92, %al
    or $2, %al
    out %al, $0x92

    # Cargamos la GDT
    lgdt gdt_descriptor

    # Activamos el bit PE de CR0
    mov %cr0, %eax
    or $0x1, %eax
    mov %eax, %cr0

    # Salto far para entrar definitivamente a modo protegido
    ljmp $0x08, $protected_mode


.code32
protected_mode:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $0x90000, %esp

    # prueba rápida: pintar una 'P' arriba a la izquierda
    movb $'P', 0xB8000
    movb $0x07, 0xB8001

hang:
    hlt
    jmp hang

gdt_start:
    .quad 0                      # descriptor nulo

gdt_code:                        # base 0, limit 4GB, ring 0, ejecutable
    .word 0xFFFF
    .word 0x0000
    .byte 0x00
    .byte 0b10011010
    .byte 0b11001111
    .byte 0x00

gdt_data:                        # base 0, limit 4GB, ring 0, SOLO LECTURA
    .word 0xFFFF
    .word 0x0000
    .byte 0x00
    .byte 0b10010000             #  W=0  -> solo lectura
    .byte 0b11001111
    .byte 0x00
gdt_end:

gdt_descriptor:
    .word gdt_end - gdt_start - 1
    .long gdt_start

.fill 510 - (. - _start), 1, 0 
.word 0xAA55
