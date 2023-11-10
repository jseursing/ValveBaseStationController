#include "LightHouse.h"
#include <simpleble/SimpleBLE.h>
#include <Windows.h>

const char* LightHouse::LIGHTHOUSE_IDENTIFIER = "LHB-";
const char* LightHouse::SVC_UUID = "00001523-1212-efde-1523-785feabcd124";
const char* LightHouse::CHARACTERISTIC_UUID = "00001525-1212-efde-1523-785feabcd124";

LightHouse::LightHouse(std::string address,
                       std::string identifier,
                       SimpleBLE::Peripheral* peripheral) :
  Address(address),
  Identifier(identifier),
  BLEPeripheral(*peripheral)
{
}

LightHouse::~LightHouse()
{
}


std::string LightHouse::GetAddress() const
{
  return Address;
}

std::string LightHouse::GetIdentifier() const
{
  return Identifier;
}

void LightHouse::AddCharacteristic(std::string service, std::string characteristic)
{
  Characteristic c;
  c.uuid  = characteristic;
  c.value = "";

  if (Services.end() == Services.find(service))
  {
    Services[service] = std::vector<Characteristic>{};
  }

  Services[service].push_back(c);
}

std::string LightHouse::GetValue(std::string service, std::string characteristic)
{
  std::string value = "";
  if (Services.end() != Services.find(service))
  {
    for (Characteristic c : Services[service])
    {
      if (c.uuid == characteristic)
      {
        value = c.value;
        break;
      }
    }
  }

  return value;
}

bool LightHouse::WriteCharacteristic(std::string service, 
                                     std::string characteristic, 
                                     std::string value)
{
  // First attempt to connect
  if (true == Connect())
  {
    BLEPeripheral.write_request(service, characteristic, value);
    Disconnect();

    return true;
  }

  return false;
}

bool LightHouse::ReadCharacteristics()
{
  if (true == Connect())
  {
    // Retrieve services/characteristics if we haven't done so
    if (true == Services.empty())
    {
      for (SimpleBLE::Service& s : BLEPeripheral.services())
      {
        for (SimpleBLE::Characteristic& c : s.characteristics())
        {
          AddCharacteristic(s.uuid(), c.uuid());
        }
      }
    }

    // Retrieve values of characteristics
    std::string device = Identifier + "\n";
    OutputDebugStringA(device.c_str());
    for (std::map<std::string, std::vector<Characteristic>>::iterator itr = Services.begin();
         itr != Services.end();
         ++itr)
    {
      for (size_t i = 0; i < itr->second.size(); ++i)
      {
        std::string debugStr = itr->first + " " + itr->second[i].uuid;
        
        // If we attempt to read something invalid, SimpleBLE throws an exception...
        try
        {
          itr->second[i].value = BLEPeripheral.read(itr->first, itr->second[i].uuid);
          debugStr += " " + itr->second[i].value + "\n";
        }
        catch (...)
        {
          debugStr += " ERROR\n";
        }
        OutputDebugStringA(debugStr.c_str());

        if ((SVC_UUID == itr->first) &&
            (CHARACTERISTIC_UUID == itr->second[i].uuid))
        {
          std::string data = itr->second[i].value;
          if (data.size())
          {
            switch (data[0])
            {
            case 0:
              Status = "OFF (0x00)";
              break;
            default:
              Status = "ON (" + std::to_string(data[0]) + ")";
              break;
            }
          }
          else
          {
            Status = "ERROR";
          }
        }
      }
    }

    Disconnect();

    return true;
  }

  return false;
}

bool LightHouse::IsValidLighthouse() const
{
  std::map<std::string, std::vector<Characteristic>>::const_iterator itr = Services.find(SVC_UUID);
  if (Services.end() != itr)
  {
    for (Characteristic c : itr->second)
    {
      if (CHARACTERISTIC_UUID == c.uuid)
      {
        return true;
      }
    }
  }

  return false;
}

void LightHouse::SetStatus(std::string status)
{
  Status = status;
}

std::string LightHouse::GetStatus() const
{
  return Status;
}

bool LightHouse::PowerOff()
{
  if (true == WriteCharacteristic(LightHouse::SVC_UUID,
                                  LightHouse::CHARACTERISTIC_UUID,
                                  std::string(1, LightHouse::PWR_OFF)))
  {
    if (true == ReadCharacteristics())
    {
      if (std::string::npos != Status.find("OFF"))
      {
        return true;
      }
    }
  }

  return false;
}

bool LightHouse::PowerOn()
{
  if (true == WriteCharacteristic(LightHouse::SVC_UUID,
                                  LightHouse::CHARACTERISTIC_UUID,
                                  std::string(1, LightHouse::PWR_ON)))
  {
    if (true == ReadCharacteristics())
    {
      if (std::string::npos != Status.find("ON"))
      {
        return true;
      }
    }
  }

  return false;
}

bool LightHouse::Connect()
{
  try
  {
    if (false == BLEPeripheral.is_connected())
    {
      BLEPeripheral.connect();
    }
  }
  catch (...)
  {
    OutputDebugStringA("Exception thrown while connecting\n");
  }

  return BLEPeripheral.is_connected();
}

void LightHouse::Disconnect()
{
  try
  {
    if (true == BLEPeripheral.is_connected())
    {
      BLEPeripheral.disconnect();
    }
  }
  catch (...)
  {
    OutputDebugStringA("Exception thrown while disconnecting\n");
  }
}