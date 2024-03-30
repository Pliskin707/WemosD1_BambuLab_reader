#include "rgb_leds.hpp"
#include "projutils/projutils.hpp"

void rgb_leds::update_with_report (const bambu_report_t& report)
{
    float max_temp = std::max({report.temperatures.nozzle, report.temperatures.bed, report.temperatures.chamber});
    color led_color = default_colors::black;

    if (max_temp > 100)
        led_color = default_colors::red;
    else if (max_temp > 60)
        led_color = default_colors::yellow;
    else if (max_temp > 40)
        led_color = default_colors::green;

    for (uint16_t pixel = 0; pixel < this->PixelCount(); pixel++)
        this->SetPixelColor(pixel, led_color);

    this->Show();

    dprintf("LED color set to %02X%02X%02X\n", led_color.R, led_color.G, led_color.B);
}