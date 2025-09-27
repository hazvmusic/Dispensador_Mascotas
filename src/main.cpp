#include <Arduino.h>
#include <WiFi.h>
#include "credentials.h" 

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configurar el pin del LED integrado como salida
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW); // Apagar el LED al inicio

  // Conectar a WiFi
  Serial.println("Conectando a WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(2, HIGH); // LED encendido mientras intenta conectar
    delay(250);
    digitalWrite(2, LOW);
    delay(250);
    Serial.print(".");
  }

  // LED encendido al conectarse
  digitalWrite(2, HIGH);
  Serial.println("");
  Serial.println("¡Conectado a WiFi!");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Parpadeo del LED cada segundo
  digitalWrite(2, HIGH);
  delay(1000);
  digitalWrite(2, LOW);
  delay(1000);
}
