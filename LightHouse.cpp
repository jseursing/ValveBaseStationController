#include "LightHouse.h"
#include <simpleble/SimpleBLE.h>
#include <Windows.h>

const char* LightHouse::LIGHTHOUSE_ID = "LHB-";
const char* LightHouse::PWR_SVC_UUID  = "00001523-1212-efde-1523-785feabcd124";
const char* LightHouse::PWR_CHAR_UUID = "00001525-1212-efde-1523-785feabcd124";

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
  Services[service][characteristic] = "";
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

std::string LightHouse::ReadCharacteristic(std::string service, 
                                           std::string characteristic)
{
  std::string value = "";

  service_itr s_itr = Services.find(service);
  if (Services.end() != s_itr)
  {
    characteristic_itr c_itr = s_itr->second.find(characteristic);
    if (s_itr->second.end() != c_itr)
    {
      if (true == Connect())
      {
        Services[s_itr->first][c_itr->first] = BLEPeripheral.read(s_itr->first, c_itr->first);
        Disconnect();
      }
      
      value = c_itr->second;
    }
  }

  return value;
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
    std::string debugStr = "Parsing " + Identifier + "\n";
    OutputDebugStringA(debugStr.c_str());

    for (service_itr s_itr = Services.begin(); s_itr != Services.end(); ++s_itr)
    {
      for (characteristic_itr c_itr = s_itr->second.begin(); 
           c_itr != s_itr->second.end();
           ++c_itr)
      {
        debugStr = s_itr->first + " " + c_itr->first + " = ";
        
        // If we attempt to read something invalid, SimpleBLE throws an exception...
        try
        {
          Services[s_itr->first][c_itr->first] = BLEPeripheral.read(s_itr->first, c_itr->first);
          debugStr += Services[s_itr->first][c_itr->first] + "\n";
        }
        catch (...)
        {
          debugStr += "ERROR\n";
        }
        OutputDebugStringA(debugStr.c_str());

        if ((PWR_SVC_UUID == s_itr->first) &&
            (PWR_CHAR_UUID == c_itr->first))
        {
          std::string data = c_itr->second;
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
  service_itr s_itr = Services.find(PWR_SVC_UUID);
  if (Services.end() != s_itr)
  {
    characteristic_itr c_itr = s_itr->second.find(PWR_CHAR_UUID);
    return (c_itr != s_itr->second.end());
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
  if (true == WriteCharacteristic(LightHouse::PWR_SVC_UUID,
                                  LightHouse::PWR_CHAR_UUID,
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
  if (true == WriteCharacteristic(LightHouse::PWR_SVC_UUID,
                                  LightHouse::PWR_CHAR_UUID,
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