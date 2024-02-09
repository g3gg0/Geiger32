
// #define TESTMODE

#include <PubSubClient.h>
#include <ESP32httpUpdate.h>
#include <Config.h>

#include "HA.h"

WiFiClient client;
PubSubClient mqtt(client);

extern int wifi_rssi;
extern float pwm_value;
extern uint32_t pwm_freq;
extern float pwm_deviation;
extern float esp32_temperature;

extern float main_duration_avg;
extern float main_duration_max;
extern float main_duration_min;
extern float main_duration;

extern float main_cycletime_avg;
extern float main_cycletime_max;
extern float main_cycletime_min;
extern float main_cycletime;

uint32_t ticks_total = 0;

uint32_t mqtt_last_publish_time = 0;
uint32_t mqtt_lastConnect = 0;
uint32_t mqtt_retries = 0;
bool mqtt_fail = false;

char command_topic[64];
char response_topic[64];

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.print("'");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.print("'");
    Serial.println();

    payload[length] = 0;

    ha_received(topic, (const char *)payload);

    if (!strcmp(topic, command_topic))
    {
        char *command = (char *)payload;
        char buf[1024];

        if (!strncmp(command, "http", 4))
        {
            snprintf(buf, sizeof(buf) - 1, "updating from: '%s'", command);
            Serial.printf("%s\n", buf);

            mqtt.publish(response_topic, buf);
            ESPhttpUpdate.rebootOnUpdate(false);
            t_httpUpdate_return ret = ESPhttpUpdate.update(command);

            switch (ret)
            {
            case HTTP_UPDATE_FAILED:
                snprintf(buf, sizeof(buf) - 1, "HTTP_UPDATE_FAILED Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                mqtt.publish(response_topic, buf);
                Serial.printf("%s\n", buf);
                break;

            case HTTP_UPDATE_NO_UPDATES:
                snprintf(buf, sizeof(buf) - 1, "HTTP_UPDATE_NO_UPDATES");
                mqtt.publish(response_topic, buf);
                Serial.printf("%s\n", buf);
                break;

            case HTTP_UPDATE_OK:
                snprintf(buf, sizeof(buf) - 1, "HTTP_UPDATE_OK");
                mqtt.publish(response_topic, buf);
                Serial.printf("%s\n", buf);
                delay(500);
                ESP.restart();
                break;

            default:
                snprintf(buf, sizeof(buf) - 1, "update failed");
                mqtt.publish(response_topic, buf);
                Serial.printf("%s\n", buf);
                break;
            }
        }
        else
        {
            snprintf(buf, sizeof(buf) - 1, "unknown command: '%s'", command);
            mqtt.publish(response_topic, buf);
            Serial.printf("%s\n", buf);
        }
    }
}

void mqtt_ota_received(const t_ha_entity *entity, void *ctx, const char *message)
{
    ota_setup();
}

