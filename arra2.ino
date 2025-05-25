//funkční program pro odeslání dat do MySQL
#include <Wire.h>
#include <BH1750.h>

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

// inicializace senzoru BH1750 z knihovny
BH1750 luxSenzor;

#define pinOut A0

// vytvoření proměnné s naším AP klíčem
String apiKeyValue = "zadej_nejake_heslo"; //například: TpmAT5Ab3j7F9
// vytvoření proměnných s názvem WiFi sítě a heslem
const char* nazevWifi = "zadej_jmeno_Wi-Fi";
const char* hesloWifi = "zadej_heslo_Wifi";
// vytvoření proměnné s názvem 
const char* serverName = "zadej_ip_adresu_serveru"; //například: http://192.168.1.212/post-esp-data1.php

// inicializace WiFi v módu klienta
WiFiClient client;

String sensorName = "ML8511BH1750"; //pojmenování zprávy
String sensorLocation = "out_svetlo";

void setup() {
  Serial.begin(9600);
  // nastavení propojovacích pinů jako vstupních
  pinMode(pinOut, INPUT);
  // zapnutí komunikace se senzorem BH1750
  luxSenzor.begin();
  // pauza před zahájením měření
  delay(200);
  // vytvoření proměnné pro uložení naměřených údajů
  // a načtení aktuální intenzity světla
  uint16_t lux = luxSenzor.readLightLevel();
  // vytištění výsledku po sériové lince
  //Serial.print("Intenzita svetla: ");
  //Serial.print(lux);
  //Serial.println(" luxu.");

  int hodnotaUV = prumerAnalogRead(pinOut); //načtení hodnoty ze senzoru UV
  // přepočet načteného napětí z UV senzoru a napětí
  // z 3,3 V Arduina pro přesnou hodnotu z UV senzoru
  float napetiOutUV = 3.3 / 1020 * hodnotaUV; //baterie - 3.29V = 1020, kabel - 3.327V = 1031
  // přepočet napětí z UV senzoru na intenzitu UV záření
  float uv = napetiOutUV/1.06; //převod měřeného napětí ze senzoru deskou, deska nadměřuje
  float intenzitaUV = prevodNapetiIntenzita(uv, 0.99, 2.8, 0.0, 15.0); //napetiOutUV
  // vytištění informací po sériové lince
  //Serial.print("ML8511 napeti: ");
  //Serial.print(uv); //napetiOutUV
  //Serial.print("V, UV Intenzita (mW/cm^2): ");
  //Serial.println(intenzitaUV);

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
     httpRequestData += String(intenzitaUV);
     httpRequestData += "&value2=";
     httpRequestData += String(lux);
     httpRequestData += "&value3=";
     httpRequestData += String(P0);
     
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
    
  ESP.deepSleep(300e6); //počet sekund spánku, e6 je dalších 6 míst kvůli milisekundám
}

void loop() {
  //opakovatelná smička, při deep sleepu se nepoužívá
}

// podprogram pro provedení 8 měření a vrácení průměrné hodnoty
int prumerAnalogRead(int pinToRead) {
  byte numberOfReadings = 8;
  unsigned int runningValue = 0; 

  for(int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;

  return(runningValue);  
  delay(50);
}

// podprogram pro převod naměřené hodnoty
// z desetinného čísla na UV intenzitu
float prevodNapetiIntenzita(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}