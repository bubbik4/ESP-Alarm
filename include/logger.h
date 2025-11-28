#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>

// Exporting theese global objects
extern WiFiServer logServer;
extern WiFiClient logClient;

void initLogger();
void handleLogger();
String getTimestamp();

void logPrint(const String &msg);

void shortBlink(int intrerval = 100);

// Macro's
// ANSI color codes
#define COL_RESET   "\033[0m"
#define COL_WHITE   "\033[37m" 
#define COL_BLUE    "\033[94m"
#define COL_YELLOW  "\033[33m"
#define COL_RED     "\033[31m"
#define COL_BRED    "\033[1;31m"  // bold red

#define RAW(msg)    logPrint(getTimestamp() + " " + String(msg) + "\r\n")
#define ALERT(msg)  logPrint(getTimestamp() + " " +   String(COL_BRED) + "  [ALERT] " + String(msg) + String(COL_RESET) + "\r\n")
#define LOG(msg)    logPrint(getTimestamp() + " " +  String(COL_WHITE) + "  [LOG]   " + String(msg) + String(COL_RESET) + "\r\n")
#define INFO(msg)   logPrint(getTimestamp() + " " +   String(COL_BLUE) + "  [INFO]  " + String(msg) + String(COL_RESET) + "\r\n")
#define WARN(msg)   logPrint(getTimestamp() + " " + String(COL_YELLOW) + "  [WARN]  " + String(msg) + String(COL_RESET) + "\r\n")
#define ERROR(msg)  logPrint(getTimestamp() + " " +    String(COL_RED) + "  [ERROR] " + String(msg) + String(COL_RESET) + "\r\n")