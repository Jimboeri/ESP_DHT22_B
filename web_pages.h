//
//  WEB_PAGES.H
// This file holds the HTML for web pages displayed by the admin interface
//

IPAddress ip;

void show_home_page()
{

  String html = "";
  html += HTML_Header;
  html += "<strong>HOME Page</strong><br>";

  html += "<a href=\"general.html\" style=\"width: 250px\" class=\"btn btn--m btn--blue\" >General Configuration</a><br>";
  html += "<a href=\"network.html\" style=\"width: 250px\" class=\"btn btn--m btn--blue\" >Network Configuration</a><br>";
  html += "<a href=\"mqtt.html\" style=\"width: 250px\" class=\"btn btn--m btn--blue\" >MQTT Configuration</a><br>";

  html += "<hr>";

  html += "<h2>Wifi</H2><P>SSID : ";
  html += config.ssid;
  html += "<br>Status : ";
  html += WifiStatusText(WiFi.status());
  html += "<br>IP Address : ";
  html += WiFi.localIP().toString();

  html += HTML_Footer;
  server.send ( 200, "text/html", html);
  adminTimer = millis();
  Serial.println(__FUNCTION__);
}

/*
   Shows the page for general data fields
*/
void show_general_page()
{
  // If there are arguments provided it means the Form has been submitted
  if (server.args() > 0 )  // Save Settings
  {
    Serial.println("General settings update");
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      if (server.argName(i) == "nodename") config.nodeName =   urldecode(server.arg(i));

      if (server.argName(i) == "dataperiod")
      {
        String temp = urldecode(server.arg(i));   // args are strings and need to be converted to ints
        config.dataPeriod = temp.toInt();
      }

      if (server.argName(i) == "statusperiod")
      {
        String temp = urldecode(server.arg(i));
        config.statusPeriod = temp.toInt();
      }
    }
    WriteConfig();
  }

  // create the webpage as a string
  String html = HTML_Header;

  html += "<a href=\"/\"  class=\"btn btn--s\"><</a>&nbsp;&nbsp;<strong>General Configuration</strong>";
  html += "<hr>General settings:<br><form action=\"\" method=\"get\"><table border=\"0\"  cellspacing=\"0\" cellpadding=\"3\" style=\"width:350px\" >";

  html += HTMLTableTextLine("Node Name : ", "nodename", config.nodeName);
  html += HTMLTableIntLine("Regular Data period (seconds) : ", "dataperiod", config.dataPeriod);
  html += HTMLTableIntLine("Status update period (seconds): ", "statusperiod", config.statusPeriod);

  html += "<tr><td colspan=\"2\" align=\"center\"><input type=\"submit\" style=\"width:150px\" class=\"btn btn--m btn--blue\" value=\"Save\"></td></tr> </table> </form>";

  html += HTML_Footer;
  server.send ( 200, "text/html", html);
  adminTimer = millis();

}

/*
   Shows the page for MQTT settings
*/
void show_mqtt_page()
{

  if (server.args() > 0 )  // If form has been submitted
  {
    Serial.println("Web update");
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      if (server.argName(i) == "host1") config.MQTT_Host1 =   urldecode(server.arg(i));
      if (server.argName(i) == "port1") config.MQTT_Port1 =  urldecode(server.arg(i)).toInt();
      if (server.argName(i) == "user1") config.MQTT_User1 =   urldecode(server.arg(i));
      if (server.argName(i) == "password1") config.MQTT_Password1 =   urldecode(server.arg(i));
      if (server.argName(i) == "topic1") config.MQTT_Topic1 =   urldecode(server.arg(i));
    }

    WriteConfig();
    //adminTimer = millis();
  }

  // create the web page as a string
  String html = HTML_Header;
  html += "<a href=\"/\"  class=\"btn btn--s\"><</a>&nbsp;&nbsp;<strong>MQTT Configuration</strong>";
  html += "<hr>MQTT settings:<br><form action=\"\" method=\"get\"><table border=\"0\"  cellspacing=\"0\" cellpadding=\"3\" style=\"width:350px\" >";

  html += HTMLTableTextLine("Host 1 : ", "host1", config.MQTT_Host1);
  html += HTMLTableIntLine("Port 1 : ", "port1", config.MQTT_Port1);
  html += HTMLTableTextLine("User 1 : ", "user1", config.MQTT_User1);
  html += HTMLTableTextLine("Password 1 : ", "password1", config.MQTT_Password1);
  html += HTMLTableTextLine("Topic 1 : ", "topic1", config.MQTT_Topic1);

  html += "<tr><td colspan=\"2\" align=\"center\"><input type=\"submit\" style=\"width:150px\" class=\"btn btn--m btn--blue\" value=\"Save\"></td></tr> </table> </form>";

  html += HTML_Footer;
  server.send ( 200, "text/html", html);
  adminTimer = millis();

}

