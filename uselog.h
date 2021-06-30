#ifndef USELOG_H
#define USELOG_H

#include <QWidget>
#include "QTime"
#include "QStringListModel"
#include "QTimer"
#include "QFile"
#include "QPointF"
#include "QTextStream"
#include "QChart"
#include "QChartView"
#include "QSplineSeries"

namespace Ui {
class useLog;
}
QT_CHARTS_USE_NAMESPACE

class useLog : public QWidget
{
    Q_OBJECT

public:
    explicit useLog(QWidget *parent = nullptr);
    ~useLog();
    void addListData(QString data);
    void clear_data();
    bool flag = false;
private slots:
    void on_pushButton_clicked();
private:
    Ui::useLog *ui;
    QStringList list;
    QTimer* timer;
    QStringListModel* model;


    int i=0;

};

#endif // USELOG_H
