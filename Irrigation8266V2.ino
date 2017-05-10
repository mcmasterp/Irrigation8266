

/*
    Esp8266-12e based irrigation remote unit. modmanpaul@gmail.com. consider it "poorware" if your poor like me it's free, if you got money share a small amount.
    monitor moisture, control solenoid, recieve commands from web connection
*/
#define DEBUG   //If you comment this line, the DPRINT & DPRINTLN lines are defined as blank.
#ifdef DEBUG    //Macros are usually in all capital letters.
#define DPRINT(...)    Serial.print(__VA_ARGS__)     //DPRINT is a macro, debug print
#define DPRINTLN(...)  Serial.println(__VA_ARGS__)   //DPRINTLN is a macro, debug print with new line
#else
#define DPRINT(...)     //now defines a blank line
#define DPRINTLN(...)   //now defines a blank line
#endif

#include <EEPROM.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>
#include <NTPClient.h>
#define NTP_OFFSET   60 * -240      // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "us.pool.ntp.org"
#define SERIAL_BAUDRATE                 115200
String webPage = "";
int solenoidPin = 14;
int moistPwrPin = 4;
int moistPin = A0;
int resetPin = 12;
int valveStatus;
String valveState;
int soil = 0;
#define DHTPIN            5         // Pin which is connected to the DHT sensor.
#define DHTTYPE           DHT11     // DHT 11 
int moistureLevel;
#define FIVEMIN (1000UL * 60 * 1) // change back to 1000UL * 60 * 5 for 5min
unsigned long rolltime = millis() + FIVEMIN;
#define FIVEMIN2 (1000UL * 60 * 1) // change back to 1000UL * 60 * 5 for 5min
unsigned long rolltime2 = millis() + FIVEMIN2;
#define FIVEMIN3 (1000UL * 60 * 1) // change back to 1000UL * 60 * 5 for 5min
unsigned long rolltime3 = millis() + FIVEMIN3;
#define FIVEMIN4 (1000UL * 60 * 3) // change back to 1000UL * 60 * 5 for 5min
unsigned long rolltime4 = millis() + FIVEMIN4;
#define TWENTYFOURHR (1000UL * 60 * 1440)
unsigned long resetrolltime = millis() + TWENTYFOURHR;
float h, f, hif;
String formattedTime;
int currentHrInt;
const char* www_username = "mcmasterp";
const char* www_password = "housedripper1";
DHT dht(DHTPIN, DHTTYPE);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);
ESP8266HTTPUpdateServer httpUpdater;
MDNSResponder mdns;
ESP8266WebServer server(80);
ESP8266WebServer httpServer(8267);
WiFiClient client;
const char* weatherServer = "api.wunderground.com";  // server's address
const char* resource = "/api/72c48896c1adb75c/conditions/lang:EN/q/zmw:19047.1.99999.json";                    // http resource
const unsigned long HTTP_TIMEOUT = 10000;  // max respone time from server
const size_t MAX_CONTENT_SIZE = 512;       // max size of the HTTP response
const char* current_observation_precip_today_metric;
long convertedstring = 0L;
long convertedTempstring;
struct UserData {
  char precip_today_metric[32];
};
int eeAddress = 0;   //Location we want the data to be put.
int onhr, offhr;
byte onhrb, offhrb;

// Open connection to the HTTP server
bool connect(const char* hostName) {
  DPRINT("Connect to ");
  DPRINTLN(hostName);

  bool ok = client.connect(hostName, 80);

  DPRINTLN(ok ? "Connected" : "Connection Failed!");
  return ok;
}




// Send the HTTP GET request to the server
bool sendRequest(const char* host, const char* resource) {
  DPRINT("GET ");
  DPRINTLN(resource);

  client.print("GET ");
  client.print(resource);
  client.println(" HTTP/1.0");
  client.print("Host: ");
  client.println(host);
  client.println("Connection: close");
  client.println();

  return true;
}





// Skip HTTP headers so that we are at the beginning of the response's body
bool skipResponseHeaders() {
  // HTTP headers end with an empty line
  char endOfHeaders[] = "\r\n\r\n";

  client.setTimeout(HTTP_TIMEOUT);
  bool ok = client.find(endOfHeaders);

  if (!ok) {
    DPRINTLN("No response or invalid response!");
  }

  return ok;
}




