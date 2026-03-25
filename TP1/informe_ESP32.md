
Simulacion realizada en https://wokwi.com/projects/new/esp32 

---
### Codigo de Prueba
```
void ejecutarPrueba(const char* tipo) {

  long inicio = millis();

  if (tipo == "entero") {

    volatile long suma = 0;

    for (long i = 0; i < 100000; i++) {
      suma += i;

    }

  } else {

    volatile float sumaF = 0.0;

    for (long i = 0; i < 500000; i++) {
      sumaF += 1.1;

    }

  }

  long fin = millis();
  Serial.print("Tiempo [");
  Serial.print(tipo);
  Serial.print("]: ");
  Serial.print(fin - inicio);
  Serial.println(" ms");

}


void setup() {

  Serial.begin(115200);

  delay(1000);
  
  // PRUEBA 1: Frecuencia Baja (8 MHz)

  setCpuFrequencyMhz(8);
  Serial.println("FRECUENCIA: 8 MHz");
  Serial.print("Frecuencia actual: ");
  Serial.print(getCpuFrequencyMhz());
  Serial.println(" MHz");

  ejecutarPrueba("entero");
  ejecutarPrueba("float");

  delay(2000);

  // PRUEBA 2: Frecuencia Alta (16 MHz - El doble)

  setCpuFrequencyMhz(16);
  Serial.println("\nFRECUENCIA: 16 MHz (Duplicada)");
  Serial.print("Frecuencia actual: ");
  Serial.print(getCpuFrequencyMhz());

  Serial.println(" MHz");

  ejecutarPrueba("entero");
  ejecutarPrueba("float");

}

void loop() {}
```

## Output del codigo
FRECUENCIA: 8 MHz 
Frecuencia actual: 8 MHz
Tiempo [entero]: 78 ms
Tiempo [float]: 4984 ms

FRECUENCIA: 16 MHz (Duplicada)
Frecuencia actual: 8 MHz
Tiempo [entero]: 80 ms
Tiempo [float]: 4984 ms

## Analisis de Speedup

Si duplicas la frecuencia (80->160 MHz):
- El tiempo deberia reducirse a la mitad aproximadamente.
- Speedup esperado = 2x(teorico).

**Nota: Wokwi limita automaticamente el maximo simulado de la frecuencia de CPU (8MHz) e impide visualizar las diferencias.**
