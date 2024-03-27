#include "ota.hpp"
#include "config.hpp"

namespace pliskin
{

void ota::begin (const char * const deviceName)
{
    ArduinoOTA.setHostname(deviceName);

    ArduinoOTA.onStart([]() 
    {
        wifi_set_sleep_type(NONE_SLEEP_T);
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) 
    {
        #ifndef DEBUG_PRINT
        digitalWrite(LEDPIN, !digitalRead(LEDPIN));
        #endif
    });

    ArduinoOTA.onError([](ota_error_t error) 
    {
        ESP.reset();
    });

    ArduinoOTA.onEnd([]() 
    {
        ESP.reset();
    });

    ArduinoOTA.begin();
}

};