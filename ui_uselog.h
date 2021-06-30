/********************************************************************************
** Form generated from reading UI file 'uselog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.5
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_USELOG_H
#define UI_USELOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QListView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_useLog
{
public:
    QGridLayout *gridLayout;
    QVBoxLayout *verticalLayout_2;
    QVBoxLayout *verticalLayout;
    QListView *listView;
    QPushButton *pushButton;

    void setupUi(QWidget *useLog)
    {
        if (useLog->objectName().isEmpty())
            useLog->setObjectName(QString::fromUtf8("useLog"));
        useLog->resize(700, 750);
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(useLog->sizePolicy().hasHeightForWidth());
        useLog->setSizePolicy(sizePolicy);
        useLog->setMinimumSize(QSize(700, 750));
        useLog->setMaximumSize(QSize(700, 750));
        QFont font;
        font.setPointSize(11);
        useLog->setFont(font);
        gridLayout = new QGridLayout(useLog);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        listView = new QListView(useLog);
        listView->setObjectName(QString::fromUtf8("listView"));
        listView->setFont(font);
        listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

        verticalLayout->addWidget(listView);

        pushButton = new QPushButton(useLog);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(pushButton->sizePolicy().hasHeightForWidth());
        pushButton->setSizePolicy(sizePolicy1);
        pushButton->setMinimumSize(QSize(0, 30));
        pushButton->setMaximumSize(QSize(16777215, 30));
        pushButton->setFont(font);

        verticalLayout->addWidget(pushButton);


        verticalLayout_2->addLayout(verticalLayout);


        gridLayout->addLayout(verticalLayout_2, 0, 0, 1, 1);


        retranslateUi(useLog);

        QMetaObject::connectSlotsByName(useLog);
    } // setupUi

    void retranslateUi(QWidget *useLog)
    {
        useLog->setWindowTitle(QApplication::translate("useLog", "\345\267\245\344\275\234\346\227\245\345\277\227\346\237\245\347\234\213\345\231\250", nullptr));
        pushButton->setText(QApplication::translate("useLog", "\346\270\205\351\231\244", nullptr));
    } // retranslateUi

};

namespace Ui {
    class useLog: public Ui_useLog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_USELOG_H
