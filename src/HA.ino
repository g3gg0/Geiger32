
#include <PubSubClient.h>
#include <ESP32httpUpdate.h>
#include <Config.h>

#include "HA.h"
#include "Macros.h"

t_ha_info ha_info;
extern PubSubClient mqtt;


void ha_addstr(char *json_str, const char *name, const char *value, bool last = false)
{
    char tmp_buf[128];

    if(value && strlen(value) > 0)
    {
        sprintf(tmp_buf, "\"%s\": \"%s\"%c ", name, value, (last ? ' ' : ','));
        strcat(json_str, tmp_buf);
    }
}

void ha_addmqtt(char *json_str, const char *name, const char *value, bool last = false)
{
    char tmp_buf[128];

    if(value && strlen(value) > 0)
    {
        char path_buffer[64];
        sprintf(path_buffer, value, current_config.mqtt_client);
        sprintf(tmp_buf, "\"%s\": \"%s\"%c ", name, path_buffer, (last ? ' ' : ','));
        strcat(json_str, tmp_buf);
    }
}


void ha_addfloat(char *json_str, const char *name, float value, bool last = false)
{
    char tmp_buf[64];

    sprintf(tmp_buf, "\"%s\": \"%f\"%c ", name, value, (last ? ' ' : ','));
    strcat(json_str, tmp_buf);
}

void ha_addint(char *json_str, const char *name, int value, bool last = false)
{
    char tmp_buf[64];

    sprintf(tmp_buf, "\"%s\": \"%d\"%c ", name, value, (last ? ' ' : ','));
    strcat(json_str, tmp_buf);
}

void ha_publish() 
{
    char *json_str = (char *)malloc(512);
    char mqtt_path[128];
    char uniq_id[128];

    Serial.printf("[HA] Publish\n");

    sprintf(ha_info.cu, "http://%s/", WiFi.localIP().toString().c_str());

    for(int pos = 0; pos < ha_info.entitiy_count; pos++) 
    {
        const char *type = "undefined";

        switch(ha_info.entities[pos].type) 
        {
            case sensor:
                Serial.printf("[HA] sensor\n");
                type = "sensor";
                break;
            case number:
                Serial.printf("[HA] number\n");
                type = "number";
                break;
            case button:
                Serial.printf("[HA] button\n");
                type = "button";
                break;
            case binary_sensor:
                Serial.printf("[HA] binary_sensor\n");
                type = "binary_sensor";
                break;
            default:
                Serial.printf("[HA] last one\n");
                return;
        }

        sprintf(uniq_id, "%s_%s", ha_info.id, ha_info.entities[pos].id);

        Serial.printf("[HA]   uniq_id %s\n", uniq_id);
        sprintf(mqtt_path, "homeassistant/%s/%s/%s/config", type, ha_info.id, ha_info.entities[pos].id);

        Serial.printf("[HA]   mqtt_path %s\n", mqtt_path);

        strcpy(json_str, "{");
        ha_addstr(json_str, "name", ha_info.entities[pos].name);
        ha_addstr(json_str, "uniq_id", uniq_id);
        ha_addstr(json_str, "ic", ha_info.entities[pos].ic);
        ha_addstr(json_str, "ent_cat", ha_info.entities[pos].ent_cat);
        ha_addmqtt(json_str, "cmd_t", ha_info.entities[pos].cmd_t);
        ha_addmqtt(json_str, "stat_t", ha_info.entities[pos].stat_t);
        ha_addmqtt(json_str, "val_tpl", ha_info.entities[pos].val_tpl);
        ha_addstr(json_str, "unit_of_meas", ha_info.entities[pos].unit_of_meas);
        strcat(json_str, "\"dev\": {");
        ha_addstr(json_str, "name", ha_info.name);
        ha_addstr(json_str, "ids", ha_info.id);
        ha_addstr(json_str, "cu", ha_info.cu);
        ha_addstr(json_str, "mf", ha_info.mf);
        ha_addstr(json_str, "mdl", ha_info.mdl);
        ha_addstr(json_str, "sw", ha_info.sw, true);
        strcat(json_str, "}}");

        Serial.printf("[HA]    topic '%s'\n", mqtt_path);
        Serial.printf("[HA]    content '%s'\n", json_str);

        if(!mqtt.publish(mqtt_path, json_str))
        {
            Serial.printf("[HA] publish failed\n");
        }
    }

    Serial.printf("[HA] done\n");
    free(json_str);
}


bool ha_loop()
{
    uint32_t time = millis();
    static uint32_t nextTime = 0;
    
    if (time >= nextTime)
    {
        ha_publish();
        nextTime = time + 60000;
    }

    return false;
}

void ha_setup()
{
    memset(&ha_info, 0x00, sizeof(ha_info));

    sprintf(ha_info.name, "%s", current_config.mqtt_client);
    sprintf(ha_info.id, "%06llX", ESP.getEfuseMac());
    sprintf(ha_info.cu, "http://%s/", WiFi.localIP().toString().c_str());
    sprintf(ha_info.mf, "g3gg0.de");
    sprintf(ha_info.mdl, "");
    sprintf(ha_info.sw, "v1." xstr(PIO_SRC_REVNUM) " (" xstr(PIO_SRC_REV) ")");
    ha_info.entitiy_count = 0;

    mqtt.setBufferSize(512);
}

void ha_add(t_ha_entity *entity)
{
    if(ha_info.entitiy_count >= MAX_ENTITIES)
    {
        return;
    }
    Serial.printf("Add: '%s' (%s)\n", entity->id, entity->name);
    memcpy(&ha_info.entities[ha_info.entitiy_count++], entity, sizeof(t_ha_entity));
}
