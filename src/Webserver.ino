
#include <WebServer.h>
#include "Config.h"

WebServer webserver(80);

#define min(a, b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
#define max(a, b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })

void www_setup()
{
    webserver.on("/", handle_OnConnect);
    webserver.on("/set_parm", handle_set_parm);
    webserver.on("/search", handle_search);
    webserver.on("/ota", handle_ota);
    webserver.on("/plot", handle_plot);
    webserver.on("/voltage", handle_voltage);
    webserver.on("/pwm", handle_pwm);
    webserver.on("/counts", handle_counts);
    webserver.on("/counts_avg", handle_counts_avg);
    webserver.on("/reset", handle_reset);
    webserver.on("/test", handle_test);
    webserver.onNotFound(handle_NotFound);

    webserver.begin();
    Serial.println("HTTP server started");

    if (!MDNS.begin(current_config.hostname))
    {
        Serial.println("Error setting up MDNS responder!");
        while (1)
        {
            delay(1000);
        }
    }
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("telnet", "tcp", 23);
}

bool www_loop()
{
    webserver.handleClient();
    return false;
}

void handle_OnConnect()
{
    webserver.send(200, "text/html", SendHTML());
}

void handle_counts()
{
    char buf[32];
    sprintf(buf, "%d", det_counts);
    webserver.send(200, "text/plain", buf);
}

void handle_counts_avg()
{
    char buf[32];
    sprintf(buf, "%f", det_last_events_avg);
    webserver.send(200, "text/plain", buf);
}

void handle_voltage()
{
    char buf[32];
    sprintf(buf, "%f", adc_voltage_avg);
    webserver.send(200, "text/plain", buf);
}

void handle_pwm()
{
    char buf[32];
    sprintf(buf, "%f", pwm_value);
    webserver.send(200, "text/plain", buf);
}

void handle_ota()
{
    ota_setup();
    webserver.send(200, "text/html", SendHTML());
}

void handle_plot()
{
    File dataFile = SPIFFS.open("/plot.html", "r");
    webserver.streamFile(dataFile, "text/html");
    dataFile.close();
}

void handle_reset()
{
    webserver.send(200, "text/html", SendHTML());
    ESP.restart();
}

void handle_search()
{
    webserver.send(200, "text/html", SendHTML());
    pwm_learn();
}

void handle_test()
{
    Serial.printf("Test\n");
    webserver.send(200, "text/html", SendHTML());
}

void handle_set_parm()
{
    current_config.pwm_freq = max(500, min(500000, webserver.arg("pwm_freq").toInt()));
    current_config.pwm_start = max(1, min(99, webserver.arg("pwm_start").toInt()));
    current_config.pwm_min = max(1, min(99, webserver.arg("pwm_min").toInt()));
    current_config.pwm_max = max(1, min(99, webserver.arg("pwm_max").toInt()));
    current_config.voltage_target = max(1, min(420, webserver.arg("voltage_target").toFloat()));
    current_config.voltage_min = max(0, min(420, webserver.arg("voltage_min").toFloat()));
    current_config.voltage_max = max(1, min(600, webserver.arg("voltage_max").toFloat()));
    current_config.voltage_avg = max(1, min(1024, webserver.arg("voltage_avg").toInt()));
    current_config.adc_corr = max(0.1f, min(2.0f, webserver.arg("adc_corr").toFloat()));
    current_config.elevated_level = max(1, min(1000, webserver.arg("elevated_level").toInt()));
    current_config.buzz_length = max(1, min(1000, webserver.arg("buzz_length").toInt()));
    current_config.buzz_freq = max(1, min(20000, webserver.arg("buzz_freq").toInt()));

    current_config.idle_color = strtoul(webserver.arg("idle_color").substring(1).c_str(), NULL, 16);
    current_config.elevated_color = strtoul(webserver.arg("elevated_color").substring(1).c_str(), NULL, 16);
    current_config.flash_color = strtoul(webserver.arg("flash_color").substring(1).c_str(), NULL, 16);


    current_config.verbose = 0;
    current_config.verbose |= (webserver.arg("verbose_c0") != "") ? 1 : 0;
    current_config.verbose |= (webserver.arg("verbose_c1") != "") ? 2 : 0;
    current_config.verbose |= (webserver.arg("verbose_c2") != "") ? 4 : 0;
    current_config.verbose |= (webserver.arg("verbose_c3") != "") ? 8 : 0;

    strncpy(current_config.hostname, webserver.arg("hostname").c_str(), sizeof(current_config.hostname));

    cfg_save();

    if (current_config.verbose)
    {
        Serial.printf("Config:\n");
        Serial.printf("  pwm_freq:         %d Hz\n", current_config.pwm_freq);
        Serial.printf("  pwm_start:        %d %%\n", current_config.pwm_start);
        Serial.printf("  pwm_min:          %d %%\n", current_config.pwm_min);
        Serial.printf("  pwm_max:          %d %%\n", current_config.pwm_max);
        Serial.printf("  voltage_target:   %2.2f V\n", current_config.voltage_target);
        Serial.printf("  voltage_min:      %2.2f V\n", current_config.voltage_min);
        Serial.printf("  voltage_max:      %2.2f V\n", current_config.voltage_max);
        Serial.printf("  voltage_avg:      %d samples\n", current_config.voltage_avg);
        Serial.printf("  adc_corr:         %2.2f\n", current_config.adc_corr);
        Serial.printf("  idle_color:       #%06X\n", current_config.idle_color);
        Serial.printf("  elevated_color:   #%06X\n", current_config.elevated_color);
        Serial.printf("  elevated_level:   %d %%\n", current_config.elevated_level);
        Serial.printf("  verbose:          %d\n", current_config.verbose);
    }
    pwm_setup();
    webserver.send(200, "text/html", SendHTML());
}

