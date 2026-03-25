
Los datos fueron obtenidos del benchmark `pts/build-linux-kernel-1.15.0` de OpenBenchmarking.org y de https://www.tomshardware.com/reviews/cpu-hierarchy,4312.html.

---

## Lista de benchmarks utiles para uso diario

### Renderizado y gráficos
- **Blender** — Renderizado 3D CPU-bound (escenas Monster, Junkshop, Classroom)
- **POV-Ray** — Ray tracing clásico, test multi-hilo intensivo
- **V-Ray** — Renderizado profesional para diseño y arquitectura

### Compilación y desarrollo de software
- **Timed Linux Kernel Compilation** — Mide el tiempo de compilar el kernel de Linux (altamente paralelo)
- **Timed LLVM Compilation** — Compilación del stack LLVM/Clang
- **Timed FFmpeg Compilation** — Compilación del framework multimedia FFmpeg
- **Timed CPython Compilation** — Compilación del intérprete de Python con optimizaciones LTO

### Compresión y procesamiento de datos
- **7-Zip Compression** — Compresión/descompresión con p7zip (escala bien con múltiples núcleos)
- **Gzip / Zstd / LZ4** — Benchmarks de distintos algoritmos de compresión

### Codificación de video
- **FFmpeg (libx265)** — Transcodificación de video H.265
- **SVT-AV1** — Codificación de video con el codec AV1 de Intel/Meta
- **x265 (standalone)** — Encoders dedicados de video

### Ciencia, datos y machine learning
- **NumPy Benchmark** — Operaciones matriciales y numéricas en Python
- **PyHPC-Benchmarks** — Suite HPC en Python (NumPy, CuPy, JAX)

### Rendimiento general / sistema
- **Stockfish** — Motor de ajedrez (mide rendimiento de IA/búsqueda)
---

## Tabla: Tareas cotidianas vs. Benchmark representativo

## Desarrollo de software

| Tarea diaria | Benchmark representativo 
|---|---|
| Compilar un proyecto grande (C++, Rust, Go) | Timed Linux Kernel Compilation, Timed LLVM Compilation |
| Compilar un script / intérprete Python | Timed CPython Compilation |
| Compilar herramientas multimedia (FFmpeg, libav) | Timed FFmpeg Compilation
| Correr tests / CI local | Timed LLVM Compilation, 7-Zip Compression |

---

## Creación de contenido y video

| Tarea diaria | Benchmark representativo |
|---|---|
| Exportar / transcodificar video (H.265 / AV1) | FFmpeg libx265, SVT-AV1 |
| Renderizar una escena 3D / animación | Blender, V-Ray |
| Comprimir / archivar assets (ZIP, 7z, Zstd) | 7-Zip Compression, Zstd / LZ4 |

---

## Diseño 3D 

| Tarea diaria | Benchmark representativo |
|---|---|
| Renderizado de arquitectura | V-Ray, POV-Ray | 
| Modelado y viewport 3D (interactividad) | Blender |
| Exportar / importar archivos CAD pesados | 7-Zip Compression, Gzip |

---

## Datos y ML

| Tarea diaria | Benchmark representativo |
|---|---|
| Correr scripts de análisis numérico (NumPy, SciPy) | NumPy Benchmark, PyHPC-Benchmarks | 
| Entrenar / ejecutar modelos ML livianos en CPU | PyHPC-Benchmarks, NumPy Benchmark | 

---

## Gaming

| Tarea diaria | Benchmark representativo | 
|---|---|
| Jugar títulos AAA | Stockfish |
| Streaming mientras jugás | FFmpeg libx265|

---

## Uso general

| Tarea diaria | Benchmark representativo |
|---|---|
| Multitasking pesado (muchas apps a la vez) | 7-Zip Compression, Timed Linux Kernel Compilation |
| Búsqueda y resolución de problemas | Stockfish | 
---
## Rendimiento para compilar el kernel Linux
### Datos obtenidos:

| Procesador | Núcleos / Hilos | TDP | Precio aprox. | Tiempo (seg) |
|-----------|----------------|------|---------------|-------------|
| Intel Core i5-13600K *(original)* | 14 (6P+8E) / 20 | 125W | ~$320 | **72s** |
| AMD Ryzen 9 5900X 12-Core | 12 / 24 | 105W | ~$550 | **76s** |
| AMD Ryzen 9 7950X 16-Core | 16 / 32 | 170W | ~$700 | **50s** |

---

## Speedup 

La fórmula del speedup que vimos en clase es:

Speedup = T_original / T_mejorado = EX_CPU_original / EX_CPU_mejorado

Entonces:

**Speedup del 5900X** respecto al i5-13600K:

Speedup(5900X) = 72 / 76 = 0.95x


**Speedup del 7950X** respecto al i5-13600K:

Speedup(7950X) = 72 / 50 = 1.44x


El 7950X compila el kernel **1.44 veces más rápido** que el i5-13600K. 

---

## ¿Cuál hace un uso más eficiente de sus núcleos?

La eficiencia según la cantidad de núcleos se calcula:

Eficiencia = Speedup / N° de núcleos

| Procesador | Speedup | Núcleos | Eficiencia por núcleo |
|-----------|---------|---------|----------------------|
| i5-13600K (original) | 1.00x | 14 | 0.071 |
| Ryzen 9 5900X | 0.95x | 12 | **0.079**  |
| Ryzen 9 7950X | 1.44x | 16 | 0.09 |

La eficiencia por núcleo muestra que **el Ryzen 9 7950X es el más eficiente**, con un valor de 0.09, superando tanto al Ryzen 9 5900X (0.079) como al i5-13600K (0.071).

Esto indica que, a pesar de tener más núcleos, el 7950X logra aprovecharlos mejor en esta tarea. Sin embargo, la mejora no es proporcional a la cantidad de núcleos, lo cual es consistente con la Ley de Amdahl: existe una fracción del proceso de compilación que no se puede paralelizar, lo que limita el beneficio de agregar más núcleos.

En particular, aunque el 7950X tiene 4 núcleos más que el 5900X, el aumento en speedup (de 0.95x a 1.44x) no escala linealmente con esa diferencia. Esto refleja que, si bien los núcleos adicionales aportan rendimiento, su impacto es cada vez menor debido a las partes secuenciales del problema.

---

## ¿Cuál es más eficiente en términos de costo?

### Eficiencia energética (Speedup por Watt):

Eficiencia energética = Speedup / TDP (Thermal Design Power)

| Procesador | Speedup | TDP | Eficiencia  |
|-----------|---------|-----|-------------------------------|
| i5-13600K | 1.00x | 125W | 0.0080 |
| Ryzen 9 5900X | 0.95x | 105W | **0.01** |
| Ryzen 9 7950X | 1.44x | 170W | 0.008 |

El **5900X gana en eficiencia energética**. Consume menos watts y aun así tiene un speedup decente. El 7950X consume bastante más para ganar solo un poco más de rendimiento en compilación.

### Eficiencia económica (Speedup por dólar):

Eficiencia económica = Speedup / Precio

| Procesador | Speedup | Precio (USD) | Speedup por $100 |
|-----------|---------|--------------|-----------------|
| i5-13600K | 1.00x | ~$320 | **0.313** |
| Ryzen 9 5900X | 0.95x | ~$550 | 0.17  |
| Ryzen 9 7950X | 1.44x | ~$700 | 0.21 |

El **i5-13600K es el más eficiente en términos de costo**. El 7950X cuesta más del doble que el i5-13600K para ganar solo un 10% más de speedup en este benchmark.

---