#ifndef __CONFIG_H__
#define __CONFIG_H__

#define CONFIG_SOFTAPNAME  "esp32-config"
#define CONFIG_OTANAME     "Geiger"

#define CONFIG_MAGIC 0xE1AAFF0A
typedef struct
{
    uint32_t magic;

    char hostname[32];
    char wifi_ssid[32];
    char wifi_password[32];

    char mqtt_server[32];
    int mqtt_port;
    char mqtt_user[32];
    char mqtt_password[32];
    char mqtt_client[32];

    float conv_usv_per_bq;
    float voltage_target;
    float voltage_min;
    float voltage_max;
    uint32_t voltage_avg;
    float adc_corr;
    float pwm_value;
    float pwm_pid_i;
    uint32_t pwm_freq;
    uint32_t pwm_freq_min;
    uint32_t pwm_freq_max;
    uint32_t verbose;
    uint32_t idle_color;
    uint32_t elevated_color;
    uint32_t flash_color;
    uint32_t elevated_level;
    uint32_t buzz_length;
    uint32_t buzz_freq;
    uint32_t mqtt_publish;
} t_cfg;


extern t_cfg current_config;


#endif