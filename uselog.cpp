#include "uselog.h"
#include "ui_uselog.h"
#include "QDebug"

useLog::useLog(QWidget *parent):
    QWidget (parent),
    ui(new Ui::useLog)
{
    ui->setupUi(this);
    model = new QStringListModel();
    timer = new QTimer(this);
    connect(timer,&QTimer::timeout,this,&useLog::clear_data);
    timer->start(50);    
}

useLog::~useLog()
{
    delete ui;
}

void useLog::addListData(QString data)
{
    if(flag)
    {
        list << QString::number(i++) + " "+QDate::currentDate().toString("yyyy-MM-dd") +" " + QTime::currentTime().toString() + " " + data;
        model->setStringList(list);
        ui->listView->setModel(model);
    }
}
void useLog::clear_data()
{
    if(list.size() > 100 )
    {
        list.clear();
        model->setStringList(list);
        for(int i=model->rowCount();i>=0;--i)
        {
            model->removeRow(i);
        }
        ui->listView->setModel(model);
    }

}
void useLog::on_pushButton_clicked()
{
    list.clear();
    model->setStringList(list);
    for(int i=model->rowCount();i>=0;--i)
    {
        model->removeRow(i);
    }
    ui->listView->setModel(model);
}

