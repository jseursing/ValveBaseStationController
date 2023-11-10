#include "AsyncMgr.h"
#include "LHV2Mgr.h"
#include <cassert>
#include <simpleble/SimpleBLE.h>
#include <Windows.h>
#include <TlHelp32.h>



LHV2Mgr* LHV2Mgr::Create(AlertCallback cb)
{
  LHV2Mgr* instance = new LHV2Mgr(cb);
  return instance;
}

void LHV2Mgr::Destroy(LHV2Mgr* instance)
{

}

void LHV2Mgr::RefreshDevices()
{
  if (IDLE == DiscState)
  {
    DiscState = SCAN;
  }
  else if (PROCESSING == DiscState)
  {
    TransitionToScan = true;
  }
}

std::vector<LightHouse*> LHV2Mgr::GetLighthouses()
{
  return Lighthouses;
}

void LHV2Mgr::PowerOnDevices()
{
  if (PROCESSING == DiscState)
  {
    DiscState = POWERING_ON;
  }
}

void LHV2Mgr::DeviceScanLoop(LHV2Mgr* instance)
{
  uint32_t shutoff_tick = 0;

  assert(nullptr != instance);

  for (;; std::this_thread::sleep_for(std::chrono::milliseconds(1000)))
  {
    switch (instance->DiscState)
    {
      case IDLE:
      {
        // Do nothing...
      }
      break;
      case SCAN:
      {
        instance->_AlertCallback(SCANNING, nullptr);
        instance->Adapters[instance->ActiveAdapter].scan_for(10000);
        instance->Peripherals = instance->Adapters[instance->ActiveAdapter].scan_get_results();

        // Scan for matching lighthouse adapter name
        for (size_t i = 0; i < instance->Peripherals.size(); ++i)
        {
          if (std::string::npos !=
              instance->Peripherals[i].identifier().find(LightHouse::LIGHTHOUSE_IDENTIFIER))
          {
            LightHouse* lighthouse = new LightHouse(instance->Peripherals[i].address(),
                                                    instance->Peripherals[i].identifier(),
                                                    &(instance->Peripherals[i]));
            instance->Lighthouses.push_back(lighthouse);
          }
        }

        if (0 == instance->Lighthouses.size())
        {
          instance->_AlertCallback(READY, nullptr);
          instance->DiscState = IDLE;
          break;
        }

        bool valid = false;
        for (size_t i = 0; i < instance->Lighthouses.size(); ++i)
        {
          instance->Lighthouses[i]->ReadCharacteristics();
          if (true == instance->Lighthouses[i]->IsValidLighthouse())
          {
            valid = true;
            break;
          }
        }

        if (false == valid)
        {
          instance->Lighthouses.clear();
          instance->DiscState = IDLE;
        }
        else
        {
          instance->DiscState = PROCESSING;
        }

        instance->_AlertCallback(READY, nullptr);
      }
      break;
      case PROCESSING:
      {
        if (true == IsValveVRActive())
        {
          shutoff_tick = 0;
          instance->_AlertCallback(VR_ACTIVE, nullptr);
          break;
        }

        for (size_t i = 0; i < instance->Lighthouses.size(); ++i)
        {
          if (true == instance->Lighthouses[i]->ReadCharacteristics())
          {
            std::string status = instance->Lighthouses[i]->GetStatus();
            if (std::string::npos != status.find("ON"))
            {
              ++shutoff_tick;
            }
          }
        }

        if (instance->Lighthouses.size() < shutoff_tick)
        {
          instance->_AlertCallback(TERMINATE, nullptr);
          instance->DiscState = TERMINATING;
          shutoff_tick = 0;
          break;
        }

        if (true == instance->TransitionToScan)
        {
          instance->TransitionToScan = false;
          instance->DiscState = SCAN;
          break;
        }

        instance->_AlertCallback(READY, nullptr);
      }
      break;
      case TERMINATING:
      {
        instance->DiscState = PROCESSING;
        for (size_t i = 0; i < instance->Lighthouses.size(); ++i)
        {
          instance->Lighthouses[i]->PowerOff();
        }
      }
      break;
      case POWERING_ON:
      {
        instance->DiscState = PROCESSING;
        instance->_AlertCallback(POWER_ON, nullptr);
        for (size_t i = 0; i < instance->Lighthouses.size(); ++i)
        {
          instance->Lighthouses[i]->PowerOn();
        }
      }
      break;
      default:
      assert(false);
      break;
    }
  }
}

bool LHV2Mgr::IsValveVRActive()
{
  bool active = false;

  HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (INVALID_HANDLE_VALUE != hSnap)
  {
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hSnap, &pe32))
    {
      do
      {
        char wtoC[256] = { 0 };
        for (size_t i = 0; i < lstrlen(pe32.szExeFile); ++i)
        {
          wtoC[i] = pe32.szExeFile[i] & 0xFF;
        }

        if (std::string::npos != std::string(wtoC).find("vrmonitor.exe"))
        {
          active = true;
          break;
        }
      } while (Process32Next(hSnap, &pe32));
    }

    CloseHandle(hSnap);
  }

  return active;
}

LHV2Mgr::LHV2Mgr(AlertCallback cb) :
  DiscState(IDLE),
  ActiveAdapter(0),
  _AlertCallback(cb),
  TransitionToScan(false)
{
  assert(nullptr != _AlertCallback);

  if (false == SimpleBLE::Adapter::bluetooth_enabled())
  {
    _AlertCallback(BT_NOT_ENABLED, nullptr);
    return;
  }

  Adapters = SimpleBLE::Adapter::get_adapters();
  if (0 == Adapters.size())
  {
    _AlertCallback(NO_ADAPTERS_FOUND, nullptr);
    return;
  }

  AsyncMgr::Instance()->Spawn(DeviceScanLoop, this);
}

LHV2Mgr::~LHV2Mgr()
{

}