// Parse the JSON from the input string and extract the interesting values
bool readReponseContent(struct UserData* userData) {
  // Compute optimal size of the JSON buffer according to what we need to parse.
  // This is only required if you use StaticJsonBuffer.
  const size_t bufferSize = JSON_OBJECT_SIZE(0) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 2 * JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(8) + JSON_OBJECT_SIZE(12) + JSON_OBJECT_SIZE(56) + 2250;


  DynamicJsonBuffer jsonBuffer(bufferSize);


  JsonObject& root = jsonBuffer.parseObject(client);

  if (!root.success()) {
    DPRINTLN("JSON parsing failed!");
    return false;
  }
  // Here were copy the strings we're interested in
  JsonObject& current_observation = root["current_observation"];
  current_observation_precip_today_metric = current_observation["precip_today_metric"];
  convertedstring = atol(current_observation_precip_today_metric);
  const char* current_observation_temp_f = current_observation["temp_f"];
  convertedTempstring = atol(current_observation_temp_f);
  return true;
}

void disconnect() {
  DPRINTLN("Disconnect");
  client.stop();
}


void printUserData(const struct UserData* userData) {
  DPRINT("Precip: ");
  DPRINTLN(convertedstring);
  DPRINT("Temp: ");
  DPRINTLN(convertedTempstring);
}


void wunderget() {

  if ((long)(millis() - rolltime4) >= 0) {
    if (connect(weatherServer)) {
      if (sendRequest(weatherServer, resource) && skipResponseHeaders()) {
        UserData userData;
        if (readReponseContent(&userData)) {
          printUserData(&userData);
        }
      }
    }
    rolltime4 += FIVEMIN4;
    disconnect();
  }

}

void reboot() {
  if ((long)(millis() - resetrolltime) >= 0) {
    DPRINTLN("daily reboot");
    ESP.restart();
    resetrolltime += TWENTYFOURHR;
  }
}

void moistureSense() {
  if ((long)(millis() - rolltime) >= 0) {
    //  Do your five minute roll stuff here
    pinMode(moistPwrPin, OUTPUT);
    digitalWrite(moistPwrPin, HIGH);
    delay(100);
    moistureLevel = analogRead(moistPin);
    moistureLevel = constrain(moistureLevel, 100, 400);
    soil = map(moistureLevel, 100, 400, 100, 0);
    delay(100);
    digitalWrite(moistPwrPin, LOW);
    DPRINT("Moisture level: ");
    DPRINT(soil);
    DPRINT("%");
    DPRINT("\t");
    rolltime += FIVEMIN;
  }
}

void tempSense() {
  if ((long)(millis() - rolltime2) >= 0) {
    //int oldH = h;
    //int oldF = f;
    //h = dht.readHumidity();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //f = dht.readTemperature(true);
    //if (isnan(h) || isnan(f)) {
    //  h = oldH;
    //  f = oldF;
    //}
    // Compute heat index in Fahrenheit (the default)
    //hif = dht.computeHeatIndex(f, h);
    //DPRINT("Humidity: ");
    // DPRINT(h);
    //DPRINT(" %\t");
    DPRINT("Temperature: ");
    DPRINT(convertedTempstring);
    DPRINT(" *F\t");
    //  DPRINT("Heat index: ");
    //  DPRINT(hif);
    //  DPRINT(" *F\t");
    rolltime2 += FIVEMIN2;
  }

}

void timeCheck() {
  if ((long)(millis() - rolltime3) >= 0) {
    timeClient.update();
    formattedTime = timeClient.getFormattedTime();
    //int firstColon = formattedTime.indexOf(':');
    String currentHr = formattedTime.substring(0, 2);
    currentHrInt = currentHr.toInt();
    DPRINT("Time: ");
    DPRINTLN(formattedTime);
    DPRINT("Current Hr: ");
    DPRINTLN(currentHr);
    rolltime3 += FIVEMIN3;
  }
  delay(200);
}

