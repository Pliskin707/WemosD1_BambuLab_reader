#ifndef __RGB_LEDS_HPP__
#define __RGB_LEDS_HPP__

#include <NeoPixelBusLg.h>
#include "bambu_mqtt/bambu_mqtt_types.hpp"

using PixelBus = NeoPixelBusLg<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod>;
using color = RgbColor;

class rgb_leds : public PixelBus
{
    public:
        using PixelBus::NeoPixelBusLg;
        void update_with_report (const bambu_report_t& report);
};

namespace default_colors
{
const color yellow  = color(150, 60, 0);   
const color blue    = color(0, 0, 255);      
const color red     = color(255, 0, 0);       
const color green   = color(0, 130, 0);
const color black   = color((uint32_t) 0x000000);
const color white   = color(255, 177, 108);
};

#endif