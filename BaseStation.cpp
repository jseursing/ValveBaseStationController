#include "BaseStation.h"
#include <QMovie>
#include <QTimer>
#pragma comment(lib, "simpleble.lib")

BaseStation* BaseStation::MyInstance = nullptr;

BaseStation* BaseStation::Instance()
{
  return MyInstance;
}

void BaseStation::LHV2AlertCallback(LHV2Mgr::AlertEnum alert, void* pParams)
{
  char* pBuf = new char[256];
  memset(pBuf, 0, 256);

  switch (alert)
  {
  case LHV2Mgr::BT_NOT_ENABLED:
    BaseStation::Instance()->SetStatus("Bluetooth not enabled");
    break;
  case LHV2Mgr::NO_ADAPTERS_FOUND:
    BaseStation::Instance()->SetStatus("No BLE Devices found");
    break;
  case LHV2Mgr::SCANNING:
    BaseStation::Instance()->SetStatus("Scanning for devices");
    emit BaseStation::Instance()->drawSignal(BaseStation::LOAD_ID);
    break;
  case LHV2Mgr::VALIDATING:
    BaseStation::Instance()->SetStatus("Validating characteristics");
    break;
  case LHV2Mgr::READY:
    emit BaseStation::Instance()->drawSignal(BaseStation::RUNNING_ID);
    break;
  case LHV2Mgr::VR_ACTIVE:
    emit BaseStation::Instance()->drawSignal(BaseStation::VR_ID);
    break;
  case LHV2Mgr::TERMINATE:
    BaseStation::Instance()->StatusList.clear();
    BaseStation::Instance()->SetStatus("Terminating Base Station(s)");
    break;
  }
}

void BaseStation::statusSlot(const char* status)
{
  ui.StatusLabel->setText(status);
  delete[] status;
}

void BaseStation::drawSlot(int drawType)
{
  switch (drawType)
  {
  case LOAD_ID:
    ui.DisplayLabel->setMovie(ScanningMovie);
    ProcessingMovie->stop();
    ScanningMovie->start();
    StatusTimer->stop();
    break;
  case RUNNING_ID:
    ui.DisplayLabel->setMovie(ProcessingMovie);
    ProcessingMovie->start();
    ScanningMovie->stop();
    processScan();
    break;
  case VR_ID:
    SetStatus("SteamVR Active");
    StatusTimer->stop();
    break;
  }
}

void BaseStation::processScan()
{
  StatusTimer->stop();

  // Build status list
  std::vector<LightHouse*> devices = LighthouseV2Mgr->GetLighthouses();

  char tempBuf[512] = { 0 };

  StatusList.clear();
  sprintf_s(tempBuf, "Managing %d Base Station(s)", devices.size());
  StatusList.push_back(tempBuf);

  for (size_t i = 0; i < devices.size(); ++i)
  {
    sprintf_s(tempBuf,
              "ID: %s\nADDR: %s\nSTATUS: %s\n",
              devices[i]->GetIdentifier().c_str(),
              devices[i]->GetAddress().c_str(),
              devices[i]->GetStatus().c_str());
    StatusList.push_back(tempBuf);

  }

  StatusTimer->start();
}

void BaseStation::statusTimerSlot()
{
  static uint32_t tick = 0;
  if (0 != StatusList.size())
  {
    emit SetStatus(StatusList[tick++ % StatusList.size()]);
  }
}

void BaseStation::SetStatus(std::string status)
{
  char* pBuf = new char[status.length() + 1];
  memset(pBuf, 0, status.length() + 1);
  memcpy(pBuf, status.c_str(), status.length());

  emit statusSignal(pBuf);
}

BaseStation::BaseStation(QWidget *parent) : 
  QMainWindow(parent)
{
  ui.setupUi(this);
  MyInstance = this;

  StatusTimer = new QTimer(this);
  StatusTimer->setSingleShot(false);
  StatusTimer->setInterval(std::chrono::milliseconds(3000));
  connect(StatusTimer, &QTimer::timeout, this, &BaseStation::statusTimerSlot);
  connect(this, &BaseStation::statusSignal, this, &BaseStation::statusSlot);
  connect(this, &BaseStation::drawSignal, this, &BaseStation::drawSlot);

  ProcessingMovie = new QMovie(":/new/prefix1/resources/processing.gif");
  ScanningMovie = new QMovie(":/new/prefix1/resources/loading.gif");
  LighthouseV2Mgr = LHV2Mgr::Create(LHV2AlertCallback);
  LighthouseV2Mgr->RefreshDevices();
}

BaseStation::~BaseStation()
{
  delete ScanningMovie;
  delete ProcessingMovie;
}
