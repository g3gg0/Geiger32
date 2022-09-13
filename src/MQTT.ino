

//#define TESTMODE

#include <PubSubClient.h>
#include <ESP32httpUpdate.h>


#define COMMAND_TOPIC "tele/geiger/command"
#define RESPONSE_TOPIC "tele/geiger/response"


WiFiClient client;
PubSubClient mqtt(client);

extern float pwm_value;
extern uint32_t pwm_freq;

int mqtt_last_publish_time = 0;
int mqtt_lastConnect = 0;
int mqtt_retries = 0;
bool mqtt_fail = false;

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

    if(!strcmp(topic, COMMAND_TOPIC))
    {
      char *command = (char *)payload;
      
      if(!strncmp(command, "http", 4))
      {
          char buf[1024];
          sprintf(buf, "updating from: '%s'", command);
          Serial.printf("%s\n", buf);
          mqtt.publish(RESPONSE_TOPIC, buf);
          t_httpUpdate_return ret = ESPhttpUpdate.update(command);
          
          sprintf(buf, "update failed");
          switch(ret)
          {
              case HTTP_UPDATE_FAILED:
                  sprintf(buf, "HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                  break;
  
              case HTTP_UPDATE_NO_UPDATES:
                  sprintf(buf, "HTTP_UPDATE_NO_UPDATES");
                  break;
  
              case HTTP_UPDATE_OK:
                  sprintf(buf, "HTTP_UPDATE_OK");
                  break;
          }
          mqtt.publish(RESPONSE_TOPIC, buf);
          Serial.printf("%s\n", buf);
      }
      else
      {
          Serial.printf("unknown command: '%s'", command);
      }
    }
}

void mqtt_setup()
{
    mqtt.setCallback(callback);
}

void mqtt_publish_float(char *name, float value)
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

void mqtt_publish_int(char *name, uint32_t value)
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
    static int nextTime = 0;

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
            
            if(current_config.mqtt_publish & 1)
            {
                mqtt_publish_int((char*)"feeds/integer/%s/ticks", counts);
            }
            if(current_config.mqtt_publish & 2)
            {
                mqtt_publish_float((char*)"feeds/float/%s/voltage", adc_voltage_avg);
                mqtt_publish_float((char*)"feeds/float/%s/pwm_freq", pwm_freq);
                mqtt_publish_float((char*)"feeds/float/%s/pwm_value", pwm_value);
            }
            if(current_config.mqtt_publish & 4)
            {
                mqtt_publish_float((char*)"feeds/float/%s/temperature", bme280_temperature);
                mqtt_publish_float((char*)"feeds/float/%s/humidity", bme280_humidity);
                mqtt_publish_float((char*)"feeds/float/%s/pressure", bme280_pressure);
            }
            if(current_config.mqtt_publish & 8)
            {
                mqtt_publish_int((char*)"feeds/integer/%s/co2", ccs811_co2);
                mqtt_publish_int((char*)"feeds/integer/%s/tvoc", ccs811_tvoc);
            }
        }
        nextTime = time + 1000;
    }

    return false;
}

void MQTT_connect()
{
    int curTime = millis();
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

    Serial.println("MQTT: Connecting to MQTT...");
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
        mqtt.subscribe(COMMAND_TOPIC);
    }
}
