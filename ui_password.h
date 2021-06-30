/********************************************************************************
** Form generated from reading UI file 'password.ui'
**
** Created by: Qt User Interface Compiler version 5.12.5
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PASSWORD_H
#define UI_PASSWORD_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_password
{
public:
    QGridLayout *gridLayout;
    QVBoxLayout *verticalLayout;
    QPlainTextEdit *plainTextEdit;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *pushButton;

    void setupUi(QWidget *password)
    {
        if (password->objectName().isEmpty())
            password->setObjectName(QString::fromUtf8("password"));
        password->resize(762, 655);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(password->sizePolicy().hasHeightForWidth());
        password->setSizePolicy(sizePolicy);
        password->setMinimumSize(QSize(0, 0));
        password->setMaximumSize(QSize(16777215, 16777215));
        gridLayout = new QGridLayout(password);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        plainTextEdit = new QPlainTextEdit(password);
        plainTextEdit->setObjectName(QString::fromUtf8("plainTextEdit"));

        verticalLayout->addWidget(plainTextEdit);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        pushButton = new QPushButton(password);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));

        horizontalLayout->addWidget(pushButton);


        verticalLayout->addLayout(horizontalLayout);


        gridLayout->addLayout(verticalLayout, 0, 0, 1, 1);


        retranslateUi(password);

        QMetaObject::connectSlotsByName(password);
    } // setupUi

    void retranslateUi(QWidget *password)
    {
        password->setWindowTitle(QApplication::translate("password", "Form", nullptr));
        pushButton->setText(QApplication::translate("password", "\346\270\205\351\231\244", nullptr));
    } // retranslateUi

};

namespace Ui {
    class password: public Ui_password {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PASSWORD_H
