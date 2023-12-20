#include "Sensors.h"
#include <Wire.h>

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
  if(status) {
      ret += formatValues();
  } else {
    ret += F("ERR: ");
    ret += error;
  }
  return ret;
}

// ===========  TemperatureSensor  ==================

void TemperatureSensor::storeValues(Point &point) {
    point.addField(FPSTR(Temp), temp);
}

String TemperatureSensor::formatValues() {
  char buff[30];
  snprintf_P(buff, 30, PSTR("%3.1f°C"), temp);
  return buff;
}

// ===========  TemperatureHumiditySensor  ==================

void TemperatureHumiditySensor::storeValues(Point &point) {
    TemperatureSensor::storeValues(point);
    point.addField(FPSTR(Hum), hum);
}

String TemperatureHumiditySensor::formatValues() {
  char buff[30];
  String ret;
  ret.reserve(50);
  ret += TemperatureSensor::formatValues();
  snprintf_P(buff,30, PSTR("  %2.0f%%"), hum);
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
    status = false;
  } else {
    error = "";
    status = true;
  }
  return status;
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
  char buff[30];
  String ret;
  ret.reserve(50);
  ret += TemperatureHumiditySensor::formatValues();
  snprintf_P(buff, 30, PSTR("  %4.0fhPa"), pressSeaLevel);
  ret += buff;
  return ret;
}

bool BME280Sensor::readValues() {
  bme.takeForcedMeasurement();
  temp = bme.readTemperature();
  error = "";
  status = false;
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
  status = true;
  return status;
}

// ===========  SHT31  ==================

bool SHTXSensor::init() {
  status = true;
  uint8_t err = sht.init();
  if(err) {
    error = name;
    error += F(" init err: ");
    error += err;
    error += F(" type: ");
    error += sht.mSensorType;
    status = false;
  } 
  return status;
}

bool SHTXSensor::readValues() {
  status = false;
  uint8_t err = sht.readSample();
  if(!err) {
    float t = sht.getTemperature();
    float h = sht.getHumidity();
    if (!isnan(t)) {  // check if 'is not a number'
      temp = t;
    } else { 
      error = name;
      error += F(" temp error");
      return false;
    }
    
    if (!isnan(h)) {  // check if 'is not a number'
      hum = h; 
    } else { 
      error = name;
      error += F(" hum error");
      return false;
    }
  } else {
    error = name;
    error += F(" read err: ");
    error += err;
    return false;
  }
  error = "";
  status = true;
  return true;
}

// ===========  SHT31  ==================

bool SHT4XSensor::init() {
  status = true;
  sht4x.begin(Wire);
  uint32_t serialNumber;
  uint16_t err =sht4x.serialNumber(serialNumber);
  if(err) {
    error = name;
    error += F(" init err: ");
    error += err;
    status = false;
  } 
  return status;
}

bool SHT4XSensor::readValues() {
  status = false;
  float t;
  float h;
  uint16_t err = sht4x.measureHighPrecision(t,h);
  if(!err) {
    if (!isnan(t)) {  // check if 'is not a number'
      temp = t;
    } else { 
      error = name;
      error += F(" temp error");
      return false;
    }
    
    if (!isnan(h)) {  // check if 'is not a number'
      hum = h; 
    } else { 
      error = name;
      error += F(" hum error");
      return false;
    }
  } else {
    error = name;
    error += F(" read err: ");
    error += err;
    return false;
  }
  error = "";
  status = true;
  return true;
}


#ifdef SENSORS_INCLUDE_ONEWIRE
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
  status = false;
  temp = sensor.getTempCByIndex(0);
  if(temp == DEVICE_DISCONNECTED_C) {
    error = F("DS18b20 error");
    return false;
  } 
  status = true;
  return true;
}

#endif
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
  status = false;
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
  status = true;
  return true;
}

void BMP280Sensor::storeValues(Point &point) {
  TemperatureSensor::storeValues(point);
  point.addField(FPSTR(Press), pressSeaLevel);
  point.addField(FPSTR(PressRaw), pressRaw);
}

String BMP280Sensor::formatValues() {
  char buff[30];
  String ret;
  ret.reserve(50);
  ret += TemperatureSensor::formatValues();
  snprintf_P(buff, 30, PSTR("  %4.0fhPa"), pressSeaLevel);
  ret += buff;
  return ret;
}

// =================== AnalogSensor ===================

AnalogSensor::AnalogSensor(const char *name, const String& fieldName, uint8_t pin, uint16_t capability, float max):
  Sensor(name),fieldName(fieldName),pin(pin), maxValue(max),capability(capability),
  averagingWindowSize(0),pAveragingWindow(nullptr),averagingWindowPointer(0),averageWindowWasTop(false) { 
}
AnalogSensor::~AnalogSensor() {
  if(pAveragingWindow) {
    delete [] pAveragingWindow;
  }
}

bool AnalogSensor::init() {
  status = true;
  return true;
}