/*
   These pages rely on javascript. You are on your own
*/
const char PAGE_NetworkConfiguration[] PROGMEM = R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<a href="/"  class="btn btn--s"><</a>&nbsp;&nbsp;<strong>Network Configuration</strong>
<hr>
Connect to Router with these settings:<br>
<form action="" method="get">
<table border="0"  cellspacing="0" cellpadding="3" style="width:310px" >
<tr><td align="right">SSID:</td><td><input type="text" id="ssid" name="ssid" value=""></td></tr>
<tr><td align="right">Password:</td><td><input type="text" id="password" name="password" value=""></td></tr>
<tr><td colspan="2" align="center"><input type="submit" style="width:150px" class="btn btn--m btn--blue" value="Save"></td></tr>
</table>
</form>
<hr>
<strong>Connection State:</strong><div id="connectionstate">N/A</div>
<hr>
<strong>Networks:</strong><br>
<table border="0"  cellspacing="3" style="width:310px" >
<tr><td><div id="networks">Scanning...</div></td></tr>
<tr><td align="center"><a href="javascript:GetState()" style="width:150px" class="btn btn--m btn--blue">Refresh</a></td></tr>
</table>

<script>

function GetState()
{
  setValues("/config/networkstate");
}
function selssid(value)
{
  document.getElementById("ssid").value = value; 
}

window.onload = function ()
{
  load("style.css","css", function() 
  {
    load("microajax.js","js", function() 
    {
          setValues("/config/network");
          setTimeout(GetState,3000);
    });
  });
}

function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}

</script>
)=====";

const char PAGE_WaitAndReload[] PROGMEM = R"=====(
<meta http-equiv="refresh" content="5; URL=network.html">
Please Wait....Configuring and Restarting.
)=====";


//
//  SEND HTML PAGE OR IF A FORM SUMBITTED VALUES, PROCESS THESE VALUES
// 

void send_network_configuration_html()
{
  
  if (server.args() > 0 )  // Save Settings
  {
    String temp = "";
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      if (server.argName(i) == "ssid") config.ssid =   urldecode(server.arg(i));
      if (server.argName(i) == "password") config.password =    urldecode(server.arg(i));
      
    }
    server.send ( 200, "text/html", PAGE_WaitAndReload );
    WriteConfig();
    ConfigureWifi();    
  }
  else
  {
    server.send ( 200, "text/html", PAGE_NetworkConfiguration ); 
  }
  adminTimer = millis();
  Serial.println(__FUNCTION__); 
}



//
//   FILL THE PAGE WITH VALUES
//

void send_network_configuration_values_html()
{

  String values ="";

  values += "ssid|" + (String) config.ssid + "|input\n";
  values += "password|" +  (String) config.password + "|input\n";
   + "|chk\n";
  server.send ( 200, "text/plain", values);
  Serial.println(__FUNCTION__); 
  
}


//
//   FILL THE PAGE WITH NETWORKSTATE & NETWORKS
//

void send_connection_state_values_html()
{

  String state = "N/A";
  String Networks = "";
  if (WiFi.status() == 0) state = "Idle";
  else if (WiFi.status() == 1) state = "NO SSID AVAILBLE";
  else if (WiFi.status() == 2) state = "SCAN COMPLETED";
  else if (WiFi.status() == 3) state = "CONNECTED";
  else if (WiFi.status() == 4) state = "CONNECT FAILED";
  else if (WiFi.status() == 5) state = "CONNECTION LOST";
  else if (WiFi.status() == 6) state = "DISCONNECTED";

  int n = WiFi.scanNetworks();

  if (n == 0)
   {
     Networks = "<font color='#FF0000'>No networks found!</font>";
   }
  else
    {
    Networks = "Found " +String(n) + " Networks<br>";
    Networks += "<table border='0' cellspacing='0' cellpadding='3'>";
    Networks += "<tr bgcolor='#DDDDDD' ><td><strong>Name</strong></td><td><strong>Quality</strong></td><td><strong>Enc</strong></td><tr>";
    for (int i = 0; i < n; ++i)
    {
      int quality=0;
      if(WiFi.RSSI(i) <= -100)
      {
          quality = 0;
      }
      else if(WiFi.RSSI(i) >= -50)
      {
          quality = 100;
      }
      else
      {
        quality = 2 * (WiFi.RSSI(i) + 100);
      }

      Networks += "<tr><td><a href='javascript:selssid(\""  +  String(WiFi.SSID(i))  + "\")'>"  +  String(WiFi.SSID(i))  + "</a></td><td>" +  String(quality) + "%</td><td>" +  String((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*")  + "</td></tr>";
    }
    Networks += "</table>";
  }
   
  String values ="";
  values += "connectionstate|" +  state + "|div\n";
  values += "networks|" +  Networks + "|div\n";
  server.send ( 200, "text/plain", values);
  Serial.println(__FUNCTION__); 
  
}
