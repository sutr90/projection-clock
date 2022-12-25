#ifndef Wifi_Manager_hpp
#define Wifi_Manager_hpp

#include <stdint.h>
#include <ESP8266WebServer.h>

class WifiManager
{
private:
    uint8_t _resetPin;
    bool testWifi(void);
    void launchWeb(void);
    void setupAP(void);
    void createWebServer(void);

    ESP8266WebServer server;
    String st;
    String content;
public:
    WifiManager(uint8_t resetPin, uint8_t webport = 80);
    void initialize();
};

#endif