void runSched() {
  onhrb = EEPROM.read(1);
  offhrb = EEPROM.read(2);
  int onhrbToInt = int(onhrb);
  if (currentHrInt == onhrbToInt) {
    if ((convertedstring < 10) && (convertedTempstring > 40)) { // if precipitation today is over 10mm or temp is below 40f
      digitalWrite(solenoidPin, LOW);
    }
  }
  int offhrbToInt = int(offhrb);
  if (currentHrInt == offhrbToInt) {
    digitalWrite(solenoidPin, HIGH);
  }
}

void wifiSetup() {
  WiFiManager wifiManager;
  wifiManager.autoConnect("dripperhubsetupAP", "password");
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.hostname("DripperHub");
  }
}

void runAllAtStart() {
  timeClient.update();
  formattedTime = timeClient.getFormattedTime();
  //int firstColon = formattedTime.indexOf(':');
  String currentHr = formattedTime.substring(0, 2);
  currentHrInt = currentHr.toInt();
  DPRINT("Time: ");
  DPRINTLN(formattedTime);
  DPRINT("Current Hr: ");
  DPRINTLN(currentHr);


  if (connect(weatherServer)) {
    if (sendRequest(weatherServer, resource) && skipResponseHeaders()) {
      UserData userData;
      if (readReponseContent(&userData)) {
        printUserData(&userData);
      }
    }
  }

  DPRINT("Temperature: ");
  DPRINT(convertedTempstring);
  DPRINT(" *F\t");
}

