/*
   This sketch monitors the temperature inside the kegerator with a DS18B20
   and the ambient temperature with a DHT22
   Pin allocations
   D0 - connected to Reset
   D2 - data pin for DHT22
   D3 - data pin for LED
   D7 - Drives admin function
   D8 - drives DHT power via MOSFAT
   
*/
#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <PubSubClient.h>
/*
   Changes to PubSubClient library
   The status update is bigger than 128 bytes so it is necessary to change MQTT_MAX_PACKET_SIZE to 256 in PubSubClient.h
*/

/*
   Script specific
*/
#define SOFTWARE  "ESP_DHT22_B"
#define SW_VERSION 0.2


#include <SimpleDHT.h>


/*
   General declarations
*/
#define STATUS_TIME 3600000   // send status updates every hour
#define NETWORKCHECK 15000    // check the network every 15 sec
#define SERIAL_BAUD   115200

#define DHT_PIN         4     // = D2
#define DHTPOWER        15    // = D8

/*
   General variables
*/

int wifiStatus = -1;
long int connectionTimer = 0;
long int status_timer = 0;
long int network_timer = 0;
boolean configUpdated = false;

// create web client and mqtt client
WiFiClient wifiClient;
PubSubClient mqtt_client(wifiClient);

// create the web server object
ESP8266WebServer server(80);

#include "led_colours.h"
#include "config.h"
#include "web_helpers.h"
#include "mqtt_helpers.h"
#include "led_helpers.h"
#include "web_pages.h"

// this is used to check the input voltage
ADC_MODE(ADC_VCC);


//************************************************************
// DHT22
// ***********************************************************
int pinDHT22 = DHT_PIN;
SimpleDHT22 dht22;
float humidity, temperature;
/********************************************************************/

/*

*/
void setup() {
  // put your setup code here, to run once:
  EEPROM.begin(512);                            // determins mow much the EEPROm can store
  Serial.begin(SERIAL_BAUD);
  Serial.println("Starting ESP8266 - DHT@@ Battery version");

  pinMode(ADMIN_PIN, INPUT_PULLUP);
  pinMode(DHTPOWER, OUTPUT);        // This pin provides power to the DHT22 via a n-channel mosfet
  digitalWrite(DHTPOWER, HIGH);     // best turn the power on

  // Read the config
  ReadConfig();
  //configPrint();

  WiFi.mode(WIFI_STA);
  WiFi.begin(config.ssid.c_str(), config.password.c_str());
  Serial.print("MAC Address is ");
  Serial.println(WiFi.macAddress());

  int wCnt = 0;
  while ((WiFi.status() != WL_CONNECTED) && (wCnt < 10))
  {
    Serial.println("WiFi not connected");
    wCnt++;
    delay(500);
  }

  // set up the LED environment. LED management code i in led_helpers.h
  pixels.begin();
  pixels.setBrightness(64);
  pixels.show();

  // Connect to MQTT
  mqtt_reconnect();

  delay(2000);
  
  mqtt_send_temperature();

  delay(1000);
  digitalWrite(DHTPOWER, LOW);     // turn the power off before going to sleep
  Serial.println("End of setup");

  Serial.print("Go to sleep for ");
  Serial.print(config.dataPeriod);
  Serial.println(" secs");
  ESP.deepSleep(config.dataPeriod * 1000000);
}

/*
   This is the main loop of the sketch
*/
void loop() {
  // put your main code here, to run repeatedly:

  // Check to see if admin node should go on/off
  checkAdmin();
  if (adminMode) server.handleClient();

  // this code controls the LED
  showLED();

  if (configUpdated)        // the config has been updated, lets reconnect to Wifi and MQTT
  {
    configUpdated = false;
    ConfigureWifi();
    mqtt_reconnect();
  }

  // check the network on a regular basis
  if ((network_timer + NETWORKCHECK) > millis())
  {
    network_timer = millis();
    showWifiStatus();   // shows wifi status on serial console
  }

  // send a regular status update
  if ((status_timer + STATUS_TIME < millis()) || (millis() < status_timer))
  {
    status_timer = millis();
    //mqtt_send_status();
  }
  mqtt_send_temperature();

  delay(config.dataPeriod * 1000);

}

//*********************************************************************************************
void mqtt_send_temperature()
{
  Serial.println("Send temperature message");

  // define a JSON structure for the payload
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& jPayload = jsonBuffer.createObject();

  // set all JSON data here
  jPayload["Nodename"] = config.nodeName;
  jPayload["localIP"] = WiFi.localIP().toString();
  jPayload["voltage(mV)"] = ESP.getVcc();
  jPayload["Software"] = SOFTWARE;
  jPayload["Version"] = SW_VERSION;
 
  int err = SimpleDHTErrSuccess;
  if ((err = dht22.read2(pinDHT22, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess)
  {
    Serial.print("Read DHT22 failed, err="); Serial.println(err);
  }
  else
  {
    jPayload["Ambient_temp"] = temperature;
    jPayload["Humidity"] = humidity;
  }

  // save the JSON as a string
  String js;
  jPayload.printTo(js);

  Serial.println("Send it now");
  sendMQTT(config.MQTT_Topic1, js);
}
