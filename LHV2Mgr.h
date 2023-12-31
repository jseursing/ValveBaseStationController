#pragma once
#include "LightHouse.h"
#include <simpleble/Adapter.h>
#include <simpleble/Peripheral.h>
#include <vector>

class LHV2Mgr
{
public:

  // Simple GUI notification callback
  enum AlertEnum
  {
    BT_NOT_ENABLED,
    NO_ADAPTERS_FOUND,
    SCANNING,
    READY,
    STATUS,
    VR_ACTIVE,
    POWER_ON,
    TERMINATE
  };
  typedef void(*AlertCallback)(const AlertEnum alert, void* pDetails);

  static LHV2Mgr* Create(AlertCallback cb);
  static void Destroy(LHV2Mgr* instance);
  void RefreshDevices();
  std::vector<LightHouse*> GetLighthouses();
  void PowerOnDevices();
  void PowerOffDevices();

private:

  static void DeviceScanLoop(LHV2Mgr* instance);
  static bool IsValveVRActive();

  LHV2Mgr(AlertCallback cb);
  ~LHV2Mgr();

  enum DiscoveryStateEnum
  {
    IDLE,
    SCAN,
    VALIDATE,
    PROCESSING,
    TERMINATING,
    POWERING_ON
  };

  DiscoveryStateEnum DiscState;
  AlertCallback _AlertCallback;

  size_t ActiveAdapter;
  std::vector<SimpleBLE::Adapter> Adapters;
  std::vector<SimpleBLE::Peripheral> Peripherals;
  std::vector<LightHouse*> Lighthouses;
  bool TransitionToScan;
};

