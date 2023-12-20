#ifndef SENSORS_H
#define SENSORS_H

#ifndef ESP32
#define SENSORS_INCLUDE_ONEWIRE
#endif

#include <Arduino.h>
#include <InfluxDbClient.h>
#include <DHTesp.h>
#include <Adafruit_BME280.h>
#ifdef SENSORS_INCLUDE_ONEWIRE
#include <OneWire.h>
#include <DallasTemperature.h>
#endif
#include <Adafruit_BMP280.h>
#include <Adafruit_SGP40.h>
#include <SparkFun_SCD30_Arduino_Library.h>
#include <ccs811.h>
#include <BH1750.h>
#include <Adafruit_Si7021.h>
#include <Adafruit_HTU21DF.h>
#include <SensirionI2CScd4x.h>
#include <SensirionI2CSen5x.h>
#include <SensirionI2CSgp41.h>
#include <SensirionI2CSht4x.h>
#include <SHTSensor.h>

extern const char *Temp;
extern const char *Hum;
extern const char *Press;
extern const char *PressRaw;
extern const char *Co2;
extern const char *Moist;

enum SensorCapability {
  CapTemperature = 1<<0,
  CapHumidity = 1<<1,
  CapPressure = 1<<2,
  CapCo2 = 1<<3,
  CapVoc = 1<<4,
  CapSoilMoisture = 1<<5,
  CapLightIntensity = 1<<6,
  CapDustPPM = 1<<7
};

class Sensor {
  protected:
    String name;
    String error;
    bool status;
  protected:
    Sensor(const char *name):name(name) { }
    Sensor() {}
  public:
    virtual ~Sensor() {};
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

class AnalogSensor : public Sensor {
  public:
    uint16_t rawValue;
    float value;
    float maxValue;
  protected:
    String fieldName;
    uint8_t pin;
    uint16_t capability;
    uint8_t averagingWindowSize;
    uint16_t *pAveragingWindow;
    uint8_t averagingWindowPointer;
    bool averageWindowWasTop;
  public:
    AnalogSensor(const char *name, const String& fieldName, uint8_t pin, uint16_t capability, float max = 3.3);
    virtual ~AnalogSensor();
    virtual bool init() override;
    virtual bool readValues() override;
    virtual void storeValues(Point &point) override;
    virtual uint16_t getCapabilities() override { return capability; }
    void setAveragingWindowSize(uint8_t size);
  protected:
    virtual String formatValues() override;
};

class IlluminationSensor : public Sensor {
  public:
    float lightIntensity;
  public:
    IlluminationSensor(const char *name):Sensor(name) {}
    virtual void storeValues(Point &point) override;
    virtual uint16_t getCapabilities() override { return SensorCapability::CapLightIntensity; }
  protected:
    virtual String formatValues() override;    
};

class VOCSensor : public Sensor {
  public:
    uint16_t vocRaw;
    uint16_t vocIndex;
  public:
    VOCSensor():vocRaw(0), vocIndex(0) {}
    VOCSensor(const char *name):Sensor(name),vocRaw(0), vocIndex(0) {}
    virtual void storeValues(Point &point) override;
    virtual uint16_t getCapabilities() override { return SensorCapability::CapVoc; }
  protected:
    virtual String formatValues() override;    
};

class CO2Sensor {
  public:
    uint16_t co2;
  public:
   void storeValues(Point &point);
   uint16_t getCapabilities() { return SensorCapability::CapCo2; }
  protected:
   String formatValues();  
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

class SHTXSensor : public TemperatureHumiditySensor {
  protected:
    SHTSensor sht;
  public:
    SHTXSensor(const char *name, SHTSensor::SHTSensorType typ):TemperatureHumiditySensor(name),sht(typ) {}
    virtual bool init() override;
    virtual bool readValues() override;
};

class SHT31Sensor : public SHTXSensor {
  public:
    SHT31Sensor():SHTXSensor("SHT31", SHTSensor::SHT3X) {}
};

#ifdef SENSORS_INCLUDE_ONEWIRE
class DS18B20Sensor : public TemperatureSensor {
  protected:
    OneWire oneWire;
    DallasTemperature sensor;
  public:
    DS18B20Sensor(uint8_t pin):TemperatureSensor("DS18B20"),oneWire(pin), sensor(&oneWire) {}
    virtual bool init() override;
    virtual bool readValues() override;
};
#endif

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


class SHTC3Sensor : public SHTXSensor {
  public:
    SHTC3Sensor():SHTXSensor("SHTC3", SHTSensor::SHTC3) {}
};

class SHT4XSensor : public TemperatureHumiditySensor {
  protected:
   SensirionI2CSht4x sht4x;
  public:
    SHT4XSensor():TemperatureHumiditySensor("SHT4X") {}
    virtual bool init() override;
    virtual bool readValues() override;
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

class SI702xSensor: public TemperatureHumiditySensor {
  private:
    Adafruit_Si7021 si7021;
    String typ;
  public:
    SI702xSensor():TemperatureHumiditySensor("SI702x") {}
    virtual bool init() override;
    virtual bool readValues() override;
    String getType() { return typ; }
};

class HTU21DSensor: public TemperatureHumiditySensor {
  private:
    Adafruit_HTU21DF htu;
  public:
    HTU21DSensor():TemperatureHumiditySensor("HTU21D") {}
    virtual bool init() override;
    virtual bool readValues() override;
};

class BH1750Sensor : public IlluminationSensor {
  public:
    BH1750 lightMeter;
  public:
    BH1750Sensor():IlluminationSensor("BH1750") {};
    virtual bool init() override;
    virtual bool readValues() override;
};

class SCD41Sensor : public TemperatureHumiditySensor, public CO2Sensor {
  protected:
    SensirionI2CScd4x scd4x;
  public:
    SCD41Sensor():TemperatureHumiditySensor("SCD41") { }
    virtual bool init() override;
    virtual bool readValues() override;
    virtual void storeValues(Point &point) override;
    virtual uint16_t getCapabilities() override { return TemperatureHumiditySensor::getCapabilities()|CO2Sensor::getCapabilities(); }
  protected:
    virtual String formatValues() override;
};

class SEN54Sensor : public TemperatureHumiditySensor {
  protected:
    SensirionI2CSen5x sen5x;
  public:
    float pm1p0;
    float pm2p5;
    float pm4p0;
    float pm10p0;
    float vocIndex;
  public:
    SEN54Sensor():TemperatureHumiditySensor("SEN54") { }
    virtual bool init() override;
    virtual bool readValues() override;
    virtual void storeValues(Point &point) override;
    virtual uint16_t getCapabilities() override { return TemperatureHumiditySensor::getCapabilities()|SensorCapability::CapVoc|SensorCapability::CapDustPPM; }
  protected:
    virtual String formatValues() override;
};

class SGP41Sensor : public VOCSensor {
  protected:
    SensirionI2CSgp41 _sgp41;
    uint16_t _conditioning_s = 10;
    uint32_t _timer = 0;
  public:
    uint16_t noxRaw = 0;
    uint16_t noxIndex = 0;
  public:
    SGP41Sensor():VOCSensor("SGP41") { }
    virtual bool init() override;
    virtual void storeValues(Point &point) override;
    bool readValues() { return false; }
    bool readValues(float temp, float hum);
    String formatValues();
};

#endif //SENSORS_H