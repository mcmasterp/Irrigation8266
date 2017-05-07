

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
int serialPrinting = 1;
float h, f, hif;
String formattedTime;
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
long convertedstring;
struct UserData {
  char precip_today_metric[32];
};


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

  //const char* json = "{\"response\":{\"version\":\"0.1\",\"termsofService\":\"http://www.wunderground.com/weather/api/d/terms.html\",\"features\":{\"conditions\":1}},\"current_observation\":{\"image\":{\"url\":\"http://icons.wxug.com/graphics/wu2/logo_130x80.png\",\"title\":\"Weather Underground\",\"link\":\"http://www.wunderground.com\"},\"display_location\":{\"full\":\"Langhorne, PA\",\"city\":\"Langhorne\",\"state\":\"PA\",\"state_name\":\"Pennsylvania\",\"country\":\"US\",\"country_iso3166\":\"US\",\"zip\":\"19047\",\"magic\":\"1\",\"wmo\":\"99999\",\"latitude\":\"40.18000031\",\"longitude\":\"-74.91000366\",\"elevation\":\"61.0\"},\"observation_location\":{\"full\":\"Highland Park, Levittown, Pennsylvania\",\"city\":\"Highland Park, Levittown\",\"state\":\"Pennsylvania\",\"country\":\"US\",\"country_iso3166\":\"US\",\"latitude\":\"40.165638\",\"longitude\":\"-74.889374\",\"elevation\":\"98 ft\"},\"estimated\":{},\"station_id\":\"KPALEVIT10\",\"observation_time\":\"Last Updated on May 5, 8:14 PM EDT\",\"observation_time_rfc822\":\"Fri, 05 May 2017 20:14:28 -0400\",\"observation_epoch\":\"1494029668\",\"local_time_rfc822\":\"Fri, 05 May 2017 20:14:33 -0400\",\"local_epoch\":\"1494029673\",\"local_tz_short\":\"EDT\",\"local_tz_long\":\"America/New_York\",\"local_tz_offset\":\"-0400\",\"weather\":\"Overcast\",\"temperature_string\":\"66.4 F (19.1 C)\",\"temp_f\":66.4,\"temp_c\":19.1,\"relative_humidity\":\"91%\",\"wind_string\":\"From the SW at 1.8 MPH Gusting to 2.5 MPH\",\"wind_dir\":\"SW\",\"wind_degrees\":233,\"wind_mph\":1.8,\"wind_gust_mph\":\"2.5\",\"wind_kph\":2.9,\"wind_gust_kph\":\"4.0\",\"pressure_mb\":\"998\",\"pressure_in\":\"29.49\",\"pressure_trend\":\"0\",\"dewpoint_string\":\"64 F (18 C)\",\"dewpoint_f\":64,\"dewpoint_c\":18,\"heat_index_string\":\"NA\",\"heat_index_f\":\"NA\",\"heat_index_c\":\"NA\",\"windchill_string\":\"NA\",\"windchill_f\":\"NA\",\"windchill_c\":\"NA\",\"feelslike_string\":\"66.4 F (19.1 C)\",\"feelslike_f\":\"66.4\",\"feelslike_c\":\"19.1\",\"visibility_mi\":\"10.0\",\"visibility_km\":\"16.1\",\"solarradiation\":\"0\",\"UV\":\"0.0\",\"precip_1hr_string\":\"0.00 in ( 0 mm)\",\"precip_1hr_in\":\"0.00\",\"precip_1hr_metric\":\" 0\",\"precip_today_string\":\"1.48 in (38 mm)\",\"precip_today_in\":\"1.48\",\"precip_today_metric\":\"38\",\"icon\":\"cloudy\",\"icon_url\":\"http://icons.wxug.com/i/c/k/nt_cloudy.gif\",\"forecast_url\":\"http://www.wunderground.com/US/PA/Langhorne.html\",\"history_url\":\"http://www.wunderground.com/weatherstation/WXDailyHistory.asp?ID=KPALEVIT10\",\"ob_url\":\"http://www.wunderground.com/cgi-bin/findweather/getForecast?query=40.165638,-74.889374\",\"nowcast\":\"\"}}";

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
  return true;
}

void disconnect() {
  DPRINTLN("Disconnect");
  client.stop();
}


