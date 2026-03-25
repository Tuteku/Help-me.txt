# Comparativa de Benchmarks: Intel Core i5-13600K vs AMD Ryzen 9 5900X vs AMD Ryzen 9 7950X

> **Fuente de referencia:** [OpenBenchmarking.org — Comparativa de los tres procesadores](https://openbenchmarking.org/vs/Processor/Intel+Core+i5-13600K,AMD+Ryzen+9+5900X+12-Core,AMD+Ryzen+9+7950X+16-Core)

---

## 1. Resumen de los procesadores

| Característica | Intel Core i5-13600K | AMD Ryzen 9 5900X | AMD Ryzen 9 7950X |
|---|---|---|---|
| Núcleos / Hilos | 14 / 20 | 12 / 24 | 16 / 32 |
| Frecuencia base / boost | 3.5 GHz / 5.1 GHz | 3.7 GHz / 4.8 GHz | 4.5 GHz / 5.7 GHz |
| Arquitectura | Raptor Lake (Intel 7) | Zen 3 (TSMC 7nm) | Zen 4 (TSMC 5nm) |
| TDP | 125 W (PL1) / 181 W (PL2) | 105 W | 170 W |
| Lanzamiento | Q4 2022 | Q4 2020 | Q4 2022 |
| Segmento | Mid-range gaming/work | Mid-high workstation | High-end workstation |

---

## 2. Lista de benchmarks disponibles en OpenBenchmarking.org

Los siguientes tests se encuentran en la plataforma Phoronix Test Suite / OpenBenchmarking.org y son relevantes para comparar estos tres procesadores:

### Renderizado y gráficos
- **Blender** — Renderizado 3D CPU-bound (escenas Monster, Junkshop, Classroom)
- **POV-Ray** — Ray tracing clásico, test multi-hilo intensivo
- **V-Ray** — Renderizado profesional para diseño y arquitectura
- **SmallPT GPU / QuadRay** — Ray tracing en tiempo real con soporte SIMD (SSE/AVX)

### Compilación y desarrollo de software
- **Timed Linux Kernel Compilation** — Mide el tiempo de compilar el kernel de Linux (altamente paralelo)
- **Timed GCC Compilation** — Tiempo de compilar el compilador GCC completo
- **Timed LLVM Compilation** — Compilación del stack LLVM/Clang
- **Timed FFmpeg Compilation** — Compilación del framework multimedia FFmpeg
- **Timed CPython Compilation** — Compilación del intérprete de Python con optimizaciones LTO

### Compresión y procesamiento de datos
- **7-Zip Compression** — Compresión/descompresión con p7zip (escala bien con múltiples núcleos)
- **Gzip / BZIP2 / XZ / Zstd / LZ4** — Benchmarks de distintos algoritmos de compresión
- **RAR Compression** — Compresión de archivos estilo Windows

### Codificación de video
- **FFmpeg (libx264 / libx265)** — Transcodificación de video H.264 y H.265
- **SVT-AV1** — Codificación de video con el codec AV1 de Intel/Meta
- **x264 / x265 (standalone)** — Encoders dedicados de video

### Ciencia, datos y machine learning
- **NumPy Benchmark** — Operaciones matriciales y numéricas en Python
- **Scikit-learn** — Benchmarks de machine learning (SVM, PCA, regresión)
- **SciMark 2.0** — Computación científica y numérica (FFT, Monte Carlo, etc.)
- **PyHPC-Benchmarks** — Suite HPC en Python (NumPy, CuPy, JAX)
- **FFTW / FFTE** — Transformadas de Fourier discretas
- **Caffe / MLPerf Inference** — Inferencia de modelos de deep learning en CPU

### Bases de datos y servidores
- **PostgreSQL** — Rendimiento de base de datos relacional
- **HammerDB (PostgreSQL / MariaDB)** — Benchmarks OLTP para bases de datos
- **MongoDB (PyMongo)** — Inserciones en base de datos NoSQL
- **nginx** — Rendimiento de servidor web

### Física y simulación
- **Bullet Physics Engine** — Simulación de física para juegos y animación
- **STARS Euler3D CFD** — Dinámica de fluidos computacional (CFD)
- **OpenRadioss** — Simulación de elementos finitos

### Rendimiento general / sistema
- **BYTE Unix Benchmark** — Suite clásica de rendimiento de sistema Unix
- **Geekbench 5 / 6** — Benchmark multi-plataforma de single y multi-core
- **Passmark CPU Mark** — Benchmark popular con 8 tipos de cargas distintas
- **Cinebench R23 / 2024** — Renderizado Cinema 4D, single y multi-core
- **Hashcat** — Cracking de contraseñas por fuerza bruta (mide operaciones criptográficas)
- **Stockfish** — Motor de ajedrez (mide rendimiento de IA/búsqueda)

---

## 3. ¿Qué benchmarks le convienen más a cada procesador?

### Intel Core i5-13600K
**Fortalezas:** Single-core muy alto, gaming, eficiencia precio/rendimiento, buen rendimiento multi-core para su segmento.

Los benchmarks que mejor reflejan sus ventajas:
- **Cinebench R23 Single-Core** — Lidera en single-thread entre los tres
- **Geekbench 6 Single-Core** — Excelente rendimiento por núcleo
- **7-Zip Compression** — Sólido con sus 14 núcleos a precio razonable
- **FFmpeg x264** — Buena relación núcleos/precio para transcoding
- **Timed Linux Kernel Compilation** — Competitivo con sus 20 hilos

### AMD Ryzen 9 5900X
**Fortalezas:** Excelente multi-core Zen 3, equilibrio entre frecuencia y núcleos, plataforma AM4 madura.

Los benchmarks que mejor reflejan sus ventajas:
- **Blender Multi-Core** — Destaca bien con sus 24 hilos Zen 3
- **7-Zip Compression/Decompression** — Muy competitivo en compresión paralela
- **Timed GCC / LLVM Compilation** — Beneficia de la latencia de caché Zen 3
- **NumPy / Scikit-learn** — Buen rendimiento en carga de datos científica
- **Cinebench R23 Multi-Core** — Muestra el músculo de sus 12 núcleos reales

### AMD Ryzen 9 7950X
**Fortalezas:** El más potente de los tres en multi-core, 16 núcleos Zen 4 a 5nm, mayor ancho de banda de memoria DDR5.

Los benchmarks que mejor reflejan sus ventajas:
- **Blender** — Domina las escenas pesadas por su cantidad de núcleos
- **Timed Linux Kernel / LLVM Compilation** — Escala perfectamente con 32 hilos
- **V-Ray / POV-Ray** — Renderizado profesional where se maximizan todos sus núcleos
- **SVT-AV1 / FFmpeg x265** — Codificación de video de alta exigencia
- **MLPerf Inference / Caffe** — Inferencia de IA en CPU
- **FFTW** — Simulación científica donde el ancho de banda de memoria importa

---

## 4. Tabla: Tareas cotidianas vs. Benchmark representativo

| Tarea diaria | Benchmark representativo | ¿Por qué lo mide? |
|---|---|---|
| Compilar un proyecto grande (kernel, LLVM, app) | **Timed Linux Kernel Compilation** | Mide el tiempo real de compilación paralela, escala con núcleos e hilos |
| Renderizar una escena 3D en Blender | **Blender (Monster / Junkshop)** | Usa todos los núcleos CPU para ray tracing de tiles de forma sostenida |
| Exportar/transcodificar video | **FFmpeg (libx264 / libx265)** | Simula codificación de video en condiciones de streaming o producción |
| Comprimir/descomprimir archivos grandes | **7-Zip Compression** | Altamente paralelo, mide compresión lossless real con p7zip |
| Entrenar o ejecutar modelos de ML | **MLPerf Inference / Scikit-learn** | Evalúa inferencia de redes y operaciones de aprendizaje automático en CPU |
| Ejecutar scripts de Python con cálculo numérico | **NumPy Benchmark / PyHPC** | Mide operaciones matriciales, que son el núcleo de numpy/scipy |
| Simular física en un motor de juego o animación | **Bullet Physics Engine** | Replica carga de motor de física en tiempo real |
| Consultas pesadas a base de datos | **HammerDB / PostgreSQL** | Simula transacciones OLTP concurrentes sobre base de datos relacional |
| Gaming (rendimiento en juegos) | **Cinebench Single-Core / Geekbench Single** | La frecuencia y rendimiento single-core domina en gaming |
| Renderizado de arquitectura / diseño industrial | **V-Ray / POV-Ray** | Renderizado profesional que explota todos los núcleos disponibles |
| Trabajar con archivos de video grandes (edición) | **FFmpeg + 7-Zip** | Combina la carga de I/O con CPU para operaciones sobre video |
| Correr simulaciones de dinámica de fluidos | **STARS Euler3D CFD / OpenRadioss** | Simula cargas FEM y CFD reales usadas en ingeniería |
| Criptografía / seguridad / hashing | **Hashcat** | Mide operaciones de hash por segundo, relevante en seguridad ofensiva/defensiva |
| Ejecutar servidores web de alto tráfico | **nginx benchmark** | Mide peticiones concurrentes por segundo bajo carga de servidor |
| Análisis de datos con pandas/scipy | **SciMark 2.0 / NumPy** | Mide FFT, álgebra lineal y Monte Carlo, bases de scipy/pandas |

---

## 5. Conclusión general

| Perfil de usuario | CPU recomendado | Benchmark clave |
|---|---|---|
| Gamer / usuario general con presupuesto ajustado | **i5-13600K** | Cinebench Single, Geekbench Single |
| Desarrollador / workstation equilibrada | **Ryzen 9 5900X** | Timed GCC, 7-Zip, Blender |
| Creador de contenido / científico / IA | **Ryzen 9 7950X** | Blender, SVT-AV1, MLPerf, FFTW |

El **Ryzen 9 7950X** lidera en cargas masivamente paralelas. El **i5-13600K** es el rey del rendimiento single-core por precio. El **Ryzen 9 5900X**, aunque más antiguo, sigue siendo un excelente equilibrio para trabajo productivo cotidiano.

---

*Datos basados en OpenBenchmarking.org (Phoronix Test Suite) y comparativas técnicas públicas actualizadas a 2025-2026.*