void handle_NotFound()
{
    webserver.send(404, "text/plain", "Not found");
}

String SendHTML()
{
    char buf[1024];

    String ptr = "<!DOCTYPE html> <html>\n";
    ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";

    sprintf(buf, "<title>Geiger_v3 Control</title>\n");

    ptr += buf;
    ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
    ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
    ptr += ".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
    ptr += ".button-on {background-color: #3498db;}\n";
    ptr += ".button-on:active {background-color: #2980b9;}\n";
    ptr += ".button-off {background-color: #34495e;}\n";
    ptr += ".button-off:active {background-color: #2c3e50;}\n";
    ptr += ".toggle-buttons input[type=\"radio\"] {visibility: hidden;}\n";
    ptr += ".toggle-buttons label { border: 1px solid #333; border-radius: 0.5em; padding: 0.3em; } \n";
    ptr += ".toggle-buttons input:checked + label { background: #40ff40; color: #116600; box-shadow: none; }\n";
    ptr += ".check-buttons input[type=\"checkbox\"] {visibility: hidden;}\n";
    ptr += ".check-buttons label { border: 1px solid #333; border-radius: 0.5em; padding: 0.3em; } \n";
    ptr += ".check-buttons input:checked + label { background: #40ff40; color: #116600; box-shadow: none; }\n";
    ptr += "input:hover + label, input:focus + label { background: #ff4040; } \n";
    ptr += ".together { position: relative; } \n";
    ptr += ".together input { position: absolute; width: 1px; height: 1px; top: 0; left: 0; } \n";
    ptr += ".together label { margin: 0.5em 0; border-radius: 0; } \n";
    ptr += ".together label:first-of-type { border-radius: 0.5em 0 0 0.5em; } \n";
    ptr += ".together label:last-of-type { border-radius: 0 0.5em 0.5em 0; } \n";
    ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n"; 
    ptr += "td {padding: 0.3em}\n";
    ptr += "</style>\n";
    /* https://github.com/mdbassit/Coloris */
    ptr += "<link rel=\"stylesheet\" href=\"https://cdn.jsdelivr.net/gh/mdbassit/Coloris@latest/dist/coloris.min.css\"/>\n";
    ptr += "<script src=\"https://cdn.jsdelivr.net/gh/mdbassit/Coloris@latest/dist/coloris.min.js\"></script>\n";
    ptr += "</head>\n";
    ptr += "<body>\n";

    sprintf(buf, "<h1>Geiger v3 - %f V</h1>\n", adc_voltage_avg);

    ptr += buf;
    if (!ota_enabled())
    {
        ptr += "<a href=\"/ota\">[Enable OTA]</a> ";
    }
    sprintf(buf, "<br>Voltage: %f V at %2.0f %%</h1>\n", adc_voltage_avg, pwm_value);
    ptr += buf;
    ptr += "<br><br>\n";

    ptr += "<form action=\"/set_parm\">\n";
    ptr += "<table>";

#define ADD_CONFIG(name, value, fmt, desc)                                                                                \
    do                                                                                                                    \
    {                                                                                                                     \
        ptr += "<tr><td><label for=\"" name "\">" desc ":</label></td>";                                                  \
        sprintf(buf, "<td><input type=\"text\" id=\"" name "\" name=\"" name "\" value=\"" fmt "\"></td></tr>\n", value); \
        ptr += buf;                                                                                                       \
    } while (0)

#define ADD_CONFIG_CHECK4(name, value, fmt, desc, text0, text1, text2, text3) \
    do \
    { \
        ptr += "<tr><td>" desc ":</td><td><div class=\"check-buttons together\">"; \
        sprintf(buf, "<input type=\"checkbox\" id=\"" name "_c0\" name=\"" name "_c0\" value=\"1\" %s>\n", (value&1)?"checked":""); \
        ptr += buf; \
        sprintf(buf, "<label for=\"" name "_c0\">" text0 "</label>\n"); \
        ptr += buf; \
        sprintf(buf, "<input type=\"checkbox\" id=\"" name "_c1\" name=\"" name "_c1\" value=\"1\" %s>\n", (value&2)?"checked":""); \
        ptr += buf; \
        sprintf(buf, "<label for=\"" name "_c1\">" text1 "</label>\n"); \
        ptr += buf; \
        sprintf(buf, "<input type=\"checkbox\" id=\"" name "_c2\" name=\"" name "_c2\" value=\"1\" %s>\n", (value&4)?"checked":""); \
        ptr += buf; \
        sprintf(buf, "<label for=\"" name "_c2\">" text2 "</label>\n"); \
        ptr += buf; \
        sprintf(buf, "<input type=\"checkbox\" id=\"" name "_c3\" name=\"" name "_c3\" value=\"1\" %s>\n", (value&8)?"checked":""); \
        ptr += buf; \
        sprintf(buf, "<label for=\"" name "_c3\">" text3 "</label>\n"); \
        ptr += buf; \
        sprintf(buf, "</div></td></tr>\n"); \
        ptr += buf; \
    } while (0)

#define ADD_CONFIG_COLOR(name, value, fmt, desc)                                                                                \
    do                                                                                                                    \
    {                                                                                                                     \
        ptr += "<tr><td><label for=\"" name "\">" desc ":</label></td>";                                                  \
        sprintf(buf, "<td><input type=\"text\" id=\"" name "\" name=\"" name "\" value=\"" fmt "\" data-coloris></td></tr>\n", value); \
        ptr += buf;                                                                                                       \
    } while (0)

    ADD_CONFIG("hostname", current_config.hostname, "%s", "Hostname");
    ADD_CONFIG("voltage_target", current_config.voltage_target, "%2.0f", "Voltage target [V]");
    ADD_CONFIG("voltage_min", current_config.voltage_min, "%2.0f", "Voltage minimum [V]");
    ADD_CONFIG("voltage_max", current_config.voltage_max, "%2.0f", "Voltage maximum [V]");
    ADD_CONFIG("voltage_avg", current_config.voltage_avg, "%d", "Voltage averaging [n]");
    ADD_CONFIG("adc_corr", current_config.adc_corr, "%1.2f", "ADC correction");
    ADD_CONFIG("pwm_freq", current_config.pwm_freq, "%d", "PWM frequency [Hz]");
    ADD_CONFIG("pwm_start", current_config.pwm_start, "%d", "PWM startup duty cycle [%]");
    ADD_CONFIG("pwm_min", current_config.pwm_min, "%d", "PWM min duty cycle [%]");
    ADD_CONFIG("pwm_max", current_config.pwm_max, "%d", "PWM max duty cycle [%]");
    ADD_CONFIG("buzz_length", current_config.buzz_length, "%d", "Buzzer duration [ms]");
    ADD_CONFIG("buzz_freq", current_config.buzz_freq, "%d", "Buzzer frequency [Hz]");
    ADD_CONFIG_CHECK4("verbose", current_config.verbose, "%d", "Verbosity", "Serial", "Beep", "Blink", "Fading");
    ADD_CONFIG_COLOR("idle_color", current_config.idle_color, "#%06X", "Idle color");
    ADD_CONFIG_COLOR("elevated_color", current_config.elevated_color, "#%06X", "Elevated color");
    ADD_CONFIG_COLOR("flash_color", current_config.flash_color, "#%06X", "Flash color");
    ADD_CONFIG("elevated_level", current_config.elevated_level, "%d", "Elevated level [cpm]");

    ptr += "<td></td><td><input type=\"submit\" value=\"Write\"></td></table></form>\n";

    ptr += "<form action=\"/search\">\n";
    ptr += "<input type=\"submit\" value=\"Raise PWM and search peak\"></form>\n";
    ptr += "</body>\n";
    ptr += "</html>\n";
    return ptr;
}
