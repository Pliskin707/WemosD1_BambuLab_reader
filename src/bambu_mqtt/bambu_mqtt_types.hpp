#ifndef __BAMBU_MQTT_TYPES_HPP__
#define __BAMBU_MQTT_TYPES_HPP__

#include <ArduinoJson.h>
#include <unordered_map>

namespace bambu_mqtt
{

#define FILAMENT_SLOT_ID_NONE       (255u)      // no filament loaded
#define TEMPERATURE_NOT_AVAILABLE   (-1)        // value could not be read
typedef std::unordered_map<uint8_t, String> filaments_t;

// negative temperatures could not be read
typedef struct
{
    float nozzle    = TEMPERATURE_NOT_AVAILABLE;
    float bed       = TEMPERATURE_NOT_AVAILABLE;
    float chamber   = TEMPERATURE_NOT_AVAILABLE;
} temperatures_t;

typedef struct
{
    String name;                                // "FINISH" etc.
    float progress_percent      = -1;           // 0 .. 100
    uint32_t remaining_minutes  = UINT32_MAX;
} state_t;

};

typedef struct
{
    bambu_mqtt::filaments_t filaments;
    uint8_t filament_slot_id = FILAMENT_SLOT_ID_NONE;   // currently loaded filament
    bambu_mqtt::temperatures_t temperatures;
    bambu_mqtt::state_t state;
} bambu_report_t;

// these are custom converters for ArduinoJson
void convertFromJson (JsonVariantConst src, bambu_report_t &dst);

#endif