bool AnalogSensor::readValues() {
  uint32_t cum = 0;
  uint8_t numReadings = 10;
  for(uint8_t i=0;i<numReadings;i++) {
    cum += analogRead(pin);
    delay(1);
  }
  rawValue = (uint16_t)(cum/numReadings);
  if(averagingWindowSize) {
    pAveragingWindow[averagingWindowPointer++] = rawValue;
    if(averagingWindowPointer == averagingWindowSize) {
      averagingWindowPointer = 0;
      averageWindowWasTop = true;
    }
    cum = 0;
    auto top = averageWindowWasTop?averagingWindowSize:averagingWindowPointer;
    for(auto i=0;i<top;i++) {
      cum += pAveragingWindow[i];
    }
    rawValue = (uint16_t)(cum/top);
  }
  
#ifdef ESP32  
  value = (rawValue/4095.0)*maxValue;
#elif defined(ESP8266)
  value = (rawValue/1023.0)*maxValue;
#else
  value = (rawValue/255.0)*maxValue;
#endif
  status = true;
  return true;
}

void AnalogSensor::storeValues(Point &point) {
  point.addField(fieldName, value);
  point.addField(fieldName + "_raw", rawValue);
}

String AnalogSensor::formatValues() {
  char buff[30];
  snprintf_P(buff, 30, PSTR(" %4d  %1.3fV"), rawValue, value);
  return buff;
}

void AnalogSensor::setAveragingWindowSize(uint8_t size) {
  if(pAveragingWindow) {
    delete [] pAveragingWindow;
    pAveragingWindow = nullptr;
  }
  averagingWindowSize = size;
  averagingWindowPointer = 0;
  if(size > 0) {
    pAveragingWindow = new uint16_t[size];
    for(auto i=0;i<size;i++) {
      pAveragingWindow[i] = 0;
    }
  }
}


// ===========  VOCSensor  ==================

void VOCSensor::storeValues(Point &point) {
  point.addField(F("voc"),(float)vocIndex);
  point.addField(F("gas_resistance"),(float)vocRaw);
}

String VOCSensor::formatValues() {
  char buff[30];
  snprintf_P(buff, 30, PSTR(" %6dr %3dv"), vocRaw, vocIndex);
  return buff;
}

// ===========  Co2Sensor  ==================

void CO2Sensor::storeValues(Point &point) {
  point.addField(FPSTR(Co2),(float)co2);
}

String CO2Sensor::formatValues() {
  char buff[30];
  snprintf_P(buff, 30, PSTR(" %5dppm"), co2);
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
  vocRaw = sgp.measureRaw(temp, hum );
  vocIndex = sgp.measureVocIndex(temp, hum);
  status = true;
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
  status = false;
  if (scd30.dataAvailable()) {
    co2 = scd30.getCO2();
    temp = scd30.getTemperature();
    hum = scd30.getHumidity();
    if(!co2) {
      error = F("SCD30 read err: invalid sample detected");
      return false;
    }
  } else {
    error = F("SCD30 read error");
    return false;
  }
  status = true;
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
  ret += ' ';
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
  ccs811.read(&co2,&vocIndex,&errstat,&vocRaw); 
  status = false;
  if( errstat != CCS811_ERRSTAT_OK ) { 
    if( errstat == CCS811_ERRSTAT_OK_NODATA ) {
      error = F("CCS811: waiting for (new) data");
    } else if( errstat & CCS811_ERRSTAT_I2CFAIL ) { 
      error = F("CCS811: I2C error");
    } else {
      error = ccs811.errstat_str(errstat);
    }
    return false;
  }
  status = true;
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
  status = false;
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
  status = true;
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
  status = false;
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
  status = true;
  return true;
}

// ===========  IlluminationSensor  ==================

void IlluminationSensor::storeValues(Point &point) {
  point.addField(F("light"),lightIntensity);
}

String IlluminationSensor::formatValues() {
  char buff[30];
  snprintf_P(buff, 30, PSTR(" %3.1flux"), lightIntensity);
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
  status = false;
  if(lightIntensity < 0) {
    error = F("BH1750 err");
    return false;
  }
  status = true;
  return true;
}


// ===========  SCD41Sensor  ==================

#define MESSAGE_SIZE 256
bool SCD41Sensor::init() {
  scd4x.begin(Wire);
  status = true;
   // stop potentially previously started measurement
  uint16_t err = scd4x.stopPeriodicMeasurement(); 
  if(err) {
    char buff[MESSAGE_SIZE];
    error = F("SCD41 init err: ");
    errorToString(err, buff, MESSAGE_SIZE);
    error += buff;
    status = false;
  } else {
    err = scd4x.startPeriodicMeasurement();
    if (err) {
      char buff[MESSAGE_SIZE];
      error = F("SCD41 start err: ");
      errorToString(err, buff, MESSAGE_SIZE);
      error += buff;
      status = false;
    }
  }
  return status;
}

