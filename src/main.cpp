#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <time.h>

// =========================
// CONFIGURACIÓN WIFI
// =========================
const char *ssid = "Xandar";
const char *password = "Nova89P13";

// ---------- IP FIJA ----------
IPAddress local_IP(172, 20, 10, 5);
IPAddress gateway(172, 20, 10, 1);
IPAddress subnet(255, 255, 255, 0);

// Servidor Web
WebServer server(80);

// WebSocket en puerto 81
WebSocketsServer webSocket = WebSocketsServer(81);

// =========================
// PINES DEL PROYECTO
// =========================
#define LED1_PIN 19
#define LED2_PIN 21

#define POWER_PIN 17
#define SIGNAL_PIN 36
#define THRESHOLD 1000

int value = 0;
bool manualOverride = false;
bool manualLedState = false;

bool led2State = false;

// Control de pulsos temporizados
unsigned long pulseEndTime = 0;
bool pulseActive = false;

// =========================
// CONFIGURACIÓN NTP
// =========================
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -6 * 3600;
const int daylightOffset_sec = 0;

// ======================================================
// HTML CON WEBSOCKETS
// ======================================================
String webpage = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Control LED + Sensor</title>
<style>
*{margin:0;padding:0;box-sizing:border-box;font-family:'Segoe UI';}
body{
  background:linear-gradient(135deg,#74ebd5,#9face6);
  min-height:100vh;display:flex;flex-direction:column;justify-content:space-between;color:#333;
}
header{
  background:rgba(255,255,255,0.2);
  backdrop-filter:blur(10px);
  padding:1.5rem;text-align:center;color:#fff;font-size:1.8rem;font-weight:bold;
  box-shadow:0 2px 10px rgba(0,0,0,0.2);
}
main{flex:1;display:flex;flex-direction:column;align-items:center;justify-content:center;}
.button{
  padding:1rem 2rem;margin:1rem;
  font-size:1.2rem;font-weight:bold;border:none;border-radius:12px;cursor:pointer;
  transition:all 0.3s ease;box-shadow:0 5px 15px rgba(0,0,0,0.2);
}
.on{background:#28a745;color:white;}
.off{background:#dc3545;color:white;}
.auto{background:#007bff;color:white;}
.button:hover{transform:scale(1.1);}
footer{text-align:center;padding:1rem;background:rgba(255,255,255,0.2);color:#fff;font-size:0.9rem;}
</style>
</head>
<body>

<header>⚡ Control Dispensador para Mascotas ⚡</header>

<main>
  <h2>Dispensador de Comida</h2>

  <!-- Botones existentes -->
  <button class="button on" onclick="sendCmd('led1_on')">Encender Dispensador de Comida</button>
  <button class="button off" onclick="sendCmd('led1_off')">Apagar Dispensador de Comida</button>

  <!-- NUEVOS botones temporizados -->
  <button class="button on" onclick="sendCmd('pulse_25')">Automatico Mascota Pequeña</button>
  <button class="button on" onclick="sendCmd('pulse_50')">Automatico Mascota Grande</button>

  <h2>Dispensador de Agua</h2>
  <button class="button on" onclick="sendCmd('led2_on')">Encender Dispensador de Agua</button>
  <button class="button off" onclick="sendCmd('led2_off')">Apagar Dispensador de Agua</button>

</main>

<script>
let socket = new WebSocket("ws://" + location.hostname + ":81/");
socket.onopen = () => console.log("WS Conectado");
socket.onmessage = (event) => console.log("Mensaje:", event.data);

function sendCmd(cmd){
    socket.send(cmd);
}
</script>

<footer>Elaborado por Arturo, Joel, Raymundo y Sebastian</footer>
</body>
</html>
)rawliteral";

// ======================================================
// WEBSOCKET HANDLER
// ======================================================
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

    if (type == WStype_TEXT)
    {

        String cmd = String((char *)payload);
        Serial.print("WebSocket comando: ");
        Serial.println(cmd);

        // LED 1 normal
        if (cmd == "led1_on")
        {
            manualOverride = true;
            pulseActive = false;
            digitalWrite(LED1_PIN, HIGH);
        }
        else if (cmd == "led1_off")
        {
            manualOverride = true;
            pulseActive = false;
            digitalWrite(LED1_PIN, LOW);
        }

        // NUEVOS: pulsos temporizados
        else if (cmd == "pulse_25")
        {
            manualOverride = true;
            pulseActive = true;
            pulseEndTime = millis() + 2500; // 2.5 segundos
            digitalWrite(LED1_PIN, HIGH);
            Serial.println("LED 1 encendido por 2.5 segundos");
        }
        else if (cmd == "pulse_50")
        {
            manualOverride = true;
            pulseActive = true;
            pulseEndTime = millis() + 5000; // 5 segundos
            digitalWrite(LED1_PIN, HIGH);
            Serial.println("LED 1 encendido por 5 segundos");
        }

        // LED 2
        else if (cmd == "led2_on")
        {
            led2State = true;
            digitalWrite(LED2_PIN, HIGH);
        }
        else if (cmd == "led2_off")
        {
            led2State = false;
            digitalWrite(LED2_PIN, LOW);
        }

        else if (cmd == "auto")
        {
            manualOverride = false;
            pulseActive = false;
        }
    }
}

// ======================================================
// CONFIGURACIÓN INICIAL
// ======================================================
void setup()
{
    Serial.begin(115200);

    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    digitalWrite(POWER_PIN, LOW);

    WiFi.config(local_IP, gateway, subnet);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(200);
    }

    Serial.println("WiFi conectado!");

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    server.on("/", []()
              { server.send(200, "text/html", webpage); });
    server.begin();

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    Serial.println("Servidor listo!");
}

// ======================================================
// LOOP PRINCIPAL
// ======================================================
void loop()
{

    webSocket.loop();
    server.handleClient();

    // Control del pulso temporizado
    if (pulseActive && millis() > pulseEndTime)
    {
        digitalWrite(LED1_PIN, LOW);
        pulseActive = false;
        Serial.println("Pulso terminado, LED1 apagado");
    }

    // Lectura del sensor en modo automático
    if (!manualOverride && !pulseActive)
    {

        digitalWrite(POWER_PIN, HIGH);
        delay(5);
        value = analogRead(SIGNAL_PIN);
        digitalWrite(POWER_PIN, LOW);

        int percentage = map(value, 0, 4095, 100, 0);
        Serial.printf("Humedad sensor: %d | %d%%\n", value, percentage);

        // -----------------------
        // LED 1
        // -----------------------
        if (value < THRESHOLD)
            digitalWrite(LED1_PIN, LOW);
        else
            digitalWrite(LED1_PIN, HIGH);

        // -----------------------
        // LED 2 INVERTIDO
        // -----------------------
        if (percentage == 0)
            digitalWrite(LED2_PIN, HIGH); // Sin humedad → ENCENDIDO
        else
            digitalWrite(LED2_PIN, LOW); // Con humedad → APAGADO
    }
}