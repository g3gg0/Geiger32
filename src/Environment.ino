#include "Config.h"

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include "Adafruit_CCS811.h"

Adafruit_BME280 bme;
Adafruit_CCS811 ccs;

#define SEALEVELPRESSURE_HPA (1013.25)

float bme280_temperature;
float bme280_humidity;
float bme280_pressure;

uint16_t ccs811_co2;
uint16_t ccs811_tvoc;

void env_setup()
{
    Serial.println(F("BME280 test"));

    unsigned status;
    
    // default settings
    //status = bme.begin();  
    // You can also pass in a Wire library object like &Wire2
    Wire.begin(26, 25);
    status = bme.begin(0x76);
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
        while (1) delay(10);
    }
    Serial.println(F("..success BME280"));
    
    ccs.begin(0x5B);
    while(!ccs.available());
    float temp = ccs.calculateTemperature();
    ccs.setTempOffset(temp - 25.0);
    ccs.setDriveMode(CCS811_DRIVE_MODE_10SEC);
    ccs.disableInterrupt();
    Serial.println(F("..success CCS811"));

    Serial.println();
}

bool env_loop()
{
    uint32_t time = millis();
    static int nextTimeBME280 = 0;
    
    if (time >= nextTimeBME280)
    {
        Serial.print("Temperature = ");
        bme280_temperature = bme.readTemperature();
        Serial.print(bme280_temperature);
        Serial.println(" °C");

        Serial.print("Pressure = ");

        bme280_pressure = bme.readPressure() / 100.0F;
        Serial.print(bme280_pressure);
        Serial.println(" hPa");

        Serial.print("Approx. Altitude = ");
        Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
        Serial.println(" m");

        Serial.print("Humidity = ");
        bme280_humidity = bme.readHumidity();
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
