#include "password.h"
#include "ui_password.h"
#include "QMessageBox"

password::password(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::password)
{
    ui->setupUi(this);
}

password::~password()
{
    delete ui;
}

void password::show_totalVector(QVector<QString> totalVector)
{    
    for(int i=0;i<totalVector.size();i++)
    {
        ui->plainTextEdit->appendPlainText("totalVector[ "+QString("%1").arg(i+8192,4,16,QLatin1Char('0'))+" ]:"+totalVector.at(i));
    }
}
void password::on_pushButton_clicked()
{
    ui->plainTextEdit->clear();
}
