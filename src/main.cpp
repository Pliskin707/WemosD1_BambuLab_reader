#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "ota/ota.hpp"
#include "projutils/projutils.hpp"
#include "config.hpp"
#include "bambu_mqtt/bambu_mqtt.hpp"
#include "rgb_leds/rgb_leds.hpp"

using namespace pliskin;

/** Wifi authentication **
 * 
 * this file needs to be created with the following content (and is obviously not included in version control):
 * 
 * #pragma once
 * 
 * const char* ssid = "<YourSSIDhere>";
 * const char* password = "<YourPasswordHere>";
 */
#include "../../../../../../wifiauth2.h"

/** Printer info
 * 
 * this file can be created by copying or renaming the \e printer_info_template.hpp 
 * and filling in the required information
 */
#include "../printer_info.hpp"

static bool mDNS_init_ok = false;
static bambu_printer printer;
static rgb_leds leds(RGBLED_NUM); // for now only the DMA method is supported -> no need to define a pin (always GPIO3 aka "RX" on D1)

void setup() {
  #ifndef DEBUG_PRINT
  pinMode(LEDPIN, OUTPUT);
  #else
  // initialize TX only since the RX pin (with its resistor) is used as 
  // data pin for the LEDs in DMA mode
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  #endif

  // Wifi
  WiFi.hostname(DEVICENAME);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // wifi_set_sleep_type(NONE_SLEEP_T);

  wl_status_t wstat;
  while (true)
  {
    delay(500);
    wstat = WiFi.status();
    if (wstat == WL_CONNECTED)
      break;

    #ifndef DEBUG_PRINT
    digitalWrite(LEDPIN, !digitalRead(LEDPIN));
    #endif
    dprintf("Connecting (%d) ...\n", wstat);
  };

  // mDNS
  mDNS_init_ok = MDNS.begin(DEVICENAME);

  // OTA
  ota::begin(DEVICENAME);

  leds.Begin();
}

void loop() {
  const uint32_t time = millis();
  static uint32_t next = 0;
  static uint32_t last_report_time = 0;

  // Wifi status
  const bool connected = WiFi.isConnected();
  #ifndef DEBUG_PRINT
  digitalWrite(LEDPIN, !connected);
  #endif

  // mDNS
  if (mDNS_init_ok)
    MDNS.update();

  // OTA
  ota::handle();

  // program logic
  if (time >= next)
  {
    next = time + 10000;
    dprintf("Systime: %lu ms; WLAN: %sconnected (as %s)\n", time, (connected ? "":"dis"), WiFi.localIP().toString().c_str());

    if (connected)
    {
      if (!printer.is_connected())
      {
        const auto conn_result = printer.connect(PRINTER_IP, PRINTER_SN, PRINTER_AUTH);
        dprintf("Connecting %s\n", (conn_result ? "was successful" : "failed"));
      }      
    }
    else
      WiFi.reconnect();
  }

  if (connected)
  {
    printer.loop();
    const auto& report = printer.report();
    if (report.last_update != last_report_time)
    {
      last_report_time = report.last_update;
      leds.update_with_report(report);
    }
  }

  yield();
}