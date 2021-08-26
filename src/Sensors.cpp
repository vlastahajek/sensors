#include "Sensors.h"

const char *Temp PROGMEM = "temp";
const char *Hum PROGMEM = "hum";
const char *Press PROGMEM = "press";
const char *PressRaw PROGMEM = "press_raw";
const char *Co2 PROGMEM = "co2";
const char *Moist PROGMEM = "moist";

String Sensor::toString() {
  String ret;
  ret.reserve(30);
  ret += name;
  ret += ": ";
  ret += formatValues();
  return ret;
}

// ===========  TemperatureSensor  ==================

void TemperatureSensor::storeValues(Point &point) {
    point.addField(FPSTR(Temp), temp);
}

String TemperatureSensor::formatValues() {
  char buff[10];
  sprintf_P(buff, PSTR("%3.1f°C"), temp);
  return buff;
}

// ===========  TemperatureHumiditySensor  ==================

void TemperatureHumiditySensor::storeValues(Point &point) {
    TemperatureSensor::storeValues(point);
    point.addField(FPSTR(Hum), hum);
}

String TemperatureHumiditySensor::formatValues() {
  char buff[10];
  String ret;
  ret.reserve(20);
  ret += TemperatureSensor::formatValues();
  sprintf_P(buff, PSTR("  %2.0f%%"), hum);
  ret += buff;
  return ret;
}

// ===========  DHT  ==================
bool DHTSensor::init() {
  dht.setup(pin, DHTesp::AM2302);
  temp = dht.getTemperature();
  if (isnan(temp)) {
    error = F("DHT err");
    status = false;
  } else {
    status = true;
  }
  return status;
}

bool DHTSensor::readValues() {
  temp = dht.getTemperature();
  hum = dht.getHumidity();
  if (isnan(temp) || isnan(hum)) {
    error = F("DHT err");
    return false;
  } else {
    error = "";
    return true;
  }
}

// ===========  BME280  ==================

bool BME280Sensor::init() {
  status = bme.begin(address);
  if(!status) {
    error = F("BME280 init error");
  } else {
    //set weather station mode (BME datasheet, ch3.5)
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF);
  }
  return status;
}

void BME280Sensor::storeValues(Point &point) {
  TemperatureHumiditySensor::storeValues(point);
  point.addField(FPSTR(Press), pressSeaLevel);
  point.addField(FPSTR(PressRaw), pressRaw);
}

String BME280Sensor::formatValues() {
  char buff[15];
  String ret;
  ret.reserve(30);
  ret += TemperatureHumiditySensor::formatValues();
  sprintf_P(buff, PSTR("  %4.0fhPa"), pressSeaLevel);
  ret += buff;
  return ret;
}

bool BME280Sensor::readValues() {
  bme.takeForcedMeasurement();
  temp = bme.readTemperature();
  error = "";
  if(isnan(temp)) {
    error = F("BME280 temp error");
    return false;
  }
  hum = bme.readHumidity();
  if(isnan(hum)) {
    error = F("BME280 hum error");
    return false;
  }
  pressRaw = bme.readPressure();
  if(isnan(pressRaw)) {
    error = F("BME280 press error");
    return false;
  } else {
    pressSeaLevel = bme.seaLevelForAltitude(altitude, pressRaw)/100.0;
    pressRaw /= 100.0;
  }
  return true;
}

// ===========  SHT31  ==================

bool SHT31Sensor::init() {
  status = sht31.begin(0x44);
  if(!status) {
    error = F("SHT31 init err");
  }
  return status;
}

bool SHT31Sensor::readValues() {
  if(sht31.readTempHum()) {
    int16_t t = sht31.readTemperature();
    int16_t h = sht31.readHumidity();
    if (t != MAXINT16) {  // check if 'is not a number'
      temp = t/10.0;
    } else { 
      error = F("SHT31 temp error");
      return false;
    }
    
    if (h != MAXINT16) {  // check if 'is not a number'
      hum = h/10.0; 
    } else { 
      error = F("SHT31 hum error");
      return false;
    }
  } else {
    error = F("SHT31 read err");
    return false;
  }
  error = "";
  return true;
}

