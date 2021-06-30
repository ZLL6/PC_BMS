#ifndef PASSWORD_H
#define PASSWORD_H

#include <QWidget>

namespace Ui {
class password;
}

class password : public QWidget
{
    Q_OBJECT

public:
    explicit password(QWidget *parent = nullptr);
    ~password();
    void show_totalVector(QVector<QString> totalVector);

private slots:
    void on_pushButton_clicked();

private:
    Ui::password *ui;
};

#endif // PASSWORD_H
