// This file contains definitions & functions used to read, write etc data in the EEPROM

// this file contains initial values
#include "credentials.h"

// define EEPROM offsets
#define EEPROM_SSID 10      // an SSID can be up to 31 characters
#define EEPROM_PASSWORD 50  // allow 30 char for the password
#define EEPROM_NODENAME 90  // allow 20 char for the node name
#define EEPROM_MQTTHOST1 200
#define EEPROM_MQTTPORT1 250
#define EEPROM_MQTTUSER1 260
#define EEPROM_MQTTPASSWORD1 300
#define EEPROM_MQTTTOPIC1 350
#define EEPROM_STATUS_PERIOD 500
#define EEPROM_DATA_PERIOD 502

#define ADMIN_ON  300000      // 5 minutes should be enough
#define ADMIN_PIN D7          // Connecting the admin pin to vcc will trigger admin mode
#define ADMIN_BOUNCE  50      // Admin pin must be down for > 50mSec to effect change
#define PIN_ON  0
#define PIN_OFF 1

// Prototype for functions
void WriteStringToEEPROM(int beginaddress, String string);
String  ReadStringFromEEPROM(int beginaddress);
void EEPROMWritelong(int address, long value);
long EEPROMReadlong(long address);
void EEPROMWriteInt(int address, int value);
int EEPROMReadInt(int address);
void configPrint();
void ReadConfig();
void WebServerSetup();
void setPrimaryLED(uint32_t pColour);
void setSecondaryLED(uint32_t pColour);
void sendMQTT(String inTopic, String inPayload);

// variable declarations
long int adminTimer;
bool adminMode = false;
boolean stringComplete = false;  // whether the string is complete
String inputString = "";         // a string to hold incoming data
int adminPinState;                    // hold the state of the admin pin
int lastAdminPinState = LOW;          // stores the last admin pin state
unsigned long lastDebounceTime = 0;   // lold time for debounce
bool pinEvent = false;                // any thing interesting happening?
bool buttonPress = false;             // has the admin button been pushed

// A C type structure to hold config details
struct strConfig {
  String ssid;
  String password;
  String nodeName;
  String MQTT_Host1;
  int MQTT_Port1;
  String MQTT_User1;
  String MQTT_Password1;
  String MQTT_Topic1;
  int statusPeriod;
  int dataPeriod;
} config;               // called config

/*
   Function writes stored config values to EEPROM
*/
void WriteConfig()
{

  Serial.println("Writing Config");
  EEPROM.write(0, 'C');
  EEPROM.write(1, 'F');
  EEPROM.write(2, 'G');

  WriteStringToEEPROM(EEPROM_SSID, config.ssid);
  WriteStringToEEPROM(EEPROM_PASSWORD, config.password);
  WriteStringToEEPROM(EEPROM_NODENAME, config.nodeName);
  WriteStringToEEPROM(EEPROM_MQTTHOST1, config.MQTT_Host1);
  EEPROMWriteInt(EEPROM_MQTTPORT1, config.MQTT_Port1);
  WriteStringToEEPROM(EEPROM_MQTTUSER1, config.MQTT_User1);
  WriteStringToEEPROM(EEPROM_MQTTPASSWORD1, config.MQTT_Password1);
  WriteStringToEEPROM(EEPROM_MQTTTOPIC1, config.MQTT_Topic1);
  EEPROMWriteInt(EEPROM_STATUS_PERIOD, config.statusPeriod);
  EEPROMWriteInt(EEPROM_DATA_PERIOD, config.dataPeriod);
  EEPROM.commit();                                                // this is required on an ESP8266
  configUpdated = true;
  configPrint();

  // lets send the new config as an MQTT message
  // define a JSON structure for the payload
  StaticJsonBuffer<250> jsonBuffer;
  JsonObject& jPayload = jsonBuffer.createObject();

  // set all JSON data here
  jPayload["Nodename"] = config.nodeName;
  jPayload["SSID"] = config.ssid;
  jPayload["StatusPeriod"] = config.statusPeriod;
  jPayload["DataPeriod"] = config.dataPeriod;
  jPayload["Action"] = "Config updated";


  // save the JSON as a string
  String js;
  jPayload.printTo(js);

  sendMQTT(config.MQTT_Topic1, js);

}

/*
   Function to read config values from EEPROM
*/
void ReadConfig()
{
  Serial.println("Reading Configuration");
  // if the first 3 chars are CFG then you can expect realistic config value
  if (EEPROM.read(0) == 'C' && EEPROM.read(1) == 'F'  && EEPROM.read(2) == 'R' )
  {
    Serial.println("Configurarion Found!");

    config.ssid           = ReadStringFromEEPROM(EEPROM_SSID);
    config.password       = ReadStringFromEEPROM(EEPROM_PASSWORD);
    config.nodeName       = ReadStringFromEEPROM(EEPROM_NODENAME);
    config.MQTT_Host1     = ReadStringFromEEPROM(EEPROM_MQTTHOST1);
    config.MQTT_Port1     = EEPROMReadInt(EEPROM_MQTTPORT1);
    config.MQTT_User1     = ReadStringFromEEPROM(EEPROM_MQTTUSER1);
    config.MQTT_Password1 = ReadStringFromEEPROM(EEPROM_MQTTPASSWORD1);
    config.MQTT_Topic1    = ReadStringFromEEPROM(EEPROM_MQTTTOPIC1);
    config.statusPeriod   = EEPROMReadInt(EEPROM_STATUS_PERIOD);
    config.dataPeriod     = EEPROMReadInt(EEPROM_DATA_PERIOD);

  }
  else      // no CFG to start with, better store some default values
  {
    Serial.println("Configurarion NOT FOUND!!!!");
    Serial.println("Setting default parameters");
    // please define the credentials either in the file credentials.h or here
    config.ssid           = mySSID; // SSID of access point
    config.password       = mypassword;  // password of access point
    config.nodeName       = "None";
    config.MQTT_Host1     = initMQTT_host;
    config.MQTT_Port1     = initMQTT_port;
    config.MQTT_User1     = initMQTT_user;
    config.MQTT_Password1 = initMQTT_password;
    config.MQTT_Topic1    = initMQTT_topic;
    config.statusPeriod   = 3600;    // an hour by default
    config.dataPeriod     = 300;
    WriteConfig();

  }
}


