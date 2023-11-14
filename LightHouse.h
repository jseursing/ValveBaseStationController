#pragma once
#include <string>
#include <simpleble/Peripheral.h>

class LightHouse
{
public:

  static const char* LIGHTHOUSE_ID;
  static const char* PWR_SVC_UUID;
  static const char* PWR_CHAR_UUID;
  static const char  PWR_ON  = 0x01;
  static const char  PWR_OFF = 0x00;

  LightHouse(std::string address, 
             std::string identifier, 
             SimpleBLE::Peripheral* peripheral);
  ~LightHouse();

  std::string GetAddress() const;
  std::string GetIdentifier() const;
  void AddCharacteristic(std::string service, std::string characteristic);
  bool WriteCharacteristic(std::string service, std::string characteristic, std::string value);
  std::string ReadCharacteristic(std::string service, std::string characteristic);
  bool ReadCharacteristics();
  bool IsValidLighthouse() const;
  void SetStatus(std::string status);
  std::string GetStatus() const;
  bool PowerOff();
  bool PowerOn();

private:

  bool Connect();
  void Disconnect();

  std::string Address;
  std::string Identifier;
  std::string Status;

  std::map<std::string, std::map<std::string, std::string>> Services;
  typedef std::map<std::string, std::map<std::string, std::string>>::const_iterator service_itr;
  typedef std::map<std::string, std::string>::const_iterator characteristic_itr;

  SimpleBLE::Peripheral& BLEPeripheral;
};

