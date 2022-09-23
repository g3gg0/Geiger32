
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <SPIFFS.h>

#include "Config.h"

int led_r = 0;
int led_g = 0;
int led_b = 0;
int loopCount = 0;

float main_duration_avg = 0;
float main_duration = 0;
float main_duration_max = 0;
float main_duration_min = 1000000;

extern bool config_valid;


void setup()
{
    Serial.begin(115200);
    Serial.printf("\n\n\n");

    Serial.printf("[i] SDK:          '%s'\n", ESP.getSdkVersion());
    Serial.printf("[i] CPU Speed:    %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("[i] Chip Id:      %06llX\n", ESP.getEfuseMac());
    Serial.printf("[i] Flash Mode:   %08X\n", ESP.getFlashChipMode());
    Serial.printf("[i] Flash Size:   %08X\n", ESP.getFlashChipSize());
    Serial.printf("[i] Flash Speed:  %d MHz\n", ESP.getFlashChipSpeed() / 1000000);
    Serial.printf("[i] Heap          %d/%d\n", ESP.getFreeHeap(), ESP.getHeapSize());
    Serial.printf("[i] SPIRam        %d/%d\n", ESP.getFreePsram(), ESP.getPsramSize());
    Serial.printf("\n");
    Serial.printf("[i] Starting\n");

    Serial.printf("[i]   Setup LEDs\n");
    led_setup();
    Serial.printf("[i]   Setup EPD\n");
    epd_setup();
    Serial.printf("[i]   Setup SPIFFS\n");
    if (!SPIFFS.begin(true))
    {
        Serial.println("[E]   SPIFFS Mount Failed");
    }
    cfg_read();
    Serial.printf("[i]   Setup ADC\n");
    adc_setup();
    Serial.printf("[i]   Setup Buzzer\n");
    buz_setup();
    Serial.printf("[i]   Setup RTTTL player\n");
    rtttl_setup();
    Serial.printf("[i]   Setup PWM\n");
    pwm_setup();
    Serial.printf("[i]   Setup Detector\n");
    det_setup();
    Serial.printf("[i]   Setup WiFi\n");
    wifi_setup();
    Serial.printf("[i]   Setup Webserver\n");
    www_setup();
    Serial.printf("[i]   Setup Time\n");
    time_setup();
    Serial.printf("[i]   Setup MQTT\n");
    mqtt_setup();
    Serial.printf("[i]   Setup BME280/CCS811\n");
    env_setup();

    Serial.println("Setup done");
}


void loop()
{
    bool hasWork = false;

    uint64_t startTime = micros();

    hasWork |= led_loop();
    hasWork |= adc_loop();
    hasWork |= env_loop();
    hasWork |= buz_loop();
    hasWork |= pwm_loop();
    hasWork |= wifi_loop();
    hasWork |= www_loop();
    hasWork |= time_loop();
    hasWork |= mqtt_loop();
    hasWork |= ota_loop();
    hasWork |= det_loop();
    hasWork |= rtttl_loop();

    uint64_t duration = micros() - startTime;

    main_duration = duration;
    main_duration_avg = (15 * main_duration_avg + duration) / 16.0f;

    if (main_duration < main_duration_min)
    {
        main_duration_min = main_duration;
    }
    if (main_duration > main_duration_max)
    {
        main_duration_max = main_duration;
    }

    bool voltage_ok = ((adc_get_voltage() >= current_config.voltage_min) && (adc_get_voltage() <= current_config.voltage_max));

    if (((current_config.verbose & 8) && voltage_ok) || !config_valid)
    {
        led_r = (sin(loopCount / 50.0f) + 1) * 16;
        led_g = 0;
        led_b = (cos(loopCount / 50.0f) + 1) * 16;
    }
    else
    {
        led_r = voltage_ok ? 0 : 255;
        led_g = voltage_ok ? 16 : 0;
        led_b = 0;
    }

    if (!config_valid)
    {
        led_set_inhibit(false);
        for(int led = 0; led < 6; led++)
        {
            led_set_adv(led, led_r, led_g, led_b, led == 5);
        }
        led_set_inhibit(true);
    }
    else
    {
        led_set(0, led_r, led_g, led_b);
    }

    loopCount++;

    if(!hasWork)
    {
        delay(100);
    }
}
