    .section .text

    .globl float_to_int
    .type  float_to_int, @function
float_to_int:
    pushq   %rbp               # prologo: salvar frame pointer anterior
    movq    %rsp, %rbp         # nuevo frame pointer apunta al top del stack

    cvttss2si %xmm0, %rax      # convertir float -> int64 (truncate toward zero)

    popq    %rbp               # epilogo: restaurar frame pointer
    ret                        # retornar; caller retoma desde dirección en stack


    .globl add_offset
    .type  add_offset, @function
add_offset:
    pushq   %rbp               # prologo
    movq    %rsp, %rbp

    movq    %rdi, %rax         # rax = value
    addq    %rsi, %rax         # rax = value + offset

    popq    %rbp               # epilogo
    ret

    .section .note.GNU-stack,"",@progbits
