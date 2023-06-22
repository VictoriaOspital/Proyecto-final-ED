#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

MAX30105 particleSensor;
const byte RATE_SIZE = 4; // Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE];    // Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; // Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;


const char* ssid = "wifi handle ";
const char* password = "password"; 


WebServer server(80);
DHT dht(14, DHT11);
OneWire oneWireObjeto(4);
DallasTemperature sensorDS18B20(&oneWireObjeto);
float tempDS18B20 = sensorDS18B20.getTempCByIndex(0);


void handleRoot() {
  float h, t, f;

  h = dht.readHumidity();
  t = dht.readTemperature();     // in Celsius
  f = dht.readTemperature(true); // in Fahrenheit

  if (isnan(h) || isnan(t) || isnan(f))   // Failed measurement?
  {
    server.send(500, "text/plain", "Error en la medición del sensor");
  }
  else
  {
    String html = "<html><body>";
    html += "<h1>Sensor Data</h1>";
    html += "<p>";
    html += "<span style='color:";
    
    // Cambiar el color de la letra a rojo si la temperatura es mayor a 37°C o el pulso es mayor a 120 BPM
    if (tempDS18B20 > 37 || beatsPerMinute > 120) {
      html += "red";
    } else {
      html += "black";
    }
    
    html += "'>";
    html += "BPM: " + String(beatsPerMinute) + "<br>";
    html += "Humidity: " + String(h) + "%<br>";
    html += "Temperature (C): " + String(t) + "<br>";
    html += "Temperature (F): " + String(f);
    html += "</span>";
    html += "</p>";
    html += "</body></html>";

    server.send(200, "text/html", html);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web server started");

  dht.begin();

  sensorDS18B20.begin();
}

void loop() {
  server.handleClient();

  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true) {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  sensorDS18B20.requestTemperatures();
  

  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);

  if (irValue < 50000)
    Serial.print(" No finger?");

  Serial.print(", Humidity=");
  Serial.print(h);
  Serial.print(", Temperature (C)=");
  Serial.print(t);
  Serial.print(", Temperature (F)=");
  Serial.print(f);
  Serial.print(", DS18B20 Temperature=");
  Serial.print(tempDS18B20);

  Serial.println();

  delay(2000);
}
