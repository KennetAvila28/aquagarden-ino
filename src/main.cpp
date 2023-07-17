#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#include <EEPROM.h>
// needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include <NTPClient.h>
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert Firebase project API Key
#define API_KEY "AIzaSyDK3pwFhA7QmA9GAWBXNdCBteZkh7h9NRs"
#define FIREBASE_PROJECT_ID "aqua-garden"
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Variable to save current epoch time
int timestamp;
#define USER_EMAIL "lokogamer9.rocha@gmail.com"
#define USER_PASSWORD "280201Ga"
// Insert RTDB URLefine the RTDB URL */
// #define DATABASE_URL "https://aqua-garden.firebaseio.com/users"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;
const int sensor_pin = A0;
const int sensor_temp = 4;
DHT dht(sensor_temp, DHT11);
// Week Days
String weekDays[7] = {"Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado"};

// Month names
String months[12] = {"Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"};

unsigned long sendDataPrevMillis = 5000;
int count = 0;
bool signupOK = false;
WiFiManager wifiManager;
unsigned long getTime()
{
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}
String read_String(char add)
{
  int i;
  char data[100]; //Max 100 Bytes
  int len=0;
  unsigned char k;
  k=EEPROM.read(add);
  while(k != '\0' && len<500)   //Read until null character
  {    
    k=EEPROM.read(add+len);
    data[len]=k;
    len++;
  }
  data[len]='\0';
  return String(data);
}

void setup()
{
  WiFi.begin("XX","XX");
  Serial.begin(9600);
  timeClient.begin();
  timeClient.setTimeOffset(-21600);
  wifiManager.autoConnect("AquaGarden");
  
  Serial.println("userId:" + read_String(10));
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  pinMode(5, OUTPUT);

  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("Session iniciada");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
  dht.begin();
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  FirebaseJson content;
  String documentPath = "users/" + wifiManager._userid + "/devices/" + WiFi.macAddress();

  if (!Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), ""))
  {
    Serial.println("Creando Dispositivo...");
    content.set("fields/latest/stringValue", String("").c_str());
    content.set("fields/humidity/stringValue", String(0).c_str());
    content.set("fields/temp/stringValue", String(0).c_str());
    content.set("fields/humidityA/stringValue", String(0).c_str());
    content.set("fields/name/stringValue", String("AquaGarden Device").c_str());
    content.set("fields/watering/boolValue", false);
    content.set("fields/isActive/boolValue", true);
    content.set("fields/isManual/boolValue", false);
    content.set("fields/celcius/boolValue", true);
    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "humidity,temp,humidityA,isManual,latest,isActive,celcius,watering,name"))
    {
      Serial.println("Dispositivo creado");
      Serial.println("Iniciando...");
    }
    else
    {
      Serial.println("Error al crear el dispositivo");
    }
  }
  else
  {
    Serial.println("Dispositivo encontrado");
    Serial.println("Iniciando...");
  }
}

void loop()
{
  time_t epochTime = getTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon + 1;
  int currentYear = ptm->tm_year + 1900;

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    String documentPath = "users/" + wifiManager._userid + "/devices/" + WiFi.macAddress();
    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), ""))
    {
      FirebaseJson payload;
      payload.setJsonData(fbdo.payload().c_str());

      FirebaseJsonData jsonData;
      payload.get(jsonData, "fields/isActive/booleanValue", true);
      FirebaseJson content;
      String formattedTime = timeClient.getFormattedTime();
      String currentMonthName = months[currentMonth - 1];
      String currentDate = String(currentMonth) + "-" + String(monthDay) + "-" + String(currentYear);
      if (jsonData.boolValue == true)
      {
        float moisture_percentage = 0;
        float temp = dht.readTemperature();
        float humidityA = dht.readHumidity();

        moisture_percentage = (100.00 - ((analogRead(sensor_pin) / 1023.00) * 100.00));
        if (String(moisture_percentage) != "nan")
        {
          content.set("fields/humidity/doubleValue", moisture_percentage);
        }
        if (String(temp) != ("nan"))
        {
          content.set("fields/temp/doubleValue", temp);
        }
        if (String(humidityA) != "nan")
        {
          content.set("fields/humidityA/doubleValue", humidityA);
        }
        if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "humidity,temp,humidityA"))
        {
          Serial.println(String("Sended:" + WiFi.macAddress()).c_str());
        }
        else
        {
          Serial.println(fbdo.errorReason());
        }
        payload.get(jsonData, "fields/isManual/booleanValue", true);
        // Manual is Active?
        if (jsonData.boolValue == true)
        {

          // content.set("fields/date/stringValue", String(currentDate).c_str());
          // Watering

          payload.get(jsonData, "fields/watering/booleanValue", true);
          bool watering = false;
          if (jsonData.boolValue == true)
          {
            content.set("fields/watering/booleanValue", true);
            Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "watering");
            Serial.printf("Regando...");
            digitalWrite(5, LOW);
            watering = true;
          }
          else
          {
            if (watering)
            {
              content.set("fields/watering/booleanValue", false);
              Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "watering");
              Serial.printf("Cerrando...");
              digitalWrite(5, HIGH);
              watering = false;
            }
          }
        }
        else
        {
          if (moisture_percentage <= 40 && temp <= 35 && humidityA <= 90)
          {
            String reportId = formattedTime + "-" + currentDate;
            String reportsPath = "users/" + wifiManager._userid + "/reports/" + reportId;
            FirebaseJson reportsContent;
            reportsContent.set("fields/date/stringValue", String(currentDate).c_str());
            reportsContent.set("fields/humidity/stringValue", String(moisture_percentage).c_str());
            reportsContent.set("fields/temp/stringValue", String(temp).c_str());
            reportsContent.set("fields/humidityA/stringValue", String(humidityA).c_str());

            if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", reportsPath.c_str(), reportsContent.raw(), "Fecha,Humedad,Temperatura,HumedadTierra"))
            {
              Serial.printf("send report");
            }

            // content.set("fields/date/stringValue", String(currentDate).c_str());
            content.set("fields/watering/booleanValue", true);
            Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "watering");
            Serial.printf("Regando...");
            digitalWrite(5, LOW);
            delay(2000);
            Serial.printf("Cerrando...");
            digitalWrite(5, HIGH);
            content.set("fields/watering/booleanValue", false);
          }
        }
      }
    }
  }
}
