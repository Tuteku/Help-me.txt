# TP2 — Iteración 2: C con ensamblador y libcurl

enlace al repositorio en github: https://github.com/Tuteku/Help-me.txt

## Descripción

En esta segunda iteración se elimina la capa Python y se integra todo en un único programa C que:

- **Consulta la API REST** del Banco Mundial directamente usando `libcurl`.
- **Parsea la respuesta JSON** de forma manual mediante búsqueda de cadenas.
- **Llama a funciones escritas en ensamblador x86-64** (`asm_functions.s`) en lugar de funciones C.

---

## Funciones de ensamblador — `asm_functions.s`

```asm
    .section .text

    .globl float_to_int
    .type  float_to_int, @function
float_to_int:
    pushq   %rbp
    movq    %rsp, %rbp
    cvttss2si %xmm0, %rax
    popq    %rbp
    ret

    .globl add_offset
    .type  add_offset, @function
add_offset:
    pushq   %rbp
    movq    %rsp, %rbp
    movq    %rdi, %rax
    addq    %rsi, %rax
    popq    %rbp
    ret

    .section .note.GNU-stack,"",@progbits
```

- `float_to_int`: recibe un `float` en `%xmm0` (convención System V x86-64), lo convierte a entero de 64 bits por truncamiento con `cvttss2si` y retorna en `%rax`.
- `add_offset`: recibe dos `long` en `%rdi` y `%rsi`, retorna su suma en `%rax`.

---

### Funcionamiento

1. `write_cb` acumula los chunks HTTP en un buffer dinámico.
2. Se realiza un `GET` a la API del Banco Mundial filtrando por país `AR` (Argentina) directamente en la URL.
3. Se recorre el JSON buscando pares `"date":"AÑO"` / `"value":NUMERO`.
4. Por cada registro válido (no `null`):
   - `float_to_int(gini)` → convierte el GINI float a entero (función ASM).
   - `add_offset(as_int, 1)` → devuelve el entero sumado en +1 (función ASM).

---

## Compilación manual

```bash
# 1. Ensamblar el archivo .s
as --64 -g -o asm_functions.o asm_functions.s

# 2. Compilar main.c a objeto
gcc -g -O0 -c -o main.o main.c

# 3. Linkear ambos objetos + libcurl
gcc -o programa main.o asm_functions.o -lcurl
```

El flag `-lcurl` es necesario en el paso de linkeo porque las funciones `curl_*` viven en la biblioteca dinámica `libcurl`. Sin él el linker lanza `undefined reference`.

---

## Ejecución y output

```bash
./programa
```

<!-- Insertar screenshot del output del programa -->

---

## Depuración con GDB

```bash
gdb ./programa
```

<!-- Insertar flujo de GDB en consola: breakpoints en float_to_int / add_offset,
     inspección de registros %xmm0, %rdi, %rsi, %rax -->

---

## Flujo de llamadas

```
main.c
    │
    ├─► curl_easy_perform()        # GET a la API del Banco Mundial
    │       └─► write_cb()         # acumula respuesta JSON en buffer
    │
    ├─► strstr / sscanf            # parseo manual del JSON
    │       └─► extrae año y valor GINI por cada registro
    │
    ├─► float_to_int(gini)         # llama función ASM
    │       └─► asm_functions.s :: cvttss2si  →  (long)gini
    │
    └─► add_offset(as_int, 1)      # llama función ASM
            └─► asm_functions.s :: addq       →  as_int + 1
```
