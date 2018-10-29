//file to hold functions used for MQTT messaging
//#define WEB_HELPERS_H

/*
   Changes to PubSubClient library
   The status update is bigger than 128 bytes so it is necessary to change MQTT_MAX_PACKET_SIZE to 256 in PubSubClient.h
*/

// Prototype for functions
void mqtt_send_status();
void showLED();
char * stringToCharArray(String inString);
void sendMQTT(String inTopic, String inPayload);
void singleLEDblink(uint32_t pColour);      // defined in led_helpers.h
int mqttFailure = 0;

//*********************************************************************************************
// Function to deal with message received from mqtt
//*********************************************************************************************
void mqtt_callback(char* topic, byte* payload, unsigned int length) {

  bool cUpdate = false;

  // define a JSON structure for the payload
  StaticJsonBuffer<200> jsonBuffer;

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Create json object
  JsonObject& jPayload = jsonBuffer.parseObject(payload);

  // if the json message contains the key
  if (jPayload.containsKey("nodeName"))
  {
    Serial.print("Received node name is ");
    String newNodeName = jPayload["nodeName"];
    Serial.println(newNodeName);
    config.nodeName = newNodeName;
    cUpdate = true;
  }

  // if we have received an update, write it to EEPROM
  if (cUpdate)
  {
    WriteConfig();
    configPrint();
  }

  Serial.println("-----------------------");
  Serial.println();

}

/*
   Function to display text version of MQTT state
*/
//*********************************************************************************************
String MQTTStateText(int inState)
{
  if (inState == -4) return ("MQTT_CONNECTION_TIMEOUT");
  else if (inState == -3) return ("MQTT_CONNECTION_LOST");
  else if (inState == -2) return ("MQTT_CONNECT_FAILED");
  else if (inState == -1) return ("MQTT_DISCONNECTED");
  else if (inState == 0) return ("MQTT_CONNECTED");
  else if (inState == 1) return ("MQTT_CONNECT_BAD_PROTOCOL");
  else if (inState == 2) return ("MQTT_CONNECT_BAD_CLIENT_ID");
  else if (inState == 3) return ("MQTT_CONNECT_UNAVAILABLE");
  else if (inState == 4) return ("MQTT_CONNECT_BAD_CREDENTIALS");
  else if (inState == 5) return ("MQTT_CONNECT_UNAUTHORIZED");
  else return ("UNKNOWN STATUS");
}

/*
  Function to keep trying for an MQTT connection
*/
//*********************************************************************************************
void mqtt_reconnect() {
  // Loop until we're reconnected

  if (!adminMode)                   // dont try and connect when its in admin mode, doesnt work anyway
  {
    while (!mqtt_client.connected()) {
      Serial.print("Attempting MQTT connection...");
      setPrimaryLED(LED_blue);
      // Attempt to connect


      // A bit of jiggery pokery to convert the host name to a usable format
      char MQTT_hostname[config.MQTT_Host1.length() + 1];
      config.MQTT_Host1.toCharArray(MQTT_hostname, config.MQTT_Host1.length() + 1);
      Serial.print("MQTT Host = ");
      Serial.println(MQTT_hostname);

      // and for the user name
      char MQTT_user[config.MQTT_User1.length() + 1];
      config.MQTT_User1.toCharArray(MQTT_user, config.MQTT_User1.length() + 1);

      // you guessed it, password too
      char MQTT_pw[config.MQTT_Password1.length() + 1];
      config.MQTT_Password1.toCharArray(MQTT_pw, config.MQTT_Password1.length() + 1);

      // you guessed it, and the nodeName
      char MQTT_node[config.nodeName.length() + 1];
      config.nodeName.toCharArray(MQTT_node, config.nodeName.length() + 1);

      // set the mqtt settings
      mqtt_client.setServer(MQTT_hostname, config.MQTT_Port1);

      // and try to connect
      if (mqtt_client.connect(MQTT_node, MQTT_user, MQTT_pw)) {
        Serial.println("MQTT connected");
        // Once connected, publish an announcement...
        setPrimaryLED(LED_blank);
        //mqtt_send_status();

        // ... and resubscribe
        mqtt_client.setCallback(mqtt_callback);

        // add /nodename to end of subscription topic
        String sTopic = config.MQTT_Topic1 + "/" + config.nodeName;
        Serial.println(sTopic);

        // Need to mangle the topic as usual
        char MQTT_topic[sTopic.length() + 1];
        sTopic.toCharArray(MQTT_topic, sTopic.length() + 1);
        mqtt_client.subscribe(MQTT_topic);
        Serial.print("MQTT subscribed to : ");
        Serial.println(MQTT_topic);
      }
      else
      {
        Serial.print("failed, rc=");
        Serial.print(mqtt_client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        setPrimaryLED(LED_blue);
        showLED();
        delay(1900);
        setPrimaryLED(LED_blank);
        showLED();
        delay(1900);
      }
      break;
    }
  }
}

//*********************************************************************************************
void mqtt_send_status()
{
  Serial.println("Send status message");
  
  // define a JSON structure for the payload
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& jPayload = jsonBuffer.createObject();

  // set all JSON data here
  jPayload["Nodename"] = config.nodeName;
  jPayload["MAC"] = GetMacAddress();
  jPayload["localIP"] = WiFi.localIP().toString();
  jPayload["Uptime(s)"] = millis() / 1000;
  jPayload["voltage(mV)"] = ESP.getVcc();
  jPayload["Software"] = SOFTWARE;
  jPayload["Version"] = SW_VERSION;

  // save the JSON as a string
  String js;
  jPayload.printTo(js);

  sendMQTT(config.MQTT_Topic1, js);
  singleLEDblink(LED_yellow);
  
}


/*
   Function to return a characher array
*/
char * stringToCharArray(String inString)
{
  char * buf = (char *) malloc (256);
  inString.toCharArray(buf, 256);
  return buf;
}

/*
   Function to translate & send MQTT message
*/
void sendMQTT(String inTopic, String inPayload)
{
  Serial.println(inTopic);
  Serial.println(inPayload);
  if (mqtt_client.publish(stringToCharArray(inTopic), stringToCharArray(inPayload)))
  {
    Serial.println("MQTT success");
    //singleLEDblink(LED_blue);
  }
  else
  {
    Serial.println("MQTT Message failed");
    Serial.println(MQTTStateText(mqtt_client.state()));
    Serial.println(mqtt_client.connected());
    mqttFailure++;
  }
  if (mqttFailure > 3)
  {
    mqttFailure = 0;
    mqtt_reconnect();
  }
}
