#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino

// needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert Firebase project API Key
#define API_KEY "AIzaSyDK3pwFhA7QmA9GAWBXNdCBteZkh7h9NRs"
#define FIREBASE_PROJECT_ID "aqua-garden"
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

unsigned long sendDataPrevMillis = 5000;
int count = 0;
bool signupOK = false;

void setup()
{
  Serial.begin(9600);
  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  // reset saved settings
  // wifiManager.resetSettings();

  // set custom ip for portal
  // wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("AutoConnectAP");
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  pinMode(5, OUTPUT);

  // /* Assign the RTDB URL (required) */
  // config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("ok");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
  dht.begin();
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop()
{
  digitalWrite(5, HIGH);
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    String documentPath = "users/025pLgXrrOPEEa5pIbPpXU8UU9V2/devices/"+WiFi.macAddress();
    FirebaseJson content;
    float moisture_percentage;
    float temp = dht.readTemperature();
    float humidityA = dht.readHumidity();
    moisture_percentage = (100.00 - ((analogRead(sensor_pin) / 1023.00) * 100.00));
    content.set("fields/humidity/stringValue", String(moisture_percentage).c_str());
    content.set("fields/temp/stringValue", String(temp).c_str());
    content.set("fields/humidityA/stringValue", String(humidityA).c_str());
    if (moisture_percentage <= 40)
    {
      Serial.printf("Regando...");
      digitalWrite(5, LOW);
      delay(2000);
      Serial.printf("Cerrando...");
      digitalWrite(5, HIGH);
    }
    else
    {
      digitalWrite(5, HIGH);
    }

    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "humidity,temp,humidityA"))
    {
      Serial.println(String("Sended:" + WiFi.macAddress()).c_str());
    }
    else
    {
      Serial.println(fbdo.errorReason());
    }
  }
}