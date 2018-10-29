//#ifndef WEB_HELPERS_H
//#define WEB_HELPERS_H

const String HTML_Header = "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /> <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" /> <HTML><HEAD><link rel=\"stylesheet\" href=\"style.css\">";
const String HTML_Footer = "</BODY></HTML>";

// Prototype for functions
void show_home_page();
void send_network_configuration_html();
void send_network_configuration_values_html();
void send_connection_state_values_html();
void show_general_page();
void show_mqtt_page();
void setPrimaryLED(uint32_t pColour);
void setSecondaryLED(uint32_t pColour);
void mqtt_reconnect();

/*
 * Function to get MAC Address
 */
String GetMacAddress()
{
  uint8_t mac[6];
  char macStr[18] = {0};
  WiFi.macAddress(mac);
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],  mac[1], mac[2], mac[3], mac[4], mac[5]);
  return  String(macStr);
}

// convert a single hex digit character to its integer value (from https://code.google.com/p/avr-netino/)
unsigned char h2int(char c)
{
  if (c >= '0' && c <= '9') {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}

String urldecode(String input) // (based on https://code.google.com/p/avr-netino/)
{
  char c;
  String ret = "";

  for (byte t = 0; t < input.length(); t++)
  {
    c = input[t];
    if (c == '+') c = ' ';
    if (c == '%') {


      t++;
      c = input[t];
      t++;
      c = (h2int(c) << 4) | h2int(input[t]);
    }

    ret.concat(c);
  }
  return ret;

}

/*
**
** CONFIGURATION HANDLING
**
*/
void ConfigureWifi()
{
  Serial.println("Configuring Wifi");

  //WiFi.begin ("WLAN", "password");

  WiFi.begin (config.ssid.c_str(), config.password.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    delay(500);
  }
}

/*
 * Function to display text version of Wifi Status
 */
String WifiStatusText(int inStatus)
{
      if (inStatus == 0) return("Idle");
      else if (inStatus == 1) return("NO SSID AVAILBLE");
      else if (inStatus == 2) return("SCAN COMPLETED");
      else if (inStatus == 4) return("CONNECT FAILED");
      else if (inStatus == 3) return("CONNECTED");
      else if (inStatus == 5) return("CONNECTION LOST");
      else if (inStatus == 6) return("DISCONNECTED");
      else return("UNKNOWN STATUS");
}


/*
 * Function to display wifi status on serial console
 */
void showWifiStatus()
{
  // if a change in Wifi status is detected, display on serial monitor
  if (wifiStatus != WiFi.status())
  {
    wifiStatus = WiFi.status();
    Serial.print("Wifi status has changed and is now ");
    Serial.println(WifiStatusText(WiFi.status()));
    Serial.print("IP Address is : ");
    Serial.println(WiFi.localIP());
  }

  if ((connectionTimer + 10000) < millis())
  {
    // status 3 is great = connected
    if (WiFi.status() != 3)
    {
      Serial.print("Wifi status: ");
      Serial.println(WifiStatusText(WiFi.status()));
      setPrimaryLED(LED_red);
    }
    else
    {
      // Wifi is up, check for mqtt client
      setPrimaryLED(LED_blank);

      if (!mqtt_client.connected()) {
        mqtt_reconnect();
      }
      mqtt_client.loop();
    }
    connectionTimer = millis();
  }
}

/*
 * Function to create an HTML Table line with text input
 */
String HTMLTableTextLine(String dispText, String itemName, String initValue)
{
  String line = "";
  line += "<tr><td align=\"right\">";
  line += dispText;
  line += "</td><td><input type=\"text\" id=\"";
  line += itemName;
  line += "\" name=\"";
  line += itemName;
  line += "\" value=\"";
  line += initValue;
  line += "\"></td></tr>";
  return(line); 

}

/*
 * Function to create an HTML Table line with integer input
 */
String HTMLTableIntLine(String dispText, String itemName, int initValue)
{
  String line = "";
  String inVal = String(initValue);
  line += "<tr><td align=\"right\">";
  line += dispText;
  line += "</td><td><input type=\"text\" id=\"";
  line += itemName;
  line += "\" name=\"";
  line += itemName;
  line += "\" value=\"";
  line += inVal;
  line += "\"></td></tr>";
  return(line); 

}


/*
   This constant defines javascript for the site
*/
const char PAGE_microajax_js[] PROGMEM = R"=====(
function microAjax(B,A){this.bindFunction=function(E,D){return function(){return E.apply(D,[D])}};this.stateChange=function(D){if(this.request.readyState==4){this.callbackFunction(this.request.responseText)}};this.getRequest=function(){if(window.ActiveXObject){return new ActiveXObject("Microsoft.XMLHTTP")}else{if(window.XMLHttpRequest){return new XMLHttpRequest()}}return false};this.postBody=(arguments[2]||"");this.callbackFunction=A;this.url=B;this.request=this.getRequest();if(this.request){var C=this.request;C.onreadystatechange=this.bindFunction(this.stateChange,this);if(this.postBody!==""){C.open("POST",B,true);C.setRequestHeader("X-Requested-With","XMLHttpRequest");C.setRequestHeader("Content-type","application/x-www-form-urlencoded");C.setRequestHeader("Connection","close")}else{C.open("GET",B,true)}C.send(this.postBody)}};

function setValues(url)
{
  microAjax(url, function (res)
  {
    res.split(String.fromCharCode(10)).forEach(function(entry) {
    fields = entry.split("|");
    if(fields[2] == "input")
    {
        document.getElementById(fields[0]).value = fields[1];
    }
    else if(fields[2] == "div")
    {
        document.getElementById(fields[0]).innerHTML  = fields[1];
    }
    else if(fields[2] == "chk")
    {
        document.getElementById(fields[0]).checked  = fields[1];
    }
    });
  });
}

)=====";

