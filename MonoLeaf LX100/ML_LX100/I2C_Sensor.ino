#include <Adafruit_AHTX0.h>

Adafruit_AHTX0 aht_1, aht_2;

bool hasAHT_1 = false , hasAHT_2 = false;

bool isAtDevice(TwoWire &bus, byte address){
  bus.beginTransmission(address);
  return (bus.endTransmission() == 0);
}

void ScanBus(){
  //Scan Bus 1
  if(isAtDevice(Wire, 0x38)){
    if(aht_1.begin(&Wire)){
      hasAHT_1 = true;
      Bus_1 = "AHT10 / AHT20";
    }
  }

  //Scan Bus 2
  if(isAtDevice(Wire1, 0x38)){
    if(aht_2.begin(&Wire1)){
      hasAHT_2 = true;
      Bus_2 = "AHT10 / AHT20";
    }
  }

}

void ReadData(){
  // Read Bus 1
  if(hasAHT_1){
    sensors_event_t hum,temp;
    aht_1.getEvent(&hum,&temp);
    temp_bus_1 = temp.temperature;
    hum_bus_1 = hum.relative_humidity;

  }

  // Read Bus 2
  /*
  if(hasAHT_2){
    sensors_event_t hum,temp;
    aht_2.getEvent(&hum,&temp);
    temp_bus_2 = temp.temperature;
    hum_bus_2 = hum.relative_humidity;

  }*/

  hasAHT_2 = true;
  Bus_2 = "BME680";
  air_bus_2 = 45; // Dummy test temperature
  //hum_bus_2 = 60;  // Dummy test humidity

}