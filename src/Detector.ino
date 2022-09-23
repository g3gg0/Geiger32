

#include "Config.h"

#define DET_GPIO 18

#define DET_FADE 8
#define DET_LAST_EVENT_COUNT 10

uint32_t det_counts = 0;
uint32_t det_counts_last = 0;
uint32_t last_detects = 0;

uint32_t led_det_r = 0;
uint32_t led_det_g = 0;
uint32_t led_det_b = 0;

uint32_t led_det_r_dest = 0;
uint32_t led_det_g_dest = 16;
uint32_t led_det_b_dest = 0;

uint32_t levels_elevated = 0;

uint32_t det_last_events[DET_LAST_EVENT_COUNT];
uint32_t det_last_event_count = 0;
float det_last_events_avg = 0;


/* https://github.com/radhoo/uradmonitor_kit1/blob/master/code/geiger/detectors.cpp 
		case GEIGER_TUBE_SBM20: 	return 0.006315; // CPM 19
		case GEIGER_TUBE_SI29BG: 	return 0.010000; // CPM 12
		case GEIGER_TUBE_SBM19: 	return 0.001500; // CPM 80
		case GEIGER_TUBE_LND712: 	return 0.005940; // CPM 20.20
		case GEIGER_TUBE_SBM20M:	return 0.013333; // CPM 9
		case GEIGER_TUBE_SI22G: 	return 0.001714; // CPM 70
		case GEIGER_TUBE_STS5: 		return 0.006666; // CPM 18
		case GEIGER_TUBE_SI3BG: 	return 0.631578; // CPM 0.19
		case GEIGER_TUBE_SBM21: 	return 0.048000; // CPM 2.5
		case GEIGER_TUBE_SBT9: 		return 0.010900; // CPM 11
		case GEIGER_TUBE_SI1G:		return 0.006000; // CPM 20
		case GEIGER_TUBE_SI8B:		return 0.001108; // 
		case GEIGER_TUBE_SBT10A:	return 0.001105; // 
        */

void IRAM_ATTR det_isr()
{
    uint32_t current_millis = millis();

    det_counts++;

    det_last_events[det_last_event_count++] = current_millis;
    det_last_event_count %= DET_LAST_EVENT_COUNT;
}

void det_setup()
{
    det_counts = 0;
    last_detects = 0;
    pinMode(DET_GPIO, INPUT);
    attachInterrupt(DET_GPIO, det_isr, FALLING);
}

bool det_loop()
{
    bool detected = last_detects != det_counts;
    bool hasWork = true;
    uint32_t current_millis = millis();

    /* averaging method: measure time distance between n ticks and average CPM from that */
    uint32_t time_min = 0xFFFFFFFF;
    for(int pos = 0; pos < DET_LAST_EVENT_COUNT; pos++)
    {
        if(det_last_events[pos])
        {
            time_min = min(time_min, det_last_events[pos]);
        }
    }

    if(time_min != 0xFFFFFFFF)
    {
        uint32_t delta = current_millis - time_min;
        float det_this = 60.0f / ((delta) / DET_LAST_EVENT_COUNT / 1000.0f);
        det_last_events_avg = (65 * det_last_events_avg + det_this) / 66;
    }

    if(isinf(det_last_events_avg) || isnan(det_last_events_avg))
    {
        det_last_events_avg = 0;
    }

    /* when averaging result is elevated, set warning level for 10s */
    if(det_last_events_avg > current_config.elevated_level)
    {
        levels_elevated = current_millis + 10000;
    }

    if(levels_elevated > current_millis)
    {
        led_det_r_dest = (current_config.elevated_color >> 16) & 0xFF;
        led_det_g_dest = (current_config.elevated_color >> 8) & 0xFF;
        led_det_b_dest = (current_config.elevated_color >> 0) & 0xFF;
    }
    else
    {
        led_det_r_dest = (current_config.idle_color >> 16) & 0xFF;
        led_det_g_dest = (current_config.idle_color >> 8) & 0xFF;
        led_det_b_dest = (current_config.idle_color >> 0) & 0xFF;
    }

    led_det_r = ((DET_FADE - 1) * led_det_r + led_det_r_dest) / DET_FADE;
    led_det_g = ((DET_FADE - 1) * led_det_g + led_det_g_dest) / DET_FADE;
    led_det_b = ((DET_FADE - 1) * led_det_b + led_det_b_dest) / DET_FADE;

    if (detected)
    {
        last_detects = det_counts;
        if(current_config.verbose & 4)
        {
            led_det_r = (current_config.flash_color >> 16) & 0xFF;
            led_det_g = (current_config.flash_color >> 8) & 0xFF;
            led_det_b = (current_config.flash_color >> 0) & 0xFF;
        }
    }
    
    led_set_adv(2, led_det_r, led_det_g, led_det_b, false);
    led_set_adv(3, led_det_r, led_det_g, led_det_b, false);
    led_set_adv(4, led_det_r, led_det_g, led_det_b, false);
    led_set_adv(5, led_det_r, led_det_g, led_det_b, true);
    
    if (detected)
    {
        if(current_config.verbose & 2)
        {
            buz_tick();
        }
        if(current_config.verbose & 1)
        {
            Serial.printf("Detected: %d\n", det_counts);
        }
    }

    if((led_det_r_dest == led_det_r) && (led_det_g_dest == led_det_g) && (led_det_b_dest == led_det_b))
    {
        hasWork = false;
    }

    return hasWork;
}

uint32_t det_fetch()
{
    det_counts_last = det_counts;

    det_counts = 0;

    return det_counts_last;
}
