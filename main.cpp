#include "batterymanage.h"
#include <QApplication>
#include "QDebug"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    BatteryManage w;

    w.move(400,0);
    w.show();

    return a.exec();
}