void mqtt_setup()
{
    mqtt.setCallback(callback);

    ha_setup();

    t_ha_entity entity;

    memset(&entity, 0x00, sizeof(entity));
    entity.id = "ota";
    entity.name = "Enable OTA";
    entity.type = ha_button;
    entity.cmd_t = "command/%s/ota";
    entity.received = &mqtt_ota_received;
    ha_add(&entity);

    memset(&entity, 0x00, sizeof(entity));
    entity.id = "voltage";
    entity.dev_class = "voltage";
    entity.name = "Tube voltage";
    entity.type = ha_sensor;
    entity.stat_t = "feeds/float/%s/voltage";
    entity.unit_of_meas = "V";
    ha_add(&entity);

    memset(&entity, 0x00, sizeof(entity));
    entity.id = "pwm_freq";
    entity.dev_class = "frequency";
    entity.name = "PWM Frequency";
    entity.type = ha_sensor;
    entity.stat_t = "feeds/float/%s/pwm_freq";
    entity.unit_of_meas = "Hz";
    ha_add(&entity);

    memset(&entity, 0x00, sizeof(entity));
    entity.id = "pwm_value";
    entity.name = "PWM duty cycle";
    entity.type = ha_sensor;
    entity.stat_t = "feeds/float/%s/pwm_value";
    entity.unit_of_meas = "%";
    ha_add(&entity);

    memset(&entity, 0x00, sizeof(entity));
    entity.id = "pwm_deviation";
    entity.name = "PWM averaged deviation";
    entity.state_class = "measurement";
    entity.type = ha_sensor;
    entity.stat_t = "feeds/float/%s/pwm_deviation";
    ha_add(&entity);

    memset(&entity, 0x00, sizeof(entity));
    entity.id = "ticks_total";
    entity.name = "Activity counter ticks total";
    entity.state_class = "total_increasing";
    entity.type = ha_sensor;
    entity.stat_t = "feeds/integer/%s/ticks_total";
    entity.unit_of_meas = "counts";
    ha_add(&entity);

    memset(&entity, 0x00, sizeof(entity));
    entity.id = "counts_per_minute";
    entity.name = "Activity";
    entity.type = ha_sensor;
    entity.stat_t = "feeds/integer/%s/ticks";
    entity.unit_of_meas = "cpm";
    ha_add(&entity);

    memset(&entity, 0x00, sizeof(entity));
    entity.id = "activity";
    entity.name = "Dose current";
    entity.type = ha_sensor;
    entity.stat_t = "feeds/float/%s/activity";
    entity.unit_of_meas = "µSv/h";
    ha_add(&entity);

    memset(&entity, 0x00, sizeof(entity));
    entity.id = "activity_total";
    entity.name = "Dose total";
    entity.type = ha_sensor;
    entity.stat_t = "feeds/float/%s/activity_total";
    entity.unit_of_meas = "µSv";
    ha_add(&entity);

    memset(&entity, 0x00, sizeof(entity));
    entity.id = "esp32_hall";
    entity.name = "ESP32 hall sensor data";
    entity.state_class = "measurement";
    entity.type = ha_sensor;
    entity.stat_t = "feeds/float/%s/esp32_hall";
    ha_add(&entity);

    memset(&entity, 0x00, sizeof(entity));
    entity.id = "temperature";
    entity.dev_class = "temperature";
    entity.name = "BME280 Temperature";
    entity.type = ha_sensor;
    entity.stat_t = "feeds/float/%s/temperature";
    entity.unit_of_meas = "°C";
    ha_add(&entity);

    memset(&entity, 0x00, sizeof(entity));
    entity.id = "humidity";
    entity.dev_class = "humidity";
    entity.name = "BME280 Humidity";
    entity.type = ha_sensor;
    entity.stat_t = "feeds/float/%s/humidity";
    entity.unit_of_meas = "%";
    ha_add(&entity);

    memset(&entity, 0x00, sizeof(entity));
    entity.id = "pressure";
    entity.dev_class = "pressure";
    entity.name = "BME280 Pressure";
    entity.type = ha_sensor;
    entity.stat_t = "feeds/float/%s/pressure";
    entity.unit_of_meas = "hPa";
    ha_add(&entity);

    memset(&entity, 0x00, sizeof(entity));
    entity.id = "rssi";
    entity.name = "WiFi RSSI";
    entity.type = ha_sensor;
    entity.stat_t = "feeds/integer/%s/rssi";
    entity.unit_of_meas = "dBm";
    ha_add(&entity);

    memset(&entity, 0x00, sizeof(entity));
    entity.id = "error";
    entity.name = "Error message";
    entity.type = ha_sensor;
    entity.stat_t = "feeds/string/%s/error";
    ha_add(&entity);
}

void mqtt_publish_string(const char *name, const char *value)
{
    char path_buffer[128];

    sprintf(path_buffer, name, current_config.mqtt_client);

    if (!mqtt.publish(path_buffer, value))
    {
        mqtt_fail = true;
    }
    Serial.printf("Published %s : %s\n", path_buffer, value);
}

void mqtt_publish_float(const char *name, float value)
{
    char path_buffer[128];
    char buffer[32];

    sprintf(path_buffer, name, current_config.mqtt_client);
    sprintf(buffer, "%0.2f", value);

    if (!mqtt.publish(path_buffer, buffer))
    {
        mqtt_fail = true;
    }
    Serial.printf("Published %s : %s\n", path_buffer, buffer);
}

void mqtt_publish_int(const char *name, uint32_t value)
{
    char path_buffer[128];
    char buffer[32];

    if (value == 0x7FFFFFFF)
    {
        return;
    }
    sprintf(path_buffer, name, current_config.mqtt_client);
    sprintf(buffer, "%d", value);

    if (!mqtt.publish(path_buffer, buffer))
    {
        mqtt_fail = true;
    }
    Serial.printf("Published %s : %s\n", path_buffer, buffer);
}

