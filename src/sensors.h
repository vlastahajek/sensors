#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <InfluxDbClient.h>
#include <DHTesp.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SHT31.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_SHTC3.h>
#include <Adafruit_SGP40.h>
#include "SparkFun_SCD30_Arduino_Library.h"
#include <ccs811.h>

extern const char *Temp;
extern const char *Hum;
extern const char *Press;
extern const char *PressRaw;
extern const char *Co2;
extern const char *Moist;

enum SensorCapability {
  CapTemperature = 1,
  CapHumidity = 2,
  CapPressure = 4,
  CapCo2 = 8,
  CapVoc = 16,
  CapSoilMoisture = 32,
  CapLightIntensity = 64
};

class Sensor {
  protected:
    String name;
    String error;
    bool status;
  public:
    Sensor(const char *name):name(name) { }
    virtual bool init() = 0;
    virtual bool readValues() = 0;
    virtual void storeValues(Point &point) = 0;
    virtual String toString();
    virtual uint16_t getCapabilities() = 0;
    String getError() { return error; }
    bool getStatus() { return status; }
    String getName() { return name; }
  protected:
    virtual String formatValues() = 0;
};

class TemperatureSensor : public Sensor {
  public:
    float temp;
  public:
    TemperatureSensor(const char *name):Sensor(name) {}
    virtual void storeValues(Point &point) override;
    virtual uint16_t getCapabilities() override { return SensorCapability::CapTemperature; }
  protected:
    virtual String formatValues() override;
};

class PressureSensor {
  public:
    float pressRaw;
    float pressSeaLevel;
    float altitude;
  public:
    virtual uint16_t getCapabilities() { return SensorCapability::CapPressure; }
  protected:
    PressureSensor(float altitude):altitude(altitude) {}
};

class TemperatureHumiditySensor : public TemperatureSensor {
  public:
    float hum;
  public:
    TemperatureHumiditySensor(const char *name):TemperatureSensor(name) {}
    virtual void storeValues(Point &point) override;
    virtual uint16_t getCapabilities() override { return SensorCapability::CapTemperature|SensorCapability::CapHumidity; }
  protected:
    virtual String formatValues() override;
};

class DHTSensor : public TemperatureHumiditySensor {
  protected:
    DHTesp dht;
    uint8_t pin;
  public:
    DHTSensor(uint8_t pin):TemperatureHumiditySensor("DHT22"),pin(pin) {}
    virtual bool init() override;
    virtual bool readValues() override;
};

class BME280Sensor : public TemperatureHumiditySensor, public PressureSensor {
  protected:
    uint8_t address;
    Adafruit_BME280 bme;
  public:
    BME280Sensor(float altitude, uint8_t address = BME280_ADDRESS_ALTERNATE):TemperatureHumiditySensor("BME280"),PressureSensor(altitude),address(address) {}
    virtual bool init() override;
    virtual bool readValues() override;
    virtual void storeValues(Point &point) override;
    virtual uint16_t getCapabilities() override { return TemperatureHumiditySensor::getCapabilities()|PressureSensor::getCapabilities(); }
  protected:
    virtual String formatValues() override;
};

class SHT31Sensor : public TemperatureHumiditySensor {
  protected:
    Adafruit_SHT31 sht31;
  public:
    SHT31Sensor():TemperatureHumiditySensor("SHT31") {}
    virtual bool init() override;
    virtual bool readValues() override;
};

class DS18B20Sensor : public TemperatureSensor {
  protected:
    OneWire oneWire;
    DallasTemperature sensor;
  public:
    DS18B20Sensor(uint8_t pin):TemperatureSensor("DS18B20"),oneWire(pin), sensor(&oneWire) {}
    virtual bool init() override;
    virtual bool readValues() override;
};

class BMP280Sensor : public TemperatureSensor, public PressureSensor {
  protected:
    Adafruit_BMP280 bmp;
  public:
    BMP280Sensor(float altitude):TemperatureSensor("BMP280"),PressureSensor(altitude) {}
    virtual bool init() override;
    virtual bool readValues() override;
    virtual void storeValues(Point &point) override;
    virtual uint16_t getCapabilities() override { return TemperatureSensor::getCapabilities()|PressureSensor::getCapabilities(); }
  protected:
    virtual String formatValues() override;
};

class AnalogSensor : public Sensor {
  public:
    uint16_t rawValue;
    float value;
  protected:
    String fieldName;
    uint8_t pin;
    uint16_t capability;
  public:
    AnalogSensor(const char *name, String fieldName, uint8_t pin, uint16_t capability):Sensor(name),fieldName(fieldName),pin(pin),capability(capability)  { }
    virtual bool init() override;
    virtual bool readValues() override;
    virtual void storeValues(Point &point) override;
    virtual uint16_t getCapabilities() override { return capability; }
  protected:
    virtual String formatValues() override;
};

class SHTC3Sensor : public TemperatureHumiditySensor {
 protected:
    Adafruit_SHTC3 shtc3;
  public:
    SHTC3Sensor():TemperatureHumiditySensor("SHTC3") {}
    virtual bool init() override;
    virtual bool readValues() override;
};

class VOCSensor : public Sensor {
  public:
    uint16_t raw;
    uint16_t vocIndex;
  public:
    VOCSensor(const char *name):Sensor(name) {}
    virtual void storeValues(Point &point) override;
    virtual uint16_t getCapabilities() override { return SensorCapability::CapVoc; }
  protected:
    virtual String formatValues() override;    
};

class SGP40Sensor : public VOCSensor {
  protected:
    Adafruit_SGP40 sgp;
  public:
    SGP40Sensor():VOCSensor("SGP40") { }
    virtual bool init() override;
    bool readValues() { return false; }
    bool readValues(float temp, float hum);
};

class CO2Sensor {
  public:
    uint16_t co2;
  public:
    virtual uint16_t getCapabilities() { return SensorCapability::CapCo2; }
};

class SCD30Sensor : public TemperatureHumiditySensor, public CO2Sensor {
  protected:
    SCD30 scd30;
  public:
    SCD30Sensor():TemperatureHumiditySensor("SCD30") {}
    virtual bool init() override;
    virtual bool readValues() override;
    virtual void storeValues(Point &point) override;
    virtual uint16_t getCapabilities() override { return CO2Sensor::getCapabilities(); }
  protected:
    virtual String formatValues() override;    
};

class CCS811Sensor : public VOCSensor, public CO2Sensor {
  protected:
    CCS811 ccs811;
  public:
    CCS811Sensor():VOCSensor("CCS811") { }
    virtual bool init() override;
    virtual bool readValues() override;
    virtual void storeValues(Point &point) override;
    virtual uint16_t getCapabilities() override { return VOCSensor::getCapabilities()|CO2Sensor::getCapabilities(); }
  protected:
    virtual String formatValues() override;
};

#endif //SENSORS_H