#ifndef __HA_H__
#define __HA_H__

#define MAX_LEN 32
#define MAX_ENTITIES 16

typedef enum {
    unused = 0,
    sensor,
    number,
    button,
    binary_sensor
} t_ha_device_type;

typedef struct {
    t_ha_device_type type;

    const char *name;
    const char *id;
    
    /* used by: sensor */
    const char *unit_of_meas;
    /* used by: sensor */
    const char *val_tpl;
    /* used by: button, number */
    const char *cmd_t;
    /* used by: sensor, binary_sensor, number */
    const char *stat_t;
    /* used by: number */
    float min;
    /* used by: number */
    float max;
    /* icon */
    const char *ic;
    /* entity_category */
    const char *ent_cat;
} t_ha_entity;

typedef struct {
    char name[MAX_LEN];
    char id[MAX_LEN];
    char cu[MAX_LEN];
    char mf[MAX_LEN];
    char mdl[MAX_LEN];
    char sw[MAX_LEN];
    t_ha_entity entities[MAX_ENTITIES];
    int entitiy_count;
} t_ha_info;


void ha_add(t_ha_entity *entity);


#endif
