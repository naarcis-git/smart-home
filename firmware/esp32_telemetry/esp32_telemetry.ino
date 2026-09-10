/*
 * Smart Home — ESP32 telemetry and automatic lighting
 * Bachelor's thesis, Faculty of Robotics, Politehnica University of Timisoara, 2022.
 *
 * Board:  ESP32 Wemos Lolin32
 * Sensors: DHT11 (temperature + humidity), photoresistor (ambient light)
 * Outputs: four indoor LEDs
 * Uplink:  Blynk over Wi-Fi — V4 temperature, V5 humidity, V0 light reading
 *
 * Credentials live in secrets.h, which is NOT committed.
 * Copy secrets.example.h to secrets.h and fill in your own values.
 */

#include "secrets.h"

#define BLYNK_TEMPLATE_ID BLYNK_TEMPLATE_ID_VALUE
#define BLYNK_DEVICE_NAME BLYNK_DEVICE_NAME_VALUE
#define BLYNK_AUTH_TOKEN  BLYNK_AUTH_TOKEN_VALUE
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

// --- pins -------------------------------------------------------------
#define SENSOR_LIGHT_PIN 36     // photoresistor, analogue in
#define DHTPIN            2
#define DHTTYPE       DHT11

const int LED  = 5;
const int LED2 = 4;
const int LED3 = 16;
const int LED4 = 17;

// --- behaviour --------------------------------------------------------
#define ANALOG_THRESHOLD 200    // below this the room counts as dark
#define SAMPLE_INTERVAL_MS 1000 // one telemetry push per second

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASS;

DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;

// Read the sensors and push the three values to the dashboard.
void sendSensor()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  float sensorValue = analogRead(A0);
  float voltage = sensorValue * (0.5 / 1023.0);
  Serial.print("Voltage ");
  Serial.print(voltage);
  Serial.println(" V");

  if (isnan(h) || isnan(t))
  {
    Serial.println("Failed to read the temperature and humidity sensor.");
    return;
  }

  Blynk.virtualWrite(V5, h);        // humidity, %
  Blynk.virtualWrite(V4, t);        // temperature, degrees C
  Blynk.virtualWrite(V0, voltage);  // light reading
}

void setup()
{
  Serial.begin(115200);

  pinMode(LED,  OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);

  Blynk.begin(auth, ssid, pass);
  dht.begin();

  timer.setInterval(SAMPLE_INTERVAL_MS, sendSensor);
}

void loop()
{
  int analogValue = analogRead(SENSOR_LIGHT_PIN);

  if (analogValue < ANALOG_THRESHOLD)
  {
    // dark: front lamps on
    digitalWrite(LED,  HIGH);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    digitalWrite(LED4, HIGH);
  }
  else
  {
    // bright enough: the other pair takes over
    digitalWrite(LED,  LOW);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    digitalWrite(LED4, LOW);
  }

  Blynk.run();
  timer.run();
}
