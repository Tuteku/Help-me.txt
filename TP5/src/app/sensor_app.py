#!/usr/bin/env python3
"""
sensor_app.py - Servidor web para visualizacion del CDD

Lee datos del Character Device Driver (/dev/sensor_cdd) y los sirve
a traves de una interfaz web accesible desde el navegador de la PC host.

Este enfoque es ideal para Raspberry Pi sin entorno grafico (como la Pi Zero),
ya que toda la visualizacion ocurre en el navegador del host.

Arquitectura:
    [RPi] sensor_drv.ko  -->  /dev/sensor_cdd  -->  sensor_app.py (servidor)
                                                          |
    [PC Host]                     navegador  <--- http://ip_rpi:8080

Uso en la RPi:
    python3 sensor_app.py

Luego abrir en el navegador del host:
    http://<IP_DE_LA_RPI>:8080

No requiere pip install: usa solo la biblioteca estandar de Python 3.
"""

import os
import sys
import json
import time
import threading
from http.server import HTTPServer, BaseHTTPRequestHandler

# =====================================================================
# Configuracion
# =====================================================================
DEVICE_PATH = "/dev/signal_cdd"
HTTP_PORT = 8080
INTERVALO_LECTURA = 1.0   # segundos

# Correccion de escala a nivel de usuario (segun consigna).
# GPIO de RPi: 0 logico = 0V, 1 logico = 3.3V
SEÑALES = {
    0: {"nombre": "Señal cuadrada (CH1)", "unidad": "V", "escala": 3.3, "offset": 0.0},
    1: {"nombre": "Señal cuadrada (CH2)", "unidad": "V", "escala": 3.3, "offset": 0.0},
}

MAX_PUNTOS = 120   # maximo de muestras almacenadas


# =====================================================================
# Estado global de las mediciones
# =====================================================================
class EstadoSensor:
    def __init__(self):
        self.lock = threading.Lock()
        self.señal_activa = 0
        self.tiempos = []
        self.valores = []
        self.tiempo_inicio = None

    def agregar_muestra(self, valor_crudo):
        with self.lock:
            if self.tiempo_inicio is None:
                self.tiempo_inicio = time.time()

            config = SEÑALES[self.señal_activa]
            valor_escalado = (valor_crudo * config["escala"]) + config["offset"]

            t = round(time.time() - self.tiempo_inicio, 1)
            self.tiempos.append(t)
            self.valores.append(valor_escalado)

            if len(self.tiempos) > MAX_PUNTOS:
                self.tiempos.pop(0)
                self.valores.pop(0)

    def resetear(self):
        with self.lock:
            self.tiempos.clear()
            self.valores.clear()
            self.tiempo_inicio = None

    def obtener_datos(self):
        with self.lock:
            config = SEÑALES[self.señal_activa]
            return {
                "tiempos": list(self.tiempos),
                "valores": list(self.valores),
                "señal_activa": self.señal_activa,
                "nombre": config["nombre"],
                "unidad": config["unidad"],
            }

    def cambiar_señal(self, nueva):
        if nueva not in (0, 1):
            return False
        try:
            with open(DEVICE_PATH, "w") as f:
                f.write(str(nueva))
        except Exception as e:
            print(f"Error escribiendo al CDD: {e}")
            return False

        self.señal_activa = nueva
        self.resetear()
        print(f"[CDD] Señal activa: {nueva} ({SEÑALES[nueva]['nombre']})")
        return True


estado = EstadoSensor()


# =====================================================================
# Hilo de lectura del CDD
# =====================================================================
def hilo_lectura():
    while True:
        try:
            with open(DEVICE_PATH, "r") as f:
                dato = f.read().strip()
                valor = int(dato)
                estado.agregar_muestra(valor)
        except Exception as e:
            print(f"Error leyendo CDD: {e}")
        time.sleep(INTERVALO_LECTURA)


