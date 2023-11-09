#pragma once
#include <string>
#include <simpleble/Peripheral.h>

class LightHouse
{
public:

  static const char* LIGHTHOUSE_IDENTIFIER;
  static const char* SVC_UUID;
  static const char* CHARACTERISTIC_UUID;
  static const char  PWR_ON  = 0x01;
  static const char  PWR_OFF = 0x00;

  LightHouse(std::string address, 
             std::string identifier, 
             SimpleBLE::Peripheral* peripheral);
  ~LightHouse();

  std::string GetAddress() const;
  std::string GetIdentifier() const;
  void AddCharacteristic(std::string service, std::string characteristic);
  std::string GetValue(std::string service, std::string characteristic);
  bool WriteCharacteristic(std::string service, std::string characteristic, std::string value);
  bool ReadCharacteristics();
  bool IsValidLighthouse() const;
  void SetStatus(std::string status);
  std::string GetStatus() const;

private:

  std::string Address;
  std::string Identifier;
  std::string Status;

  struct Characteristic
  {
    std::string uuid;
    std::string value;
  };
  std::map<std::string, std::vector<Characteristic>> Services;

  SimpleBLE::Peripheral& BLEPeripheral;
};

