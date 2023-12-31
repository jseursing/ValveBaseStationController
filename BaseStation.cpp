#include "BaseStation.h"
#include <QMenu>
#include <QMessageBox>
#include <QMovie>
#include <QSystemTrayIcon>
#include <QTimer>
#pragma comment(lib, "simpleble.lib")

BaseStation* BaseStation::MyInstance = nullptr;

BaseStation* BaseStation::Instance()
{
  return MyInstance;
}

void BaseStation::LHV2AlertCallback(LHV2Mgr::AlertEnum alert, void* pParams)
{
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
  case LHV2Mgr::READY:
    emit BaseStation::Instance()->drawSignal(BaseStation::RUNNING_ID);
    break;
  case LHV2Mgr::VR_ACTIVE:
    emit BaseStation::Instance()->drawSignal(BaseStation::VR_ID);
    break;
  case LHV2Mgr::TERMINATE:
    BaseStation::Instance()->SetStatus("Terminating Base Station(s)");
    emit BaseStation::Instance()->drawSignal(BaseStation::LOAD_ID);
    break;
  case LHV2Mgr::POWER_ON:
    BaseStation::Instance()->SetStatus("Powering on Base Station(s)");
    emit BaseStation::Instance()->drawSignal(BaseStation::LOAD_ID);
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
  StatusTimer->stop();

  switch (drawType)
  {
  case LOAD_ID:
    BaseStation::Instance()->StatusList.clear();
    ui.DisplayLabel->setMovie(ScanningMovie);
    ProcessingMovie->stop();
    ScanningMovie->start();
    break;
  case RUNNING_ID:
    ui.DisplayLabel->setMovie(ProcessingMovie);
    ProcessingMovie->start();
    ScanningMovie->stop();
    processScan();
    StatusTimer->start();
    break;
  case VR_ID:
    SetStatus("SteamVR Active");
    break;
  }
}

void BaseStation::processScan()
{
  // Build status list
  std::vector<LightHouse*> devices = LighthouseV2Mgr->GetLighthouses();

  char tempBuf[512] = { 0 };

  StatusList.clear();
  sprintf_s(tempBuf, "Managing %zd Base Station(s)", devices.size());
  StatusList.push_back(tempBuf);

  for (size_t i = 0; i < devices.size(); ++i)
  {
    sprintf_s(tempBuf,
              "Identifier: %s\nAddress: %s\nStatus: %s\n",
              devices[i]->GetIdentifier().c_str(),
              devices[i]->GetAddress().c_str(),
              devices[i]->GetStatus().c_str());
    StatusList.push_back(tempBuf);
  }
}

void BaseStation::statusTimerSlot()
{
  static uint32_t tick = 0;
  if (0 != StatusList.size())
  {
    SetStatus(StatusList[tick++ % StatusList.size()]);
  }
}

void BaseStation::refreshSlot()
{
  LighthouseV2Mgr->RefreshDevices();
}

void BaseStation::powerOnSlot()
{
  LighthouseV2Mgr->PowerOnDevices();
}

void BaseStation::powerOffSlot()
{
  LighthouseV2Mgr->PowerOffDevices();
}

void BaseStation::closeEvent(QCloseEvent* closeEvent)
{
  QMessageBox::StandardButton resBtn = 
    QMessageBox::question(this, 
                          "Valve Base Station Controller",
                          "Minimize to tray instead?",
                          QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                          QMessageBox::No);
  switch (resBtn)
  {
  case QMessageBox::Yes:
    TrayIcon->setVisible(true);
    closeEvent->ignore();
    hide();
    break;
  case QMessageBox::No:
    closeEvent->accept();
    break;
  case QMessageBox::Cancel:
    closeEvent->ignore();
    break;
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
  setWindowIcon(QIcon(QPixmap(":/new/prefix1/resources/trayicon.png")));

  StatusTimer = new QTimer(this);
  StatusTimer->setSingleShot(false);
  StatusTimer->setInterval(std::chrono::milliseconds(3000));
  connect(StatusTimer, &QTimer::timeout, this, &BaseStation::statusTimerSlot);
  connect(this, &BaseStation::statusSignal, this, &BaseStation::statusSlot);
  connect(this, &BaseStation::drawSignal, this, &BaseStation::drawSlot);

  // Configure Graphical Label and menu
  ui.DisplayLabel->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui.DisplayLabel, &QLabel::customContextMenuRequested, this, 
    [this](const QPoint& pos)
    {
      QMenu menu;
      QAction* action = menu.addAction("Refresh Devices");
      connect(action, &QAction::triggered, this, &BaseStation::refreshSlot);

      action = menu.addAction("Power On Devices");
      connect(action, &QAction::triggered, this, &BaseStation::powerOnSlot);
      
      action = menu.addAction("Power Off Devices");
      connect(action, &QAction::triggered, this, &BaseStation::powerOffSlot);

      menu.exec(mapToGlobal(pos));
    });

  // Configure Tray Icon and menu
  TrayMenu = new QMenu();
  QAction* action = TrayMenu->addAction("Open Display");
  connect(action, &QAction::triggered, this, 
    [this]() 
    {
      TrayIcon->hide();
      show();
    });
  TrayMenu->addSeparator();
  action = TrayMenu->addAction("Refresh Devices");
  connect(action, &QAction::triggered, this, &BaseStation::refreshSlot);
  TrayMenu->addSeparator();
  action = TrayMenu->addAction("Power On Devices");
  connect(action, &QAction::triggered, this, &BaseStation::powerOnSlot);
  action = TrayMenu->addAction("Power Off Devices");
  connect(action, &QAction::triggered, this, &BaseStation::powerOffSlot);
  TrayMenu->addSeparator();
  action = TrayMenu->addAction("Exit");
  connect(action, &QAction::triggered, this, [this](){ exit(0); });
  TrayIcon = new QSystemTrayIcon(QIcon(QPixmap(":/new/prefix1/resources/trayicon.png")));
  TrayIcon->setContextMenu(TrayMenu);

  // Configure GIFs and Lighthouse manager
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
