#ifndef __BAMBU_MQTT_HPP__
#define __BAMBU_MQTT_HPP__

#include <Arduino.h>
#include "bambu_mqtt_types.hpp"

class bambu_printer_data;
class bambu_printer
{
    private:
        bambu_printer_data * _data;

    public:
        bambu_printer();
        ~bambu_printer();

        bool connect (const String ip, const String serial_num, const String auth);
        bool is_connected (void) const;

        /** @returns `true` when still connected, `false` otherwise */
        bool loop (void);   

        /** @returns the last received report */
        const bambu_report_t& report (void) const;
};

#endif