// ===========  DS18B20Sensor  ==================

bool DS18B20Sensor::init() {
  sensor.begin();
  status = sensor.getDeviceCount() > 0;
  if(!status) {
    error = F("No 1W device found");
  }
  return status;
}

bool DS18B20Sensor::readValues() {
  sensor.requestTemperatures();
  
  temp = sensor.getTempCByIndex(0);
  if(temp == DEVICE_DISCONNECTED_C) {
    error = F("DS18b20 error");
    return false;
  } 
  return true;
}

// ===========  BMP280Sensor  ==================

bool BMP280Sensor::init() {
  status = bmp.begin(BMP280_ADDRESS_ALT);
  if(!status) {
    error = F("BMP280 error");
  }
  return status;
}

bool BMP280Sensor::readValues() {
  temp = bmp.readTemperature();
  error = "";
  if(isnan(temp)) {
    error = F("BMP280 temp error");
    return false;
  }
  pressRaw = bmp.readPressure();
  if(isnan(pressRaw)) {
    error = F("BME280 press error");
    return false;
  } else {
    pressSeaLevel = bmp.seaLevelForAltitude(altitude, pressRaw)/100.0;
    pressRaw /= 100.0;
  }
  return true;
}

void BMP280Sensor::storeValues(Point &point) {
  TemperatureSensor::storeValues(point);
  point.addField(FPSTR(Press), pressSeaLevel);
  point.addField(FPSTR(PressRaw), pressRaw);
}

String BMP280Sensor::formatValues() {
  char buff[15];
  String ret;
  ret.reserve(30);
  ret += TemperatureSensor::formatValues();
  sprintf_P(buff, PSTR("  %4.0fhPa"), pressSeaLevel);
  ret += buff;
  return ret;
}

// =================== AnalogSensor ===================

bool AnalogSensor::init() {
  status = true;
  return true;
}

bool AnalogSensor::readValues() {
  rawValue = analogRead(pin);
  value = (rawValue/4095.0)*3.3;
  return true;
}

void AnalogSensor::storeValues(Point &point) {
  point.addField(fieldName, value);
}

String AnalogSensor::formatValues() {
  char buff[15];
  sprintf_P(buff, PSTR(" %4d  %1.3fV"), rawValue, value);
  return buff;
}


// ===========  SHTC3Sensor  ==================

bool SHTC3Sensor::init() {
  status = shtc3.begin();
  if(!status) {
    error = F("SHTC3 init err");
  }
  return status;
}

bool SHTC3Sensor::readValues() {
  sensors_event_t humidity, temperature;
  if(shtc3.getEvent(&humidity, &temperature)) {
    temp = temperature.temperature;
    hum = humidity.relative_humidity;
  } else {
    error = F("SHTC3 err");
    return false;
  }
  return true;
}

// ===========  VOCSensor  ==================

void VOCSensor::storeValues(Point &point) {
  point.addField(F("voc"),(float)vocIndex);
  point.addField(F("gas_resistance"),(float)raw);
}

String VOCSensor::formatValues() {
  char buff[30];
  sprintf_P(buff, PSTR(" %6dr %3dv"), raw, vocIndex);
  return buff;
}

// ===========  Co2Sensor  ==================

void CO2Sensor::storeValues(Point &point) {
  point.addField(FPSTR(Co2),(float)co2);
}

String CO2Sensor::formatValues() {
  char buff[30];
  sprintf_P(buff, PSTR(" %5dppm"), co2);
  return buff;
}

// ===========  SGP40Sensor  ==================

bool SGP40Sensor::init() {
  status = sgp.begin();
  if(!status) {
    error = F("SGP40 init err");
  }
  return status;
}

bool SGP40Sensor::readValues(float temp, float hum) {
  raw = sgp.measureRaw(temp, hum );
  vocIndex = sgp.measureVocIndex(temp, hum);
  return true;
}

// ===========  SCD30Sensor  ==================

bool SCD30Sensor::init() {
  status = scd30.begin();
  if(!status) {
    error = F("SCD30 init err");
  }
  return status;
}