bool mqtt_loop()
{
    uint32_t time = millis();
    static uint32_t nextTime = 0;

#ifdef TESTMODE
    return false;
#endif
    if (mqtt_fail)
    {
        mqtt_fail = false;
        mqtt.disconnect();
    }

    MQTT_connect();

    if (!mqtt.connected())
    {
        return false;
    }

    mqtt.loop();

    ha_loop();

    if (time >= nextTime)
    {
        bool do_publish = false;

        if ((time - mqtt_last_publish_time) > 60000)
        {
            do_publish = true;
        }

        if (do_publish)
        {
            mqtt_last_publish_time = time;
            int counts = det_fetch();

            if ((current_config.mqtt_publish & 1) && pwm_is_stable())
            {
                ticks_total += counts;
                mqtt_publish_int((char *)"feeds/integer/%s/ticks", counts);
                mqtt_publish_int((char *)"feeds/integer/%s/ticks_total", ticks_total);
                mqtt_publish_float((char *)"feeds/float/%s/activity", current_config.conv_usv_per_bq * counts);
                mqtt_publish_float((char *)"feeds/float/%s/activity_total", current_config.conv_usv_per_bq * ticks_total / 60.0f);
            }
            if (current_config.mqtt_publish & 2)
            {
                mqtt_publish_float((char *)"feeds/float/%s/voltage", adc_voltage_avg);
                mqtt_publish_float((char *)"feeds/float/%s/pwm_freq", pwm_freq);
                mqtt_publish_float((char *)"feeds/float/%s/pwm_value", pwm_value);
                mqtt_publish_float((char *)"feeds/float/%s/pwm_deviation", pwm_deviation);
                mqtt_publish_int((char *)"feeds/integer/%s/version", PIO_SRC_REVNUM);
                mqtt_publish_int((char *)"feeds/integer/%s/rssi", wifi_rssi);

                mqtt_publish_float((char *)"feeds/float/%s/esp32_hall", esp32_hall);

                if ((main_duration_max > 0) && (main_duration_max < 1000000) && (main_duration_min > 0) && (main_duration_min < 1000000))
                {
                    mqtt_publish_float((char *)"feeds/float/%s/main_duration", main_duration);
                    mqtt_publish_float((char *)"feeds/float/%s/main_duration_min", main_duration_min);
                    mqtt_publish_float((char *)"feeds/float/%s/main_duration_max", main_duration_max);
                    mqtt_publish_float((char *)"feeds/float/%s/main_duration_avg", main_duration_avg);
                }
                if ((main_cycletime_max > 0) && (main_cycletime_max < 10000000) && (main_cycletime_min > 0) && (main_cycletime_min < 10000000))
                {
                    mqtt_publish_float((char *)"feeds/float/%s/main_cycletime", main_cycletime);
                    mqtt_publish_float((char *)"feeds/float/%s/main_cycletime_min", main_cycletime_min);
                    mqtt_publish_float((char *)"feeds/float/%s/main_cycletime_max", main_cycletime_max);
                    mqtt_publish_float((char *)"feeds/float/%s/main_cycletime_avg", main_cycletime_avg);
                }
                main_duration_max = 0;
                main_duration_min = 1000000;
                main_cycletime_max = 0;
                main_cycletime_min = 10000000;
            }
            if (current_config.mqtt_publish & 4)
            {
                mqtt_publish_float((char *)"feeds/float/%s/temperature", bme280_temperature);
                mqtt_publish_float((char *)"feeds/float/%s/humidity", bme280_humidity);
                mqtt_publish_float((char *)"feeds/float/%s/pressure", bme280_pressure);
            }
            if (current_config.mqtt_publish & 8)
            {
                mqtt_publish_int((char *)"feeds/integer/%s/co2", ccs811_co2);
                mqtt_publish_int((char *)"feeds/integer/%s/tvoc", ccs811_tvoc);
            }
        }
        nextTime = time + 10000;
    }

    return false;
}

void MQTT_connect()
{
    uint32_t curTime = millis();
    int8_t ret;

    if (strlen(current_config.mqtt_server) == 0)
    {
        return;
    }

    mqtt.setServer(current_config.mqtt_server, current_config.mqtt_port);

    if (WiFi.status() != WL_CONNECTED)
    {
        return;
    }

    if (mqtt.connected())
    {
        return;
    }

    if ((mqtt_lastConnect != 0) && (curTime - mqtt_lastConnect < (1000 << mqtt_retries)))
    {
        return;
    }

    mqtt_lastConnect = curTime;

    Serial.println("MQTT: Connecting to MQTT... ");

    sprintf(command_topic, "tele/%s/command", current_config.mqtt_client);
    sprintf(response_topic, "tele/%s/response", current_config.mqtt_client);

    ret = mqtt.connect(current_config.mqtt_client, current_config.mqtt_user, current_config.mqtt_password);

    if (ret == 0)
    {
        mqtt_retries++;
        if (mqtt_retries > 8)
        {
            mqtt_retries = 8;
        }
        Serial.printf("MQTT: (%d) ", mqtt.state());
        Serial.println("MQTT: Retrying MQTT connection");
        mqtt.disconnect();
    }
    else
    {
        /* discard counts till then */
        det_fetch();
        Serial.println("MQTT Connected!");
        mqtt.subscribe(command_topic);
        ha_connected();
        mqtt_publish_string((char *)"feeds/string/%s/error", "");
    }
}
