#ifdef ESP32
#include <WiFi.h>
#include <HTTPClient.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#endif

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include "Adafruit_SHT31.h"

#define BMP388_ADRESA (0x77)
#define SDA 2
#define SCL 1
// inicializace senzoru BMP z knihovny
Adafruit_BMP3XX bmp;
// inicializace senzoru SHT z knihovny
Adafruit_SHT31 sht31 = Adafruit_SHT31();
// vytvoření proměnné s naším AP klíčem
String apiKeyValue = "zadej_nejake_heslo"; //například: tPmAT5Ab3j7F9
// vytvoření proměnných s názvem WiFi sítě a heslem
const char* nazevWifi = "nazev_vasi_Wi-Fi";
const char* hesloWifi = "heslo_Wifi";
// vytvoření proměnné s názvem 
const char* serverName = "ip_adresa_severu"; //http://192.168.1.212/post-esp-data.php

// inicializace WiFi v módu klienta
WiFiClient client;

String sensorName = "SHT31BME388";
String sensorLocation = "out";

float tepl;
float vlh;
float teplota;
float tep;
float tlak;
float nadV;
float t;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Adafruit BMP388 test");

  if (!bmp.begin(BMP388_ADRESA)) {   // hardware I2C mode, can pass in address & alt Wire
    Serial.println("Nelze najít platný snímač BMP3, zkontrolujte zapojení!");
    while (1);
  }
  // Nastavte převzorkování a inicializaci filtru
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);

  Wire.begin();
  //SHT31 I2C Address a test připojení senzoru SHT31
  Serial.println("SHT31 test");
  if (! sht31.begin(0x44))   {     // Set to 0x45 for alternate i2c addr 
   Serial.println("nenalezen SHT31");
   while (1) delay(1);
  }

  // zahájení komunikace po sériové lince
  Serial.begin(9600);
  // zahájení bezdrátové WiFi komunikace s připojením
  // na router skrze zadané přihl. údaje
  WiFi.begin(nazevWifi, hesloWifi);
  // čekání na úspěšné připojení k routeru,
  // v průběhu čekání se vytiskne každých
  // 500 milisekund tečka po sériové lince
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Pripojeno k WiFi siti: ");
  Serial.println(nazevWifi);
  Serial.print("IP adresa: ");
  Serial.println(WiFi.localIP());

}
void loop() {
  if (! bmp.performReading()) {
    Serial.println("Čtení se nezdařilo :(");
    return;
  }
  tepl = (bmp.temperature);
  tlak = (bmp.pressure / 100.0);
  //float P0= tlak/pow(1.0-0.0065*187/(tepl+273.15), 5.255); chyba!!!  pomocí ICAO
  float P0= -tlak * (187 + 16000.0 + 64.0 * teplota) / (187 - 16000.0 - 64.0 * teplota); //pomocí Babinet

  if (P0 >= 900) {
   teplota = sht31.readTemperature(); 
   vlh = sht31.readHumidity(); 

   Serial.print("TEPLOTA:");
   Serial.print(sht31.readTemperature());
   Serial.print("\t");
   Serial.print("VLHKOST:");
   Serial.println(sht31.readHumidity());
   Serial.print("TEPLO = ");
   Serial.print(bmp.temperature);
   Serial.println(" *C");

   Serial.print("TLAK = ");
   Serial.print(bmp.pressure / 100.0);
   Serial.println(" hPa");
   Serial.println();

   if (WiFi.status() == WL_CONNECTED) {
     WiFiClient client;
     HTTPClient http;

     http.begin(serverName);
     http.addHeader("Content-Type", "application/x-www-form-urlencoded");

     String httpRequestData = "api_key=";
     httpRequestData += apiKeyValue;
     httpRequestData += "&sensor=";
     httpRequestData += sensorName;
     httpRequestData += "&location=";
     httpRequestData += sensorLocation;
     httpRequestData += "&value1=";
     httpRequestData += String(teplota);
     httpRequestData += "&value2=";
     httpRequestData += String(vlh);
     httpRequestData += "&value3=";
     httpRequestData += String(P0);

     Serial.print("httpRequestData: ");
     Serial.println(httpRequestData);

     int httpResponseCode = http.POST(httpRequestData);

     if (httpResponseCode > 0) {
       Serial.print("HTTP Response code: ");
       Serial.println(httpResponseCode);
      }
     else {
       Serial.print("Chybový kód: ");
       Serial.println(httpResponseCode);
      }
     http.end();
    }
   else {
     Serial.print("WiFi odpojeno");
    }
  }
  delay(300000);
}
