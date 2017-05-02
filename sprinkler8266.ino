

/*
    Esp8266-12e based irrigation remote unit. modmanpaul@gmail.com. consider it "poorware" if your poor like me it's free, if you got money share a small amount.
    monitor moisture, control solenoid, recieve commands from web connection
*/
#include <DHT.h>
#include <DHT_U.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>
#include <NTPClient.h>
#define NTP_OFFSET   60 * -240      // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "us.pool.ntp.org"
#define SERIAL_BAUDRATE                 115200
MDNSResponder mdns;
ESP8266WebServer server(80);
String webPage = "";
int solenoidPin = 14;
int moistPwrPin = 4;
int moistPin = A0;
int resetPin = 12;
#define DHTPIN            5         // Pin which is connected to the DHT sensor.
#define DHTTYPE           DHT11     // DHT 11 
int moistureLevel;
#define FIVEMIN (1000UL * 3 * 1) // change back to 1000UL * 60 * 5 for 5min
unsigned long rolltime = millis() + FIVEMIN;
#define FIVEMIN2 (1000UL * 3 * 1) // change back to 1000UL * 60 * 5 for 5min
unsigned long rolltime2 = millis() + FIVEMIN2;
#define FIVEMIN3 (1000UL * 3 * 1) // change back to 1000UL * 60 * 5 for 5min
unsigned long rolltime3 = millis() + FIVEMIN3;
#define TWENTYHR (1000UL * 60 * 1200)
unsigned long resetrolltime = millis() + TWENTYHR;
int serialPrinting = 1;
float h, f, hif;
String formattedTime;
DHT dht(DHTPIN, DHTTYPE);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

void reboot() {
  if ((long)(millis() - resetrolltime) >= 0) {
    if (serialPrinting == 1) {
      Serial.println("daily reboot");
    }
    pinMode(resetPin, OUTPUT);
    digitalWrite(resetPin, LOW);
    resetrolltime += TWENTYHR;
  }
}

void moistureSense() {
  if ((long)(millis() - rolltime) >= 0) {
    //  Do your five minute roll stuff here
    pinMode(moistPwrPin, OUTPUT);
    digitalWrite(moistPwrPin, HIGH);
    moistureLevel = analogRead(moistPin);
    delay(100);
    digitalWrite(moistPwrPin, LOW);
    if (serialPrinting == 1) {
      Serial.print("Moisture level: ");
      Serial.print(moistureLevel);
      Serial.print("\t");
    }
    rolltime += FIVEMIN;
  }
}

void tempSense() {
  if ((long)(millis() - rolltime2) >= 0) {
    h = dht.readHumidity();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    f = dht.readTemperature(true);
    // Compute heat index in Fahrenheit (the default)
    hif = dht.computeHeatIndex(f, h);
    if (serialPrinting == 1) {
      Serial.print("Humidity: ");
      Serial.print(h);
      Serial.print(" %\t");
      Serial.print("Temperature: ");
      Serial.print(f);
      Serial.print(" *F\t");
      //  Serial.print("Heat index: ");
      //  Serial.print(hif);
      //  Serial.print(" *F\t");
    }
    rolltime2 += FIVEMIN2;
  }

}

void timeCheck() {
  if ((long)(millis() - rolltime3) >= 0) {
    timeClient.update();
    formattedTime = timeClient.getFormattedTime();
    if (serialPrinting == 1) {
      Serial.print("Time: ");
      Serial.println(formattedTime);
    }
    rolltime3 += FIVEMIN3;
  }
  delay(250);
}

void wifiSetup() {
  WiFiManager wifiManager;
  wifiManager.autoConnect("dripper1setupAP", "password");
}
void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  dht.begin();
  timeClient.begin();
  pinMode(solenoidPin, OUTPUT);
  pinMode(moistPin, INPUT);
  // Wifi
  wifiSetup();
mdns.begin("dripper1", WiFi.localIP());
  if (mdns.begin("dripper1", WiFi.localIP())) {
    if (serialPrinting == 1) {
      Serial.println("MDNS responder started");
    }
  }

  server.begin();
  if (serialPrinting == 1) {
    Serial.println("HTTP server started");
  }

  server.on("/", []() {
    int valveStatus = digitalRead(solenoidPin);
    webPage = "<h1>Dripper1</h1><p><a href=\"off\"><button>Off</button></a> <a href=\"on\"><button>On</button></a>&nbsp;</p><p><h2>Status: " + String(valveStatus) + "</h2><p><p><h2>Moisture is " + String(moistureLevel) + "</h2><p><p><h2>Temp is " + String(f) + "</h2><p>";
    server.send(200, "text/html", webPage);
  });

  server.on("/on", []() {
    //if (moistureLevel > 500) {
    digitalWrite(solenoidPin, HIGH);
    // }
    // else {
    //   digitalWrite(solenoidPin, LOW);
    // }
    int valveStatus = digitalRead(solenoidPin);
    webPage = "<h1>Dripper1</h1><p><a href=\"off\"><button>Off</button></a>&nbsp;</p><p><h2>Status: " + String(valveStatus) + "</h2><p><p><h2>Moisture is " + String(moistureLevel) + "</h2><p><p><h2>Temp is " + String(f) + "</h2><p>";
    server.send(200, "text/html", webPage);
    if (serialPrinting == 1) {
      Serial.println("Dripper 1 on");
    }
  });
  server.on("/off", []() {

    digitalWrite(solenoidPin, LOW);
    int valveStatus = digitalRead(solenoidPin);
    webPage = "<h1>Dripper1</h1><p><a href=\"on\"><button>On</button></a>&nbsp;</p><p><h2>Status: " + String(valveStatus) + "</h2><p><p><h2>Moisture is " + String(moistureLevel) + "</h2><p><p><h2>Temp is " + String(f) + "</h2><p>";
    server.send(200, "text/html", webPage);
    if (serialPrinting == 1) {
      Serial.println("Dripper 1 off");
    }
  });

  server.on("/moisture", []() {
    webPage = moistureLevel;
    server.send(200, "text/html", webPage);
    if (serialPrinting == 1) {
      Serial.println("moisture read");
    }
  });

  server.on("/status", []() {
    int valveStatus = digitalRead(solenoidPin);
    webPage = String(valveStatus);
    server.send(200, "text/html", webPage);
    if (serialPrinting == 1) {
      Serial.println("valve status read");
    }
  });



}
void loop() {
  server.handleClient();
  moistureSense();
  tempSense();
  timeCheck();
  reboot();
}