# =====================================================================
# Pagina HTML con graficacion en Chart.js
# =====================================================================
HTML_PAGE = """<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>TP5 - Sensado de señales</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', system-ui, sans-serif;
            background: #1a1a2e;
            color: #e0e0e0;
            min-height: 100vh;
        }
        .header {
            background: #16213e;
            padding: 20px 30px;
            border-bottom: 2px solid #0f3460;
        }
        .header h1 { font-size: 1.4em; color: #e94560; }
        .header p { font-size: 0.9em; color: #888; margin-top: 4px; }
        .container { max-width: 1000px; margin: 0 auto; padding: 20px; }
        .controls {
            display: flex; gap: 12px; margin-bottom: 20px;
            align-items: center; flex-wrap: wrap;
        }
        .btn {
            padding: 10px 24px; border: 2px solid #0f3460;
            border-radius: 8px; cursor: pointer;
            font-size: 1em; font-weight: 600;
            background: #16213e; color: #e0e0e0;
            transition: all 0.2s;
        }
        .btn:hover { background: #0f3460; }
        .btn.active-ch0 { background: #2196F3; border-color: #2196F3; color: #fff; }
        .btn.active-ch1 { background: #FF5722; border-color: #FF5722; color: #fff; }
        .status {
            margin-left: auto; padding: 8px 16px;
            background: #0a0a1a; border-radius: 6px;
            font-family: monospace; font-size: 0.9em;
        }
        .chart-container {
            background: #16213e; border-radius: 12px;
            padding: 20px; border: 1px solid #0f3460;
        }
        .info {
            display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 12px; margin-top: 16px;
        }
        .info-card {
            background: #0a0a1a; padding: 14px 18px;
            border-radius: 8px; border-left: 3px solid #0f3460;
        }
        .info-card .label { font-size: 0.8em; color: #888; }
        .info-card .value { font-size: 1.3em; font-weight: 600; margin-top: 4px; }
    </style>
</head>
<body>
    <div class="header">
        <h1>TP5 — Character Device Driver</h1>
        <p>Sensado de señales digitales via GPIO · Sistemas de Computación · UNC</p>
    </div>
    <div class="container">
        <div class="controls">
            <button class="btn active-ch0" id="btn-ch0" onclick="cambiarSenal(0)">
                Canal 1 (CH1)
            </button>
            <button class="btn" id="btn-ch1" onclick="cambiarSenal(1)">
                Canal 2 (CH2)
            </button>
            <div class="status" id="status">Conectando...</div>
        </div>

        <div class="chart-container">
            <canvas id="chart"></canvas>
        </div>

        <div class="info">
            <div class="info-card">
                <div class="label">Señal activa</div>
                <div class="value" id="info-nombre">—</div>
            </div>
            <div class="info-card">
                <div class="label">Último valor</div>
                <div class="value" id="info-valor">—</div>
            </div>
            <div class="info-card">
                <div class="label">Muestras</div>
                <div class="value" id="info-muestras">0</div>
            </div>
            <div class="info-card">
                <div class="label">Período de muestreo</div>
                <div class="value">1 s</div>
            </div>
        </div>
    </div>

    <script>
    const COLORES = { 0: '#2196F3', 1: '#FF5722' };
    let señalActiva = 0;

    // Configurar Chart.js
    const ctx = document.getElementById('chart').getContext('2d');
    const chart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Señal cuadrada (CH1)',
                data: [],
                borderColor: COLORES[0],
                backgroundColor: COLORES[0] + '33',
                borderWidth: 2,
                stepped: 'after',        // grafico tipo escalera
                pointRadius: 3,
                pointBackgroundColor: COLORES[0],
                fill: true,
            }]
        },
        options: {
            responsive: true,
            animation: { duration: 200 },
            scales: {
                x: {
                    title: { display: true, text: 'Tiempo [s]', color: '#aaa', font: { size: 14 } },
                    ticks: { color: '#888' },
                    grid: { color: '#ffffff10' }
                },
                y: {
                    title: { display: true, text: 'Tensión [V]', color: '#aaa', font: { size: 14 } },
                    min: -0.5, max: 4.0,
                    ticks: {
                        color: '#888',
                        callback: function(val) {
                            if (val === 0) return '0 V (LOW)';
                            if (val === 3.3) return '3.3 V (HIGH)';
                            return '';
                        },
                        stepSize: 0.5,
                    },
                    grid: { color: '#ffffff10' }
                }
            },
            plugins: {
                legend: { labels: { color: '#ccc', font: { size: 13 } } },
                title: {
                    display: true,
                    text: 'Señal sensada: Señal cuadrada (CH1)',
                    color: '#eee',
                    font: { size: 16 }
                }
            }
        }
    });

    // Polling: pedir datos al servidor cada 1 segundo
    async function actualizarDatos() {
        try {
            const resp = await fetch('/api/datos');
            const datos = await resp.json();

            chart.data.labels = datos.tiempos.map(t => t.toFixed(0));
            chart.data.datasets[0].data = datos.valores;
            chart.data.datasets[0].label = datos.nombre;
            chart.data.datasets[0].borderColor = COLORES[datos.señal_activa];
            chart.data.datasets[0].pointBackgroundColor = COLORES[datos.señal_activa];
            chart.data.datasets[0].backgroundColor = COLORES[datos.señal_activa] + '33';

            chart.options.plugins.title.text = 'Señal sensada: ' + datos.nombre;
            chart.options.scales.y.title.text = 'Tensión [' + datos.unidad + ']';

            chart.update();

            // Actualizar info cards
            document.getElementById('info-nombre').textContent = datos.nombre;
            document.getElementById('info-muestras').textContent = datos.valores.length;
            if (datos.valores.length > 0) {
                const ultimo = datos.valores[datos.valores.length - 1];
                document.getElementById('info-valor').textContent = ultimo.toFixed(1) + ' ' + datos.unidad;
            }
            document.getElementById('status').textContent =
                'Conectado · ' + datos.valores.length + ' muestras';

        } catch (err) {
            document.getElementById('status').textContent = 'Error de conexión';
        }
    }

    // Cambiar de señal: POST al servidor
    async function cambiarSenal(num) {
        try {
            await fetch('/api/cambiar', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({señal: num})
            });
            señalActiva = num;

            // Actualizar botones
            document.getElementById('btn-ch0').className =
                'btn' + (num === 0 ? ' active-ch0' : '');
            document.getElementById('btn-ch1').className =
                'btn' + (num === 1 ? ' active-ch1' : '');

        } catch (err) {
            console.error('Error cambiando señal:', err);
        }
    }

    // Polling cada 1 segundo
    setInterval(actualizarDatos, 1000);
    actualizarDatos();
    </script>
</body>
</html>"""


