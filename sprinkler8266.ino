


/*
    Esp8266-12e based irrigation remote unit. modmanpaul@gmail.com. consider it "poorware" if your poor like me it's free, if you got money share a small amount.
    monitor moisture, control solenoid, recieve commands from web connection
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>
#define SERIAL_BAUDRATE                 115200
const char* ssid = "FiOS-SGVFL";            // Replace with your network credentials
const char* password = "jib7584rows3699tan";
MDNSResponder mdns;
ESP8266WebServer server(80);
String webPage = "";
int solenoidPin = 14;
int serialPrinting = 1;
int moistPin = A0;
int resetPin = 12;
int moistureLevel;
#define FIVEMIN (1000UL * 10 * 1)
unsigned long rolltime = millis() + FIVEMIN;
#define TWENTYHR (1000UL * 60 * 1200)
unsigned long resetrolltime = millis() + TWENTYHR;

void reboot() {
  if ((long)(millis() - resetrolltime) >= 0) {
    pinMode(resetPin, OUTPUT);
    digitalWrite(resetPin, LOW);
    resetrolltime += TWENTYHR;
  }
}

void moistureSense() {
  if ((long)(millis() - rolltime) >= 0) {
    //  Do your five minute roll stuff here
    moistureLevel = analogRead(moistPin);
    if (serialPrinting == 1) {
      Serial.println(moistureLevel);
    }
    rolltime += FIVEMIN;
  }
}

void wifiSetup() {
  WiFiManager wifiManager;
  wifiManager.autoConnect("dripper1setupAP", "password");
}
void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  pinMode(solenoidPin, OUTPUT);
  pinMode(moistPin, INPUT);
  // Wifi
  wifiSetup();

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
    int valveVal = digitalRead(solenoidPin);
    if (valveVal == 1) {
      webPage = "<h1>Dripper1</h1><p><a href=\"off\"><button>Off</button></a>&nbsp;</p><p><h2>Status: ON</h2><p><p><h2>Moisture is " + String(moistureLevel) + "</h2><p>";
    }
    else {
      webPage = "<h1>Dripper1</h1><p><a href=\"on\"><button>On</button></a>&nbsp;</p><p><h2>Status: OFF</h2><p><p><h2>Moisture is " + String(moistureLevel) + "</h2><p>";
    }
    server.send(200, "text/html", webPage);
  });

  server.on("/on", []() {
    webPage = "<h1>Dripper1</h1><p><a href=\"off\"><button>Off</button></a>&nbsp;</p><p><h2>Status: ON</h2><p><p><h2>Moisture is " + String(moistureLevel) + "</h2><p>";
    server.send(200, "text/html", webPage);
    digitalWrite(solenoidPin, HIGH);
    if (serialPrinting == 1) {
      Serial.println("Dripper 1 on");
    }
  });
  server.on("/off", []() {
    webPage = "<h1>Dripper1</h1><p><a href=\"on\"><button>On</button></a>&nbsp;</p><p><h2>Status: OFF</h2><p><p><h2>Moisture is " + String(moistureLevel) + "</h2><p>";
    server.send(200, "text/html", webPage);
    digitalWrite(solenoidPin, LOW);
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
    int valveVal = digitalRead(solenoidPin);
    if (valveVal == 1) {
      webPage = "1";
    }
    else {
      webPage = "0";
    }
    server.send(200, "text/html", webPage);
    if (serialPrinting == 1) {
      Serial.println("valve status read");
    }
  });



}
void loop() {
  server.handleClient();
  moistureSense();
  reboot();
}


