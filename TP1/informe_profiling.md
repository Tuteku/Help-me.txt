# Informe de Performance y Profiling -  Help-me.txt

## 1\. Análisis de Benchmarks de Hardware

En esta sección se evalúa el rendimiento de diversos procesadores en la tarea de compilación del Kernel de Linux, utilizando datos de referencia de **OpenBenchmarking (PTS)**.

| Procesador | Núcleos / Hilos | Rendimiento (Compilación Kernel) |
| :--- | :--- | :--- |
| **Intel Core i5-13600K** | 14 Cores / 20 Hilos | \~78 segundos |
| **AMD Ryzen 9 5900X** | 12 Cores / 24 Hilos | \~72 segundos |
| **AMD Ryzen 9 7950X** | 16 Cores / 32 hilos | \~48 segundos |

**Conclusión de Hardware:** El análisis permite observar que la cantidad de núcleos físicos y la gestión de hilos son determinantes en tareas de compilación paralela. El **Ryzen 9 7950X** presenta la mayor eficiencia, logrando una aceleración ($S$) de aproximadamente **1.5x** respecto al modelo 5900X, reduciendo el tiempo de ejecución en un 33% mediante la explotación de su arquitectura de 16 núcleos.

-----

## 2\. Validación Experimental de Performance (Profiling)

El objetivo de esta etapa es validar experimentalmente el tiempo de ejecución y el uso de CPU mediante herramientas de perfilado. Se utilizó **GNU Gprof** para analizar el comportamiento interno del software.

### Metodología de Perfilado

1.  **Compilación:** Se habilitó la generación de perfiles mediante el flag `-pg` durante las etapas de compilación y enlace.
2.  **Ejecución:** Se ejecutó el binario para producir el archivo de datos estadísticos `gmon.out`.
3.  **Procesamiento:** Se utilizó la herramienta `gprof` para transformar los datos en reportes legibles: .

### Análisis de Resultados en Directorio `scripts/`

Dentro de la carpeta `scripts`, se almacenaron las distintas variantes del perfilado utilizando flags de personalización:

#### A. Perfil Plano (Flat Profile) - `gprof -p -b`

Este reporte brinda una descripción general del tiempo consumido por cada función:.

  * **func1:** Identificada como el principal cuello de botella con un **56.50%** del tiempo total de ejecución.
  * **func2:** Representa el **36.61%** del tiempo, siendo la segunda función con mayor carga.
  * **Uso del flag `-b`:** Se utilizó para eliminar la verbosidad de los textos explicativos y obtener una tabla de datos limpia.

#### B. Supresión de Funciones Estáticas - `gprof -a`

  * Al ejecutar el perfilado con el flag `-a`, se suprimió la información relacionada con funciones declaradas estáticamente (privadas).
  * En este caso, **func2** desaparece del informe, permitiendo centrar el análisis exclusivamente en las funciones públicas del sistema.

#### C. Gráfico de Llamadas (Call Graph)

El gráfico de llamadas se enfoca en las relaciones jerárquicas:.

  * Determina que `main` es la raíz del programa, delegando la mayor parte del tiempo de CPU a `func1`: 
  * El tiempo reportado en `children` para `func1` coincide con el tiempo `self` de sus subrutinas, validando la propagación correcta del consumo de recursos.

-----

## 3\. Visualización de Perfilado

Para una interpretación intuitiva de los "hotspots" del código, se generó un diagrama de flujo utilizando la herramienta `gprof2dot`.

**Conclusiones Finales:**
El uso de perfiladores permite trascender la intuición algorítmica y obtener datos reales del hardware. La validación experimental confirma que `func1` requiere optimización prioritaria. La combinación de Git para la sincronización de estos resultados asegura que cada integrante del grupo **Help-me.txt** posea un registro individual y actualizado del rendimiento del proyecto.