# =====================================================================
# Servidor HTTP
# =====================================================================
class Handler(BaseHTTPRequestHandler):
    """Maneja las peticiones HTTP del navegador."""

    def do_GET(self):
        if self.path == '/':
            # Servir la pagina principal
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.end_headers()
            self.wfile.write(HTML_PAGE.encode('utf-8'))

        elif self.path == '/api/datos':
            # Devolver los datos actuales como JSON
            datos = estado.obtener_datos()
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(json.dumps(datos).encode('utf-8'))

        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        if self.path == '/api/cambiar':
            # Recibir el cambio de señal desde el navegador
            content_len = int(self.headers.get('Content-Length', 0))
            body = self.rfile.read(content_len)
            try:
                data = json.loads(body)
                nueva = int(data.get('señal', 0))
                ok = estado.cambiar_señal(nueva)
                resp = {"ok": ok, "señal_activa": estado.señal_activa}
            except Exception as e:
                resp = {"ok": False, "error": str(e)}

            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(json.dumps(resp).encode('utf-8'))
        else:
            self.send_response(404)
            self.end_headers()

    def do_OPTIONS(self):
        # CORS preflight
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()

    def log_message(self, format, *args):
        # Silenciar logs de cada request para no llenar la consola
        pass


# =====================================================================
# Main
# =====================================================================
def main():
    # Verificar que el dispositivo exista
    if not os.path.exists(DEVICE_PATH):
        print(f"ERROR: {DEVICE_PATH} no existe.")
        print()
        print("En la Raspberry Pi, ejecutar:")
        print("  sudo insmod sensor_drv.ko")
        print("  sudo chmod 666 /dev/sensor_cdd")
        sys.exit(1)

    print("=" * 50)
    print("  TP5 - Servidor de sensado de señales")
    print("=" * 50)
    print()

    # Iniciar hilo de lectura del CDD
    t = threading.Thread(target=hilo_lectura, daemon=True)
    t.start()
    print(f"[OK] Leyendo {DEVICE_PATH} cada {INTERVALO_LECTURA}s")

    # Iniciar servidor HTTP
    server = HTTPServer(('0.0.0.0', HTTP_PORT), Handler)
    print(f"[OK] Servidor web en puerto {HTTP_PORT}")
    print()
    print(f"  Abrir en el navegador del host:")
    print(f"  http://<IP_DE_ESTA_RPI>:{HTTP_PORT}")
    print()
    print("  Ctrl+C para detener")
    print()

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nServidor detenido.")
        server.server_close()


if __name__ == "__main__":
    main()