void setup() {

  Serial.begin(SERIAL_BAUDRATE);
  dht.begin();
  timeClient.begin();
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("dripperhub");
  ArduinoOTA.setPassword(www_password);
  ArduinoOTA.onStart([]() {
    DPRINTLN("Start");
  });
  ArduinoOTA.onEnd([]() {
    DPRINTLN("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) DPRINTLN("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) DPRINTLN("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) DPRINTLN("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) DPRINTLN("Receive Failed");
    else if (error == OTA_END_ERROR) DPRINTLN("End Failed");
  });
  ArduinoOTA.begin();

  pinMode(solenoidPin, OUTPUT);
  digitalWrite(solenoidPin, HIGH);
  pinMode(moistPin, INPUT);
  // Wifi
  wifiSetup();

  mdns.begin("dripperhub", WiFi.localIP());
  httpUpdater.setup(&httpServer);  //serve a webpage to upload new firmware
  httpServer.begin();

  MDNS.addService("http", "tcp", 8267);
  if (mdns.begin("dripperhub", WiFi.localIP())) {
    DPRINTLN("MDNS responder started");
  }
  EEPROM.begin(4);
  server.begin();
  DPRINTLN("HTTP server started");

  onhrb = EEPROM.read(1);
  offhrb = EEPROM.read(2);
  DPRINT("On Time: ");
  DPRINTLN(onhrb);
  DPRINT("Off Time: ");
  DPRINTLN(offhrb);

  runAllAtStart();

  server.on("/post", []() {
    valveStatus = digitalRead(solenoidPin);
    if (valveStatus == 1) {
      valveState = "Closed";
    }
    else if (valveStatus == 0) {
      valveState = "Open";
    }
    if (server.args() > 0 ) {
      for ( uint8_t i = 0; i < server.args(); i++ ) {
        if (server.argName(i) == "onhr") {
          onhr = server.arg(i).toInt();
          EEPROM.write(1, onhr);
          DPRINTLN(onhr);
        }
        if (server.argName(i) == "offhr") {
          offhr = server.arg(i).toInt();
          EEPROM.write(2, offhr);
          DPRINTLN(offhr);
        }
      }
      EEPROM.commit();
    }
    webPage = "<h1>DripperHub</h1>";
    webPage += "<p><a href=\"close\"><button>Close</button></a> <a href=\"open\"><button>Open</button></a>&nbsp;</p>";
    webPage += "<p><a href=\"forceopen\"><button>Force Open</button></a></p>";
    webPage += "<p><h2>Main Valve: " + String(valveState) + "</h2></p>";
    //webPage += "<p><h2>Moisture is " + String(soil) + "%</h2></p>";
    webPage += "<p><h2>Temp is " + String(convertedTempstring) + "F</h2></p>";
    webPage += "<p><h2>Time: " + String(formattedTime) + "</h2></p>";
    webPage += "<p><h2>Rain Today: " + String(convertedstring) + "mm</h2></p>";
    webPage += "<form id='form_30320' method='post' action='/post'><h2>Set Dripper schedule</h2><ul><li><label>On Time HR</label><input id='onhr' name='onhr'type='text' maxlength='2' value=''/></li><li><label>Off Time HR</label><input id='offhr' name='offhr'type='text' maxlength='2' value=''/></li><li><input type='hidden' name='form_id' value='30320' /><input id='saveForm' type='submit' name='submit' value='Submit' /></li></ul></form>  ";
    server.send(200, "text/html", webPage);

  });

  server.on("/", []() {
    valveStatus = digitalRead(solenoidPin);
    if (valveStatus == 1) {
      valveState = "Closed";
    }
    else if (valveStatus == 0) {
      valveState = "Open";
    }
    webPage = "<h1>DripperHub</h1>";
    webPage += "<p><a href=\"close\"><button>Close</button></a> <a href=\"open\"><button>Open</button></a>&nbsp;</p>";
    webPage += "<p><a href=\"forceopen\"><button>Force Open</button></a></p>";
    webPage += "<p><h2>Main Valve: " + String(valveState) + "</h2></p>";
    //webPage += "<p><h2>Moisture is " + String(soil) + "%</h2></p>";
    webPage += "<p><h2>Temp is " + String(convertedTempstring) + "F</h2></p>";
    webPage += "<p><h2>Time: " + String(formattedTime) + "</h2></p>";
    webPage += "<p><h2>Rain Today: " + String(convertedstring) + "mm</h2></p>";
    webPage += "<form id='form_30320' method='post' action='/post'><h2>Set Dripper schedule</h2><ul><li><label>On Time HR</label><input id='onhr' name='onhr'type='text' maxlength='2' value=''/></li><li><label>Off Time HR</label><input id='offhr' name='offhr'type='text' maxlength='2' value=''/></li><li><input type='hidden' name='form_id' value='30320' /><input id='saveForm' type='submit' name='submit' value='Submit' /></li></ul></form>  ";
    server.send(200, "text/html", webPage);

  });

  server.on("/open", []() {

    if ((convertedstring < 10) && (convertedTempstring > 40)) { // if precipitation today is over 10mm or temp is below 40f
      digitalWrite(solenoidPin, LOW);
    }
    valveStatus = digitalRead(solenoidPin);
    if (valveStatus == 1) {
      valveState = "Closed";
    }
    else if (valveStatus == 0) {
      valveState = "Open";
    }
    webPage = "<h1>DripperHub</h1>";
    webPage += "<p><a href=\"close\"><button>Close</button></a> <a href=\"open\"><button>Open</button></a>&nbsp;</p>";
    webPage += "<p><a href=\"forceopen\"><button>Force Open</button></a></p>";
    webPage += "<p><h2>Main Valve: " + String(valveState) + "</h2></p>";
    //webPage += "<p><h2>Moisture is " + String(soil) + "%</h2></p>";
    webPage += "<p><h2>Temp is " + String(convertedTempstring) + "F</h2></p>";
    webPage += "<p><h2>Time: " + String(formattedTime) + "</h2></p>";
    webPage += "<p><h2>Rain Today: " + String(convertedstring) + "mm</h2></p>";
    webPage += "<form id='form_30320' method='post' action='/post'><h2>Set Dripper schedule</h2><ul><li><label>On Time HR</label><input id='onhr' name='onhr'type='text' maxlength='2' value=''/></li><li><label>Off Time HR</label><input id='offhr' name='offhr'type='text' maxlength='2' value=''/></li><li><input type='hidden' name='form_id' value='30320' /><input id='saveForm' type='submit' name='submit' value='Submit' /></li></ul></form>  ";
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    else {
      server.send(200, "text/html", webPage);
    }
    DPRINTLN("Main Valve Open");
  });
  server.on("/forceopen", []() {

    digitalWrite(solenoidPin, LOW);
    valveStatus = digitalRead(solenoidPin);
    if (valveStatus == 1) {
      valveState = "Closed";
    }
    else if (valveStatus == 0) {
      valveState = "Open";
    }
    webPage = "<h1>DripperHub</h1>";
    webPage += "<p><a href=\"close\"><button>Close</button></a> <a href=\"open\"><button>Open</button></a>&nbsp;</p>";
    webPage += "<p><a href=\"forceopen\"><button>Force Open</button></a></p>";
    webPage += "<p><h2>Main Valve: " + String(valveState) + "</h2></p>";
    //webPage += "<p><h2>Moisture is " + String(soil) + "%</h2></p>";
    webPage += "<p><h2>Temp is " + String(convertedTempstring) + "F</h2></p>";
    webPage += "<p><h2>Time: " + String(formattedTime) + "</h2></p>";
    webPage += "<p><h2>Rain Today: " + String(convertedstring) + "mm</h2></p>";
    webPage += "<form id='form_30320' method='post' action='/post'><h2>Set Dripper schedule</h2><ul><li><label>On Time HR</label><input id='onhr' name='onhr'type='text' maxlength='2' value=''/></li><li><label>Off Time HR</label><input id='offhr' name='offhr'type='text' maxlength='2' value=''/></li><li><input type='hidden' name='form_id' value='30320' /><input id='saveForm' type='submit' name='submit' value='Submit' /></li></ul></form>  ";
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    else {
      server.send(200, "text/html", webPage);
    }
    DPRINTLN("Main Valve Open");
  });
  server.on("/close", []() {

    digitalWrite(solenoidPin, HIGH);
    valveStatus = digitalRead(solenoidPin);
    if (valveStatus == 1) {
      valveState = "Closed";
    }
    else if (valveStatus == 0) {
      valveState = "Open";
    }
    webPage = "<h1>DripperHub</h1>";
    webPage += "<p><a href=\"close\"><button>Close</button></a> <a href=\"open\"><button>Open</button></a>&nbsp;</p>";
    webPage += "<p><a href=\"forceopen\"><button>Force Open</button></a></p>";
    webPage += "<p><h2>Main Valve: " + String(valveState) + "</h2></p>";
    //webPage += "<p><h2>Moisture is " + String(soil) + "%</h2></p>";
    webPage += "<p><h2>Temp is " + String(convertedTempstring) + "F</h2></p>";
    webPage += "<p><h2>Time: " + String(formattedTime) + "</h2></p>";
    webPage += "<p><h2>Rain Today: " + String(convertedstring) + "mm</h2></p>";
    webPage += "<form id='form_30320' method='post' action='/post'><h2>Set Dripper schedule</h2><ul><li><label>On Time HR</label><input id='onhr' name='onhr'type='text' maxlength='2' value=''/></li><li><label>Off Time HR</label><input id='offhr' name='offhr'type='text' maxlength='2' value=''/></li><li><input type='hidden' name='form_id' value='30320' /><input id='saveForm' type='submit' name='submit' value='Submit' /></li></ul></form>  ";
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    else {
      server.send(200, "text/html", webPage);
    }
    DPRINTLN("Main Valve closed");
  });

  server.on("/moisture", []() {
    webPage = soil;
    server.send(200, "text/html", webPage);
    DPRINTLN("moisture read");
  });

  server.on("/status", []() {
    int valveStatus = digitalRead(solenoidPin);
    webPage = String(valveStatus);
    server.send(200, "text/html", webPage);
    DPRINTLN("valve status read");
  });

  server.on("/temp", []() {
    webPage = convertedTempstring;
    server.send(200, "text/html", webPage);
    DPRINTLN("temp read");
  });

  //server.on("/humidity", []() {
  //  webPage = h;
  //  server.send(200, "text/html", webPage);
  //  DPRINTLN("humidity read");
  //});

  server.on("/time", []() {
    webPage = formattedTime;
    server.send(200, "text/html", webPage);
    DPRINTLN("time read");
  });



}
void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  httpServer.handleClient();
  //moistureSense();
  timeCheck();
  wunderget();
  tempSense();
  runSched();
  reboot();
}


