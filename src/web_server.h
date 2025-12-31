#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <FS.h>

// WiFi credentials - you can change these
#define WIFI_SSID "CYD-MIDI"
#define WIFI_PASSWORD "midi1234"
#define WEB_SERVER_PORT 80

// External references needed from main file
extern bool sdCardAvailable;
extern SPIClass sdSPI;

// SD card pin (also defined in main file)
#ifndef SD_CS
#define SD_CS 5
#endif

// Web server instance
extern WebServer server;
extern bool wifiEnabled;
extern String wifiIPAddress;

// Core functions
void initializeWebServer();
void handleWebServer();
void stopWebServer();

// Web interface handlers
void handleRoot();
void handleFileList();
void handleFileUpload();
void handleFileDownload();
void handleFileDelete();
void handleScreenshot();
void handleScreenshots();
void handleWiFiGet();
void handleWiFiPost();
void handleNotFound();

// WiFi config helpers
bool loadWiFiConfig(String &ssid, String &password);
bool saveWiFiConfig(const String &ssid, const String &password);

#endif // WEB_SERVER_H