void printUserData(const struct UserData* userData) {
  DPRINT("Precip: ");
  DPRINTLN(current_observation_precip_today_metric);
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
    int oldH = h;
    int oldF = f;
    h = dht.readHumidity();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    f = dht.readTemperature(true);
    if (isnan(h) || isnan(f)) {
      h = oldH;
      f = oldF;
    }
    // Compute heat index in Fahrenheit (the default)
    //hif = dht.computeHeatIndex(f, h);
    DPRINT("Humidity: ");
    DPRINT(h);
    DPRINT(" %\t");
    DPRINT("Temperature: ");
    DPRINT(f);
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
    DPRINT("Time: ");
    DPRINTLN(formattedTime);
    rolltime3 += FIVEMIN3;
  }
  delay(200);
}



void wifiSetup() {
  WiFiManager wifiManager;
  wifiManager.autoConnect("dripperhubsetupAP", "password");
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.hostname("DripperHub");
  }
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

  server.begin();
  DPRINTLN("HTTP server started");

  server.on("/", []() {
    valveStatus = digitalRead(solenoidPin);
    if (valveStatus == 1) {
      valveState = "Closed";
    }
    else if (valveStatus == 0) {
      valveState = "Open";
    }
    webPage = "<h1>DripperHub</h1><p><a href=\"close\"><button>Close</button></a> <a href=\"open\"><button>Open</button></a>&nbsp;</p><p><a href=\"forceopen\"><button>Force Open</button></a></p><p><h2>Main Valve: " + String(valveState) + "</h2></p><p><h2>Moisture is " + String(soil) + "%</h2></p><p><h2>Temp is " + String(f) + "F</h2></p><p><h2>Time: " + String(formattedTime) + "</h2></p><p><h2>Rain Today: " + String(convertedstring) + "mm</h2></p>";

      server.send(200, "text/html", webPage);

  });

  server.on("/open", []() {
    
    if (convertedstring <= 10 && f >= 40) { // if precipitation today is over 10mm or temp is below 40f
      digitalWrite(solenoidPin, LOW);
    }
    valveStatus = digitalRead(solenoidPin);
    if (valveStatus == 1) {
      valveState = "Closed";
    }
    else if (valveStatus == 0) {
      valveState = "Open";
    }
    webPage = "<h1>DripperHub</h1><p><a href=\"close\"><button>Close</button></a>&nbsp;</p><p><h2>Main Valve: " + String(valveState) + "</h2></p><p><h2>Moisture is " + String(soil) + "</h2></p><p><h2>Temp is " + String(f) + "</h2></p><p><h2>Time: " + String(formattedTime) + "</h2></p><p><h2>Rain Today: " + String(convertedstring) + "mm</h2></p>";
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
    webPage = "<h1>DripperHub</h1><p><a href=\"close\"><button>Close</button></a>&nbsp;</p><p><h2>Main Valve: " + String(valveState) + "</h2></p><p><h2>Moisture is " + String(soil) + "</h2></p><p><h2>Temp is " + String(f) + "</h2></p><p><h2>Time: " + String(formattedTime) + "</h2></p><p><h2>Rain Today: " + String(convertedstring) + "mm</h2></p>";
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
    webPage = "<h1>DripperHub</h1><p><a href=\"open\"><button>Open</button></a>&nbsp;</p><p><a href=\"forceopen\"><button>Force Open</button></a></p><p><h2>Main Valve: " + String(valveState) + "</h2></p><p><h2>Moisture is " + String(soil) + "</h2></p><p><h2>Temp is " + String(f) + "</h2><p><p><h2>Time: " + String(formattedTime) + "</h2></p><p><h2>Rain Today: " + String(convertedstring) + "mm</h2></p>";
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
    webPage = f;
    server.send(200, "text/html", webPage);
    DPRINTLN("temp read");
  });

  server.on("/humidity", []() {
    webPage = h;
    server.send(200, "text/html", webPage);
    DPRINTLN("humidity read");
  });

  server.on("/time", []() {
    webPage = formattedTime;
    server.send(200, "text/html", webPage);
    DPRINTLN("time read");
  });


moistureSense();
  tempSense();
  timeCheck();
  wunderget();
}
void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  httpServer.handleClient();
  moistureSense();
  tempSense();
  timeCheck();
  wunderget();
  reboot();
}


