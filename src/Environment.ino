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

float bme280_temperature;
float bme280_humidity;
float bme280_pressure;
bool bme280_detected = false;

uint16_t ccs811_co2;
uint16_t ccs811_tvoc;

void env_setup()
{
    Serial.println(F("[BME280] Detection"));

    Wire.begin(26, 25);
    bool status = bme.begin(0x76);
    if (!status)
    {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
    }
    else
    {
        bme280_detected = true;
        bme.setSampling(
            Adafruit_BME280::MODE_FORCED,
            Adafruit_BME280::SAMPLING_X1,
            Adafruit_BME280::SAMPLING_X1,
            Adafruit_BME280::SAMPLING_X1,
            Adafruit_BME280::FILTER_OFF);
            
        Serial.println(F("..success BME280"));
    }

    
    
    ccs.begin(0x5B);
    if(ccs.available())
    {
        float temp = ccs.calculateTemperature();
        ccs.setTempOffset(temp - 25.0);
        ccs.setDriveMode(CCS811_DRIVE_MODE_10SEC);
        ccs.disableInterrupt();
        Serial.println(F("..success CCS811"));
    }

    Serial.println();
}

bool env_loop()
{
    uint32_t time = millis();
    static int nextTimeBME280 = 0;

    if(!bme280_detected)
    {
        return false;
    }
    
    if (time >= nextTimeBME280)
    {
        bme.takeForcedMeasurement();
        float temp = bme.readTemperature();
        float humd = bme.readHumidity();
        float pres = bme.readPressure() / 100.0F;
        float pres_corr = pres / pow(1 - BME_ALTITUDE/44330.0, 5.255);

        bme280_temperature = temp;
        bme280_pressure = pres_corr;
        bme280_humidity = humd;

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
        ccs.setEnvironmentalData(bme280_humidity, bme280_temperature);
        if(ccs.available()){
            if(!ccs.readData()){
                Serial.print("CO2: ");
                ccs811_co2 = ccs.geteCO2();
                Serial.print(ccs811_co2);
                Serial.print("ppm, TVOC: ");
                ccs811_tvoc = ccs.getTVOC();
                Serial.println(ccs811_tvoc);
            }
            else{
                Serial.println("ERROR reading CSS811!");
            }
        }
        Serial.println();

        nextTimeBME280 = time + 10000;
    }
    return true;
}
