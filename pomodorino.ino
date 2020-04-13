#include "constants.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIN D4
#define NUMPIXELS 24.0
#define MINUTE 60 * 1000
#define POMO_WORK 25.0
#define POMO_SBREAK 5.0
#define POMO_LBREAK 15.0
#define POMO_WORK_REP 4
#define MIN_BRIGHTNESS 1
#define MAX_BRIGHTNESS 255
#define STOP_REPEAT 4
#define UTC_ADJUSTMENT 2
#define UPDATE_INTERVAL 60


const float WORK_DELAY = POMO_WORK / NUMPIXELS * MINUTE;
const float SBREAK_DELAY = POMO_SBREAK / NUMPIXELS * MINUTE;
const float LBREAK_DELAY = POMO_LBREAK / NUMPIXELS * MINUTE;
int workCounter = 0;

// LED
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", UTC_ADJUSTMENT * 3600, 60000); // Time server pool and the offset, (in seconds), update interval (in milliseconds)

// ArduinoJson
const size_t capacity = JSON_OBJECT_SIZE(3);
DynamicJsonDocument doc(capacity);

int lastUpdate = 0;

void setup()
{
  Serial.begin(9600);

  // WIFI initialization
  Serial.println("--- WIFI INIT ---");
  WiFi.begin(ssid, password);
  Serial.println("Connecting to " + String(ssid));
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: " + WiFi.localIP().toString());
  Serial.println();

  // NTP initialization
  timeClient.begin();
  timeClient.update();

// LED initialization
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  pixels.begin();
  pixels.clear();
  pixels.show();
}

void loop()
{
  workCounter++;
  if (workCounter > POMO_WORK_REP)
  {
    flash(pixels.Color(255, 0, 255));
    fillDecrement(pixels.Color(255, 0, 255), LBREAK_DELAY, "lbreak");
    workCounter = 0;
  }
  else
  {
    flash(pixels.Color(0, 255, 0));
    fillDecrement(pixels.Color(0, 255, 0), WORK_DELAY, "work");

    flash(pixels.Color(255, 0, 0));
    fillDecrement(pixels.Color(255, 0, 0), SBREAK_DELAY, "sbreak");
  }
}

void sendUpdate(long epochTime, float duration, const char *pomoType)
{
  Serial.println("Sending update " + String(timeClient.getFormattedTime()));
  // HTTP Client
  HTTPClient http;
  http.begin(POST_URL);
  http.addHeader("Content-Type", "application/json");

  String json;
  doc["timeEnd"] = epochTime;
  doc["duration"] = duration;
  doc["type"] = pomoType;
  serializeJson(doc, json);
  Serial.println(json);
  http.POST(json);
  Serial.println(http.getString());
  http.end();
}

void flash(uint32_t color)
{
  // stop
  pixels.setBrightness(MAX_BRIGHTNESS);
  for (int i = 0; i < STOP_REPEAT; i++)
  {
    pixels.fill(color);
    pixels.show();
    delay(500);
    pixels.clear();
    pixels.show();
    delay(500);
  }
}

void fillDecrement(uint32_t color, float delayDuration, char *stepName)
{
  pixels.setBrightness(MIN_BRIGHTNESS);
  for (int i = 0; i < NUMPIXELS; i++)
  {
    pixels.setPixelColor(i, color);
    pixels.show();
    delay(50);
  }
  for (int i = NUMPIXELS; i >= 0; i--)
  {
    delay(delayDuration);
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    pixels.show();
  }
  sendUpdate(timeClient.getEpochTime(), delayDuration * NUMPIXELS / 1000, stepName);
}