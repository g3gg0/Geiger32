#ifndef __CONFIG_H__
#define __CONFIG_H__

#define CONFIG_MAGIC 0xE1AAFF02
typedef struct
{
    uint32_t magic;
    float voltage_target;
    float voltage_min;
    float voltage_max;
    uint32_t voltage_avg;
    float adc_corr;
    uint32_t pwm_freq;
    float pwm_start;
    uint32_t pwm_min;
    uint32_t pwm_max;
    uint32_t verbose;
    uint32_t idle_color;
    uint32_t elevated_color;
    uint32_t flash_color;
    uint32_t elevated_level;
    uint32_t buzz_length;
    uint32_t buzz_freq;
    char hostname[32];
} t_cfg;


extern t_cfg current_config;


#endif