#include "bambu_mqtt.hpp"
#include "projutils/projutils.hpp"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

class bambu_printer_data
{
    public:
        WiFiClientSecure wifi_secure;
        PubSubClient mqtt_client;
        String client_id = "D1Reader-";   

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
            #ifdef DEBUG_PRINT
            for (unsigned int ii = 0; (ii < length) && (ii < 100); ii++)
                Serial.print((char) payload[ii]);
            Serial.println();
            #endif
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