/*
 * This constant defines CSS for the site
 */
const char PAGE_Style_css[] PROGMEM = R"=====(
body { color: #000000; font-family: avenir, helvetica, arial, sans-serif;  letter-spacing: 0.15em;} 
hr {    background-color: #eee;    border: 0 none;   color: #eee;    height: 1px; } 
.btn, .btn:link, .btn:visited {  
  border-radius: 0.3em;  
  border-style: solid;  
  border-width: 1px;  
color: #111;  
display: inline-block;  
  font-family: avenir, helvetica, arial, sans-serif;  
  letter-spacing: 0.15em;  
  margin-bottom: 0.5em;  
padding: 1em 0.75em;  
  text-decoration: none;  
  text-transform: uppercase;  
  -webkit-transition: color 0.4s, background-color 0.4s, border 0.4s;  
transition: color 0.4s, background-color 0.4s, border 0.4s; 
} 
.btn:hover, .btn:focus {
color: #7FDBFF;  
border: 1px solid #7FDBFF;  
  -webkit-transition: background-color 0.3s, color 0.3s, border 0.3s;  
transition: background-color 0.3s, color 0.3s, border 0.3s; 
}
  .btn:active {  
color: #0074D9;  
border: 1px solid #0074D9;  
    -webkit-transition: background-color 0.3s, color 0.3s, border 0.3s;  
transition: background-color 0.3s, color 0.3s, border 0.3s; 
  } 
  .btn--s 
  {  
    font-size: 12px; 
  }
  .btn--m { 
    font-size: 14px; 
  }
  .btn--l {  
    font-size: 20px;  border-radius: 0.25em !important; 
  } 
  .btn--full, .btn--full:link {
    border-radius: 0.25em; 
display: block;  
      margin-left: auto; 
      margin-right: auto; 
      text-align: center; 
width: 100%; 
  } 
  .btn--blue:link, .btn--blue:visited {
color: #fff;  
    background-color: #0074D9; 
  }
  .btn--blue:hover, .btn--blue:focus {
color: #fff !important;  
    background-color: #0063aa;  
    border-color: #0063aa; 
  }
  .btn--blue:active {
color: #fff; 
    background-color: #001F3F;  border-color: #001F3F; 
  }
  @media screen and (min-width: 32em) {
    .btn--full {  
      max-width: 16em !important; } 
  }
)=====";

/*
 * Function to start the web server
 */
void WebServerSetup()
{
  server.on ( "/", show_home_page);

  server.on ( "/favicon.ico",   []() {
    //Serial.println("favicon.ico");
    server.send ( 200, "text/html", "" );
  }  );
  server.on ( "/style.css", []() {
    Serial.println("style.css");
    server.send ( 200, "text/plain", PAGE_Style_css );
  } );
  server.on ( "/microajax.js", []() {
    //Serial.println("microajax.js");
    server.send ( 200, "text/plain", PAGE_microajax_js );
  } );

  server.on ( "/network.html", send_network_configuration_html );
  server.on ( "/config/network", send_network_configuration_values_html );
  server.on ( "/config/networkstate", send_connection_state_values_html );

  server.on ( "/mqtt.html", show_mqtt_page );
  server.on ( "/general.html", show_general_page );

  server.begin();
  Serial.println("HTTP server started");

}