bool SCD41Sensor::readValues() {
  uint16_t err = scd4x.readMeasurement(co2, temp, hum); 
  status = false;
  if (err) {
      char buff[MESSAGE_SIZE];
      error = F("SCD41 read err: ");
      errorToString(err, buff, MESSAGE_SIZE);
      error += buff;
      return false;
  }
  if(!co2) {
    error = F("SCD41 read err: invalid sample detected");
    return false;
  }
  status = true;
  return true;
}

void SCD41Sensor::storeValues(Point &point) {
  TemperatureHumiditySensor::storeValues(point);
  CO2Sensor::storeValues(point);
}

String SCD41Sensor::formatValues() {
  String ret;
  ret.reserve(50);
  ret += CO2Sensor::formatValues();
  ret += " ";
  ret += TemperatureHumiditySensor::formatValues();
  return ret;
}

// ===========  SEN54Sensor  ==================

bool SEN54Sensor::init() {
  sen5x.begin(Wire);
  status = true;
   // stop potentially previously started measurement
  uint16_t err = sen5x.deviceReset();
  if(err) {
    char buff[MESSAGE_SIZE];
    error = F("SEN54 reset err: ");
    errorToString(err, buff, MESSAGE_SIZE);
    error += buff;
    status = false;
  } else {
    err = sen5x.startMeasurement();
    if (err) {
      char buff[MESSAGE_SIZE];
      error = F("SEN54 start err: ");
      errorToString(err, buff, MESSAGE_SIZE);
      error += buff;
      status = false;
    }
  }
  return status;
}

bool SEN54Sensor::readValues() {
  float noxIndex;
  uint16_t err = sen5x.readMeasuredValues(pm1p0, pm2p5, pm4p0,pm10p0, hum, temp, vocIndex, noxIndex);
  status = false;
  if (err) {
      char buff[MESSAGE_SIZE];
      error = F("SEN54 read err: ");
      errorToString(err, buff, MESSAGE_SIZE);
      error += buff;
      return false;
  }
  status = true;
  return true;
}

void SEN54Sensor::storeValues(Point &point) {
  TemperatureHumiditySensor::storeValues(point);
  point.addField(F("voc"), vocIndex);
  point.addField(F("pm1.0"), pm1p0);
  point.addField(F("pm2.5"), pm2p5);
  point.addField(F("pm4.0"), pm4p0);
  point.addField(F("pm10.0"), pm10p0);
}

String SEN54Sensor::formatValues() {
  char buff[60];
  String ret;
  ret.reserve(100);
  snprintf_P(buff, 60, PSTR(" %3.0fvoc, pm1 %2.1f, pm2.5 %2.1f,pm4 %2.1f,pm10 %2.1f "), vocIndex, pm1p0, pm2p5, pm4p0, pm10p0);
  ret += buff;
  ret += TemperatureHumiditySensor::formatValues();
  return ret;
}
// ===========  SGP41Sensor  ==================

static const uint16_t DefaultRh = 0x8000;
static const uint16_t DefaultT = 0x6666; 

bool SGP41Sensor::init() {
  char errorMessage[256];  
  uint16_t testResult;
  _sgp41.begin(Wire);
  uint16_t err = _sgp41.executeSelfTest(testResult); 
  status = false;
  if (err) {
        Serial.print("Error trying to execute executeSelfTest(): ");
        error = F("SPG41 init err: ");
        errorToString(err, errorMessage, 256);
        error += errorMessage;
    } else if (testResult != 0xD400) {
        error = F("SPG41 test err: ");
        error += String(testResult, HEX);
    } else {
      status = true;
    }
  return status;
}

void SGP41Sensor::storeValues(Point &point) {
  VOCSensor::storeValues(point);
  point.addField(F("nox"),(float)noxIndex);
  point.addField(F("nox_gas_resistance"),(float)noxRaw);
}

bool SGP41Sensor::readValues(float temp, float hum) {
  uint16_t err;
  if(!_timer || ((millis()-_timer)/1000)<10) {
    if(!_timer) {
      _timer = millis();
    }
    err = _sgp41.executeConditioning(DefaultRh, DefaultT, vocRaw); 
  } else {
    temp +=45;
    temp *= 65535;
    temp /= 175;
    hum *= 65535;
    hum /=100;
    err = _sgp41.measureRawSignals(hum, temp, vocRaw, noxRaw);
  }
  status = true;
  if(err) {
    char buff[256];
    error = F("SGP41 read err: ");
    errorToString(err, buff, MESSAGE_SIZE);
    error += buff;
    status = false;
  }
  return status;
}

String SGP41Sensor::formatValues() {
  char buff[60];
  String ret;
  ret.reserve(100);
  ret += VOCSensor::formatValues();
  snprintf_P(buff, 60, PSTR(" nox: %6dr %3dv"), noxRaw, noxIndex);
  ret += buff;
  return ret;
}