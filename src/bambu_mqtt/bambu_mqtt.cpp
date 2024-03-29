#include "bambu_mqtt.hpp"
#include "projutils/projutils.hpp"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

namespace _bambu_mqtt
{

static JsonDocument filter;
static bool filter_initialized = false;

static void filter_init (void)
{
    if (!filter_initialized)
    {
        JsonObject filter_print = filter["print"].to<JsonObject>();
        JsonObject filter_print_ams = filter_print["ams"].to<JsonObject>();
        JsonObject filter_print_ams_ams_0_tray_0 = filter_print_ams["ams"][0]["tray"].add<JsonObject>();
        JsonObject filter_print_vt_tray = filter_print["vt_tray"].to<JsonObject>();

        filter_print_ams_ams_0_tray_0["id"] = true;
        filter_print_ams_ams_0_tray_0["tray_type"] = true;
        filter_print_ams["tray_now"] = true;
        filter_print["bed_temper"] = true;
        filter_print["chamber_temper"] = true;
        filter_print["gcode_state"] = true;
        filter_print["mc_percent"] = true;
        filter_print["mc_remaining_time"] = true;
        filter_print["nozzle_temper"] = true;

        filter_print_vt_tray["id"] = true;
        filter_print_vt_tray["tray_type"] = true;
    }

    filter_initialized = true;
}
};
// NOTE since this uses JsonVariantConst, all references created inside *must* also be non-mutable (i.e. JsonArrayConst instead of JsonArray)
void convertFromJson (JsonVariantConst src, bambu_report_t &dst)
{
    // reset all values to default
    dst = bambu_report_t();

    // read all AMS filament slots
    for (JsonObjectConst print_ams_ams_0_tray_item : src[F("ams")][F("ams")][0][F("tray")].as<JsonArrayConst>()) 
    {
        // get the ID as uint8_t
        // for whatever reason, bambu transfers this as string instead of integer
        JsonVariantConst print_ams_ams_0_tray_item_id = print_ams_ams_0_tray_item[F("id")]; // "0", "1", "2", "3"
        if (print_ams_ams_0_tray_item_id.isNull())
            continue;

        uint8_t id = print_ams_ams_0_tray_item_id.as<uint8_t>();
        const char* print_ams_ams_0_tray_item_tray_type = print_ams_ams_0_tray_item[F("tray_type")] | "<?>"; // "PLA", ...

        dst.filaments.insert({id, print_ams_ams_0_tray_item_tray_type});
    }

    // read the extenal spool holder slot
    JsonVariantConst print_vt_tray_id = src[F("vt_tray")][F("id")]; // "254"
    if (!print_vt_tray_id.isNull())
    {
        uint8_t id = print_vt_tray_id.as<uint8_t>();
        const char* print_vt_tray_tray_type = src[F("vt_tray")][F("tray_type")]; // i.e. "TPU"

        dst.filaments.insert({id, print_vt_tray_tray_type});
    }

    // read the loaded filament slot
    JsonVariantConst print_ams_tray_now = src[F("ams")][F("tray_now")]; // i.e. "255" if nothing is loaded
    if (!print_ams_tray_now.isNull())
        dst.filament_slot_id = print_ams_tray_now.as<uint8_t>();

    // read the relevant temperatures
    dst.temperatures.nozzle  = src[F("nozzle_temper")].as<float>(); // 20
    dst.temperatures.bed     = src[F("bed_temper")].as<float>(); // 17
    dst.temperatures.chamber = src[F("chamber_temper")].as<float>(); // 22

    // read the status infos
    dst.state.name              = src[F("gcode_state")].as<String>();                           // "FINISH", "RUNNING"
    dst.state.progress_percent  = src[F("mc_percent")]          | dst.state.progress_percent;   // 100
    dst.state.remaining_minutes = src[F("mc_remaining_time")]   | dst.state.remaining_minutes;  // 0

    return;
}

class bambu_printer_data
{
    public:
        WiFiClientSecure wifi_secure;
        PubSubClient mqtt_client;
        String client_id = "D1Reader-";   
        bambu_report_t report;

        bambu_printer_data() : mqtt_client(PubSubClient(wifi_secure)) 
        {
            client_id += String(random(0xffff), HEX);
        }
};


bambu_printer::bambu_printer() : _data(new bambu_printer_data) {}
bambu_printer::~bambu_printer()
{
    if (_data)
    {
        if (is_connected())
            _data->mqtt_client.disconnect();

        delete _data;
        _data = nullptr;
    }
}

bool bambu_printer::connect (const String ip, const String serial_num, const String auth)
{
    bool
        result = false;

    char
        topic[100];

    do
    {
        _bambu_mqtt::filter_init();

        if (!_data)
            break;

        if (is_connected())
            break;

        _data->wifi_secure.setInsecure();
        if (!_data->mqtt_client.setBufferSize(12000))   // last time I checked this was ~10284 bytes (excluding the MQTT header)
        {
            dprintf("MQTT buffer allocation failed");
            break;
        }

        _data->mqtt_client.setServer(ip.c_str(), 8883);
        // _data->mqtt_client.setStream(stream);
        // _data->mqtt_client.setCallback(mqttCallback);
        _data->mqtt_client.setSocketTimeout(20);

        if (!_data->mqtt_client.connect(_data->client_id.c_str(), "bblp", auth.c_str()))
        {
            dprintf("MQTT connection failed with %d\n", _data->mqtt_client.state());
            break;
        }

        snprintf_P(topic, sizeof(topic), PSTR("device/%s/report"), serial_num.c_str());
        topic[sizeof(topic) - 1] = 0;

        if (!_data->mqtt_client.subscribe(topic))
        {
            dprintf("MQTT subscription to \"%s\" failed\n", topic);
            break;
        }

        dprintf("Subscribed for \"%s\"\n", topic);

        _data->mqtt_client.setCallback([this](const char* topic, byte* payload, unsigned int length)
        {
            dprintf("MQTT data received (%lu bytes):\n", length)

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload, length, DeserializationOption::Filter(_bambu_mqtt::filter));

            // #ifdef DEBUG_PRINT
            // if (error)
            // {
            //     dprintf("Deserialize failed with %s\n", error.c_str());
            // }
            // else
            // {
            //     char printbuf[768];
            //     size_t printlen = serializeJson(doc, printbuf);

            //     for (size_t ii = 0; (ii < printlen) && (ii < sizeof(printbuf)); ii++)
            //         Serial.print((char) printbuf[ii]);

            //     Serial.println();
            // }
            // #endif

            if (!error)
            {
                auto& report = this->_data->report;
                report = doc[F("print")];
                dprintf("%lu filaments reported\n", report.filaments.size());

                for (const auto& fil : report.filaments)
                {
                    dprintf("Slot ID %u: \"%s\"%s\n", fil.first, fil.second, ((fil.first == report.filament_slot_id) ? " <-- loaded":""));
                }
                dprintf("State: \"%s\"\nPrinting progress: %u %%\nTime remaining: %lu min\nNozzle Temp: %u C\n", 
                report.state.name, (uint8_t) report.state.progress_percent, report.state.remaining_minutes, (uint8_t) report.temperatures.nozzle);
            }
        });

        result = true;  // success
    } while (false);
    
    return result;
}

bool bambu_printer::is_connected (void) const
{
    return (_data ? _data->mqtt_client.connected() : false);
}

bool bambu_printer::loop (void)
{
    return (_data ? _data->mqtt_client.loop() : false);
}