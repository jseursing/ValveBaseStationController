#pragma once

#include <QCloseEvent>
#include <QMainWindow>
#include "ui_BaseStation.h"
#include "LHV2Mgr.h"

class QMenu;
class QMovie;
class QSystemTrayIcon;
class QTimer;

class BaseStation : public QMainWindow
{
  Q_OBJECT

public:
  enum DrawEnum
  {
    LOAD_ID,
    RUNNING_ID,
    VR_ID
  };

  static BaseStation* Instance();

  void SetStatus(std::string status);
  BaseStation(QWidget *parent = nullptr);
  ~BaseStation();

signals:
  void statusSignal(const char* status);
  void drawSignal(int drawType);
  void processScanSignal();

public slots:
  void statusSlot(const char* status);
  void drawSlot(int drawType);
  void statusTimerSlot();
  void refreshSlot();
  void powerOnSlot();
  void powerOffSlot();

private:

  void closeEvent(QCloseEvent* closeEvent) override;
  void processScan();
  static void LHV2AlertCallback(LHV2Mgr::AlertEnum alert, void* pParams);

  Ui::BaseStationClass ui;
  static BaseStation* MyInstance;
  LHV2Mgr* LighthouseV2Mgr;
  QMovie* ScanningMovie;
  QMovie* ProcessingMovie;
  QTimer* StatusTimer;
  QSystemTrayIcon* TrayIcon;
  QMenu* TrayMenu;
  std::vector<std::string> StatusList;
};