bool SCD30Sensor::readValues() {
  if (scd30.dataAvailable()) {
    co2 = scd30.getCO2();
    temp = scd30.getTemperature();
    hum = scd30.getHumidity();
  } else {
    error = F("SCD30 read error");
    return false;
  }
  return true;
}

void SCD30Sensor::storeValues(Point &point) {
  TemperatureHumiditySensor::storeValues(point);
  CO2Sensor::storeValues(point);
}

String SCD30Sensor::formatValues() {
  String ret;
  ret.reserve(50);
  ret += CO2Sensor::formatValues();
  ret += TemperatureHumiditySensor::formatValues();
  return ret;
}

// ===========  CCS811  ==================

bool CCS811Sensor::init() {
  ccs811.set_i2cdelay(50); // Needed for ESP8266 because it doesn't handle I2C clock stretch correctly
  status = ccs811.begin();
  if(!status) {
    error = F("CCS811 init error");
  } else {
    status = ccs811.start(CCS811_MODE_10SEC);
    if(!status) {
      error = F("CCS811 start error");
    }
  }
  return status;
}

bool CCS811Sensor::readValues() {
  uint16_t errstat;
  ccs811.read(&co2,&vocIndex,&errstat,&raw); 
  if( errstat!=CCS811_ERRSTAT_OK ) { 
    if( errstat==CCS811_ERRSTAT_OK_NODATA ) {
      error = F("CCS811: waiting for (new) data");
    } else if( errstat & CCS811_ERRSTAT_I2CFAIL ) { 
      error = F("CCS811: I2C error");
    } else {
      error =  ccs811.errstat_str(errstat);
    }
    return false;
  }
  return true;
}

void CCS811Sensor::storeValues(Point &point) {
  CO2Sensor::storeValues(point); 
  VOCSensor::storeValues(point);
}

String CCS811Sensor::formatValues() {
  String ret;
  ret.reserve(50);
  ret += CO2Sensor::formatValues();
  ret += VOCSensor::formatValues();
  return ret;
}

// ===========  SI702xSensor  ==================

bool SI702xSensor::init() {
  status = si7021.begin();
  if(status) {
    switch(si7021.getModel()) {
      case SI_Engineering_Samples:
        typ = F("SI engineering sample");
        break;
      case SI_7013:
        typ = F("Si7013");
        break;
      case SI_7020:
        typ = F("Si7020");
        break;
      case SI_7021:
        typ = F("Si7021");
        break;
      case SI_UNKNOWN:
      default:
        typ = F("Unknown");
    }
  } else {
    error = F("Si702x init err");
  }
  return status;
}

bool SI702xSensor::readValues() {
  float t = si7021.readTemperature();
  if(!isnan(t)) {
    temp = t;
    float h = si7021.readHumidity();
    if(!isnan(h)) {
      hum = h;
    }
  } else {
    error = F("SI702X err");
    return false;
  }
  return true;
}

// ===========  SHTC3Sensor  ==================

bool HTU21DSensor::init() {
  status = htu.begin();
  if(!status) {
    error = F("HTU21D init err");
  }
  return status;
}

bool HTU21DSensor::readValues() {
  float t = htu.readTemperature();
  if(!isnan(t)) {
    temp = t;
    float h = htu.readHumidity();
    if(!isnan(h)) {
      hum = h;
    }
  } else {
    error = F("HTU21D err");
    return false;
  }
  return true;
}

// ===========  IlluminationSensor  ==================

void IlluminationSensor::storeValues(Point &point) {
  point.addField(F("light"),lightIntensity);
}

String IlluminationSensor::formatValues() {
  char buff[20];
  sprintf_P(buff, PSTR(" %3.1flux"), lightIntensity);
  return buff;
}


// ===========  BH1750Sensor  ==================

bool BH1750Sensor::init() {
  status = lightMeter.begin();
  if(!status) {
    error = F("BH1750 init err");
  }
  return status;
}

bool BH1750Sensor::readValues() {
  lightIntensity = lightMeter.readLightLevel();;
  if(lightIntensity < 0) {
    error = F("BH1750 err");
    return false;
  }
  return true;
}
