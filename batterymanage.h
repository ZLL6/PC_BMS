#ifndef BATTERYMANAGE_H
#define BATTERYMANAGE_H
//
#include <QMainWindow>

#include <QComboBox>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QPushButton>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QDebug>
#include <QTextEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTime>
#include <QTimer>
//#include <string.h>
#include <QInputDialog>
#include <QDesktopServices>
#include <QThread>
#include <QElapsedTimer>
#include <QTextStream>
#include <QTextCodec>
#include "QtXlsx"
#include "xlsxdocument.h"
#include "QSerialPortInfo"
#include "QFontComboBox"
#include "QChart"
#include "QChartView"
#include "QSplineSeries"
#include "QValueAxis"
#include "QDateTimeAxis"
#include "QFrame"
#include "QProgressBar"
#include "QAbstractAxis"
#include "QProcess"
#include "QRegExp"
#include "QPushButton"

#include "uselog.h"
#include "password.h"

namespace Ui {
class BatteryManage;
}
using namespace QXlsx;

QT_CHARTS_USE_NAMESPACE
class BatteryManage : public QMainWindow
{
    Q_OBJECT

public:
    explicit BatteryManage(QWidget *parent = nullptr);
    ~BatteryManage();
    void init_serial();
    void read_BatteryData(void);
    void linK_serial(void);
    void init_QtableWidget();

