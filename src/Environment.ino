#include "Config.h"

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include "Adafruit_CCS811.h"

Adafruit_BME280 bme;
Adafruit_CCS811 ccs;

#define BME_ALTITUDE  395
#define SEALEVELPRESSURE_HPA (1013.25)

float esp32_hall = 0;

float bme280_temperature = 0;
float bme280_humidity = 0;
float bme280_pressure = 0;

bool bme280_detected = false;
bool ccs811_detected = false;

uint16_t ccs811_co2;
uint16_t ccs811_tvoc;

void env_setup()
{
    Serial.println(F("[BME280] Searching"));

    Wire.begin(26, 25);
    bool status = bme.begin(0x76);
    if (status)
    {
        bme280_detected = true;
        Serial.println(F("[BME280] Detected successfully"));

        bme.setSampling(Adafruit_BME280::MODE_FORCED, Adafruit_BME280::SAMPLING_X1, Adafruit_BME280::SAMPLING_X1, Adafruit_BME280::SAMPLING_X1, Adafruit_BME280::FILTER_OFF);
    }
    else
    {
        Serial.println("[BME280] Could not find a valid sensor");
    }
    
    Serial.println(F("[CCS811] Searching..."));
    ccs.begin(0x5B);
    if(ccs.available())
    {
        ccs811_detected = true;
        Serial.println(F("[CCS811] Detected successfully"));

        float temp = ccs.calculateTemperature();
        ccs.setTempOffset(temp - 25.0);
        ccs.setDriveMode(CCS811_DRIVE_MODE_10SEC);
        ccs.disableInterrupt();
    }
    else
    {
        Serial.println("[CCS811] Could not find a valid sensor");
    }

    Serial.println();
}

bool env_loop()
{
    uint32_t curTime = millis();
    static int nextTime = 0;

    if (curTime >= nextTime)
    {
        esp32_hall = hallRead();
        
        if(bme280_detected)
        {
            if(bme.takeForcedMeasurement())
            {
                float temp = bme.readTemperature();
                float humd = bme.readHumidity();
                float pres = bme.readPressure() / 100.0F;
                float pres_corr = pres / pow(1 - BME_ALTITUDE/44330.0, 5.255);

                bme280_temperature = temp;
                bme280_pressure = pres_corr;
                bme280_humidity = humd;

                if(current_config.verbose & 2)
                {
                    Serial.print("Temperature = ");
                    Serial.print(bme280_temperature);
                    Serial.println(" °C");

                    Serial.print("Pressure = ");
                    Serial.print(bme280_pressure);
                    Serial.println(" hPa");

                    Serial.print("Humidity = ");
                    Serial.print(bme280_humidity);
                    Serial.println(" %");

                    Serial.println();
                }
            }
            else
            {
                Serial.println("[BME280] Failed to read data");
            }
        }

        ccs.setEnvironmentalData(bme280_humidity, bme280_temperature);

        if(ccs811_detected && ccs.available())
        {
            if(!ccs.readData())
            {
                ccs811_co2 = ccs.geteCO2();
                ccs811_tvoc = ccs.getTVOC();
                
                if(current_config.verbose & 2)
                {
                    Serial.print("CO2: ");
                    Serial.print(ccs811_co2);
                    Serial.print("ppm, TVOC: ");
                    Serial.println(ccs811_tvoc);
                }
            }
            else
            {
                Serial.println("[CCS811] Failed to read data");
            }
        }

        nextTime = curTime + 10000;
        return true;
    }

    return false;
}
