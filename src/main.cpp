#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>

MAX30105 particleSensor;
const byte RATE_SIZE = 4; // Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE];    // Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; // Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;

const char* ssid = "Fibertel WiFi376 2.4GHz";
const char* password = "01423472526"; 

WebServer server(80);

// DHT11 Temperature and Humidity Sensor
const int DHTPIN = 14;
const int DHTTYPE = DHT11;
DHT dht(DHTPIN, DHTTYPE);

// DS18B20 Temperature Sensor
const int pinDatosDQ = 4;
OneWire oneWireObjeto(pinDatosDQ);
DallasTemperature sensorDS18B20(&oneWireObjeto);
float temperatureDS18B20;

void handleRoot() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  String html = "<html><body>";
  html += "<h1>BPM Actual:</h1>";
  html += "<h2 style='color:";
  html += (beatsPerMinute > 120) ? "red" : "black";
  html += "'>" + String(beatsPerMinute) + "</h2>";
  html += "<h1>BPM Promedio:</h1>";
  html += "<h2>" + String(beatAvg) + "</h2>";
  html += "<h1>Humedad y Temperatura Ambiental:</h1>";
  html += "<h2>Humedad: " + String(h) + "%</h2>";
  html += "<h2>Temperatura: " + String(t) + "°C</h2>";
  html += "<h1>Temperatura Corporal:</h1>";
  html += "<h2>Temperatura: " + String(temperatureDS18B20) + "°C</h2>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Initialize MAX30105 sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 was not found. Please check wiring/power.");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);

  // Initialize DHT11 sensor
  dht.begin();

  // Initialize DS18B20 sensor
  sensorDS18B20.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();

  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true) {
    lastBeat = millis();
    beatsPerMinute = 60 / ((lastBeat - lastBeat) / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  sensorDS18B20.requestTemperatures();
  temperatureDS18B20 = sensorDS18B20.getTempCByIndex(0);

  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(" Error en la medición del sensor DHT11");
  } else {
    Serial.print(", Humedad=");
    Serial.print(h);
    Serial.print("%, Temperatura=");
    Serial.print(t);
    Serial.print("°C");
  }

  Serial.print(", Temperatura (DS18B20)=");
  Serial.print(temperatureDS18B20);
  Serial.println("°C");

  if (irValue < 50000)
    Serial.print(" No finger?");

  delay(1000);
}
