#ifndef __OTA_HPP__
#define __OTA_HPP__

#include <ArduinoOTA.h>

namespace pliskin
{

/**
 * @brief This is just a wrapper for the individual ArduinoOTA steps 
 * 
 * Used to tidy up the code a bit
 */
class ota
{
    private:
        ota();  // no need to construct (everything is static)
        ~ota();

    public:
        static void begin (const char * const deviceName);
        static void handle (void) {ArduinoOTA.handle();};
};

};

#endif