/*
   Function to write a string to EEPROM
*/
void WriteStringToEEPROM(int beginaddress, String string)
{
  char  charBuf[string.length() + 1];
  string.toCharArray(charBuf, string.length() + 1);
  for (int t =  0; t < sizeof(charBuf); t++)
  {
    EEPROM.write(beginaddress + t, charBuf[t]);
  }
}

/*
   Function to read a string from EEPROM
*/
String  ReadStringFromEEPROM(int beginaddress)
{
  volatile byte counter = 0;
  char rChar;
  String retString = "";
  while (1)
  {
    rChar = EEPROM.read(beginaddress + counter);
    if (rChar == 0) break;
    if (counter > 31) break;
    counter++;
    retString.concat(rChar);

  }
  return retString;
}

/*
   Function to write a long int to EEPROM
*/
void EEPROMWritelong(int address, long value)
{
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

/*
   Function to read long int from EEPROM
*/
long EEPROMReadlong(long address)
{
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);
}

/*
   Function to write an int to EEPROM
*/
void EEPROMWriteInt(int address, int value)
{

  //Write the 2 bytes into the eeprom memory.
  EEPROM.write(address, highByte(value));
  EEPROM.write(address + 1, lowByte(value));
}

/*
   Function to read long int from EEPROM
*/
int EEPROMReadInt(int address)
{
  byte high = EEPROM.read(address);
  byte low = EEPROM.read(address + 1);
  int myInteger = word(high, low);
  return (myInteger);
}


/*
   Function to display corrent config
*/
void configPrint()
{
  Serial.print("SSID: ");
  Serial.println(config.ssid);
  //Serial.print("Password: ");
  //Serial.println(config.password);
  Serial.print("Node Name: ");
  Serial.println(config.nodeName);
  Serial.print("MQTT Host 1: ");
  Serial.println(config.MQTT_Host1);
  Serial.print("MQTT Port 1: ");
  Serial.println(config.MQTT_Port1);
  Serial.print("MQTT User 1: ");
  Serial.println(config.MQTT_User1);
  Serial.print("MQTT Password 1: ");
  Serial.println(config.MQTT_Password1);
  Serial.print("MQTT Topic 1: ");
  Serial.println(config.MQTT_Topic1);

}


/*
  Function to start the admin service
*/
void startAdmin()
{
  adminMode = true;
  adminTimer = millis();
  setSecondaryLED(LED_green);
  WiFi.softAP(apSSID, apPassword);
  WiFi.mode(WIFI_AP_STA);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("Admin service available on Wifi network ");
  Serial.print(apSSID);
  Serial.print(" on IP address ");
  Serial.println(myIP);
  WebServerSetup();
}


/*
  Function to stop the admin service
*/
void stopAdmin()
{
  WiFi.mode(WIFI_STA);
  Serial.println("AP Turned off, operating in station mode");
  adminMode = false;
  // stop the web server, only needed in admin mode
  server.stop();
  Serial.println("Webserver turned off, only needed in admin mode");
  setSecondaryLED(LED_blank);
}

/*
   This function checks to see if the admin switch has been pressed. If so we turn on / off the admin mode
*/
void checkAdmin()
{
  int adm = digitalRead(ADMIN_PIN);

  // lets remember if the pin has been triggered at all
  if (adm == PIN_ON) pinEvent = true;

  // if pin is now off and switch pressed for longer than debounce time
  if ((adm == PIN_OFF) && pinEvent)
  {
    pinEvent = false;
    //Serial.println("Reset pinEvent");

    if ((millis() - lastDebounceTime) > ADMIN_BOUNCE)
    {
      Serial.println("Button event");
      buttonPress = true;
    }
  }

  // update the debounce timer on change of state
  if (adm != lastAdminPinState)
  {
    lastAdminPinState = adm;
    lastDebounceTime = millis();
    //Serial.println("Update debounce time");
  }
  
  if (buttonPress)          // if button pressed, toggle admin mode
  {
    buttonPress = false;
    if (adminMode)
    {
      Serial.println("ADMIN OFF");
      stopAdmin();
    }
    else
    {
      Serial.println("ADMIN ON");
      startAdmin();
    }
  }

  // check serial input to see if admin mode needed
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();

    if (inChar == 'a')
    {
      startAdmin();   // defined in config.h
    }
    else if (inChar == 'o')
    {
      stopAdmin();   // defined in config.h
    }
    else
    {
      Serial.println("Valid serial control characters are 'a' to turn admin on and 'o' to turn it off");
      Serial.println("-------------------------------------------------------------------------------");
    }
  }

  // Admin mode time out
  if (adminMode)
  {
    if ((adminTimer + ADMIN_ON) < millis())   // run out of time, turn the admin function off
    {
      stopAdmin();
    }
  }

}