    void show_warnMessage();                //显示电池告警信息
    void show_singleV();                    //显示单体电压值
    void show_readonlyMessage();            //显示剩下的只读内容
    void parmaterShow();                    //参数设置--显示
    /*用于鼠标点击菜单栏底部弹出页面*/
    bool eventFilter(QObject *watched, QEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void processEvents(QEventLoop::ProcessEventsFlags);
    void closeEvent(QCloseEvent *);
    void dataSummary();                     //数据处理--分数组汇入总数组
    int addr(QString data);                 //数据处理--计算数组下标
    QByteArray toASCII(QString str);        //数据处理--转ASCII
    quint16 calCRC16(QVector<quint8> buff, int len);    //数据处理--计算CRC
    quint8 calNum(QList<QString> buff, int len );
    void listToArray();                     //数据处理--链表转quint8数组


    void init_Chart();
    void Cell_SOC_SOH_init();
    void show_chart(qreal currentC,qreal averC);

    //打包好数据起始地址和数据长度,校验码自动添加
    void pack_sendFrame(quint8 cmd, quint8 addr1, quint8 addr2, quint8 len);
    void delay_ms(int ms);    
    void Pack_sendData(QVector<quint8> sendVector,int len);
    void history_analyise();
    void history_ThresholdVlaueSave();
    void history_ThresholdVlaueRead();
    void history_realDataArrayOtherSave();
    void upgrade_packFrame(quint16 addr, quint8 len, quint16 num);
    uint32_t upgrade_Cal32CRC(QVector<quint32> buff,int len);
    void upgrade_FileRead();
    void send_historyData_Readframe(quint8 cmd, quint16 addr);
signals:
    void his_receFrameSignal();
private slots:
    void serial_read(void);    
    void on_open_Bt_clicked();
    void on_pushButton_clicked();
    void his_writeToExcel();
    //定时器
    void Timer_Init();
    void Timer_UpdatePort();
    void Timer_slot();
    void writeCorrespondData();
    void history_realShow();
    void upgrade_Timerslot();    
    //顶部菜单栏
    void on_menu_exit_aboutToShow();
private:    
    Ui::BatteryManage *ui;    

    int receFrameCnt = 0;                   //写入判断是否回复标志
    quint8 cellNum ;                        //电池节数
    quint8 TempNum;                         //电池温度点数
    quint8 LastFrameNum;
    QLineEdit* timeLineEdit;                //时间
    bool Batfouse = false;
    bool BMSfouse = false;
    QEventLoop loop;                        //阻塞事件对象
    QEventLoop loopWRet;
    //串口   
    QSerialPort *M_port;                    //串口
//    QComboBox *sexcombox_port;
    QTimer* timer_port;
    QStringList port_List;
    QTimer* timer;                          //定时器、升级时关闭
    int timerT;                             //定时器定时时间
    QTimer* timerProcessData;               //用于定时显示（定时从总数组读取）的定时器、升级时关闭
    quint16 flag = 0;
    quint32 count = 0;
    QPalette palette_green;
    QPalette palette_red;
    QPalette palette_gray;

    //状态栏
    QLabel* label;                          //菜单栏底部的label
    QString DateTime = QDate::currentDate().toString("yyyy-MM-dd")
                        +" "+QTime::currentTime().toString()+" ";
    useLog* logWin;                         //日志对象
    //表格
    QFontComboBox* FCP_FontBox;             //飞控协议版本
    QFontComboBox* cell_FontBox;            //电芯类型指针对象
    QFontComboBox* bq_FontBox;              //前段芯片类型指针对象
    QFontComboBox* T_FontBox;               //温度传感器类型指针对象

    //曲线图
    QChart* chart;                          //当前电流和平均电流
    QSplineSeries* Currect_series;          //当前电流的实例对象
    QSplineSeries* Aver_series;             //平均电流的实例对象
    QVector<qreal> axY;
    qreal max = 0;
    qreal min = 0;
    int x=100000;
    int x_value = 0;

    //画电池--SOC-SOH
    QProgressBar* SOC_cellBar;
    QProgressBar* SOH_cellBar;
    QLabel* SOC_LabelNum;
    QLabel* SOH_LabelNum;

    //处理数据包
//    QFile frameFile;
//    QTextStream frameIn;
    QString baseAddr = "2000";              //通讯地址基地址
    QString result;                         //接收的全部数据
    int listLen;                            //帧数据区长度
    QVector<QString> dataVector;            //储存数据区的数组(不包含历史数据查询值)
    int dataSize = 200;                     //数组大小/*************根据需要修改************/
    QVector<quint8> buff;                   //校验所用的缓存数组
    QVector<QString> totalVector;           //总储存数组
    QVector<quint8> sendVector;             //发送问询帧的数组
    QByteArray sendData;                    //发给转接盒的数据
    QVector<QString> hisVector;             //历史数据阈值查询
    bool ok;                                //16进制转10进制参数
    /***********************************************/
    bool frame_flag = false;                //收到完整帧待处理标志
    bool head_flag = false;                 //捕捉到完整帧头标志
    int index= 0;                           //链表角标
    QString frameHead = "55";               //等待接收的字符，每帧首个字符是0x55
    QString aimAddr;                        //目标地址
    QString cmd;                            //读写指令
    QString dataStartAddr;                  //数据起始地址
    QString dataLen;                        //数据长度
    QString crcRx;                          //收到的CRC校验码
    quint16 crcCal = 0xFFFF;                //计算得到的校验码(初始值为0xFFFF)

    /**********参数设置*********/
    int n;                                  //用于总数组的下标
    bool waitFlag = true;
    bool ifwrite = false;
    /**********历史数据*********/
    QStringList colHeadList;                //历史数据列标题链表                                
    QTextStream in;
    //历史数据
    QFile hisfile;
    bool colheadFlag = true;                //用于历史数据列标题只一次，不每次点击追加
    QList<QString> hisList;
    QList<QList<QString>> hisListList;
    int hisDataNum;                    //历史数据返回的总数据总条数
    int hisFrameCnt;
    bool finishFlag;                        //读取完成标志
        //历史阈值读取
    int hisCount = 0;
    //实时数据
    QFile realfile;
    uint saveCycleCnt;                      //用于实时数据存储周期
    bool logFlag = false;                   //用于正在保存的动态刷新
    QTimer* realhisData_timer;              //升级时关闭
    bool hisheadFlag = true;                //用于实时数据列标题只一次，不每次点击追加
    QList<QString> realhead_list;           //实时数据Excel列标题
    //升级    
    QVector<quint8> File_Vector;            //储存文件的每个字节
    QVector<quint8> upSend_Vector;
    bool upgradeFlag = false;    
    QTimer* upTimer;
    quint32 whileCnt = 0;
    int frameNum = 0;
    bool waitAckFlag = false;
    int switchFlag = 1;
    int k = 0;
    bool FileOneFrameTXFlag = false;
    int FileAddrCnt = 0;
    //测试
    QStringList TestList;

private slots:
    void on_pushButton_timeCalib_clicked();
    void on_pushButton_bigCCalib_clicked();
    void on_pushButton_smallCCalib_clicked();
    void on_pushButton_bigVCalib_clicked();
    void on_pushButton_smallVCalib_clicked();
    void on_pushButton_6_clicked();    
    void on_pushButton_4_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_readBarcode_clicked();
    void on_pushButton_balanceCalib_clicked();
    void on_pushButton_savehistory_clicked();
    void on_pushButton_saveReal_clicked();
    void on_pushButton_5_clicked();

    void on_Bt_upgrade_clicked();
    void on_Bt_endUpgrade_clicked();
    void on_pushButton_BMSWrite_clicked();
    //void on_pushButton_7_clicked();
    void on_checkBox_stateChanged(int);
    void on_pushButton_zeroCalib_clicked();
    void on_pushButton_clearForever_clicked();
    void on_buttonBox_clicked(QAbstractButton *button);

    void on_pushButton_write_clicked();
    void on_pushButton_read_clicked();
    void on_tabWidget_currentChanged(int index);    
    void on_pushButton_8_clicked();
    void on_pushButton_9_clicked();
    void on_pushButton_frontChip_clicked();
};

class MyFrame : public QFrame
{
    Q_OBJECT
public:
    MyFrame(QWidget *) {}
protected:
    void paintEvent(QPaintEvent *);
};

#endif // BATTERYMANAGE_H
