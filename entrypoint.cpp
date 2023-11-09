#include "BaseStation.h"
#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
  QApplication a(argc, argv);
  BaseStation w;
  w.show();
  return a.exec();
}
