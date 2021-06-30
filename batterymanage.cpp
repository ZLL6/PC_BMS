#include "batterymanage.h"
#include "ui_batterymanage.h"
#include "QPainter"
static bool ENGLISH;
static int TabIndex = 0;
BatteryManage::BatteryManage(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::BatteryManage)
{    
    ui->setupUi(this);
    //初始化串口对象
    M_port = new QSerialPort();
    /*读串口数据*/    
    connect(M_port,&QSerialPort::readyRead,this,&BatteryManage::serial_read);
//    frameFile.setFileName("frame.txt");
//    if(frameFile.open(QIODevice::WriteOnly | QIODevice::Text))
//    {
//        frameIn.setDevice(&frameFile);
//    }
    init_serial();
    /*定时器的初始化*/
    Timer_Init();
    /*状态栏日志*/
    logWin = new useLog();                         //指向日志查看器
    //logWin->addListData("已就绪");
    /***********************底部状态栏label**********************************
    label->setText();失效不打印情况
    解决：bool BatteryManage::eventFilter(QObject *watched, QEvent *event)
            return false!
    ***********************************************************************/
    label = new QLabel(statusBar());
    label->setMinimumSize(geometry().width(),22);
    label->setText(DateTime+" 已就绪");
    QFont font;
    font.setPointSize(11);
    label->setFont(font);
    label->installEventFilter(this);  //下载事件过滤器    
    //条形枪--键盘
    ui->lineEdit_tiaoxingma->installEventFilter(this);
    ui->lineEdit_BMSCode->installEventFilter(this);
    //表格初始化
    init_QtableWidget();
    //通讯状态指示灯圆角设置
    palette_red = ui->label_flag->palette();
    palette_green = ui->label_flag->palette();
    palette_gray = ui->label_flag->palette();
    palette_green.setColor(QPalette::Background,Qt::green);
    palette_red.setColor(QPalette::Background,Qt::red);
    palette_gray.setColor(QPalette::Background,Qt::gray);
    ui->label_flag->setAutoFillBackground(true);
    ui->label_flag->setPalette(palette_gray);
    /**************串口发送**************/
    //数据处理    
    dataVector.resize(dataSize);        //临时数组
    buff.resize(dataSize);              //储存校验的数据
    totalVector.resize(350);            //0x2000~;
    sendVector.resize(300);             //发送问询帧的数组(包括帧尾)
    hisVector.resize(200);              //历史数据阈值
    sendVector[0] = 0x55;
    sendVector[1] = 0xAA;
    sendVector[2] = 0x00;
    sendVector[3] = 0x00;
    sendVector[4] = 0xAA;
    sendVector[5] = 0x55;
    sendVector[6] = 0x01;               //读写命令

    //升级
    upSend_Vector.resize(150);
    upSend_Vector[0] = 0x55;
    upSend_Vector[1] = 0xAA;
    upSend_Vector[2] = 0x00;
    upSend_Vector[3] = 0x00;
    upSend_Vector[4] = 0xAA;
    upSend_Vector[5] = 0x55;
    upSend_Vector[6] = 0x01;            //写入
    //主题
    QPalette pal = window()->palette();
    pal.setColor(QPalette::Window,qRgb(218,252,252));
    pal.setColor(QPalette::WindowText,QRgb(0x404044));
    QPixmap pixmap(":/image/6.png");
    pal.setBrush(QPalette::Background,QBrush(pixmap));
    window()->setPalette(pal);
    //时间
    timeLineEdit = new QLineEdit();
    timeLineEdit->setFixedSize(400,22);
    timeLineEdit->setFont(QFont("楷体",14,3));
    timeLineEdit->setFrame(false);
    ui->tabWidget->setCornerWidget(timeLineEdit,Qt::TopRightCorner);
    //画电池
    Cell_SOC_SOH_init();
    //曲线图
    init_Chart();    
    ENGLISH = false;
}

BatteryManage::~BatteryManage()
{
    delete ui;
}
//初始化
void BatteryManage::Timer_Init()
{
    /*串口更新*/
    timer_port = new QTimer(this);
    connect(timer_port,&QTimer::timeout,this,&BatteryManage::Timer_UpdatePort);
    timer_port->start(1000);
    /*实时数据发送指令、实时数据显示*/
    timer = new QTimer(this);
    timerProcessData = new QTimer(this);
    timerT = 200;
    connect(timer,&QTimer::timeout,this,&BatteryManage::Timer_slot);
    connect(timerProcessData,&QTimer::timeout,this,&BatteryManage::writeCorrespondData);
    /*历史数据*/
    realhisData_timer = new QTimer(this);
    connect(realhisData_timer,&QTimer::timeout,this,&BatteryManage::history_realShow);
    /*升级*/
    upTimer = new QTimer(this);
    connect(upTimer,&QTimer::timeout,this,&BatteryManage::upgrade_Timerslot);

}
//日志--事件过滤器
bool BatteryManage::eventFilter(QObject *watched, QEvent *event)
{
    if(watched==label)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            if(logWin->isWindow())
            {
                /*模态，父窗口不可操作状态*/
                logWin->setAttribute(Qt::WA_ShowModal,true);
                logWin->setWindowModality(Qt::ApplicationModal);
                logWin->show();
                label->setText(DateTime+"进入日志查看器");
                //logWin->addListData("进入日志查看器");
            }
        }
    }
    if(watched == ui->lineEdit_tiaoxingma)
    {
        Batfouse = true;
        BMSfouse = false;
    }
    if(watched == ui->lineEdit_BMSCode)
    {
        BMSfouse = true;
        Batfouse = false;
    }
    if(watched != ui->lineEdit_BMSCode && watched != ui->lineEdit_tiaoxingma)
    {
        BMSfouse = false;
        Batfouse = false;
    }
    return false;
}
//条形码--键盘事件
void BatteryManage::keyPressEvent(QKeyEvent *event)
{
    ui->label_tiaomahint->clear();
    if(event->key() == Qt::Key_Return)
    {
        if(timer->isActive())
            timer->stop();
        delay_ms(200);
        if(TabIndex == 3)
        {            
            if(Batfouse && !BMSfouse)
            {
                Batfouse = false;
                int k=6;
                sendVector[k++] = 0x01;
                sendVector[k++] = 0x8F;
                sendVector[k++] = 0x20;
                sendVector[k++] = 0x10;
                QByteArray BatCode = ui->lineEdit_tiaoxingma->text().toLatin1();
                if(BatCode.size() > 16)
                {                    
                    ui->label_tiaomahint->setText("条形码最长不能超过16位");
                    return ;
                }
                for(int i=0;i<BatCode.size();i++)
                {
                    sendVector[k++] = static_cast<quint8>(BatCode.at(i));
                }
                if(BatCode.size() < 16)
                {
                    for(int i=0;i<16-BatCode.size();i++)
                    {
                        sendVector[k++] = 0x00;
                    }
                }
                if(BatCode != nullptr)
                {
                    Pack_sendData(sendVector,26);
                    dataStartAddr.clear();
                    delay_ms(500);
                    if(dataStartAddr == "208F")
                    {
                        ui->label_tiaomahint->setText("PACK条码数据写入成功");
                    }else{
                        ui->label_tiaomahint->setText("PACK条码数据写入失败");
                    }
                    ui->lineEdit_tiaoxingma->clear();
                }

            }
            if(BMSfouse && !Batfouse)
            {               
                BMSfouse = false;
                k=6;
                sendVector[k++] = 0x01;
                sendVector[k++] = 0xA1;
                sendVector[k++] = 0x20;
                sendVector[k++] = 0x10;
                QByteArray BMSCode = ui->lineEdit_BMSCode->text().toLatin1();
                if(BMSCode.size() > 16)
                {
                    ui->label_tiaomahint->setText("条形码最长不能超过16位");
                    return ;
                }
                for(int i=0;i<BMSCode.size();i++)
                {
                    sendVector[k++] = static_cast<quint8>(BMSCode.at(i));
                }
                if(BMSCode.size() < 16)
                {
                    for(int i=0;i<16-BMSCode.size();i++)
                    {
                        sendVector[k++] = 0x00;
                    }
                }
                if(BMSCode != nullptr)
                {
                    Pack_sendData(sendVector,26);
                    dataStartAddr.clear();
                    delay_ms(500);
                    if(dataStartAddr == "20A1")
                    {
                        ui->label_tiaomahint->setText("BMS条形码数据写入成功");
                    }else{
                        ui->label_tiaomahint->setText("BMS条形码数据写入失败");
                    }
                    ui->lineEdit_BMSCode->clear();
                }


            }
        }       
    }
    return ;
}
//串口--关闭串口
void BatteryManage::closeEvent(QCloseEvent *)
{
    if(M_port->isOpen())
    {
        M_port->close();
    }
    if(hisfile.isOpen())
    {
        hisfile.close();
    }
    if(realfile.isOpen())
    {
        realfile.close();
    }
    BatteryManage::close();
}
//电池--绘图事件
void MyFrame::paintEvent(QPaintEvent *)
{    
    //QFrame::paintEvent(event);//先调用父类的paintEvent为了显示背景
    QPainter SOCpainter(this);
    QPen SOCpen;
    SOCpen.setWidth(5);
    SOCpainter.setPen(SOCpen);
    SOCpainter.drawRect(51,10,28,8);
    SOCpainter.drawLine(51,14,79,14);
    SOCpainter.drawRoundedRect(15,18,100,130,20,15);
    SOCpainter.end();    

    QPainter SOHpainter(this);
    QPen SOHpen;
    SOHpen.setWidth(5);
    SOHpainter.setPen(SOHpen);
    SOHpainter.drawRect(51+120,10,28,8);
    SOHpainter.drawLine(51+120,14,79+120,14);
    SOHpainter.drawRoundedRect(15+120,18,100,130,20,15);
    SOHpainter.end();
}
//曲线图--初始化
void BatteryManage::init_Chart()
{    
    //初始化QChart实例
    chart = new QChart();
    chart->setTheme(QChart::ChartThemeBrownSand);
    //初始化两个QSplineSeries实例
    Currect_series = new QSplineSeries(this);
    Aver_series = new QSplineSeries(this);
    //设置两条曲线名称与颜色
    QPen currectPen(Qt::yellow,2);
    QPen averPen(Qt::red,2);
    Currect_series->setName("当前电流值");
    Aver_series->setName("平均电流值");
    Currect_series->setPen(currectPen);
    Aver_series->setPen(averPen);
    //把曲线添加到chart中
    chart->addSeries(Currect_series);
    chart->addSeries(Aver_series);

    //声明并初始化X,Y轴
    QValueAxis* axisX = new QValueAxis();
    QValueAxis* axisY = new QValueAxis();
    //设置坐标轴显示范围
    axisX->setMin(0);
    axisX->setMax(50);
    axisY->setMin(-100.0);
    axisY->setMax(200.0);
    //设置坐标轴上的格点
    axisY->setTickCount(9);
    axisX->setTickCount(50);
    //设置坐标轴显示的名称
    //axisX->setTitleText("时间轴");
    axisY->setTitleText("电流值");
    //设置坐标轴的颜色，粗细，设置网格不显示
    axisY->setGridLineVisible(true);
    axisX->setGridLineVisible(true);
    QPen pen(Qt::darkGray,1,Qt::SolidLine,Qt::RoundCap);
    axisY->setLinePen(pen);
    //axisX->setLinePen(pen);
    //axisX->setLabelFormat("%.1f");
    axisY->setLabelFormat("%.1f");

//    chart->createDefaultAxes();

    //把坐标轴添加到chart
    chart->addAxis(axisX,Qt::AlignBottom);
    chart->addAxis(axisY,Qt::AlignLeft);

    //将曲线关联到坐标轴
    Currect_series->attachAxis(axisX);
    Currect_series->attachAxis(axisY);
    Aver_series->attachAxis(axisX);
    Aver_series->attachAxis(axisY);

    //把chart显示到窗口上
    ui->graphicsView->setChart(chart);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
}
//电池--进度条
void BatteryManage::Cell_SOC_SOH_init()
{
    //画进度条
    SOC_cellBar = new QProgressBar(ui->frame_5);
    SOC_cellBar->move(17,20);
    SOC_cellBar->setFixedSize(96,126);
    SOC_cellBar->setMaximum(100);
    SOC_cellBar->setMinimum(0);
    SOC_cellBar->setValue(0);
    SOC_cellBar->setOrientation(Qt::Vertical);                          //设置垂直增幅
    SOC_cellBar->setStyleSheet("QProgressBar{border:none; background:rgb(255,255,255);border-radius:15px;}"
                               " QProgressBar::chunk{border-radius:14px;background-color: rgb(18, 159, 26);}");
    SOC_cellBar->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);     //设置字体位置
    QFont pen;
    pen.setPointSize(15);
    SOC_cellBar->setFont(pen);                                          //设置百分数字体大小

    SOH_cellBar = new QProgressBar(ui->frame_5);
    SOH_cellBar->setTextVisible(false);                                 //设置SOH电池进度条的百分比文字不可见，因为SOH会超过100%
    SOH_cellBar->move(17+120,20);
    SOH_cellBar->setFixedSize(96,126);
    SOH_cellBar->setMaximum(100);
    SOH_cellBar->setMinimum(0);
    SOH_cellBar->setValue(0);
    SOH_cellBar->setOrientation(Qt::Vertical);                          //设置垂直增幅
    SOH_cellBar->setStyleSheet("QProgressBar{border:none; background:rgb(255,255,255);border-radius:15px;}"
                               "QProgressBar::chunk{border-radius:14px;background-color: rgb(18, 159, 26);} ");

    //创建标签
    QLabel* SOC_Label = new QLabel(ui->frame_5);
    SOC_Label->move(51,30);
    SOC_Label->setText("SOC");
    SOC_Label->setFont(pen);
    SOC_Label->setStyleSheet("QLabel{background-color:rgba(255,255,224,0%); border:0px;}");
    QLabel* SOH_Label = new QLabel(ui->frame_5);
    SOH_Label->move(51+120,30);
    SOH_Label->setText("SOH");
    SOH_Label->setFont(pen);
    SOH_Label->setStyleSheet("QLabel{background-color:rgba(255,255,224,0%); border:0px;}");
    //创建显示百分比的标签（取代进度条本身的百分比文本）
    SOH_LabelNum = new QLabel(ui->frame_5);
    SOH_LabelNum->move(51+120,30+40);
    SOH_LabelNum->setFont(pen);
    SOH_LabelNum->setText(QString::number(0)+"%");
    SOH_LabelNum->setStyleSheet("QLabel{background-color:rgba(255,255,224,0%); border:0px;}");
}
//串口--初始化
void BatteryManage::init_serial()
{
    //端口号
//    QComboBox* sexcombox_port = new QComboBox();
//    sexcombox_port = ui->comboBox_port;
    ui->comboBox_port->setEditable(false);  //不可编辑
    ui->comboBox_port->clear();
    foreach(const QSerialPortInfo &info,QSerialPortInfo::availablePorts())
    {
        ui->comboBox_port->addItem(info.portName());
        port_List << info.portName();
    }
}
//串口--更新端口号
void BatteryManage::Timer_UpdatePort()
{
    QStringList newPortList;
    foreach(const QSerialPortInfo &info,QSerialPortInfo::availablePorts())
    {
        newPortList << info.portName();
    }
    if(port_List.size() != newPortList.size())
    {
        ui->comboBox_port->clear();
        port_List = newPortList;
        ui->comboBox_port->addItems(port_List);
    }
}
//串口--连接串口
void BatteryManage::linK_serial()
{
    if(M_port->isOpen())
    {
        if(ENGLISH){
            ui->pushButton->setText("Connect");
        }
        ui->pushButton->setText("连接");              
        if(timer->isActive())
            timer->stop();
        if(timerProcessData->isActive())
            timerProcessData->stop();
        ui->label_flag->setAutoFillBackground(true);
        ui->label_flag->setPalette(palette_gray);
        M_port->close();
        return ;
    }
    //设置端口号
    M_port->setPortName(ui->comboBox_port->currentText());
    //设置波特率
    switch (ui->comboBox_baudrate->currentText().toInt()) {
    case 115200:
        M_port->setBaudRate(QSerialPort::Baud115200);
        break;
    case 57600:
        M_port->setBaudRate(QSerialPort::Baud57600);
        break;
    case 9600:
        M_port->setBaudRate(QSerialPort::Baud9600);
        break;
    case 38400:
        M_port->setBaudRate(QSerialPort::Baud38400);
        break;
    case 4800:
        M_port->setBaudRate(QSerialPort::Baud4800);
        break;
    case 19200:
        M_port->setBaudRate(QSerialPort::Baud19200);
        break;
    case 2400:
        M_port->setBaudRate(QSerialPort::Baud2400);
        break;
    default:
        break;
    }
    //设置数据位
    M_port->setDataBits(QSerialPort::Data8);
    //设置流控制
    M_port->setFlowControl(QSerialPort::NoFlowControl);
    //设置校验位
    M_port->setParity(QSerialPort::NoParity);
    //设置停止位
    M_port->setStopBits(QSerialPort::OneStop);
    //打开串口
    if(!M_port->open(QIODevice::ReadWrite))
    {
        if(ENGLISH)
            QMessageBox::about(this,"Message hint","Serial port open failed");
        QMessageBox::about(this,"消息提示","串口打开失败");
        return ;
    }
    else{
        if(ENGLISH)
        {
            ui->pushButton->setText("disconnect");
            ui->label_upHint->setText("upgrade hint");
        }
        ui->pushButton->setText("断开连接");
        ui->label_upHint->setText("升级提示框");
        ui->label_flag->setAutoFillBackground(true);
        ui->label_flag->setPalette(palette_green);
    }
    if(!timer->isActive())
        timer->start(timerT);//每隔timerT毫秒发送一次问询帧，刷新数据显示
    if(!timerProcessData->isActive())
        timerProcessData->start(timerT+10);

    if(TabIndex == 3 || TabIndex == 5)
    {        
        if(timer->isActive())
            timer->stop();              //关闭发送问询帧的定时器
        if(timerProcessData->isActive())
            timerProcessData->stop();   //关闭显示数据定时器，因为要写入新数据
    }   
    return ;
}
//表格--初始化
void BatteryManage::init_QtableWidget()
{
    //设置表格参数属性
    ui->tableWidget->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents); //读写数据
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);    //最后一列伸展至最右端
    QStringList tableList;
    tableList << "博鹰" << "博鹰接收" << "UAVCAN" << "极翼";
    FCP_FontBox = new QFontComboBox();  //(0,0)
    FCP_FontBox->clear();
    FCP_FontBox->addItems(tableList);
    ui->tableWidget->setCellWidget(0,1,FCP_FontBox);
    tableList.clear();
    tableList << "聚合物" << "三元" << "铁锂" << "钛酸锂";
    //电芯类型
    cell_FontBox = new QFontComboBox();//(3,5)
    cell_FontBox->clear();
    cell_FontBox->addItems(tableList);
    ui->tableWidget->setCellWidget(3,5,cell_FontBox);
    bq_FontBox = new QFontComboBox();//(5,5)
    tableList.clear();
    tableList << "bq76940" << "bq76930" << "bq76920" << "bq40z80" << "bq40z50";
    bq_FontBox->clear();
    bq_FontBox->addItems(tableList);
    ui->tableWidget->setCellWidget(5,5,bq_FontBox);
    T_FontBox = new QFontComboBox();//(10,5)
    tableList.clear();
    tableList << "10K" << "100K";
    T_FontBox->clear();
    T_FontBox->addItems(tableList);
    ui->tableWidget->setCellWidget(10,5,T_FontBox);
    for(int i=1;i<8;i=i+2)
    {
        ui->tableWidget->setColumnWidth(i,82);
    }

    ui->tableWidget->setItem(31,4,new QTableWidgetItem("充电存储间隔设置时间(s)"));
    ui->tableWidget->setItem(32,4,new QTableWidgetItem("放电存储间隔设置时间(s)"));
    ui->tableWidget->setItem(33,4,new QTableWidgetItem("空闲存储间隔设置时间(s)"));
    //历史数据显示表格
    ui->tableWidget_his->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);  //固定列宽度
    ui->tableWidget_his->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);    //固定行宽
    for(int i=0;i<10;i++)
    {
        ui->tableWidget_his->setColumnWidth(i,90);
    }
}
//串口--连接串口
void BatteryManage::on_pushButton_clicked()
{    
    linK_serial();   
}
//串口--发送数据
void BatteryManage::Timer_slot()
{
    //时间的放置
    QDateTime dateTime = QDateTime::currentDateTime();
    timeLineEdit->setText(dateTime.toString(" hh:mm:ss dddd"));
    /*只有收到*/
    count++;
    flag++;    
    QSerialPortInfo portInfo(*M_port);
    switch(count)
    {
        case 1:pack_sendFrame(0x00,0x00,0x20,0x42);//66:0x2000~0x2020
        break;
        case 2:pack_sendFrame(0x00,0x21,0x20,0x3E);//62:0x2021~0x203F
        break;
        case 3:pack_sendFrame(0x00,0x40,0x20,0x3C);//60:0x2040~0x205D
        break;
        case 4:pack_sendFrame(0x00,0x5E,0x20,0x30);//48:0x205E~0x2075
        break;
        case 5:pack_sendFrame(0x00,0x76,0x20,0x72);//114:0x2076~0x20AE
        break;
        case 6:pack_sendFrame(0x00,0x00,0x21,static_cast<quint8>(LastFrameNum));//frameNum*2:0x2100~
        break;
        case 7:pack_sendFrame(0x00,0x00,0x50,0xB4);break;   //历史数据阈值读取
        case 8:pack_sendFrame(0x00,0x00,0x24,0x18);break;//24:0x20AF~0x20BA
        default:count = 0;
            break;
    }
    //通讯通断检测
    if(flag > 6)
    {
        ui->label_flag->setAutoFillBackground(true);
        ui->label_flag->setPalette(palette_red);
        if(flag > 10)       //防止溢出
            flag = 10;
    }else{
        ui->label_flag->setAutoFillBackground(true);
        ui->label_flag->setPalette(palette_green);
        if(realfile.isOpen())     //如果实时数据文件未关闭，并且实时定时器关闭，但是通讯已连接上
        {
            if(!realhisData_timer->isActive())      //如果定时器未激活
                realhisData_timer->start();
        }
    }
    return ;
}
//串口--读取串口数据
void BatteryManage::serial_read()
{
    QByteArray data = M_port->readAll();        //收到的数据是16进制的
    qDebug() << data;
    if(!data.isNull())                          //如果有收到接收帧则通讯指示灯变绿
    {        
        receFrameCnt++;
        flag = 0;                              //重新判断通讯是否断开
        ui->label_flag->setPalette(palette_green);
    }
    result = data.toHex(' ');
    result = " "+result.toUpper();
    //frameIn << "接收：" << result;
    logWin->addListData("接收："+result);
    read_BatteryData();
    if(receFrameCnt > 1000)
        receFrameCnt = 0;    
    return ;
}
//串口--延时函数
void BatteryManage::delay_ms(int ms)
{
    QTime dieTime = QTime::currentTime().addMSecs(ms);
    while(QTime::currentTime() < dieTime)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents,100);
    }
}
//数据处理--解析--数据装入List
void BatteryManage::read_BatteryData()
{
    /**************************************************************/    
    QString temp = result;
    result.clear();
    QStringList list = temp.split(" ",QString::SkipEmptyParts);
    if(frame_flag)
    {        
        dataVector.remove(0,4);                         //去除前面帧头等一些数据，只留数据
        dataVector.remove(dataLen.toInt(&ok,16),2);
        frameHead = "55";
        index = 0;
        head_flag = false;
        frame_flag = false;
        if(dataStartAddr.toUShort(&ok,16) >= 32768 )    //如果数据起始地址是0x8000以上
        {
            if(dataVector[0] != "00")
            {
                upgradeFlag = false;
            }
            else{
                upgradeFlag = true;
            }
        }
        if(dataStartAddr == "5000")     //历史数据阈值
        {
            if(dataLen.toUShort(nullptr,16) != 180)
            {
                logWin->addListData("历史数据阈值长度不等于180字节");

            }
            history_ThresholdVlaueSave();
        }
        if(dataStartAddr == "4FFF")
        {
            logWin->addListData("已识别到4FFF");
            if(dataVector.at(3)+dataVector.at(2)+dataVector.at(1)+dataVector.at(0) == "FFFF") //数据读取完成
            {
                finishFlag = true;
                his_writeToExcel();
            }else{
                hisDataNum = (dataVector.at(3)+dataVector.at(2)+dataVector.at(1)+dataVector.at(0)).toInt(nullptr,16);                
                loop.exit(0);
            }
        }
        if(dataStartAddr.toUShort(&ok,16) >= 0x4000 && dataStartAddr.toUShort(&ok,16) <= 0x4FFE) //有效的历史数据
        {
            if(!finishFlag)             //未收到读取结束指令
            history_analyise();
        }else if(dataStartAddr.toUShort(&ok,16) < 0x4000){
            dataSummary();
        }        
        dataVector.resize(dataSize);      //临时数组        
    }else{
        for(int i=0;i<list.size();i++)  //处理链表每个数据
        {
            if(!head_flag) //尚未捕捉到完整帧头
            {
                if(frameHead != list[i])
                {
                    index = 0;
                    frameHead = "55";
                    //非等待字符，但是帧开始字符0x55 55 AA 55 AA 00 00 AA 55应对这种情况
                    if(list[i] == "55")
                    {
                        dataVector[index++] = list[i];
                        frameHead = "AA";
                    }
                }else{
                    dataVector[index++] = list[i];                    
                    switch(index) //根据当前角标分配下一个有效帧头是什么
                    {
                        case 1: frameHead = "AA";
                        break;
                        case 2: frameHead = "00";
                            break;
                        case 3: frameHead = "00";
                            break;
                        case 4: frameHead = "AA";
                            break;
                        case 5: frameHead = "55";
                        break;
                    }
                    if(index==6)  //收到完整帧头
                    head_flag = true;
                }
            }else{ //捕捉到完整帧头
                dataVector[index++] = list[i];
                if(index > dataSize || index == dataSize)
                {
                    frame_flag = false;
                    frameHead = "55";
                    index = 0;
                    head_flag = false;
                    return ;
                }
                if(index == 10) //这时dataVector接收完成了6byte帧头、
                                //1byte读写、2byte数据起始地址、1byte数据长度
                                //当index=9时，dataVector[9]还没有被赋值
                {
                    cmd = dataVector[6];
                    dataStartAddr = dataVector[8]+dataVector[7];                    
                    //logWin->addListData("数据起始地址"+dataStartAddr);
                    dataLen = dataVector[9];                    
                    //logWin->addListData("数据长度："+dataLen);
                    if(dataLen.toInt(nullptr,16)+12 > dataSize)          //长度过长会导致数组越界
                    {
                        logWin->addListData("数据帧长度超过数组范围，将导致数组越界，已抛弃");
                        frame_flag = false;
                        frameHead = "55";
                        index = 0;
                        head_flag = false;
                        return ;
                    }
                }           
                if(index == 12+dataLen.toInt(&ok,16))//接收完了所有数据和2byte校验码
                {
                    crcRx = (dataVector[index-1]+dataVector[index-2]);

                    listToArray();                      //dataVector链表转quint8[]
                    if(buff.size() > dataSize)
                        return ;                    
                    quint16 crcCal = calCRC16(buff,4+dataLen.toInt(&ok,16));   //计算CRC校验
                    if(dataStartAddr.toUShort(&ok,16) >= 0x4000 && dataStartAddr.toUShort(&ok,16) <= 0x4FFF)    //历史数据不走校验
                    {                
                        frame_flag = true;
                    }else{                        
                        if(crcRx.toUShort(&ok,16) == crcCal)   //校验通过
                        {
                            waitFlag = true;      //写入得数据收到回复帧且通过校验
                            frame_flag = true;    //数据处理完记得false
                            loopWRet.exit(0);         //退出阻塞
                        }else{ //校验不通过                            
                            frameHead = "55";
                            index = 0;
                            head_flag = false;
                            frame_flag = false;
                            label->setText("校验不通过"+dataStartAddr);
                            //logWin->addListData("校验不成功 地址："+dataStartAddr);
                            dataVector.resize(dataSize);      //临时数组
                            return ;
                        }
                    }
                }
                if(frame_flag)
                {
                    dataVector.remove(0,4); //帧头已在listArray去除，去除读写、数据起始地址、数据长度
                    dataVector.remove(dataLen.toInt(&ok,16),2);
                    frameHead = "55";
                    index = 0;
                    head_flag = false;
                    frame_flag = false;
                    //logWin->addListData("数据解析:"+dataStartAddr);
                    if(dataStartAddr.toUShort(&ok,16) >= 0x8000 ) //如果数据起始地址是0x8000以上
                    {
                        if(dataVector[0] != "00")
                        {
                            upgradeFlag = false;
                        }
                        else{
                            upgradeFlag = true;
                        }                        
                    }
                    if(dataStartAddr == "5000")     //历史数据阈值
                    {
                        if(dataLen.toUShort(nullptr,16) != 180)
                        {
                            logWin->addListData("历史数据阈值长度不等于180字节");
                        }
                        history_ThresholdVlaueSave();
                    }
                    if(dataStartAddr == "4FFF")
                    {
                        if(dataVector.at(3)+dataVector.at(2)+dataVector.at(1)+dataVector.at(0) == "FFFFFFFF") //数据读取完成
                        {                            
                            finishFlag = true;
                            his_writeToExcel();
                        }else{                            
                            hisDataNum = (dataVector.at(3)+dataVector.at(2)+dataVector.at(1)+dataVector.at(0)).toInt(nullptr,16);
                            ui->progressBar_hisdata->setMaximum(hisDataNum);
                            ui->progressBar_hisdata->setMinimum(0);                            
                            if(loop.isRunning())
                                loop.exit(0);                            
                        }

                    }
                    if(dataStartAddr.toUShort(&ok,16) >= 0x4000 && dataStartAddr.toUShort(&ok,16) <= 0x4FFE) //有效的历史数据
                    {
                        if(!finishFlag)             //未收到读取结束指令
                        history_analyise();
                    }else if(dataStartAddr.toUShort(&ok,16) < 0x4000){
                        dataSummary();
                    }
                    dataVector.clear();
                    dataVector.resize(dataSize);      //临时数组
                }
            }
        }
    }
    return ;
}
//数据处理--分数组汇总
void BatteryManage::dataSummary()
{
    //单字节并成双字节（u16）
    QVector<QString> tempVector;
    if(dataVector.size() %2 == 1)
    {
        dataVector << "00";
    }
    for(int i=0;i<dataVector.size();i=i+2)
    {
        tempVector << dataVector[i+1]+dataVector[i]; //小端模式，低字节在前，高字节在后        
    }    
    dataVector.clear();

    if(dataStartAddr.toInt(nullptr,16) >=0x3000 && dataStartAddr.toInt(nullptr,16) < 0x3100)
    {
        TestList.clear();
        for(int i=0;i<tempVector.size();i=i+2)
        {
            TestList << tempVector.at(i+1) + tempVector.at(i);
        }
    }
    switch(dataStartAddr.toInt(&ok,16))
    {
        //0x2000~0x2020
        case 8192:
        for(int i=0;i<33;i++)
        {
            totalVector[i] = tempVector[i];
        }
        break;
        //0x2021~0x203F
        case 8225:
        for(int i=0;i<31;i++)
        {
            totalVector[addr("2021")+i] = tempVector[i];
        }
        cellNum = static_cast<quint8>(totalVector[addr("2027")].toInt(nullptr,16) >> 8);
        TempNum = static_cast<quint8>(totalVector[addr("2032")].toInt(nullptr,16) >> 8);
        if(TempNum<=0 || TempNum >= 4)
            TempNum = 2;
        LastFrameNum = (cellNum/16+1+cellNum)*2+TempNum;
        break;
        //0x2040~0x205D
        case 8256:
        for(int i=0;i<30;i++)
        {
            totalVector[addr("2040")+i] = tempVector[i];
        }
        break;
        //0x205E~0x2075
        case 8286:
        for(int i=0;i<24;i++)
        {
            totalVector[addr("205E")+i] = tempVector[i];
        }
        break;
        //0x2076~0x20AE
        case 8310:
        for(int i=0;i<57;i++)
        {
            totalVector[addr("2076")+i] = tempVector[i];
        }
        break;
        //0x2100~
        case 8448:
        for(int i=0;i<LastFrameNum/2+1;i++)
        {
            totalVector[addr("2100")+i] = tempVector[i];
        }
        break;
        //0x20AF~
        case 0x20AF:
        for(int i=0;i<12;i++)
        {
            totalVector[addr("20AF")+i] = tempVector[i];
        }
        break;
        /********* 待编程 *********/
    }    
    if(totalVector.size() > 350)
        return ;
    totalVector.resize(350);

    return ;
}
//数据处理--计算数组下标
int BatteryManage::addr(QString data) //data是16进制的字符串
{
    return data.toInt(nullptr,16)-baseAddr.toInt(nullptr,16);
}
//数据处理--转ASCII
QByteArray BatteryManage::toASCII(QString str)
{
    if(str.size() < 4)
        return  "00";
    //如果低字节或则高字节包含"FF"则remove
    if(str.at(0) == 'F' && str.at(1) == 'F')
    {
        str = str.remove(0,2);
    }else if(str.at(2) == 'F' && str.at(3) == 'F')
    {
        str = str.remove(2,2);
    }
    QString temp = QString(str.toUShort(&ok,16) >> 8);
    QString temp1 = QString(str.toUShort(&ok,16) & 0xFF);
    return temp1.toLatin1()+temp.toLatin1();
}
//数据处理--校验计算
quint16 BatteryManage::calCRC16(QVector<quint8> buff, int len)
{
    quint16 poly = 0x1021;
    crcCal = 0xFFFF;
    int i=0;
    while(len--)
    {
        uint32_t j;
        crcCal ^= (static_cast<uint16_t>(buff[i++] << 8));
        for (j = 0; j < 8; j++)
        {
            crcCal=static_cast<quint16>((crcCal & 0x8000U)?((crcCal << 1) ^ poly):(crcCal  << 1)) ;
        }
    }
    return crcCal;
}
//数据处理--校验和
quint8 BatteryManage::calNum(QList<QString> buff, int len)
{
    quint8 sum = 0;
    if(len<=0)
    {
        return 0;
    }
    for(int i=0;i<len;i++)
    {
        sum += buff.at(i).toUShort(nullptr,16) & 0xFF;
    }
    return sum;
}
//数据处理--链表转quint8数组，CRC校验
void BatteryManage::listToArray()
{
    //去掉帧头，不参与校验
    dataVector.remove(0,6);
    for(int i=0;i<dataVector.size();i++)
    {
        quint16 temp1 = dataVector[i].toUShort(&ok,16);
        buff[i] = static_cast<quint8>(temp1);
    }
}
//数据处理--打包问询帧
void BatteryManage::pack_sendFrame(quint8 cmd, quint8 addr1, quint8 addr2, quint8 len)
{
    QSerialPortInfo portInfo(*M_port);
    sendVector[6] = cmd;
    sendVector[7] = addr1;
    sendVector[8] = addr2;
    sendVector[9] = len;
    QVector<quint8> temp(10);
    for(int i=0;i<10;i++)
    {
        temp[i] = sendVector[i];
    }
    temp.remove(0,6);//移除帧头
    quint16 crcCal = calCRC16(temp,4);
    quint8 crc1 = crcCal >> 8;
    quint8 crc2 = (crcCal & 0xFF);
    sendVector[10] = crc2;
    sendVector[11] = crc1;
    sendData.clear();
    QString data;
    for(int i=0;i<12;i++)
    {
        sendData[i] = static_cast<char>(sendVector[i]);
        QString hex = QString("%1").arg(sendVector[i],2,16,QLatin1Char('0'));
        data.append(hex.toUpper()+" ");
    }
    if(portInfo.isBusy())
    M_port->write(sendData);
    label->setText(DateTime+"帧间隔:"+QString::number(timerT)+"ms) "+data);
    logWin->addListData("发:(帧间隔:"+QString::number(timerT)+"ms) "+data);
    //ui->plainTextEdit_out->setPlainText(data);
    return ;
}
//数据处理--打包发送的数据
void BatteryManage::Pack_sendData(QVector<quint8> sendVector, int len)
{
    QSerialPortInfo portInfo(*M_port);
    /* sendVector:需要发送的数据数组
     * len:除去两字节检验的全部数据
    */
    QVector<quint8> temp(300);
    if(len > sendVector.size())
    {
        QMessageBox::warning(this,"错误提示","打包发送帧数组越界");
        return ;
    }
    for(int i=0;i<len;i++)
    {
        temp[i] = sendVector.at(i); //全部数据赋值
    }
    temp.remove(0,6);//移除帧头
    quint16 crcCal = calCRC16(temp,len-6);
    quint8 crc1 = crcCal >> 8;
    quint8 crc2 = (crcCal & 0xFF);
    sendVector[len] = crc2;
    sendVector[len+1] = crc1;
    sendData.clear();
    QString data;
    for(int i=0;i<len+2;i++)
    {
        sendData[i] = static_cast<char>(sendVector.at(i));
        QString hex = QString("%1").arg(sendVector[i],2,16,QLatin1Char('0'));
        data.append(hex.toUpper()+" ");
    }
    if(portInfo.isBusy())
        M_port->write(sendData);
    label->setText(DateTime+"帧间隔:"+QString::number(timerT)+"ms) "+data);

    logWin->addListData("发:(帧间隔:"+QString::number(timerT)+"ms) "+data);
    return ;
}
//数据显示--显示曲线图
void BatteryManage::show_chart(qreal currentC,qreal averC)
{
    axY << currentC << averC;
    if(axY.size() > 100)
        axY.remove(0,2);
    max = axY.at(0);
    min = axY.at(0);
    for(int i=0;i<axY.size();i++)
    {
        max = (axY.at(i) > max) ? axY.at(i) : max;
        min = (axY.at(i) < min) ? axY.at(i) : min;
    }
    //从左往右
    chart->axes(Qt::Horizontal).first()->setMin(x);
    chart->axes(Qt::Horizontal).first()->setMax(x+50);

    if(Currect_series->count() > 50 || Aver_series->count() > 50)
    {
        Currect_series->removePoints(0,1);
        Aver_series->removePoints(0,1);
    }
    Currect_series->append(x,currentC);
    Aver_series->append(x,averC);
    chart->axes(Qt::Vertical).first()->setMin(min-5);
    chart->axes(Qt::Vertical).first()->setMax(max+5);
    x--;
    if(x == -10000)
        x = 10000;
}
//数据显示--显示对应的数据
void BatteryManage::writeCorrespondData()
{
    hisCount++;
    /*告警及保护信息*/
    show_warnMessage();
    /*单体电压值*/
    show_singleV();
    /*值域*/
    show_readonlyMessage();
    /*历史数据阈值*/
    if(hisCount > 5)
    {
        hisCount = 0;
        history_ThresholdVlaueRead();
    }    
    return ;
}
//数据显示--显示电池告警信息
void BatteryManage::show_warnMessage()
{
    QPalette paltte_red;
    QPalette paltte_black;
    paltte_red.setBrush(QPalette::WindowText,Qt::red);
    paltte_black.setBrush(QPalette::WindowText,Qt::black);
    quint16 data1 = totalVector[addr("200A")].toUShort(nullptr,16);
    quint16 data2 = totalVector[addr("200B")].toUShort(nullptr,16);
    quint16 data3 = totalVector[addr("200C")].toUShort(nullptr,16);
    quint16 data4 = totalVector[addr("200D")].toUShort(nullptr,16);


    QVector<QLabel*> labelVector;//48
    labelVector << ui->label_singleOVW << ui->label_singOVP << ui->label_singUVW << ui->label_singleUVP <<
                   ui->label_singleVchadaWH << ui->label_singleVchadaPH << ui->label_singleVchadaWL << ui->label_singleVchadaPL <<
                   ui->label_chargeOCW << ui->label_chargeOCP << ui->label_chargeShortCircuit << ui->label_disOCW << ui->label_disOCP <<
                   ui->label_disShortCircuit << ui->label_chargeHTW << ui->label_chargeHTP << ui->label_chargeLTW << ui->label_chargeLTP <<
                   ui->label_chargeTchada << ui->label_disHTW << ui->label_disHTP << ui->label_disLTW << ui->label_disLTP <<
                   ui->label_disTchada << ui->label_chargeMOSHTW << ui->label_chargeMOSHTP << ui->label_disMOSHTW << ui->label_disdisMOSHTP <<
                   ui->label_geratthan << ui->label_currentTLess << ui->label_SOClow << ui->label_BatHT << ui->label_BatLT <<
                   ui->label_BatOV << ui->label_BatUV << ui->label_Vchada << ui->label_nocharger << ui->label_getICunusual << ui->label_getVstub <<
                   ui->label_getTstub << ui->label_getCcircuitBreak << ui->label_flashBreak << ui->label_discircuitBreak <<
                   ui->label_heatingMOS << ui->label_chargeMOS << ui->label_disMOS << ui->label_discircuitBreak_2 << ui->label_preMOS;
    for(int i=0;i<16;i++)
    {
        if(data1 & (1 << i))
        {
            labelVector[i]->setPalette(paltte_red);
        }else{
            labelVector[i]->setPalette(paltte_black);
        }
        if(data2 & (1 << i))
        {
            labelVector[16+i]->setPalette(paltte_red);
        }else{
            labelVector[16+i]->setPalette(paltte_black);
        }

        if(i < 5)
        {
            if(data3 & (1 << i))
            {
                labelVector[32+i]->setPalette(paltte_red);
            }else{
                labelVector[32+i]->setPalette(paltte_black);
            }
        }

        if(i >= 8 && i <= 15)
        {
            if(data3 & (1 << i))
            {
                labelVector[37+i-8]->setPalette(paltte_red);
            }else{
                labelVector[37+i-8]->setPalette(paltte_black);
            }
        }

        if(i < 3)
        {
            if(data4 & (1 << i))
            {
                labelVector[45+i]->setPalette(paltte_red);
            }else{
                labelVector[45+i]->setPalette(paltte_black);
            }
        }
    }
    //自放电电开关状态
    if(data4 & (1 << 3))
    {
        ui->checkBox_selfdis->setCheckState(Qt::Checked);        
    }else{
        ui->checkBox_selfdis->setCheckState(Qt::Unchecked);        
    }
    //自加热开关状态
    if(data4 & (1 << 4))
    {
        ui->checkBox_selfheating->setCheckState(Qt::Checked);       
    }else{
        ui->checkBox_selfheating->setCheckState(Qt::Unchecked);        
    }
    //预充电开关状态
    if(data4 & (1 << 5))
    {
        ui->checkBox_prechge->setCheckState(Qt::Checked);        
    }else{
        ui->checkBox_prechge->setCheckState(Qt::Unchecked);        
    }
    //预放电开关状态
    if(data4 & (1 << 6))
    {
        ui->checkBox_predis->setCheckState(Qt::Checked);        
    }else{
        ui->checkBox_predis->setCheckState(Qt::Unchecked);        
    }
    //放电MOS开关状态
    if(data4 & (1 << 7))
    {
        ui->checkBox_dismosState->setCheckState(Qt::Checked);        
    }else{
        ui->checkBox_dismosState->setCheckState(Qt::Unchecked);       
    }
    //充电MOS开关状态
    if(data4 & (1 << 8))
    {
        ui->checkBox_chgemosState->setCheckState(Qt::Checked);        
    }else{
        ui->checkBox_chgemosState->setCheckState(Qt::Unchecked);        
    }
    //状态与故障次数显示
    n = addr("20AF");

    //过压总次数
    ui->lineEdit_OVNum->setText(QString::number(totalVector[n++].toUShort(nullptr,16)));
    //欠压总次数
    ui->lineEdit_UVNum->setText(QString::number(totalVector[n++].toUShort(nullptr,16)));
    //压差H过大总次数
    ui->lineEdit_VchaHNum->setText(QString::number(totalVector[n++].toUShort(nullptr,16)));
    //压差L过大总次数
    ui->lineEdit_VchaLNum->setText(QString::number(totalVector[n++].toUShort(nullptr,16)));
    //充电过流总次数
    ui->lineEdit_chargeOCNum->setText(QString::number(totalVector[n++].toUShort(nullptr,16)));
    //放电过流总次数
    ui->lineEdit_disOCNum->setText(QString::number(totalVector[n++].toUShort(nullptr,16)));
    //充电高温总次数
    ui->lineEdit_ChargeHTNum->setText(QString::number(totalVector[n++].toUShort(nullptr,16)));
    //充电低温总次数
    ui->lineEdit_chargeLTNum->setText(QString::number(totalVector[n++].toUShort(nullptr,16)));
    //放电高温总次数
    ui->lineEdit_disHTNum->setText(QString::number(totalVector[n++].toUShort(nullptr,16)));
    //放电低温的总次数
    ui->lineEdit_disLTNum->setText(QString::number(totalVector[n++].toUShort(nullptr,16)));
    //使用非原装充电器
    ui->lineEdit_NotcNum->setText(QString::number(totalVector[n++].toUShort(nullptr,16)));
    return ;
}
//数据显示--显示单体电压值
void BatteryManage::show_singleV()
{
    //均衡状态    
    quint16 balance1=0;
    quint16 balance2=0;
    QPalette pal_red;
    pal_red.setColor(QPalette::WindowText,Qt::red);
    QPalette pal_black;
    pal_black.setColor(QPalette::WindowText,Qt::black);
    QVector<int> VVector;
    if(cellNum > 0 && cellNum <= 16)
    {
        balance1 = totalVector[addr("2100")].toUShort(&ok,16);
        for(int i=0;i<cellNum;i++)      //根据电芯串数选定单体电压的起始地址
        {
            VVector << addr("2101")+i;
        }
    }else if(cellNum > 16 && cellNum <= 32)
    {
        balance1 = totalVector[addr("2100")].toUShort(&ok,16);
        balance2 = totalVector[addr("2101")].toUShort(&ok,16);
        for(int i=0;i<cellNum;i++)      //根据电芯串数选定单体电压的起始地址
        {
            VVector << addr("2102")+i;
        }
    }    
    quint8 b11 = balance1 >> 8;
    quint8 b12 = balance1 & 0xFF;    
    quint8 b21 = balance2 >> 8;
    QVector<QCheckBox*> checkVector;
    checkVector << ui->checkBox_1 << ui->checkBox_2 << ui->checkBox_3 << ui->checkBox_4 << ui->checkBox_5 << ui->checkBox_6
                << ui->checkBox_7 << ui->checkBox_8 << ui->checkBox_9 << ui->checkBox_10 << ui->checkBox_11 << ui->checkBox_12
                << ui->checkBox_13 << ui->checkBox_14 << ui->checkBox_15 << ui->checkBox_16 << ui->checkBox_17 << ui->checkBox_18
                << ui->checkBox_19 << ui->checkBox_20 << ui->checkBox_21 << ui->checkBox_22 << ui->checkBox_23 << ui->checkBox_24;


    QVector<QProgressBar*> progressVector;
    progressVector << ui->progressBar_1 << ui->progressBar_2 << ui->progressBar_3 << ui->progressBar_4 << ui->progressBar_5 << ui->progressBar_6
                << ui->progressBar_7 << ui->progressBar_8 << ui->progressBar_9 << ui->progressBar_10 << ui->progressBar_11 << ui->progressBar_12
                   << ui->progressBar_13 << ui->progressBar_14 << ui->progressBar_15 << ui->progressBar_16 << ui->progressBar_17 << ui->progressBar_18
                << ui->progressBar_19 << ui->progressBar_20 << ui->progressBar_21 << ui->progressBar_22 << ui->progressBar_23 << ui->progressBar_24;
    QVector<QLabel*> labelVector;
    labelVector << ui->label_V1 << ui->label_V2 << ui->label_V3 << ui->label_V4 << ui->label_V5 << ui->label_V6
                   << ui->label_V7 << ui->label_V8 << ui->label_V9 << ui->label_V10 << ui->label_V11 << ui->label_V12
                      << ui->label_V13 << ui->label_V14 << ui->label_V15 << ui->label_V16 << ui->label_V17 << ui->label_V18
                      << ui->label_V19 << ui->label_V20 << ui->label_V21 << ui->label_V22 << ui->label_V23 <<ui->label_V24;    

    for(int i=0;i<24;i++)
    {        
        progressVector[i]->setMaximum(4500);
        progressVector[i]->setMinimum(0);
        if(i < cellNum)
        {
            quint16 V = totalVector[VVector.at(i)].toUShort(&ok,16);

            (V >= 4500) ? progressVector[i]->setValue(4500) : progressVector[i]->setValue(V);
            labelVector[i]->setText(QString::number(V)+"mV");
            if(i < 8)
            {
                if(b12 & (1 << i))
                {
                    checkVector[i]->setCheckState(Qt::Checked);
                    checkVector[i]->setPalette(pal_red);
                }else{
                    checkVector[i]->setCheckState(Qt::Unchecked);
                    checkVector[i]->setPalette(pal_black);
                }
            }
            if(i >=8 && i < 16)
            {
                if(b11 & (1 << (i-8)))
                {
                    checkVector[i]->setCheckState(Qt::Checked);
                    checkVector[i]->setPalette(pal_red);
                }else{
                    checkVector[i]->setCheckState(Qt::Unchecked);
                    checkVector[i]->setPalette(pal_black);
                }
            }
            if(i >=16 && i<24)
            {
                if(b21 & (1 << (i-16)))
                {
                    checkVector[i]->setCheckState(Qt::Checked);
                    checkVector[i]->setPalette(pal_red);
                }else{
                    checkVector[i]->setCheckState(Qt::Unchecked);
                    checkVector[i]->setPalette(pal_black);
                }
            }
        }else{

            checkVector[i]->setCheckState(Qt::Unchecked);
            checkVector[i]->setPalette(pal_black);
            progressVector[i]->setValue(0);
            labelVector[i]->setText("0mV");
        }

    }   
    return ;
}
//数据显示--显示其余只读值
void BatteryManage::show_readonlyMessage()
{
    /*产品信息块*/
    //BMS内部时间
    QString BMSInteralTime = "20"+QString("%1").arg(totalVector[addr("2014")].toUShort(nullptr,16) & 0xFF,2,10,QLatin1Char('0'))+"/"+
            QString("%1").arg(totalVector[addr("2014")].toUShort(nullptr,16) >> 8,2,10,QLatin1Char('0'))+"/"+
            QString("%1").arg(totalVector[addr("2015")].toUShort(nullptr,16) & 0xFF,2,10,QLatin1Char('0'))+" "+
            QString("%1").arg(totalVector[addr("2015")].toUShort(nullptr,16) >> 8,2,10,QLatin1Char('0'))+":"+
            QString("%1").arg(totalVector[addr("2016")].toUShort(nullptr,16) & 0xFF,2,10,QLatin1Char('0'))+":"+
            QString("%1").arg(totalVector[addr("2016")].toUShort(nullptr,16) >> 8,2,10,QLatin1Char('0'));
    ui->lineEdit_BMSInteralTime->setText(BMSInteralTime);
    //电池型号
    QString produceTypeNum = toASCII(totalVector[addr("2097")])+toASCII(totalVector[addr("2098")])+toASCII(totalVector[addr("2099")])
                                +toASCII(totalVector[addr("209A")])+toASCII(totalVector[addr("209B")]);
    ui->lineEdit_produceTypeNum->setText(produceTypeNum);
    //硬件版本号
    QString hardWareVision = toASCII(totalVector[addr("2000")])+toASCII(totalVector[addr("2001")])+toASCII(totalVector[addr("2002")])
            +toASCII(totalVector[addr("2003")])+toASCII(totalVector[addr("2004")]);
    ui->lineEdit_hardwareVisionNum->setText(hardWareVision);

    //软件版本号
    QString softVision = toASCII(totalVector[addr("2005")])+toASCII(totalVector[addr("2006")])+toASCII(totalVector[addr("2007")])
            +toASCII(totalVector[addr("2008")])+toASCII(totalVector[addr("2009")]);
    ui->lineEdit_softmareVisionNum->setText(softVision);
    //电池生产日期
    QString makeDate = "20"+QString("%1").arg((totalVector[addr("20AA")].toUShort(nullptr,16) >> 8),2,10,QLatin1Char('0'))+"/"+
            QString("%1").arg((totalVector[addr("20AB")].toUShort(nullptr,16) & 0xFF),2,10,QLatin1Char('0'))+"/"+
            QString("%1").arg((totalVector[addr("20AB")].toUShort(nullptr,16) >> 8),2,10,QLatin1Char('0'));
    ui->lineEdit_makeDate->setText(makeDate);

    //飞控协议
    int flight = totalVector[addr("2076")].toInt(nullptr,16);
    switch (flight & 0xFF)
    {
        case 1:ui->lineEdit_flaigprotocol->setText("博鹰");
            break;
        case 2:ui->lineEdit_flaigprotocol->setText("博鹰接收");
            break;
        case 3:ui->lineEdit_flaigprotocol->setText("UAVCAN");
            break;
        case 4:ui->lineEdit_flaigprotocol->setText("极翼");
            break;
        default:ui->lineEdit_flaigprotocol->setText("NULL");
        break;
    }
    //飞控版本号
    QString revision = toASCII(totalVector[addr("207B")])+toASCII(totalVector[addr("207C")])+toASCII(totalVector[addr("207D")])
            +toASCII(totalVector[addr("207E")])+toASCII(totalVector[addr("207F")]);
    ui->lineEdit_flightVision->setText(revision);
    //客户编码
    QString clientcode = toASCII(totalVector[addr("2085")])+toASCII(totalVector[addr("2086")])
            +toASCII(totalVector[addr("2087")])+toASCII(totalVector[addr("2088")])+toASCII(totalVector[addr("2089")]);
    ui->lineEdit_clientCode->setText(clientcode);
    //电池产商编码
    QString procode = toASCII(totalVector[addr("208A")])+toASCII(totalVector[addr("208B")])
            +toASCII(totalVector[addr("208C")])+toASCII(totalVector[addr("208D")])+toASCII(totalVector[addr("208E")]);
    ui->lineEdit_cellProCode->setText(procode);
    //电池条形码显示
    QString batCode = toASCII(totalVector[addr("208F")])+toASCII(totalVector[addr("2090")])
            +toASCII(totalVector[addr("2091")])+toASCII(totalVector[addr("2092")])+
            toASCII(totalVector[addr("2093")])+toASCII(totalVector[addr("2094")])+
            toASCII(totalVector[addr("2095")])+toASCII(totalVector[addr("2096")]);
    ui->lineEdit_barCode->setText(batCode);
    //BMS型号
    QString BMSType = toASCII(totalVector[addr("209C")])+toASCII(totalVector[addr("209D")])
            +toASCII(totalVector[addr("209E")])+toASCII(totalVector[addr("209F")])+toASCII(totalVector[addr("20A0")]);
    ui->lineEdit_BMSNum->setText(BMSType);
    //BMS条形码
    QString BMSCode = toASCII(totalVector[addr("20A1")])+toASCII(totalVector[addr("20A2")])
            +toASCII(totalVector[addr("20A3")])+toASCII(totalVector[addr("20A4")])+
            toASCII(totalVector[addr("20A5")])+toASCII(totalVector[addr("20A6")])+
            toASCII(totalVector[addr("20A7")])+toASCII(totalVector[addr("20A8")]);
    ui->lineEdit_BMSCode_2->setText(BMSCode);
    //BMS生产日期
    QString BMSmakeDate = "20"+QString("%1").arg((totalVector[addr("20A9")].toUShort(&ok,16) & 0xFF),2,10,QLatin1Char('0'))+"/"+
            QString("%1").arg((totalVector[addr("20A9")].toUShort(&ok,16) >> 8),2,10,QLatin1Char('0'))+"/"+
            QString("%1").arg((totalVector[addr("20AA")].toUShort(&ok,16) & 0xFF),2,10,QLatin1Char('0'));
    ui->lineEdit_barCode_2->setText(BMSmakeDate);

    //销售商编码
    QString sellerCode = toASCII(totalVector[addr("2080")])+toASCII(totalVector[addr("2081")])+toASCII(totalVector[addr("2082")])
            +toASCII(totalVector[addr("2083")])+toASCII(totalVector[addr("2084")]);
    ui->lineEdit_sellerCode->setText(sellerCode);
    //SOC值（字节低位）、SOH值（字节高位）
    quint16 SO = totalVector[addr("2025")].toUShort(&ok,16);
    quint8 soc = SO & 0xFF;
    quint8 soh = SO >> 8;
    ui->lineEdit_soc->setText(QString::number(soc));
    ui->lineEdit_soh->setText(QString::number(soh));
    //设置电池电量百分比
    (soc > 100) ? SOC_cellBar->setValue(100) : SOC_cellBar->setValue(soc);
    (soh > 100) ? SOH_cellBar->setValue(100) : SOH_cellBar->setValue(soh);
    (soh <= 0) ? SOH_LabelNum->setText(QString::number(0)+"%") : SOH_LabelNum->setText(QString::number(soh)+"%");

    //最新满充容量
    uint cap = totalVector[addr("2022")].toUInt(&ok,16);
    ui->lineEdit_captotal->setText(QString::number(static_cast<double>(cap)/100,'f',2));

    //剩余容量
    uint least = totalVector[addr("2023")].toUInt(&ok,16);
    ui->lineEdit_shengyucap->setText(QString::number(static_cast<double>(least)/100,'f',2));

    //充电累计AH
    int chargecap = totalVector[addr("2024")].toInt(&ok,16);
    ui->lineEdit_chargetotal->setText(QString::number(chargecap));

    //电芯类型、电芯串数
    quint16 cell = static_cast<quint16>(totalVector[addr("2027")].toInt(&ok,16));
    quint8 cellType = cell & 0xFF;
    if(ENGLISH)
    {
        switch(cellType)
        {
            case 0:ui->lineEdit_cellType->setText("polymer");break;
            case 1:ui->lineEdit_cellType->setText("ternary");break;
            case 2:ui->lineEdit_cellType->setText("Ferrolithium");break;
            case 3:ui->lineEdit_cellType->setText("Lto");break;
        }
    }
    switch(cellType)
    {
        case 0:ui->lineEdit_cellType->setText("聚合物");break;
        case 1:ui->lineEdit_cellType->setText("三元");break;
        case 2:ui->lineEdit_cellType->setText("铁锂");break;
        case 3:ui->lineEdit_cellType->setText("钛酸锂");break;
    }
    ui->lineEdit_cellseialN->setText(QString::number(cellNum));
    //前端芯片类型、电芯型号
    quint16 fcell = totalVector[addr("2028")].toUShort(&ok,16);
    quint8 fcellType = fcell & 0xFF;
    quint8 fcelltypeN = fcell >> 8;
    switch (fcellType) {
        case 0:ui->lineEdit_frontCellType->setText("bq76940");break;
        case 1:ui->lineEdit_frontCellType->setText("bq76930");break;
        case 2:ui->lineEdit_frontCellType->setText("bq76920");break;
        case 3:ui->lineEdit_frontCellType->setText("bq40z80");break;
        case 4:ui->lineEdit_frontCellType->setText("bq40z50");break;
    }
    ui->lineEdit_frontCellType_2->setText(QString::number(fcelltypeN));
    //PACK设计容量
    int Dcap = totalVector[addr("2021")].toInt(&ok,16);
    ui->lineEdit_desCap->setText(QString::number(static_cast<double>(Dcap)/100,'f',2));
    //分流器阻值
    quint16 fenliu = totalVector[addr("2029")].toUShort(&ok,16);
    ui->lineEdit_fenliuqi->setText(QString::number(fenliu));
    //压差判断分界电压


    //循环次数
    quint16 circul = static_cast<quint16>(totalVector[addr("2026")].toInt(&ok,16));
    ui->lineEdit_crilTime->setText(QString::number(circul)+" ");

    //显示总电压
    uint totalV = totalVector[addr("200E")].toUInt(&ok,16);
    ui->lineEdit_totalV->setText(QString::number(static_cast<double>(totalV)/10,'f',1));
    //平均电压
    quint16 averV = totalVector[addr("200F")].toUShort(&ok,16);
    ui->lineEdit_averageV->setText(QString::number(averV));
    //最小输出电压
    quint16 smallV = totalVector[addr("2010")].toUShort(&ok,16);
    ui->lineEdit_minOutputV->setText(QString::number(smallV));
    //最大输出电压
    quint16 bigV = totalVector[addr("2011")].toUShort(&ok,16);
    ui->lineEdit_maxOutputV->setText(QString::number(bigV));
    //最小输出电压通道（字节高位）、最大输出电压通道（字节低位）
    quint16 channel = totalVector[addr("2012")].toUShort(&ok,16);
    quint8 channelH = channel >> 8;
    quint8 channelL = channel & 0xFF;
    ui->lineEdit_minOutputChannel->setText(QString::number(channelL+1));
    ui->lineEdit_maxOutChannel->setText(QString::number(channelH+1));
    //输出压差    
    quint16 outV = totalVector[addr("2013")].toUShort(&ok,16);
    ui->lineEdit_OutVdiffer->setText(QString::number(outV));   
    //当前电流值
    int currC = totalVector[addr("2019")].toInt(&ok,16);
    (currC > 0x8000) ? ui->lineEdit_currectValue->setText(QString::number(static_cast<double>(~(0xFFFF-currC))/10,'f',1)) :
                     ui->lineEdit_currectValue->setText(QString::number(static_cast<double>(currC)/10,'f',1));
    qreal currectC;
    currectC = (currC > 0x8000) ? static_cast<double>(~(0xFFFF-currC))/10 : static_cast<double>(currC)/10;

    //平均电流值
    int averC = totalVector[addr("201A")].toInt(&ok,16);
    (averC > 0x8000) ? ui->lineEdit_averCurrect->setText(QString::number(static_cast<double>(~(0xFFFF-averC))/10,'f',1)) :
                     ui->lineEdit_averCurrect->setText(QString::number(static_cast<double>(averC)/10,'f',1));
    qreal averageC;
    averageC = (averC > 0x8000) ? static_cast<double>(~(0xFFFF-averC))/10 : static_cast<double>(averC)/10;
        //设置显示曲线
        show_chart(currectC,averageC);
    //允许的最大放电电流
    int maxdisC = totalVector[addr("201B")].toInt(&ok,16);
    (maxdisC > 0x8000) ? ui->lineEdit_maxDisCur->setText(QString::number(static_cast<double>(~(0xFFFF-maxdisC))/10,'f',1)) :
                     ui->lineEdit_maxDisCur->setText(QString::number(static_cast<double>(maxdisC)/10,'f',1));    
    //允许的最大充电电流    
    int maxchargeC = totalVector[addr("201C")].toInt(&ok,16);
    (maxchargeC > 0x8000) ? ui->lineEdit_maxMaxCharge->setText(QString::number(static_cast<double>(~(0xFFFF-maxchargeC))/10,'f',1)) :
                     ui->lineEdit_maxMaxCharge->setText(QString::number(static_cast<double>(maxchargeC)/10,'f',1));
    //平均温度（字节高位）
    qint16 averT = static_cast<qint16>(totalVector[addr("201D")].toInt(&ok,16));
    ((averT & 0xFF) > 0x80) ? ui->lineEdit_averTemp->setText(QString::number(~(0xFF-(averT & 0xFF)))) :
                       ui->lineEdit_averTemp->setText(QString::number((averT & 0xFF)));
    //当前工作模式
    if(ENGLISH)
    {
        switch (averT >> 8) {
            case 0:ui->lineEdit_model->setText("startuping");break;
            case 1:ui->lineEdit_model->setText("charging");break;
            case 2:ui->lineEdit_model->setText("dischging");break;
            case 3:ui->lineEdit_model->setText("static");break;
            case 4:ui->lineEdit_model->setText("balancing");break;
            case 5:ui->lineEdit_model->setText("self-dischg");break;
            case 6:ui->lineEdit_model->setText("sleep");break;
            case 7:ui->lineEdit_model->setText("PowerDown");break;
        default:ui->lineEdit_model->setText("模式错误！");
            break;
        }
    }
    switch (averT >> 8) {
        case 0:ui->lineEdit_model->setText("准备启动");break;
        case 1:ui->lineEdit_model->setText("充电");break;
        case 2:ui->lineEdit_model->setText("放电");break;
        case 3:ui->lineEdit_model->setText("静止");break;
        case 4:ui->lineEdit_model->setText("均衡");break;
        case 5:ui->lineEdit_model->setText("自放电");break;
        case 6:ui->lineEdit_model->setText("休眠");break;
        case 7:ui->lineEdit_model->setText("掉电");break;
    default:ui->lineEdit_model->setText("模式错误！");
        break;
    }
    //最高温度（字节低位）、最低温度（字节高位）
    qint16 mostT = static_cast<qint16>(totalVector[addr("201E")].toInt(&ok,16));
    ((mostT >> 8) > 0x80) ? ui->lineEdit_maxTemp->setText(QString::number(~(0xFF-(mostT >> 8)))) :
                       ui->lineEdit_maxTemp->setText(QString::number((mostT >> 8)));
    ((mostT & 0xFF) > 0x80) ? ui->lineEdit_minTemp->setText(QString::number(~(0xFF-(mostT & 0xFF)))) :
                       ui->lineEdit_minTemp->setText(QString::number((mostT & 0xFF)));
    //最低温度通道数（字节低位）、最高温度通道数（字节高位）
    quint16 channelT = totalVector[addr("201F")].toUShort(&ok,16);
    quint8 channelTH = channelT >> 8;
    quint8 channelTL = channelT & 0xFF;    
    ui->lineEdit_maxTempChannel->setText(QString::number(channelTH+1));
    ui->lineEdit_minTempChannel->setText(QString::number(channelTL+1));
    //输出温差
    qint16 outTcha = static_cast<qint16>(totalVector[addr("2020")].toInt(&ok,16));
    ((outTcha & 0xFF) > 0x80) ? ui->lineEdit_outTcha->setText(QString::number(~(0xFF-(outTcha & 0xFF)))) :
                       ui->lineEdit_outTcha->setText(QString::number((outTcha & 0xFF)));
    //热敏电阻温度
    quint16 temp1 = 0;
    quint16 temp2 = 0;
    if(cellNum > 0 && cellNum <= 16)
    {        
        temp1 = totalVector[addr("2101")+cellNum].toUShort(&ok,16);
        temp2 = totalVector[addr("2101")+cellNum+1].toUShort(&ok,16);
    }else if(cellNum > 16 &&cellNum <= 32)
    {
        temp1 = totalVector[addr("2102")+cellNum].toUShort(&ok,16);
        temp2 = totalVector[addr("2102")+cellNum+1].toUShort(&ok,16);
    }
    qint8 t1 = static_cast<qint8>(temp1 & 0xFF);
    qint8 t2 = static_cast<qint8>(temp1 >> 8);
    qint8 t3 = static_cast<qint8>(temp2 & 0xFF);
    qint8 t4 = static_cast<qint8>(temp2 >> 8);
    if(t1 == 0)
    {
        qint8 temp;
        temp = t1;
        t1 = t2;
        t2 = temp;
    }else if(t3 == 0){
        qint8 temp;
        temp = t3;
        t3 = t4;
        t4 = temp;
    }
    ui->lineEdit_t1->setText(QString::number(t1));
    ui->lineEdit_t2->setText(QString::number(t2));
    ui->lineEdit_t3->setText(QString::number(t3));
    ui->lineEdit_t4->setText(QString::number(t4));

    //预计放电剩余时间
    quint16 distime = totalVector[addr("2017")].toUShort(&ok,16);
    ui->lineEdit_disTime->setText(QString::number(distime));
    //预计充满剩余时间
    quint16 chargetime = totalVector[addr("2018")].toUShort(&ok,16);
    ui->lineEdit_chargeTime->setText(QString::number(chargetime));    
    return ;
}
//数据显示--显示参数设置
void BatteryManage::parmaterShow()
{
n = addr("2076");
    //飞控协议
    int flight = totalVector[n].toInt(nullptr,16);
    switch(flight & 0xFF)
    {
        case 1:FCP_FontBox->setCurrentText("博鹰");
        break;
        case 2:FCP_FontBox->setCurrentText("博鹰接收");
        break;
        case 3:FCP_FontBox->setCurrentText("UAVCAN");
        break;
        case 4:FCP_FontBox->setCurrentText("极翼");
        break;
        default:FCP_FontBox->setCurrentText("NULL");
        break;
    }
    ui->tableWidget->setItem(0,1,new QTableWidgetItem(flight));
    //飞控版本号
n += 5;
    QString revision = toASCII(totalVector[n])+toASCII(totalVector[n+1])+toASCII(totalVector[n+2])
            +toASCII(totalVector[n+3])+toASCII(totalVector[n+4]);
    ui->tableWidget->setItem(1,1,new QTableWidgetItem(revision));
    //销售商编码
n += 5;
    QString sellerCode = toASCII(totalVector[n])+toASCII(totalVector[n+1])+toASCII(totalVector[n+2])
            +toASCII(totalVector[n+3])+toASCII(totalVector[n+4]);
    ui->tableWidget->setItem(0,3,new QTableWidgetItem(sellerCode));
    //客户编码
n += 5;
    QString clientcode = toASCII(totalVector[n])+toASCII(totalVector[n+1])+toASCII(totalVector[n+2])
            +toASCII(totalVector[n+3])+toASCII(totalVector[n+4]);
    ui->tableWidget->setItem(1,3,new QTableWidgetItem(clientcode));
    //电池产商编码
n += 5;
    QString procode = toASCII(totalVector[n])+toASCII(totalVector[n+1])+toASCII(totalVector[n+2])
            +toASCII(totalVector[n+3])+toASCII(totalVector[n+4]);
    ui->tableWidget->setItem(2,3,new QTableWidgetItem(procode));    

    //电池包型号
n = addr("2097");
    QString batType = toASCII(totalVector[n])+toASCII(totalVector[n+1])+toASCII(totalVector[n+2])
            +toASCII(totalVector[n+3])+toASCII(totalVector[n+4]);
    ui->tableWidget->setItem(0,5,new QTableWidgetItem(batType));
    //BMS型号
n += 5;
    QString BMSNum = toASCII(totalVector[n])+toASCII(totalVector[n+1])+toASCII(totalVector[n+2])
            +toASCII(totalVector[n+3])+toASCII(totalVector[n+4]);
    ui->tableWidget->setItem(1,5,new QTableWidgetItem(BMSNum));
    //BMS生产日期
    QString BMSMakeDate = "20"+QString("%1").arg((totalVector[addr("20A9")].toUShort(&ok,16) & 0xFF),2,10,QLatin1Char('0'))+"/"+
            QString("%1").arg((totalVector[addr("20A9")].toUShort(&ok,16) >> 8),2,10,QLatin1Char('0'))+"/"+
            QString("%1").arg((totalVector[addr("20AA")].toUShort(&ok,16) & 0xFF),2,10,QLatin1Char('0'));
    ui->tableWidget->setItem(0,7,new QTableWidgetItem(BMSMakeDate));
    //电池生产日期
    QString BatMakeDate = "20"+QString("%1").arg((totalVector[addr("20AA")].toUShort(nullptr,16) >> 8),2,10,QLatin1Char('0'))+"/"+
            QString("%1").arg((totalVector[addr("20AB")].toUShort(nullptr,16) & 0xFF),2,10,QLatin1Char('0'))+"/"+
            QString("%1").arg((totalVector[addr("20AB")].toUShort(nullptr,16) >> 8),2,10,QLatin1Char('0'));
    ui->tableWidget->setItem(1,7,new QTableWidgetItem(BatMakeDate));
    //充电存储间隔设置
    ui->tableWidget->setItem(31,5,new QTableWidgetItem(QString::number(totalVector[addr("20AC")].toUShort(nullptr,16))));
    //放电存储间隔设置
    ui->tableWidget->setItem(32,5,new QTableWidgetItem(QString::number(totalVector[addr("20AD")].toUShort(nullptr,16))));
    //空闲存储间隔设置
    ui->tableWidget->setItem(33,5,new QTableWidgetItem(QString::number(totalVector[addr("20AE")].toUShort(nullptr,16))));

n = addr("2021");
    //PACK设计容量    
    quint16 Dcap = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(3,1,new QTableWidgetItem(QString::number(static_cast<double>(Dcap)/100,'f',2)));
    //最新满充容量
    quint16 cap = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(4,1,new QTableWidgetItem(QString::number(static_cast<double>(cap)/100,'f',2)));
    //剩余容量
    quint16 least = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(5,1,new QTableWidgetItem(QString::number(static_cast<double>(least)/100,'f',2)));
    //充电累计AH
    quint16 chargecap = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(6,1,new QTableWidgetItem(QString::number(chargecap)));
    //SOC值（字节低位）、SOH值（字节高位）
    quint16 SO = totalVector[n++].toUShort(&ok,16);
    quint8 soc = SO & 0xFF;
    quint8 soh = SO >> 8;
    ui->tableWidget->setItem(3,3,new QTableWidgetItem(QString::number(soc)));
    ui->tableWidget->setItem(4,3,new QTableWidgetItem(QString::number(soh)));
    //循环次数
    quint16 cycleNum = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(5,3,new QTableWidgetItem(QString::number(cycleNum)));

n = addr("2027");
    //电芯类型、电芯串数
    quint16 cell = totalVector[n++].toUShort(&ok,16);
    quint8 cellType = cell & 0xFF;
    quint8 cellNum = cell >> 8;
    switch(cellType)
    {
        case 0://ui->tableWidget->setItem(3,5,new QTableWidgetItem("聚合物"));
                cell_FontBox->setCurrentText("聚合物");
        break;
        case 1://ui->tableWidget->setItem(3,5,new QTableWidgetItem("三元"));
                cell_FontBox->setCurrentText("三元");
        break;
        case 2://ui->tableWidget->setItem(3,5,new QTableWidgetItem("铁锂"));
                cell_FontBox->setCurrentText("铁锂");
        break;
        case 3://ui->tableWidget->setItem(3,5,new QTableWidgetItem("钛酸锂"));
                cell_FontBox->setCurrentText("钛酸锂");
        break;
    }
    ui->tableWidget->setItem(4,5,new QTableWidgetItem(QString::number(cellNum)));
    //前端芯片类型、电芯型号
    quint16 fcell = totalVector[n++].toUShort(&ok,16);
    quint8 fcellType = fcell & 0xFF;
    quint8 fcelltypeN = fcell >> 8;
//    fcellType ? ui->tableWidget->setItem(5,5,new QTableWidgetItem("BQ76940")) : ui->tableWidget->setItem(5,5,new QTableWidgetItem("BQ76930"));
    fcellType ? bq_FontBox->setCurrentText("bq76930") : bq_FontBox->setCurrentText("bq76940");
    switch(fcellType)
    {
        case 0:bq_FontBox->setCurrentText("bq76940");break;
        case 1:bq_FontBox->setCurrentText("bq76930");break;
        case 2:bq_FontBox->setCurrentText("bq76920");break;
        case 3:bq_FontBox->setCurrentText("bq40z80");break;
        case 4:bq_FontBox->setCurrentText("bq40z50");break;
    }
    ui->tableWidget->setItem(6,5,new QTableWidgetItem(QString::number(fcelltypeN)));
    //分流器阻值
    quint16 fenliu = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(6,3,new QTableWidgetItem(QString::number(fenliu)));
    //压差判断分界电压
    quint16 ifVcha = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(6,7,new QTableWidgetItem(QString::number(ifVcha)));
    //均衡开启的电压值
    quint16 BSV = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(8,1,new QTableWidgetItem(QString::number(BSV)));
    //均衡关闭的电压值
    quint16 BCV = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(9,1,new QTableWidgetItem(QString::number(BCV)));
    //均衡开启的压差值
    quint16 BSVI = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(10,1,new QTableWidgetItem(QString::number(BSVI)));
    //均衡关闭的压差值
    quint16 BCVI = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(11,1,new QTableWidgetItem(QString::number(BCVI)));
    //均衡门限开启时间
    quint16 BST = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(12,1,new QTableWidgetItem(QString::number(BST)));
    //强制关机电压
    quint16 forceOFFV = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(3,7,new QTableWidgetItem(QString::number(forceOFFV)));
    //静置休眠时间、静置休眠唤醒时间
    quint16 quietSleep = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(4,7,new QTableWidgetItem(QString::number(quietSleep & 0xFF)));
    ui->tableWidget->setItem(5,7,new QTableWidgetItem(QString::number(quietSleep >> 8)));

    /****************************************************************************/
n = addr("2032");
    //温度检测使能（bit0）、温度采集点数（字节高位）
    quint16 temp = totalVector[n++].toUShort(&ok,16);
    quint8 tempPN = temp >> 8;
    quint8 tempType = temp & 0xFF;
    ui->tableWidget->setItem(9,5,new QTableWidgetItem(QString::number(tempPN)));
    ui->tableWidget->setItem(8,5,new QTableWidgetItem(QString::number(tempType & 0x01)));
    //温度传感器类型、低电量报警
    quint16 TtypeSO = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(11,5,new QTableWidgetItem(QString::number(TtypeSO >> 8)));
//    ((TtypeSO & 0xFF) & 0x80) ? ui->tableWidget->setItem(10,5,new QTableWidgetItem("100K")) :
//                              ui->tableWidget->setItem(10,5,new QTableWidgetItem("10K"));
    (TtypeSO & 0xFF) ? T_FontBox->setCurrentText("100K") : T_FontBox->setCurrentText("10K");
    //充电截止总电压
    ui->tableWidget->setItem(8,7,new QTableWidgetItem(QString::number(static_cast<double>(totalVector[n++].toUInt(&ok,16))/10,'f',1)));
    //充电恢复总电压
    ui->tableWidget->setItem(9,7,new QTableWidgetItem(QString::number(static_cast<double>(totalVector[n++].toUInt(&ok,16))/10,'f',1)));
    //充电短路电流值
    int chshortC = totalVector[n++].toInt(&ok,16);
    (chshortC > 0x8000) ? ui->tableWidget->setItem(10,7,new QTableWidgetItem(QString::number(static_cast<double>(~(0xFFFF-chshortC))/10,'f',1))) :
                          ui->tableWidget->setItem(10,7,new QTableWidgetItem(QString::number(static_cast<double>(chshortC)/10,'f',1)));
    //放电短路电流值
    int disshortC = totalVector[n++].toInt(&ok,16);
    (disshortC > 0x8000) ? ui->tableWidget->setItem(11,7,new QTableWidgetItem(QString::number(static_cast<double>(~(0xFFFF-disshortC))/10,'f',1))) :
                          ui->tableWidget->setItem(11,7,new QTableWidgetItem(QString::number(static_cast<double>(disshortC)/10,'f',1)));
    //充电温差过大、放电温差过大
    qint16 Tchabig = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((Tchabig & 0xFF) > 0x80) ? ui->tableWidget->setItem(12,5,new QTableWidgetItem(QString::number(~(0xFF-(Tchabig & 0xFF))))) :
                                ui->tableWidget->setItem(12,5,new QTableWidgetItem(QString::number(Tchabig & 0xFF)));
    ((Tchabig >> 8) > 0x80) ? ui->tableWidget->setItem(12,7,new QTableWidgetItem(QString::number(~(0xFF-(Tchabig >> 8))))) :
                                ui->tableWidget->setItem(12,7,new QTableWidgetItem(QString::number(Tchabig >> 8)));
    //自放电启动时间
    ui->tableWidget->setItem(14,1,new QTableWidgetItem(QString::number(totalVector[n++].toUShort(&ok,16))));
    //自放电开启电压
    ui->tableWidget->setItem(14,3,new QTableWidgetItem(QString::number(totalVector[n++].toUShort(&ok,16))));
    //自放电截止电压
    ui->tableWidget->setItem(14,5,new QTableWidgetItem(QString::number(totalVector[n++].toUShort(&ok,16))));

    //电池高温永久失效、电池低温永久失效
    qint16 BatTdis = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((BatTdis & 0xFF) > 0x80) ? ui->tableWidget->setItem(8,3,new QTableWidgetItem(QString::number(~(0xFF-(BatTdis & 0xFF))))) :
                                ui->tableWidget->setItem(8,3,new QTableWidgetItem(QString::number(BatTdis & 0xFF)));
    ((BatTdis >> 8) > 0x80) ? ui->tableWidget->setItem(9,3,new QTableWidgetItem(QString::number(~(0xFF-(BatTdis >> 8))))) :
                                ui->tableWidget->setItem(9,3,new QTableWidgetItem(QString::number(BatTdis >> 8)));
    //单体过压永久失效
    ui->tableWidget->setItem(10,3,new QTableWidgetItem(QString::number(totalVector[n++].toUShort(&ok,16))));
    //单体欠压永久失效
    ui->tableWidget->setItem(11,3,new QTableWidgetItem(QString::number(totalVector[n++].toUShort(&ok,16))));
    //电池压差过大永久失效
    ui->tableWidget->setItem(12,3,new QTableWidgetItem(QString::number(totalVector[n++].toUShort(&ok,16))));
    /********************************************************************************************************/

    /*******************************************************************************************************/
n = addr("2040");
    //单体过压告警值
    quint16 SOVW = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(16,1,new QTableWidgetItem(QString::number(SOVW)));
    //单体过压告警恢复值
    quint16 SOVWP = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(17,1,new QTableWidgetItem(QString::number(SOVWP)));
    //单体过压保护值
    quint16 SOVP = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(18,1,new QTableWidgetItem(QString::number(SOVP)));
    //单体过压保护恢复值
    quint16 SOVPR = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(19,1,new QTableWidgetItem(QString::number(SOVPR)));
    //单体过压保护恢复时间（字节高位）、单体过压保护恢复次数（字节低位）
    quint16 SOVPRT = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(21,1,new QTableWidgetItem(QString::number(SOVPRT >> 8)));
    ui->tableWidget->setItem(20,1,new QTableWidgetItem(QString::number(SOVPRT & 0xFF)));

    //单体欠压告警值
    quint16 SUVW = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(16,3,new QTableWidgetItem(QString::number(SUVW)));
    //单体欠压告警恢复值
    quint16 SUVWP = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(17,3,new QTableWidgetItem(QString::number(SUVWP)));
    //单体欠压保护值
    quint16 SUVP = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(18,3,new QTableWidgetItem(QString::number(SUVP)));
    //单体欠压保护恢复值
    quint16 SUVPR = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(19,3,new QTableWidgetItem(QString::number(SUVPR)));
    //单体欠压保护恢复时间（字节高位）、单体欠压保护恢复次数（字节低位）
    quint16 SUVPRT = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(21,3,new QTableWidgetItem(QString::number(SUVPRT >> 8)));
    ui->tableWidget->setItem(20,3,new QTableWidgetItem(QString::number(SUVPRT & 0xFF)));

    //单体压差过大告警值1
    quint16 SVBW = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(16,5,new QTableWidgetItem(QString::number(SVBW)));
    //单体压差过大告警恢复值1
    quint16 SVBWP = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(17,5,new QTableWidgetItem(QString::number(SVBWP)));
    //单体压差过大保护值1
    quint16 SVBP = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(18,5,new QTableWidgetItem(QString::number(SVBP)));
    //单体压差过大保护恢复值1
    quint16 SVBPR = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(19,5,new QTableWidgetItem(QString::number(SVBPR)));
    //单体压差过大保护恢复时间（字节高位）1、单体压差过大保护恢复次数（字节低位）1
    quint16 SVBPRT = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(21,5,new QTableWidgetItem(QString::number(SVBPRT >> 8)));
    ui->tableWidget->setItem(20,5,new QTableWidgetItem(QString::number(SVBPRT & 0xFF)));

    //单体压差过大告警值2
    quint16 SVBW2 = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(16,7,new QTableWidgetItem(QString::number(SVBW2)));
    //单体压差过大告警恢复值2
    quint16 SVBWP2 = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(17,7,new QTableWidgetItem(QString::number(SVBWP2)));
    //单体压差过大保护值2
    quint16 SVBP2 = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(18,7,new QTableWidgetItem(QString::number(SVBP2)));
    //单体压差过大保护恢复值2
    quint16 SVBPR2 = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(19,7,new QTableWidgetItem(QString::number(SVBPR2)));
    //单体压差过大保护恢复时间（字节高位）2、单体压差过大保护恢复次数（字节低位）2
    quint16 SVBPRT2 = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(21,7,new QTableWidgetItem(QString::number(SVBPRT2 >> 8)));
    ui->tableWidget->setItem(20,7,new QTableWidgetItem(QString::number(SVBPRT2 & 0xFF)));
    /********************************************************************************/
    //充电过流告警值
    int COCW = totalVector[n++].toInt(&ok,16);
    (COCW > 0x8000) ? ui->tableWidget->setItem(23,1,new QTableWidgetItem(QString::number(static_cast<double>(~(0xFFFF-COCW))/10,'f',1))) :
                          ui->tableWidget->setItem(23,1,new QTableWidgetItem(QString::number(static_cast<double>(COCW)/10,'f',1)));
    //充电过流告警恢复值
    int COCWR = totalVector[n++].toInt(&ok,16);
    (COCWR > 0x8000) ? ui->tableWidget->setItem(24,1,new QTableWidgetItem(QString::number(static_cast<double>(~(0xFFFF-COCWR))/10,'f',1))) :
                          ui->tableWidget->setItem(24,1,new QTableWidgetItem(QString::number(static_cast<double>(COCWR)/10,'f',1)));
    //充电过流保护值    
    int COCP = totalVector[n++].toInt(&ok,16);
    (COCP > 0x8000) ? ui->tableWidget->setItem(25,1,new QTableWidgetItem(QString::number(static_cast<double>(~(0xFFFF-COCP))/10,'f',1))) :
                          ui->tableWidget->setItem(25,1,new QTableWidgetItem(QString::number(static_cast<double>(COCP)/10,'f',1)));
    //充电过流保护恢复值    
    int COCPR = totalVector[n++].toInt(&ok,16);
    (COCPR > 0x8000) ? ui->tableWidget->setItem(26,1,new QTableWidgetItem(QString::number(static_cast<double>(~(0xFFFF-COCPR))/10,'f',1))) :
                          ui->tableWidget->setItem(26,1,new QTableWidgetItem(QString::number(static_cast<double>(COCPR)/10,'f',1)));
    //充电过流保护恢复时间（字节高位）、充电过流保护恢复次数（字节低位）
    quint16 COCPRT = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(28,1,new QTableWidgetItem(QString::number(COCPRT >> 8)));
    ui->tableWidget->setItem(27,1,new QTableWidgetItem(QString::number(COCPRT & 0xFF)));

    //放电过流告警值
    int DOCW = totalVector[n++].toInt(&ok,16);
    (DOCW > 0x8000) ? ui->tableWidget->setItem(23,3,new QTableWidgetItem(QString::number(static_cast<double>(~(0xFFFF-DOCW))/10,'f',1))) :
                          ui->tableWidget->setItem(23,3,new QTableWidgetItem(QString::number(static_cast<double>(DOCW)/10,'f',1)));
    //放电过流告警恢复值    
    int DOCWR = totalVector[n++].toInt(&ok,16);
    (DOCWR > 0x8000) ? ui->tableWidget->setItem(24,3,new QTableWidgetItem(QString::number(static_cast<double>(~(0xFFFF-DOCWR))/10,'f',1))) :
                          ui->tableWidget->setItem(24,3,new QTableWidgetItem(QString::number(static_cast<double>(DOCWR)/10,'f',1)));
    //放电过流保护值
    int DOCP = totalVector[n++].toInt(&ok,16);
    (DOCP > 0x8000) ? ui->tableWidget->setItem(25,3,new QTableWidgetItem(QString::number(static_cast<double>(~(0xFFFF-DOCP))/10,'f',1))) :
                          ui->tableWidget->setItem(25,3,new QTableWidgetItem(QString::number(static_cast<double>(DOCP)/10,'f',1)));
    //放电过流保护恢复值
    int DOCPR = totalVector[n++].toInt(&ok,16);
    (DOCPR > 0x8000) ? ui->tableWidget->setItem(26,3,new QTableWidgetItem(QString::number(static_cast<double>(~(0xFFFF-DOCPR))/10,'f',1))) :
                          ui->tableWidget->setItem(26,3,new QTableWidgetItem(QString::number(static_cast<double>(DOCPR)/10,'f',1)));
    //放电过流保护恢复时间（字节高位）、放电过流保护恢复次数（字节低位）
    quint16 DOCPRT = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(28,3,new QTableWidgetItem(QString::number(DOCPRT >> 8)));
    ui->tableWidget->setItem(27,3,new QTableWidgetItem(QString::number(DOCPRT & 0xFF)));

    //加热使能bit1，预放使能bit2，充电MOS使能bit3，放电MOS使能bit4
    ui->tableWidget->setItem(30,1,new QTableWidgetItem(QString::number(tempType >> 1 & 0x01)));
    ui->tableWidget->setItem(30,3,new QTableWidgetItem(QString::number(tempType >> 2 & 0x01)));
    ui->tableWidget->setItem(30,5,new QTableWidgetItem(QString::number(tempType >> 3 & 0x01)));
    ui->tableWidget->setItem(30,7,new QTableWidgetItem(QString::number(tempType >> 4 & 0x01)));

    //充电高温告警值（字节低位）、充电高温告警恢复值（字节高位）
    qint16 chHTW = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((chHTW & 0xFF) > 0x80) ? ui->tableWidget->setItem(31,1,new QTableWidgetItem(QString::number(~(0xFF-(chHTW & 0xFF))))) :
                        ui->tableWidget->setItem(31,1,new QTableWidgetItem(QString::number((chHTW & 0xFF))));
    ((chHTW >> 8) > 0x80) ? ui->tableWidget->setItem(32,1,new QTableWidgetItem(QString::number(~(0xFF-(chHTW >> 8))))) :
                        ui->tableWidget->setItem(32,1,new QTableWidgetItem(QString::number((chHTW >> 8))));
    //充电高温保护值（字节低位）、充电高温保护恢复值（字节高位）
    qint16 chHTP = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((chHTP & 0xFF) > 0x80) ? ui->tableWidget->setItem(33,1,new QTableWidgetItem(QString::number(~(0xFF-(chHTP & 0xFF))))) :
                        ui->tableWidget->setItem(33,1,new QTableWidgetItem(QString::number((chHTP & 0xFF))));
    ((chHTP >> 8) > 0x80) ? ui->tableWidget->setItem(34,1,new QTableWidgetItem(QString::number(~(0xFF-(chHTP >> 8))))) :
                        ui->tableWidget->setItem(34,1,new QTableWidgetItem(QString::number((chHTP >> 8))));
    //充电高温保护延迟时间（字节低位）、充电高温保护恢复次数（字节高位）
    qint16 chHTPT = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((chHTPT & 0xFF) > 0x80) ? ui->tableWidget->setItem(35,1,new QTableWidgetItem(QString::number(~(0xFF-(chHTPT & 0xFF))))) :
                        ui->tableWidget->setItem(35,1,new QTableWidgetItem(QString::number((chHTPT & 0xFF))));
    ((chHTPT >> 8) > 0x80) ? ui->tableWidget->setItem(36,1,new QTableWidgetItem(QString::number(~(0xFF-(chHTPT >> 8))))) :
                        ui->tableWidget->setItem(36,1,new QTableWidgetItem(QString::number((chHTPT >> 8))));

    //充电低温告警值（字节低位）、充电低温告警恢复值（字节高位）
    qint16 chLTW = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((chLTW & 0xFF) > 0x80) ? ui->tableWidget->setItem(31,3,new QTableWidgetItem(QString::number(~(0xFF-(chLTW & 0xFF))))) :
                        ui->tableWidget->setItem(31,3,new QTableWidgetItem(QString::number((chLTW & 0xFF))));
    ((chLTW >> 8) > 0x80) ? ui->tableWidget->setItem(32,3,new QTableWidgetItem(QString::number(~(0xFF-(chLTW >> 8))))) :
                        ui->tableWidget->setItem(32,3,new QTableWidgetItem(QString::number((chLTW >> 8))));
    //充电低温保护值（字节低位）、充电低温保护恢复值（字节高位）
    qint16 chLTP = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((chLTP & 0xFF) > 0x80) ? ui->tableWidget->setItem(33,3,new QTableWidgetItem(QString::number(~(0xFF-(chLTP & 0xFF))))) :
                        ui->tableWidget->setItem(33,3,new QTableWidgetItem(QString::number((chLTP & 0xFF))));
    ((chLTP >> 8) > 0x80) ? ui->tableWidget->setItem(34,3,new QTableWidgetItem(QString::number(~(0xFF-(chLTP >> 8))))) :
                        ui->tableWidget->setItem(34,3,new QTableWidgetItem(QString::number((chLTP >> 8))));
    //充电低温保护恢复时间（字节高位）、充电低温保护恢复次数（字节低位）
    quint16 chLTPT = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(36,3,new QTableWidgetItem(QString::number(chLTPT >> 8)));
    ui->tableWidget->setItem(35,3,new QTableWidgetItem(QString::number(chLTPT & 0xFF)));

    /********************************************************************************************************************/
    //放电高温告警值（字节低位）、放电高温告警恢复值（字节高位）
n = addr("2064");
    qint16 disHTW = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((disHTW & 0xFF) > 0x80) ? ui->tableWidget->setItem(38,1,new QTableWidgetItem(QString::number(~(0xFF-(disHTW & 0xFF))))) :
                        ui->tableWidget->setItem(38,1,new QTableWidgetItem(QString::number(disHTW & 0xFF)));
    ((disHTW >> 8) > 0x80) ? ui->tableWidget->setItem(39,1,new QTableWidgetItem(QString::number(~(0xFF-(disHTW >> 8))))) :
                        ui->tableWidget->setItem(39,1,new QTableWidgetItem(QString::number(disHTW >> 8)));
    //放电高温保护值（字节低位）、放电高温保护恢复值（字节高位）
    qint16 disHTP = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((disHTP & 0xFF) > 0x80) ? ui->tableWidget->setItem(40,1,new QTableWidgetItem(QString::number(~(0xFF-(disHTP & 0xFF))))) :
                        ui->tableWidget->setItem(40,1,new QTableWidgetItem(QString::number((disHTP & 0xFF))));
    ((disHTP >> 8) > 0x80) ? ui->tableWidget->setItem(41,1,new QTableWidgetItem(QString::number(~(0xFF-(disHTP >> 8))))) :
                        ui->tableWidget->setItem(41,1,new QTableWidgetItem(QString::number((disHTP >> 8))));
    //放电高温保护恢复时间（字节低位）、放电高温保护恢复次数（字节高位）
    quint16 disHTPRT = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(43,1,new QTableWidgetItem(QString::number(disHTPRT >> 8)));
    ui->tableWidget->setItem(42,1,new QTableWidgetItem(QString::number(disHTPRT & 0xFF)));

    //放电低温告警值（字节低位）、放电低温告警恢复值（字节高位）
    qint16 disLTW = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((disLTW & 0xFF) > 0x80) ? ui->tableWidget->setItem(38,3,new QTableWidgetItem(QString::number(~(0xFF-(disLTW & 0xFF))))) :
                        ui->tableWidget->setItem(38,3,new QTableWidgetItem(QString::number((disLTW & 0xFF))));
    ((disLTW >> 8) > 0x80) ? ui->tableWidget->setItem(39,3,new QTableWidgetItem(QString::number(~(0xFF-(disLTW >> 8))))) :
                        ui->tableWidget->setItem(39,3,new QTableWidgetItem(QString::number((disLTW >> 8))));
    //放电低温保护值（字节低位）、放电低温保护恢复值（字节高位）
    qint16 disLTP = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((disLTP & 0xFF) > 0x80) ? ui->tableWidget->setItem(40,3,new QTableWidgetItem(QString::number(~(0xFF-(disLTP & 0xFF))))) :
                        ui->tableWidget->setItem(40,3,new QTableWidgetItem(QString::number((disLTP & 0xFF))));

    ((disLTP >> 8) > 0x80) ? ui->tableWidget->setItem(41,3,new QTableWidgetItem(QString::number(~(0xFF-(disLTP >> 8))))) :
                        ui->tableWidget->setItem(41,3,new QTableWidgetItem(QString::number((disLTP >> 8))));
    //放电低温保护恢复时间（字节高位）
    quint16 disLTPRT = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(43,3,new QTableWidgetItem(QString::number(disLTPRT >> 8)));
    ui->tableWidget->setItem(42,3,new QTableWidgetItem(QString::number(disLTPRT & 0xFF)));

    //充电MOS高温告警（字节高位）、充电MOS高温告警恢复值
    qint16 chMosHTW = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((chMosHTW & 0xFF) > 0x80) ? ui->tableWidget->setItem(38,5,new QTableWidgetItem(QString::number(~(0xFF-(chMosHTW & 0xFF))))) :
                        ui->tableWidget->setItem(38,5,new QTableWidgetItem(QString::number((chMosHTW & 0xFF))));
    ((chMosHTW >> 8) > 0x80) ? ui->tableWidget->setItem(39,5,new QTableWidgetItem(QString::number(~(0xFF-(chMosHTW >> 8))))) :
                        ui->tableWidget->setItem(39,5,new QTableWidgetItem(QString::number((chMosHTW >> 8))));
    //充电MOS高温保护（字节高位）、充电MOS高温保护恢复值
    qint16 chMosHTP = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((chMosHTP & 0xFF) > 0x80) ? ui->tableWidget->setItem(40,5,new QTableWidgetItem(QString::number(~(0xFF-(chMosHTP & 0xFF))))) :
                        ui->tableWidget->setItem(40,5,new QTableWidgetItem(QString::number((chMosHTP & 0xFF))));

    ((chMosHTP >> 8) > 0x80) ? ui->tableWidget->setItem(41,5,new QTableWidgetItem(QString::number(~(0xFF-(chMosHTP >> 8))))) :
                        ui->tableWidget->setItem(41,5,new QTableWidgetItem(QString::number((chMosHTP >> 8))));
    //充电MOS高温保护恢复时间、充电MOS高温保护恢复次数
     quint16 chMosHTPRT = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(43,5,new QTableWidgetItem(QString::number(chMosHTPRT >> 8)));
    ui->tableWidget->setItem(42,5,new QTableWidgetItem(QString::number(chMosHTPRT & 0xFF)));

    //充电MOS低温告警（字节高位）、充电MOS低温告警恢复值
    qint16 chMosLTW = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((chMosLTW & 0xFF) > 0x80) ? ui->tableWidget->setItem(38,7,new QTableWidgetItem(QString::number(~(0xFF-(chMosLTW & 0xFF))))) :
                        ui->tableWidget->setItem(38,7,new QTableWidgetItem(QString::number((chMosLTW & 0xFF))));
    ((chMosLTW >> 8) > 0x80) ? ui->tableWidget->setItem(39,7,new QTableWidgetItem(QString::number(~(0xFF-(chMosLTW >> 8))))) :
                        ui->tableWidget->setItem(39,7,new QTableWidgetItem(QString::number((chMosLTW >> 8))));
    //充电MOS低温保护（字节高位）、充电MOS低温保护恢复值
    qint16 chMosLTP = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((chMosLTP & 0xFF) > 0x80) ? ui->tableWidget->setItem(40,7,new QTableWidgetItem(QString::number(~(0xFF-(chMosLTP & 0xFF))))) :
                        ui->tableWidget->setItem(40,7,new QTableWidgetItem(QString::number((chMosLTP & 0xFF))));
    ((chMosLTP >> 8) > 0x80) ? ui->tableWidget->setItem(41,7,new QTableWidgetItem(QString::number(~(0xFF-(chMosLTP >> 8))))) :
                        ui->tableWidget->setItem(41,7,new QTableWidgetItem(QString::number((chMosLTP >> 8))));
    //充电MOS低温保护恢复时间、充电MOS低温保护恢复次数
    quint16 chMosLTPRT = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(43,7,new QTableWidgetItem(QString::number(chMosLTPRT >> 8)));
    ui->tableWidget->setItem(42,7,new QTableWidgetItem(QString::number(chMosLTPRT & 0xFF)));

    //放电MOS高温告警（字节高位）、放电MOS高温告警恢复值
    qint16 disMosHTH = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((disMosHTH & 0xFF) > 0x80) ? ui->tableWidget->setItem(23,5,new QTableWidgetItem(QString::number(~(0xFF-(disMosHTH & 0xFF))))) :
                        ui->tableWidget->setItem(23,5,new QTableWidgetItem(QString::number((disMosHTH & 0xFF))));
    ((disMosHTH >> 8) > 0x80) ? ui->tableWidget->setItem(24,5,new QTableWidgetItem(QString::number(~(0xFF-(disMosHTH >> 8))))) :
                        ui->tableWidget->setItem(24,5,new QTableWidgetItem(QString::number((disMosHTH >> 8))));
    //放电MOS高温保护（字节高位）、放电MOS高温保护恢复值
    qint16 disMosHTPH = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((disMosHTPH & 0xFF) > 0x80) ? ui->tableWidget->setItem(25,5,new QTableWidgetItem(QString::number(~(0xFF-(disMosHTPH & 0xFF))))) :
                        ui->tableWidget->setItem(25,5,new QTableWidgetItem(QString::number((disMosHTPH & 0xFF))));
    ((disMosHTPH >> 8) > 0x80) ? ui->tableWidget->setItem(26,5,new QTableWidgetItem(QString::number(~(0xFF-(disMosHTPH >> 8))))) :
                        ui->tableWidget->setItem(26,5,new QTableWidgetItem(QString::number((disMosHTPH >> 8))));
    //放电MOS高温保护恢复时间、放电MOS高温保护恢复次数
    quint16 disMosHTPT = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(28,5,new QTableWidgetItem(QString::number(disMosHTPT >> 8)));
    ui->tableWidget->setItem(27,5,new QTableWidgetItem(QString::number(disMosHTPT & 0xFF)));

    //放电MOS低温告警（字节高位）、放电MOS低温告警恢复值
    qint16 disMosLTH = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((disMosLTH & 0xFF) > 0x80) ? ui->tableWidget->setItem(23,7,new QTableWidgetItem(QString::number(~(0xFF-(disMosLTH & 0xFF))))) :
                        ui->tableWidget->setItem(23,7,new QTableWidgetItem(QString::number((disMosLTH & 0xFF))));
    ((disMosLTH >> 8) > 0x80) ? ui->tableWidget->setItem(24,7,new QTableWidgetItem(QString::number(~(0xFF-(disMosLTH >> 8))))) :
                        ui->tableWidget->setItem(24,7,new QTableWidgetItem(QString::number((disMosLTH >> 8))));
    //放电MOS低温保护（字节高位）、放电MOS低温保护恢复值
    qint16 disMosLTPH = static_cast<qint16>(totalVector[n++].toInt(&ok,16));
    ((disMosLTPH & 0xFF) > 0x80) ? ui->tableWidget->setItem(25,7,new QTableWidgetItem(QString::number(~(0xFF-(disMosLTPH & 0xFF))))) :
                        ui->tableWidget->setItem(25,7,new QTableWidgetItem(QString::number((disMosLTPH & 0xFF))));
    ((disMosLTPH >> 8) > 0x80) ? ui->tableWidget->setItem(26,7,new QTableWidgetItem(QString::number(~(0xFF-(disMosLTPH >> 8))))) :
                        ui->tableWidget->setItem(26,7,new QTableWidgetItem(QString::number((disMosLTPH >> 8))));
    //放电MOS低温保护恢复时间、放电MOS低温保护恢复次数
    quint16 disMosLTPT = totalVector[n++].toUShort(&ok,16);
    ui->tableWidget->setItem(28,7,new QTableWidgetItem(QString::number(disMosLTPT >> 8)));
    ui->tableWidget->setItem(27,7,new QTableWidgetItem(QString::number(disMosLTPT & 0xFF)));
    return ;
}
//菜单栏--退出
void BatteryManage::on_menu_exit_aboutToShow()
{
    BatteryManage::close();
}
//校准--条形码
void BatteryManage::on_pushButton_readBarcode_clicked()
{
    ui->pushButton_readBarcode->setEnabled(false);
    if(M_port->isOpen())
    {
        int k=6;
        sendVector[k++] = 0x01;
        sendVector[k++] = 0x8F;
        sendVector[k++] = 0x20;
        sendVector[k++] = 0x10;
        QByteArray BatCode = ui->lineEdit_tiaoxingma->text().toLatin1();
        if(BatCode.size() > 16)
        {
            QMessageBox::warning(this,"错误提示","条形码最长不能超过16位");
            ui->pushButton_readBarcode->setEnabled(true);
            return ;
        }
        for(int i=0;i<BatCode.size();i++)
        {
            sendVector[k++] = static_cast<quint8>(BatCode.at(i));
        }
        if(BatCode.size() < 16)
        {
            for(int i=0;i<16-BatCode.size();i++)
            {
                sendVector[k++] = 0x00;
            }
        }
        waitFlag = false;
        Pack_sendData(sendVector,26);        
        delay_ms(1500);
        if(waitFlag)
        {
            QMessageBox::about(this,"提示","数据写入成功");
        }else{
            QMessageBox::warning(this,"提示","数据写入失败！！！");
        }
    }else{
        QMessageBox::warning(this,"错误提示","串口未打开");
    }
    ui->pushButton_readBarcode->setEnabled(true);
    return ;
}
//校准--BMS条码
void BatteryManage::on_pushButton_BMSWrite_clicked()
{
    ui->pushButton_BMSWrite->setEnabled(false);
    if(M_port->isOpen())
    {
        int k=6;
        sendVector[k++] = 0x01;
        sendVector[k++] = 0xA1;
        sendVector[k++] = 0x20;
        sendVector[k++] = 0x10;
        QByteArray BMSCode = ui->lineEdit_BMSCode->text().toLatin1();
        if(BMSCode.size() > 16)
        {
            QMessageBox::warning(this,"错误提示","条形码最长不能超过16位");
            ui->pushButton_BMSWrite->setEnabled(true);
            return ;
        }
        for(int i=0;i<BMSCode.size();i++)
        {
            sendVector[k++] = static_cast<quint8>(BMSCode.at(i));
        }
        if(BMSCode.size() < 16)
        {
            for(int i=0;i<16-BMSCode.size();i++)
            {
                sendVector[k++] = 0x00;
            }
        }
        receFrameCnt = 0;
        Pack_sendData(sendVector,26);
        delay_ms(1500);
        if(receFrameCnt > 0)
        {
            QMessageBox::about(this,"提示","数据写入成功");
        }else{
            QMessageBox::warning(this,"提示","数据写入失败！！！");
        }
    }else{
        QMessageBox::warning(this,"错误提示","串口未打开");
    }
    ui->pushButton_BMSWrite->setEnabled(true);
    return ;
}
//校准--清除历史数据
void BatteryManage::on_pushButton_zeroCalib_clicked()
{
    ui->pushButton_zeroCalib->setEnabled(false);
    if(M_port->isOpen())
    {
        sendVector[6] = 0x01;
        sendVector[7] = 0x03;
        sendVector[8] = 0x23;
        sendVector[9] = 0x00;       //无长度
        receFrameCnt = 0;
        Pack_sendData(sendVector,10);
        delay_ms(1500);
        if(receFrameCnt > 0)
        {
            QMessageBox::about(this,"提示","清除成功");
        }else{
            QMessageBox::warning(this,"提示","清除历史数据失败！！！");
        }
    }else{
        QMessageBox::warning(this,"错误提示","串口未打开");
    }
    ui->pushButton_zeroCalib->setEnabled(true);
    return ;
}
//校准--清除永久失效
void BatteryManage::on_pushButton_clearForever_clicked()
{
    ui->pushButton_clearForever->setEnabled(false);
    if(M_port->isOpen())
    {
        sendVector[6] = 0x01;
        sendVector[7] = 0x0B;
        sendVector[8] = 0x23;
        sendVector[9] = 0x00;       //无长度
        receFrameCnt = 0;
        Pack_sendData(sendVector,10);
        delay_ms(1500);
        if(receFrameCnt > 0)
        {
            QMessageBox::about(this,"提示","数据写入成功");
        }else{
            QMessageBox::warning(this,"提示","数据写入失败！！！");
        }
    }else{
        QMessageBox::warning(this,"错误提示","串口未打开");
    }
    ui->pushButton_clearForever->setEnabled(true);
    return ;
}
//校准--自放电
void BatteryManage::on_buttonBox_clicked(QAbstractButton *button)
{
    ui->buttonBox->setEnabled(false);
    if(M_port->isOpen())
    {
        sendVector[6] = 0x01;
        sendVector[7] = 0x0C;
        sendVector[8] = 0x23;
        sendVector[9] = 0x01;       //无长度
        if(button == static_cast<QAbstractButton *>(ui->buttonBox->button(QDialogButtonBox::Open)))
        {
            sendVector[10] = 0x00;
        }else if(button == static_cast<QAbstractButton *>(ui->buttonBox->button(QDialogButtonBox::Close))){
            sendVector[10] = 0xFF;
        }
        Pack_sendData(sendVector,11);
        waitFlag = false;
        QTimer::singleShot(1500,&loopWRet,SLOT(quit()));
        loopWRet.exec();
        delay_ms(100);
        if(waitFlag)
        {
            QMessageBox::about(this,"提示","数据写入成功");
        }else{
            QMessageBox::warning(this,"提示","数据写入失败！！！");
        }
    }else{
        QMessageBox::warning(this,"错误提示","串口未打开");
    }
    ui->buttonBox->setEnabled(true);
}
//校准--时间校准
void BatteryManage::on_pushButton_timeCalib_clicked()
{
    ui->pushButton_timeCalib->setEnabled(false);
    if(!M_port->isOpen())
    {
        QMessageBox::warning(this,"错误提示","串口未打开");
        ui->pushButton_timeCalib->setEnabled(true);
        return ;
    }
    QString time = QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss");
    if(!time.contains(' ') && !time.contains('/') && !time.contains(':'))
    {
        QMessageBox::warning(this,"错误提示","获取电脑时间错误");
        ui->pushButton_timeCalib->setEnabled(true);
        return ;
    }
    QStringList dateTime = time.split(" ",QString::SkipEmptyParts);    
    QStringList date = dateTime[0].split("/",QString::SkipEmptyParts);
    QString year = date[0];
    year = year.remove(0,2);
    QString month = date[1];
    QString day = date[2];
    QStringList time1 = dateTime[1].split(":",QString::SkipEmptyParts);
    QString hour = time1[0];
    QString min = time1[1];
    QString sec = time1[2];

    sendVector[6] = 0x01;
    sendVector[7] = 0x00;
    sendVector[8] = 0x23;
    sendVector[9] = 0x06;
    sendVector[10] = static_cast<quint8>(year.toUShort());
    sendVector[11] = static_cast<quint8>(month.toUShort());
    sendVector[12] = static_cast<quint8>(day.toUShort());
    sendVector[13] = static_cast<quint8>(hour.toUShort());
    sendVector[14] = static_cast<quint8>(min.toUShort());
    sendVector[15] = static_cast<quint8>(sec.toUShort());

    waitFlag = false;
    Pack_sendData(sendVector,16);    
    delay_ms(1500);
    if(waitFlag)
    {
        QMessageBox::about(this,"提示","数据写入成功");
    }else{
        QMessageBox::warning(this,"提示","数据写入失败！！！");
    }
    ui->pushButton_timeCalib->setEnabled(true);
    return ;
}
//校准--放电电流校准
void BatteryManage::on_pushButton_bigCCalib_clicked()
{
    ui->pushButton_bigCCalib->setEnabled(false);    
    if(ui->lineEdit_currectValue->text().contains('-'))
    if(!M_port->isOpen())
    {
        QMessageBox::warning(this,"错误提示","串口未打开");
        ui->pushButton_bigCCalib->setEnabled(true);
        return ;
    }
    QString bigC = ui->lineEdit_bigCCalib->text();
    qint16 bigC1;
    if(bigC.contains('-'))
    {
        bigC = bigC.remove(QChar('-'),Qt::CaseInsensitive);
        bigC1 = static_cast<qint16>(~static_cast<int>(bigC.toFloat()*10)+1);
    }else{
        bigC1 = static_cast<qint16>(bigC.toFloat()*10);
    }
    qint8 bigCH = bigC1 >> 8;
    qint8 bigCL = static_cast<qint8>(bigC1 & 0xFF);
    sendVector[6] = 0x01;
    sendVector[7] = 0x05;
    sendVector[8] = 0x23;
    sendVector[9] = 0x02;
    sendVector[10] = static_cast<quint8>(bigCL);
    sendVector[11] = static_cast<quint8>(bigCH);
    waitFlag = false;
    Pack_sendData(sendVector,12);    
    delay_ms(1500);
    if(waitFlag)
    {
        QMessageBox::about(this,"提示","数据写入成功");
    }else{
        QMessageBox::warning(this,"提示","数据写入失败！！！");
    }
    ui->pushButton_bigCCalib->setEnabled(true);
    return ;
}
//校准--充电电流校准
void BatteryManage::on_pushButton_smallCCalib_clicked()
{
    ui->pushButton_smallCCalib->setEnabled(false);
    if(!M_port->isOpen())
    {
        QMessageBox::warning(this,"错误提示","串口未打开");
        ui->pushButton_smallCCalib->setEnabled(true);
        return ;
    }
    QString smallC = ui->lineEdit_smallCCalib->text();
    qint16 smallC1;
    if(smallC.contains('-'))
    {
        smallC.remove(QChar('-'),Qt::CaseInsensitive);
        smallC1 = static_cast<qint16>(~static_cast<int>(smallC.toFloat()*10)+1);
    }else{
        smallC1 = static_cast<qint16>(smallC.toFloat()*10);        
    }
    qint8 smallCH = smallC1 >> 8;
    qint8 smallCL = static_cast<qint8>(smallC1 & 0xFF);
    sendVector[6] = 0x01;
    sendVector[7] = 0x04;
    sendVector[8] = 0x23;
    sendVector[9] = 0x02;
    sendVector[10] = static_cast<quint8>(smallCL);
    sendVector[11] = static_cast<quint8>(smallCH);
    waitFlag = false;
    Pack_sendData(sendVector,12);    
    delay_ms(1500);
    if(waitFlag)
    {
        QMessageBox::about(this,"提示","数据写入成功");
    }else{
        QMessageBox::warning(this,"提示","数据写入失败！！！");
    }
    ui->pushButton_smallCCalib->setEnabled(true);
    return ;
}
//校准--单体电压大端校准
void BatteryManage::on_pushButton_bigVCalib_clicked()
{
    ui->pushButton_bigVCalib->setEnabled(false);
    if(!M_port->isOpen())
    {
        QMessageBox::warning(this,"错误提示","串口未打开");
        ui->pushButton_bigVCalib->setEnabled(true);
        return ;
    }
    QString singleV = ui->lineEdit_singleVbigcalib->text();
    QString cell = ui->comboBox_singlebig->currentText();
    quint8 cellnum = cell.toUShort() & 0xFF;
    if(cellnum > cellNum)
    {
        QMessageBox::warning(this,"错误提示","所选电池数超过总电池数");
        ui->pushButton_bigVCalib->setEnabled(true);
        return ;
    }
    if(ui->lineEdit_singleVbigcalib->text().toInt() < 3700)
    {
        QMessageBox::warning(this,"错误提示","单体电压大端校准值不得小于3700");
        ui->pushButton_bigVCalib->setEnabled(true);
        return ;
    }
    quint8 vH = singleV.toUShort() >> 8;
    quint8 vL = singleV.toUShort() & 0xFF;
    sendVector[6] = 0x01;
    sendVector[7] = 0x06;
    sendVector[8] = 0x23;
    sendVector[9] = 0x04;
    sendVector[10] = vL;
    sendVector[11] = vH;
    sendVector[12] = cellnum;      //被校准电池序号 预留（字节低位）
    sendVector[13] = 0x00;
    waitFlag = false;
    Pack_sendData(sendVector,14);  
    delay_ms(1500);    
    if(waitFlag)
    {
        QMessageBox::about(this,"提示","数据写入成功");
    }else{
        QMessageBox::warning(this,"提示","数据写入失败！！！");
    }
    ui->pushButton_bigVCalib->setEnabled(true);
}
//校准--单体电压小端校准
void BatteryManage::on_pushButton_smallVCalib_clicked()
{
    ui->pushButton_smallVCalib->setEnabled(false);
    if(!M_port->isOpen())
    {
        QMessageBox::warning(this,"错误提示","串口未打开");
        ui->pushButton_smallVCalib->setEnabled(false);
        return ;
    }
    QString singleV = ui->lineEdit_singleVsCalib->text();
    QString cell = ui->comboBox_singlesmall->currentText();
    quint8 cellnum = cell.toUShort() & 0xFF;
    if(cellnum > cellNum)
    {
        QMessageBox::warning(this,"错误提示","所选电池数超过总电池数");
        ui->pushButton_smallVCalib->setEnabled(true);
        return ;
    }
    if(ui->lineEdit_singleVsCalib->text().toInt() > 3700)
    {
        QMessageBox::warning(this,"错误提示","单体电压小端校准值不得大于3700");
        ui->pushButton_smallVCalib->setEnabled(true);
        return ;
    }
    quint8 vH = singleV.toUShort() >> 8;
    quint8 vL = singleV.toUShort() & 0xFF;
    sendVector[6] = 0x01;
    sendVector[7] = 0x08;
    sendVector[8] = 0x23;
    sendVector[9] = 0x04;
    sendVector[10] = vL;
    sendVector[11] = vH;
    sendVector[12] = cellnum;      //被校准电池序号 预留（字节低位）
    sendVector[13] = 0x00;
    waitFlag = false;
    Pack_sendData(sendVector,14); 
    delay_ms(1500); 
    if(waitFlag)
    {
        QMessageBox::about(this,"提示","数据写入成功");
    }else{
        QMessageBox::warning(this,"提示","数据写入失败！！！");
    }
    ui->pushButton_smallVCalib->setEnabled(true);
    return ;
}
//校准--温度校准
void BatteryManage::on_pushButton_6_clicked()
{
    ui->pushButton_6->setEnabled(false);
    if(!M_port->isOpen())
    {
        QMessageBox::warning(this,"错误提示","串口未打开");
        ui->pushButton_6->setEnabled(true);
        return ;
    }
    QString temp = ui->lineEdit_tempCalib->text();
    qint8 temp1 = static_cast<qint8>(temp.toShort() & 0xFF);
    if(temp.contains('.') || temp1 < -40 || temp1 > 126)
    {
        QMessageBox::warning(this,"错误提示","温度值错误");
        ui->pushButton_6->setEnabled(true);
        return ;
    }
    QString cell = ui->lineEdit_tempCalibserial->text();

    quint8 cellnum = cell.toUShort() & 0xFF;
    sendVector[6] = 0x01;
    sendVector[7] = 0x0A;
    sendVector[8] = 0x23;
    sendVector[9] = 0x02;
    sendVector[10] = cellnum;
    sendVector[11] = temp1;
    waitFlag = false;
    Pack_sendData(sendVector,12);
    delay_ms(1500);

    if(waitFlag)
    {
        QMessageBox::about(this,"提示","数据写入成功");
    }else{
        QMessageBox::warning(this,"提示","数据写入失败！！！");
    }
    ui->pushButton_6->setEnabled(true);
    return ;
}
//校准--强制均衡开关
void BatteryManage::on_pushButton_balanceCalib_clicked()
{
    ui->pushButton_balanceCalib->setEnabled(false);
    quint32 balance = 0;
    if(!M_port->isOpen())
    {
        QMessageBox::warning(this,"错误提示","串口未打开");
        ui->pushButton_balanceCalib->setEnabled(true);
        return ;
    }
    QList<QCheckBox*> list;
    list << ui->checkBox_cell1 << ui->checkBox_cell2 << ui->checkBox_cell3 << ui->checkBox_cell4
         << ui->checkBox_cell5 << ui->checkBox_cell6 << ui->checkBox_cell7 << ui->checkBox_cell8
            << ui->checkBox_cell9 << ui->checkBox_cell10 << ui->checkBox_cell11 << ui->checkBox_cell12
               << ui->checkBox_cell13 << ui->checkBox_cell14 << ui->checkBox_cell15 << ui->checkBox_cell16
                  << ui->checkBox_cell17 << ui->checkBox_cell18 << ui->checkBox_cell19 << ui->checkBox_cell20
                     << ui->checkBox_cell21 << ui->checkBox_cell22 << ui->checkBox_cell23 << ui->checkBox_cell24;
    for(int i=0;i<24;i++)
    {
        if(list[i]->isChecked())
        {
            balance |= 1 << i;
        }else{
            balance &= ~(1 << i);
        }
    }
    sendVector[6] = 0x01;
    int k=7;
    sendVector[k++] = 0x0D;
    sendVector[k++] = 0x23;
    sendVector[k++] = 0x03;
    sendVector[k++] = balance & 0xFF;
    sendVector[k++] = (balance >> 8) & 0xFF;
    sendVector[k++] = (balance >> 16) & 0xFF;
    waitFlag = false;
    Pack_sendData(sendVector,13);  
    delay_ms(1500);

    if(waitFlag)
    {
        QMessageBox::about(this,"提示","数据写入成功");
    }else{
        QMessageBox::warning(this,"提示","数据写入失败！！！");
    }
    ui->pushButton_balanceCalib->setEnabled(true);
    return ;
}
//校准--强制休眠
void BatteryManage::on_pushButton_8_clicked()
{
    QMessageBox::about(this,"提示","强制休眠成功后会断开串口连接");
    ui->pushButton_8->setEnabled(false);
    if(M_port->isOpen())
    {
        sendVector[6] = 0x01;
        sendVector[7] = 0x10;
        sendVector[8] = 0x23;
        sendVector[9] = 0x01;       //无长度
        sendVector[10] = 0xFF;
        receFrameCnt = 0;
        Pack_sendData(sendVector,11);
        delay_ms(1500);
        if(receFrameCnt > 0)
        {
            QMessageBox::about(this,"提示","强制休眠成功");
            ui->pushButton->setText("连接");
            label->setText(DateTime+"串口已被打开");
            if(timer->isActive())
                timer->stop();
            if(timerProcessData->isActive())
                timerProcessData->stop();
            ui->label_flag->setAutoFillBackground(true);
            ui->label_flag->setPalette(palette_gray);
            M_port->close();
        }else{
            QMessageBox::warning(this,"提示","强制休眠失败！！！");
        }
    }else{
        QMessageBox::warning(this,"错误提示","串口未打开");
    }
    ui->pushButton_8->setEnabled(true);
    return ;
}
//校准--外部FLASH测试指令
void BatteryManage::on_pushButton_9_clicked()
{
    ui->pushButton_9->setEnabled(false);
    if(M_port->isOpen())
    {
        sendVector[6] = 0x01;
        sendVector[7] = 0x11;
        sendVector[8] = 0x23;
        sendVector[9] = 0x01;       //无长度
        sendVector[10] = 0x55;
        receFrameCnt = 0;
        Pack_sendData(sendVector,11);
        delay_ms(1500);
        if(receFrameCnt > 0)
        {
            QMessageBox::about(this,"提示","外部FLASH测试指令发送成功");
        }else{
            QMessageBox::warning(this,"提示","发送外部FLASH测试指令失败！！！");
        }
    }else{
        QMessageBox::warning(this,"错误提示","串口未打开");
    }
    ui->pushButton_9->setEnabled(true);
    return ;
}
//校准--前端芯片测试
void BatteryManage::on_pushButton_frontChip_clicked()
{
    ui->pushButton_frontChip->setEnabled(false);
    if(M_port->isOpen())
    {
        sendVector[6] = 0x01;
        sendVector[7] = 0x12;
        sendVector[8] = 0x23;
        sendVector[9] = 0x01;       //无长度
        if(ui->pushButton_frontChip->text() == "开")
        {
            sendVector[10] = 0xFF;
        }else{
            sendVector[10] = 0x00;            
        }
        receFrameCnt = 0;
        Pack_sendData(sendVector,11);
        delay_ms(1500);
        if(receFrameCnt > 0)
        {
            QMessageBox::about(this,"提示","指令发送成功");
            if(ui->pushButton_frontChip->text() == "开")
                ui->pushButton_frontChip->setText("关");
            else
                ui->pushButton_frontChip->setText("开");
        }else{
            QMessageBox::warning(this,"提示","操作失败");
        }
    }else{
        QMessageBox::warning(this,"错误提示","串口未打开");
    }
    ui->pushButton_frontChip->setEnabled(true);
    return;
}
//参数设置--显示数据
void BatteryManage::on_pushButton_5_clicked()
{
    QSerialPortInfo portInfo(*M_port);
    if(!M_port->isOpen())
    {
        QMessageBox::warning(this,"错误提示","串口未打开");
        return ;
    }
    receFrameCnt = 0;
    pack_sendFrame(0x00,0x21,0x20,0x3E);//62:0x2021~0x203F    
    delay_ms(200);
    pack_sendFrame(0x00,0x40,0x20,0x3C);//60:0x2040~0x205D    
    delay_ms(200);
    pack_sendFrame(0x00,0x5E,0x20,0x30);//48:0x205E~0x2075    
    delay_ms(200);
    pack_sendFrame(0x00,0x76,0x20,0x72);//114:0x2076~0x20AE    
    delay_ms(200);    
    /*参数设置*/
    if(receFrameCnt >= 5)
    {
        QMessageBox::about(this,"提示","读取成功");
        parmaterShow();
    }else{
        QMessageBox::about(this,"错误提示","读取失败");
    }
    ifwrite = true;
}
//参数设置--发送给转接盒
void BatteryManage::on_pushButton_4_clicked()
{    
    ui->pushButton_4->setEnabled(false);
    int msTime = 3000;
    waitFlag = false;
    if(!M_port->isOpen())
    {
        QMessageBox::warning(this,"错误提示","串口未打开");
        ui->pushButton_4->setEnabled(true);
        return ;
    }
    if(timer->isActive())
    {
        timer->stop();
    }
    //判断表格每个item是否是null
    if(!ifwrite)
    {
        QMessageBox::warning(this,"错误提示","参数表未导入/未读取数据");
        ui->pushButton_4->setEnabled(true);
        return ;
    }
    k = 6;
    sendVector[k++] = 0x01;
    sendVector[k++] = 0x21;
    sendVector[k++] = 0x20;
    sendVector[k++] = 0xAA;             //170个字节数据
    //PACK设计容量    
    sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(3,1)->text().toFloat()*100) & 0xFF);
    sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(3,1)->text().toFloat()*100) >> 8);
    //最新满充容量
    sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(4,1)->text().toFloat()*100) & 0xFF);
    sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(4,1)->text().toFloat()*100) >> 8);
    //剩余容量
    sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(5,1)->text().toFloat()*100) & 0xFF);
    sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(5,1)->text().toFloat()*100) >> 8);
    //充电累计AH
    sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(6,1)->text().toFloat()) & 0xFF);
    sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(6,1)->text().toFloat()) >> 8);
    //SOC值（字节低位）、SOH值（字节高位）
    sendVector[k++] = ui->tableWidget->item(3,3)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(4,3)->text().toInt() & 0xFF;
    //循环次数
    sendVector[k++] = ui->tableWidget->item(5,3)->text().toInt() & 0xFF;
    sendVector[k++] = (ui->tableWidget->item(5,3)->text().toInt() >> 8) & 0xFF;
    //电芯类型（字节低位）、电芯串数（字节高位）
    if(cell_FontBox->currentText() == "聚合物")
    {
        sendVector[k++] = 0x00;
    }else if(cell_FontBox->currentText() == "三元")
    {
        sendVector[k++] = 0x01;
    }else if(cell_FontBox->currentText() == "铁锂")
    {
        sendVector[k++] = 0x02;
    }else if(cell_FontBox->currentText() == "钛酸锂")
    {
        sendVector[k++] = 0x03;
    }else{
        QMessageBox::warning(this,"错误提示","电芯类型仅选聚合物/三元/铁锂/钛酸锂");
        ui->pushButton_4->setEnabled(true);
        return ;
    }
    sendVector[k++] = ui->tableWidget->item(4,5)->text().toInt() & 0xFF;
    //前端芯片型号、电芯型号
    if(bq_FontBox->currentText() == "bq76940")
    {
        sendVector[k++] = 0x00;
    }else if(bq_FontBox->currentText() == "bq76930")
    {
        sendVector[k++] = 0x01;
    }else if(bq_FontBox->currentText() == "bq76920")
    {
        sendVector[k++] = 0x02;
    }else if(bq_FontBox->currentText() == "bq40z80")
    {
        sendVector[k++] = 0x03;
    }else if(bq_FontBox->currentText() == "bq40z50")
    {
        sendVector[k++] = 0x04;
    }else{
        QMessageBox::warning(this,"错误提示","前段芯片类型仅选bq76940/bq76930");
        ui->pushButton_4->setEnabled(true);
        return ;
    }
    sendVector[k++] = ui->tableWidget->item(6,5)->text().toInt() & 0xFF;
    //分流器阻值
    sendVector[k++] = ui->tableWidget->item(6,3)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(6,3)->text().toInt() >> 8 & 0xFF;
    //压差判断分界电压
    sendVector[k++] = ui->tableWidget->item(6,7)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(6,7)->text().toInt() >> 8 & 0xFF;
    //均衡开启的电压值
    sendVector[k++] = ui->tableWidget->item(8,1)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(8,1)->text().toInt() >> 8 & 0xFF;
    //均衡关闭的电压值
    sendVector[k++] = ui->tableWidget->item(9,1)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(9,1)->text().toInt() >> 8 & 0xFF;
    //均衡开启的压差值
    sendVector[k++] = ui->tableWidget->item(10,1)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(10,1)->text().toInt() >> 8 & 0xFF;
    //均衡关闭的压差值
    sendVector[k++] = ui->tableWidget->item(11,1)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(11,1)->text().toInt() >> 8 & 0xFF;
    //均衡门限开启时间
    sendVector[k++] = ui->tableWidget->item(12,1)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(12,1)->text().toInt() >> 8 & 0xFF;
    //强制关机电压（字节高位）
    if(ui->tableWidget->item(3,7)->text().toInt() > 2700)
    {
        QMessageBox::warning(this,"错误提示","强制关机电压不得高于2700mV");
        ui->pushButton_4->setEnabled(true);
        return ;
    }
    sendVector[k++] = ui->tableWidget->item(3,7)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(3,7)->text().toInt() >> 8 & 0xFF;
    //静置休眠时间、静置休眠唤醒时间
    sendVector[k++] = ui->tableWidget->item(4,7)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(5,7)->text().toInt() & 0xFF;
    //功能使能、温度采集点数（字节高位）
    sendVector[k++] = ((ui->tableWidget->item(8,5)->text().toInt() & 0x01) +
                      ((ui->tableWidget->item(30,1)->text().toInt() & 0x01) << 1)+
                        ((ui->tableWidget->item(30,3)->text().toInt() & 0x01) << 2)+
                        ((ui->tableWidget->item(30,5)->text().toInt() & 0x01) << 3)+
                        ((ui->tableWidget->item(30,7)->text().toInt() & 0x01) << 4)) & 0xFF;
    sendVector[k++] = ui->tableWidget->item(9,5)->text().toInt() & 0xFF;
    //温度传感器类型、低电量报警    
    if(T_FontBox->currentText() == "10K")
    {
        sendVector[k++] = 0x00;
    }else if(T_FontBox->currentText() == "100K")
    {
        sendVector[k++] = 0x01;
    }else{
        QMessageBox::warning(this,"错误提示","温度传感器类型仅选10K/100K");
        ui->pushButton_4->setEnabled(false);
        return ;
    }
    sendVector[k++] = ui->tableWidget->item(11,5)->text().toInt() & 0xFF;

    //充电截止总电压
    sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(8,7)->text().toFloat()*10) & 0xFF);
    sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(8,7)->text().toFloat()*10) >> 8);
    //放电恢复总电压
    sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(9,7)->text().toFloat()*10) & 0xFF);
    sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(9,7)->text().toFloat()*10) >> 8);
    //充电短路电流值    
    if(ui->tableWidget->item(10,7)->text().contains('-'))
    {
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(10,7)->text().remove('-').toFloat()*10-1) & 0xFF);
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(10,7)->text().remove('-').toFloat()*10-1) >> 8);
    }else{
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(10,7)->text().toFloat()*10) & 0xFF);
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(10,7)->text().toFloat()*10) >> 8);
    }
    //放电短路电流值
    if(ui->tableWidget->item(11,7)->text().contains('-'))
    {
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(11,7)->text().remove('-').toFloat()*10-1) & 0xFF);
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(11,7)->text().remove('-').toFloat()*10-1) >> 8);
    }else{
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(11,7)->text().toFloat()*10) & 0xFF);
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(11,7)->text().toFloat()*10) >> 8);
    }
    //充电温差过大、放电温差过大
    if(ui->tableWidget->item(12,5)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(12,5)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(12,5)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(12,7)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(12,7)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(12,7)->text().toInt() & 0xFF;
    }
    //自放电启动时间
    sendVector[k++] = ui->tableWidget->item(14,1)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(14,1)->text().toInt() >> 8 & 0xFF;
    //自放电开启电压
    sendVector[k++] = ui->tableWidget->item(14,3)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(14,3)->text().toInt() >> 8 & 0xFF;
    //自放电截止电压
    sendVector[k++] = ui->tableWidget->item(14,5)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(14,5)->text().toInt() >> 8 & 0xFF;
    //电池高温永久失效、电池低温永久失效
    if(ui->tableWidget->item(8,3)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(8,3)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(8,3)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(9,3)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(9,3)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(9,3)->text().toInt() & 0xFF;
    }
    //单体过压永久失效
    sendVector[k++] = ui->tableWidget->item(10,3)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(10,3)->text().toInt() >> 8 & 0xFF;
    //单体欠压永久失效
    sendVector[k++] = ui->tableWidget->item(11,3)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(11,3)->text().toInt() >> 8 & 0xFF;
    //电池压差过大永久失效
    sendVector[k++] = ui->tableWidget->item(12,3)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(12,3)->text().toInt() >> 8 & 0xFF;

    //单体过压告警值
    sendVector[k++] = ui->tableWidget->item(16,1)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(16,1)->text().toInt() >> 8 & 0xFF;
    //单体过压告警恢复值
    sendVector[k++] = ui->tableWidget->item(17,1)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(17,1)->text().toInt() >> 8 & 0xFF;
    //单体过压保护值
    sendVector[k++] = ui->tableWidget->item(18,1)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(18,1)->text().toInt() >> 8 & 0xFF;
    //单体过压保护恢复值
    sendVector[k++] = ui->tableWidget->item(19,1)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(19,1)->text().toInt() >> 8 & 0xFF;
    //单体过压保护延迟时间（字节高位）、单体过压保护恢复次数（字节低位）
    sendVector[k++] = ui->tableWidget->item(20,1)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(21,1)->text().toInt() & 0xFF;

    //单体欠压告警值
    sendVector[k++] = ui->tableWidget->item(16,3)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(16,3)->text().toInt() >> 8 & 0xFF;
    //单体欠压告警恢复值
    sendVector[k++] = ui->tableWidget->item(17,3)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(17,3)->text().toInt() >> 8 & 0xFF;
    //单体欠压保护值
    sendVector[k++] = ui->tableWidget->item(18,3)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(18,3)->text().toInt() >> 8 & 0xFF;
    //单体欠压保护恢复值
    sendVector[k++] = ui->tableWidget->item(19,3)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(19,3)->text().toInt() >> 8 & 0xFF;
    //单体欠压保护延迟时间（字节高位）、单体欠压保护恢复次数（字节低位）
    sendVector[k++] = ui->tableWidget->item(20,3)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(21,3)->text().toInt() & 0xFF;
    //单体压差过大告警值1
    sendVector[k++] = ui->tableWidget->item(16,5)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(16,5)->text().toInt() >> 8 & 0xFF;
    //单体压差过大告警恢复值1
    sendVector[k++] = ui->tableWidget->item(17,5)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(17,5)->text().toInt() >> 8 & 0xFF;
    //单体压差过大保护值1
    sendVector[k++] = ui->tableWidget->item(18,5)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(18,5)->text().toInt() >> 8 & 0xFF;
    //单体压差过大保护恢复值1
    sendVector[k++] = ui->tableWidget->item(19,5)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(19,5)->text().toInt() >> 8 & 0xFF;
    //单体压差过大保护延迟时间（字节高位）1、单体压差过大保护恢复次数（字节低位）1
    sendVector[k++] = ui->tableWidget->item(20,5)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(21,5)->text().toInt() & 0xFF;

    //单体压差过大告警值2
    sendVector[k++] = ui->tableWidget->item(16,7)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(16,7)->text().toInt() >> 8 & 0xFF;
    //单体压差过大告警恢复值2
    sendVector[k++] = ui->tableWidget->item(17,7)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(17,7)->text().toInt() >> 8 & 0xFF;
    //单体压差过大保护值2
    sendVector[k++] = ui->tableWidget->item(18,7)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(18,7)->text().toInt() >> 8 & 0xFF;
    //单体压差过大保护恢复值2
    sendVector[k++] = ui->tableWidget->item(19,7)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(19,7)->text().toInt() >> 8 & 0xFF;
    //单体压差过大保护延迟时间（字节高位）2、单体压差过大保护恢复次数（字节低位）2
    sendVector[k++] = ui->tableWidget->item(20,7)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(21,7)->text().toInt() & 0xFF;

    //充电过流告警值
    if(ui->tableWidget->item(23,1)->text().contains('-'))
    {
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(23,1)->text().remove('-').toFloat()*10-1) & 0xFF);
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(23,1)->text().remove('-').toFloat()*10-1) >> 8);
    }else{
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(23,1)->text().toFloat()*10) & 0xFF);
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(23,1)->text().toFloat()*10) >> 8);
    }
    //充电过流告警恢复值
    if(ui->tableWidget->item(24,1)->text().contains('-'))
    {
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(24,1)->text().remove('-').toFloat()*10-1) & 0xFF);
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(24,1)->text().remove('-').toFloat()*10-1) >> 8);
    }else{
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(24,1)->text().toFloat()*10) & 0xFF);
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(24,1)->text().toFloat()*10) >> 8);
    }
    //充电过流保护值
    if(ui->tableWidget->item(25,1)->text().contains('-'))
    {
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(25,1)->text().remove('-').toFloat()*10-1) & 0xFF);
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(25,1)->text().remove('-').toFloat()*10-1) >> 8);
    }else{
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(25,1)->text().toFloat()*10) & 0xFF);
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(25,1)->text().toFloat()*10) >> 8);
    }
    //充电过流保护恢复值
    if(ui->tableWidget->item(26,1)->text().contains('-'))
    {
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(26,1)->text().remove('-').toFloat()*10-1) & 0xFF);
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(26,1)->text().remove('-').toFloat()*10-1) >> 8);
    }else{
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(26,1)->text().toFloat()*10) & 0xFF);
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(26,1)->text().toFloat()*10) >> 8);
    }
    //充电过流保护延迟时间（字节高位）、充电过流保护恢复次数（字节低位）
    if(ui->tableWidget->item(27,1)->text().contains('-'))
    {
        sendVector[k++] = (ui->tableWidget->item(27,1)->text().toInt() & 0xFF) | 0x80;
    }else{
        sendVector[k++] = ui->tableWidget->item(27,1)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(28,1)->text().contains('-'))
    {
        sendVector[k++] = (ui->tableWidget->item(28,1)->text().toInt() & 0xFF) | 0x80;
    }else{
        sendVector[k++] = ui->tableWidget->item(28,1)->text().toInt() & 0xFF;
    }

    //放电过流告警值
    if(ui->tableWidget->item(23,3)->text().contains('-'))
    {
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(23,3)->text().remove('-').toFloat()*10-1) & 0xFF);
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(23,3)->text().remove('-').toFloat()*10-1) >> 8);
    }else{
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(23,3)->text().toFloat()*10) & 0xFF);
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(23,3)->text().toFloat()*10) >> 8);
    }
    //放电过流告警恢复值
    if(ui->tableWidget->item(24,3)->text().contains('-'))
    {
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(24,3)->text().remove('-').toFloat()*10-1) & 0xFF);
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(24,3)->text().remove('-').toFloat()*10-1) >> 8);
    }else{
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(24,3)->text().toFloat()*10) & 0xFF);
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(24,3)->text().toFloat()*10) >> 8);
    }
    //放电过流保护值
    if(ui->tableWidget->item(25,3)->text().contains('-'))
    {
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(25,3)->text().remove('-').toFloat()*10-1) & 0xFF);
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(25,3)->text().remove('-').toFloat()*10-1) >> 8);
    }else{
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(25,3)->text().toFloat()*10) & 0xFF);
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(25,3)->text().toFloat()*10) >> 8);
    }
    //放电过流保护恢复值
    if(ui->tableWidget->item(26,3)->text().contains('-'))
    {
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(26,3)->text().remove('-').toFloat()*10-1) & 0xFF);
        sendVector[k++] =static_cast<quint8>(~static_cast<int>(ui->tableWidget->item(26,3)->text().remove('-').toFloat()*10-1) >> 8);
    }else{
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(26,3)->text().toFloat()*10) & 0xFF);
        sendVector[k++] = static_cast<quint8>(static_cast<int>(ui->tableWidget->item(26,3)->text().toFloat()*10) >> 8);
    }
    //放电过流保护延迟时间（字节高位）、放电过流保护恢复次数（字节低位）
    if(ui->tableWidget->item(27,3)->text().contains('-'))
    {
        sendVector[k++] = (ui->tableWidget->item(27,3)->text().toInt() & 0xFF) | 0x80;
    }else{
        sendVector[k++] = ui->tableWidget->item(27,3)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(28,3)->text().contains('-'))
    {
        sendVector[k++] = (ui->tableWidget->item(28,3)->text().toInt() & 0xFF) | 0x80;
    }else{
        sendVector[k++] = ui->tableWidget->item(28,3)->text().toInt() & 0xFF;
    }
    //充电高温告警值（字节高位）、充电高温告警恢复值（字节低位）
    if(ui->tableWidget->item(31,1)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(31,1)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(31,1)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(32,1)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(31,1)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(32,1)->text().toInt() & 0xFF;
    }
    //充电高温保护值（字节高位）、充电高温保护恢复值（字节低位）
    if(ui->tableWidget->item(33,1)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(33,1)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(33,1)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(34,1)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(34,1)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(34,1)->text().toInt() & 0xFF;
    }
    //充电高温保护延迟时间（字节高位）、充电高温保护恢复次数（字节低位）
    sendVector[k++] = ui->tableWidget->item(35,1)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(36,1)->text().toInt() & 0xFF;
    //充电低温告警值（字节高位）、充电低温告警恢复值（字节低位）
    if(ui->tableWidget->item(31,3)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(31,3)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(31,3)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(32,3)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(32,3)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(32,3)->text().toInt() & 0xFF;
    }

    //充电低温保护值（字节高位）、充电低温保护恢复值（字节低位）
    if(ui->tableWidget->item(33,3)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(33,3)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(33,3)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(34,3)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(34,3)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(34,3)->text().toInt() & 0xFF;
    }
    //充电低温保护延迟时间（字节高位）、充电低温保护恢复次数（字节低位）
    sendVector[k++] = ui->tableWidget->item(35,3)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(36,3)->text().toInt() & 0xFF;

    //放电高温告警值（字节高位）、放电高温告警恢复值（字节低位）
    if(ui->tableWidget->item(38,1)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(38,1)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(38,1)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(39,1)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(39,1)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(39,1)->text().toInt() & 0xFF;
    }
    //放电高温保护值（字节高位）、放电高温保护恢复值（字节低位）
    if(ui->tableWidget->item(40,1)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(40,1)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(40,1)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(41,1)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(41,1)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(41,1)->text().toInt() & 0xFF;
    }
    //放电高温保护延迟时间（字节高位）、放电高温保护恢复次数（字节低位）
    sendVector[k++] = ui->tableWidget->item(42,1)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(43,1)->text().toInt() & 0xFF;
    //放电低温告警值（字节高位）、放电低温告警恢复值（字节低位）
    if(ui->tableWidget->item(38,3)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(38,3)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(38,3)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(39,3)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(39,3)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(39,3)->text().toInt() & 0xFF;
    }
    //放电低温保护值（字节高位）、放电低温保护恢复值（字节低位）
    if(ui->tableWidget->item(40,3)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(40,3)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(40,3)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(41,3)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(41,3)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(41,3)->text().toInt() & 0xFF;
    }
    //放电低温保护延迟时间（字节高位）、放电低温保护恢复次数（字节低位）
    sendVector[k++] = ui->tableWidget->item(42,3)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(43,3)->text().toInt() & 0xFF;

    //充电MOS高温告警（字节高位）、充电MOS高温告警恢复值
    if(ui->tableWidget->item(38,5)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(38,5)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(38,5)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(39,5)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(39,5)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(39,5)->text().toInt() & 0xFF;
    }
    //充电MOS高温保护（字节高位）、充电MOS高温保护恢复值
    if(ui->tableWidget->item(40,5)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(40,5)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(40,5)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(41,5)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(41,5)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(41,5)->text().toInt() & 0xFF;
    }
    //充电MOS高温保护恢复时间、充电MOS高温保护恢复次数
    sendVector[k++] = ui->tableWidget->item(42,5)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(43,5)->text().toInt() & 0xFF;
    //充电MOS低温告警（字节高位）、充电MOS低温告警恢复值
    if(ui->tableWidget->item(38,7)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(38,7)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(38,7)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(39,7)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(39,7)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(39,7)->text().toInt() & 0xFF;
    }

    //充电MOS低温保护（字节高位）、充电MOS低温保护恢复值
    if(ui->tableWidget->item(40,7)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(40,7)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(40,7)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(41,7)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(41,7)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(41,7)->text().toInt() & 0xFF;
    }
    //充电MOS低温保护恢复时间、充电MOS低温保护恢复次数
    sendVector[k++] = ui->tableWidget->item(42,7)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(43,7)->text().toInt() & 0xFF;

    //放电MOS高温告警（字节高位）、放电MOS高温告警恢复值
    if(ui->tableWidget->item(23,5)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(23,5)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(23,5)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(24,5)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(24,5)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(24,5)->text().toInt() & 0xFF;
    }    
    //放电MOS高温保护（字节高位）、放电MOS高温保护恢复值
    if(ui->tableWidget->item(25,5)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(25,5)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(25,5)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(26,5)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(26,5)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(26,5)->text().toInt() & 0xFF;
    }    
    //放电MOS高温保护恢复时间、放电MOS高温保护恢复次数
    sendVector[k++] = ui->tableWidget->item(27,5)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(28,5)->text().toInt() & 0xFF;
    //放电MOS低温告警（字节高位）、放电MOS低温告警恢复值
    if(ui->tableWidget->item(23,7)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(23,7)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(23,7)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(24,7)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(24,7)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(24,7)->text().toInt() & 0xFF;
    }
    //放电MOS低温保护（字节高位）、放电MOS低温保护恢复值
    if(ui->tableWidget->item(25,7)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(25,7)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(25,7)->text().toInt() & 0xFF;
    }
    if(ui->tableWidget->item(26,7)->text().contains('-'))
    {
        sendVector[k++] = ~((ui->tableWidget->item(26,7)->text().remove('-').toInt() & 0xFF)-1);
    }else{
        sendVector[k++] = ui->tableWidget->item(26,7)->text().toInt() & 0xFF;
    }

    //放电MOS低温保护恢复时间、放电MOS低温保护恢复次数
    sendVector[k++] = ui->tableWidget->item(27,7)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(28,7)->text().toInt() & 0xFF;    
    Pack_sendData(sendVector,180);
    dataStartAddr.clear();
    QTimer::singleShot(msTime,&loopWRet,SLOT(quit()));
    loopWRet.exec();
    delay_ms(100);
    if(waitFlag && dataStartAddr.toUShort(nullptr,16) == 0x2021)
    {
        waitFlag = false;
    }else{
        QMessageBox::warning(this,"错误提示","第一帧写入失败");        
        logWin->addListData("dataStarAddr:"+dataStartAddr);
        logWin->addListData("waitFlag:"+QString::number(waitFlag));
        ui->pushButton_4->setEnabled(true);
        return ;
    }

    k = 6;
    sendVector[k++] = 0x01;
    sendVector[k++] = 0x76;
    sendVector[k++] = 0x20;
    sendVector[k++] = 0x72;             //114个字节数据      2076~20AE
    //飞控协议    
    if(FCP_FontBox->currentText() == "博鹰")
    {
        sendVector[k++] = 0x01;
    }else if(FCP_FontBox->currentText() == "博鹰接收")
    {
        sendVector[k++] = 0x02;
    }else if(FCP_FontBox->currentText() == "UAVCAN")
    {
        sendVector[k++] = 0x03;
    }else if(FCP_FontBox->currentText() == "极翼")
    {
        sendVector[k++] = 0x04;
    }

    for(int i=0;i<9;i++)
    {
        sendVector[k++] = 0x00;
    }

    //飞控版本号
    QByteArray flightVision = ui->tableWidget->item(1,1)->text().toLatin1();
    if(flightVision.size() <= 10)
    {
        for(int i=0;i<flightVision.size();i++)
        {
            sendVector[k++] = static_cast<quint8>(flightVision.at(i));
        }
        for(int i=0;i<10-flightVision.size();i++)
        {
            sendVector[k++] = 0x00;
        }
    }else{
        QMessageBox::warning(this,"错误提示","飞控版本号字符个数不能大于10个");
        ui->pushButton_4->setEnabled(true);
        return ;
    }
    //销售商编码
    QByteArray sellerCode = ui->tableWidget->item(0,3)->text().toLatin1();
    if(sellerCode.size() <= 10)
    {
        for(int i=0;i<sellerCode.size();i++)
        {
            sendVector[k++] = static_cast<quint8>(sellerCode.at(i));
        }
        for(int i=0;i<10-sellerCode.size();i++)
        {
            sendVector[k++] = 0x00;
        }
    }else{
        QMessageBox::warning(this,"错误提示","销售商编码字符个数不能大于10个");
        ui->pushButton_4->setEnabled(true);
        return ;
    }
    //客户编码
    QByteArray clientNum = ui->tableWidget->item(1,3)->text().toLatin1();
    if(clientNum.size() <= 10)
    {
        for(int i=0;i<clientNum.size();i++)
        {
            sendVector[k++] = static_cast<quint8>(clientNum.at(i));
        }
        for(int i=0;i<10-clientNum.size();i++)
        {
            sendVector[k++] = 0x00;
        }
    }else{
        QMessageBox::warning(this,"错误提示","客户编码字符个数不能大于10个");
        ui->pushButton_4->setEnabled(true);
        return ;
    }
    //电池厂商编码
    QByteArray batProCode = ui->tableWidget->item(2,3)->text().toLatin1();
    if(batProCode.size() <= 10)
    {
        for(int i=0;i<batProCode.size();i++)
        {
            sendVector[k++] = static_cast<quint8>(batProCode.at(i));
        }
        for(int i=0;i<10-batProCode.size();i++)
        {
            sendVector[k++] = 0x00;
        }
    }else{
        QMessageBox::warning(this,"错误提示","电池厂商编码字符个数不能大于10个");
        ui->pushButton_4->setEnabled(true);
        return ;
    }
    //电池包条形码
    int n = 0;
    QByteArray BatCode = ui->lineEdit_barCode->text().toLatin1();
    if(BatCode.size() < 16)
    {
        for(int i=0;i<16-BatCode.size();i++)
        {
            BatCode.prepend(' ');
        }
    }
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    //电池型号
    QByteArray batType = ui->tableWidget->item(0,5)->text().toLatin1();
    if(batType.size() <= 10)
    {
        for(int i=0;i<batType.size();i++)
        {
            sendVector[k++] = static_cast<quint8>(batType.at(i));
        }
        for(int i=0;i<10-batType.size();i++)
        {
            sendVector[k++] = 0x00;
        }
    }else{
        QMessageBox::warning(this,"错误提示","电池型号字符个数不能大于10个");
        ui->pushButton_4->setEnabled(true);
        return ;
    }
    //BMS编号
    QByteArray BMSNum = ui->tableWidget->item(1,5)->text().toLatin1();
    if(BMSNum.size() <= 10)
    {
        for(int i=0;i<BMSNum.size();i++)
        {
            sendVector[k++] = static_cast<quint8>(BMSNum.at(i));
        }
        for(int i=0;i<10-BMSNum.size();i++)
        {
            sendVector[k++] = 0x00;
        }
    }else{
        QMessageBox::warning(this,"错误提示","BMS编号字符个数不能大于10个");
        ui->pushButton_4->setEnabled(true);
        return ;
    }
    //BMS条形码
    n = 0;
    QByteArray BMSCode = ui->lineEdit_BMSCode_2->text().toLatin1();
    if(BMSCode.size() < 16)
    {
        for(int i=0;i<16-BMSCode.size();i++)
        {
            BMSCode.prepend(' ');
        }
    }
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    sendVector[k++] = static_cast<quint8>(BatCode.at(n++));
    //BMS生产日期
    QString BMSProDate = ui->tableWidget->item(0,7)->text();
    BMSProDate = BMSProDate.remove(0,2);//移除开头的20
    QStringList BMSProDateList = BMSProDate.split("/",QString::SkipEmptyParts);
    sendVector[k++] = BMSProDateList[0].toInt() & 0xFF;
    sendVector[k++] = BMSProDateList[1].toInt() & 0xFF;
    sendVector[k++] = BMSProDateList[2].toInt() & 0xFF;
    //电池生产日期
    QString BMSProDate1 = ui->tableWidget->item(1,7)->text();
    BMSProDate1 = BMSProDate1.remove(0,2);//移除开头的20
    QStringList BMSProDateList1 = BMSProDate1.split("/",QString::SkipEmptyParts);
    sendVector[k++] = BMSProDateList1[0].toInt() & 0xFF;
    sendVector[k++] = BMSProDateList1[1].toInt() & 0xFF;
    sendVector[k++] = BMSProDateList1[2].toInt() & 0xFF;
    //充电存储间隔设置
    sendVector[k++] = ui->tableWidget->item(31,5)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(31,5)->text().toInt() >> 8 & 0xFF;
    //放电存储间隔设置
    sendVector[k++] = ui->tableWidget->item(32,5)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(32,5)->text().toInt() >> 8 & 0xFF;
    //空闲存储间隔设置
    sendVector[k++] = ui->tableWidget->item(33,5)->text().toInt() & 0xFF;
    sendVector[k++] = ui->tableWidget->item(33,5)->text().toInt() >> 8 & 0xFF;

    Pack_sendData(sendVector,124);
    dataStartAddr.clear();
    QTimer::singleShot(msTime,&loopWRet,SLOT(quit()));
    loopWRet.exec();
    delay_ms(100);
    if(waitFlag && dataStartAddr.toUShort(nullptr,16) == 0x2076)
    {
        waitFlag = false;
        QMessageBox::about(this,"信息提示","数据写入成功");
    }else{
        QMessageBox::warning(this,"错误提示","数据写入失败");
        logWin->addListData("dataStarAddr:"+dataStartAddr);
        logWin->addListData("waitFlag:"+QString::number(waitFlag));
        ui->pushButton_4->setEnabled(true);        
        return ;
    }
    ui->pushButton_4->setEnabled(true);
    return ;
}
//参数设置--生成Excel
void BatteryManage::on_pushButton_3_clicked()
{    
    QString filename = QFileDialog::getSaveFileName(ui->tableWidget,"保存路径","/","Excel 文件(*.xlsx)");
    if(filename != nullptr)
    {
        //获取数据的行列数
        int colcount = ui->tableWidget->columnCount();
        int rowcount = ui->tableWidget->rowCount();
        Document xlsx;
        Format format;
        format.setHorizontalAlignment(Format::AlignHCenter);

        for(int i=0;i<rowcount;i++)
        {
            for(int j=0;j<colcount;j++)
            {
                if(i==0 && j==1)
                {
                    xlsx.write(i+1,j+1,FCP_FontBox->currentText(),format);
                    xlsx.setColumnWidth(j+1,30);
                }else if(i== 3 && j == 5)
                {
                    xlsx.write(i+1,j+1,cell_FontBox->currentText(),format);
                    xlsx.setColumnWidth(j+1,30);
                }else if(i == 5 && j == 5)
                {
                    xlsx.write(i+1,j+1,bq_FontBox->currentText(),format);
                    xlsx.setColumnWidth(j+1,30);
                }else if(i == 10 && j == 5)
                {
                    xlsx.write(i+1,j+1,T_FontBox->currentText(),format);
                    xlsx.setColumnWidth(j+1,30);
                }else{
                xlsx.write(i+1,j+1,(ui->tableWidget->item(i,j) ? ui->tableWidget->item(i,j)->text():""),format);
                xlsx.setColumnWidth(j+1,30);
                }
            }
        }
        xlsx.saveAs(filename);
        QMessageBox::about(this,"提示","数据已生存Excel文件");
    }
}
//参数设置--导入Excel
void BatteryManage::on_pushButton_2_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(nullptr,"Open File","/","Excel 文件(*.xlsx)");
    if(fileName == nullptr)
    {
        return;
    }
    Document xlsx(fileName);
    Workbook* workbook = xlsx.workbook();
    Worksheet* worksheet = static_cast<Worksheet*>(workbook->sheet(0));
    QString value;
    for(int i=1;i <= worksheet->dimension().rowCount();i++)
    {
        for(int j=1;j <= worksheet->dimension().columnCount();j++)
        {
            Cell* cell = worksheet->cellAt(i,j);
            value = cell->value().toString();
            if(i-1 == 0 && j-1 == 1)
            {
                FCP_FontBox->setCurrentText(value);
            }
            if(i-1== 3 && j-1 == 5)
            {
                cell_FontBox->setCurrentText(value);
            }else if(i-1 == 5 && j-1 == 5)
            {
                bq_FontBox->setCurrentText(value);
            }else if(i-1 == 10 && j-1 == 5)
            {
                T_FontBox->setCurrentText(value);
            }else{
                ui->tableWidget->setItem(i-1,j-1,new QTableWidgetItem(value));
                ui->tableWidget->item(i-1,j-1)->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
            }
        }
    }
    //BMS生产日期
    QString BMSMake = QDateTime::currentDateTime().toString("yyyy/MM/dd");
    ui->tableWidget->setItem(0,7,new QTableWidgetItem(BMSMake));
    //电池包生产日期
    ui->tableWidget->setItem(1,7,new QTableWidgetItem(BMSMake));
    ifwrite = true;
    QMessageBox::about(this,"提示","导入Excel成功");
}
//参数设置--表格是否编辑
void BatteryManage::on_checkBox_stateChanged(int )
{
    if(ui->checkBox->checkState() == Qt::Checked)
    {
        /*Editing starts for all above actions*/
        ui->tableWidget->setEditTriggers(QAbstractItemView::AllEditTriggers);
    }else {
        /*No editing possible*/
        ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);//不可编辑状态
    }
}
//历史数据--阈值存取
void BatteryManage::history_ThresholdVlaueSave()
{
    QVector<QString> hisTemp;
    for(int i=0;i<dataVector.size();i=i+2)
    {
        hisTemp << dataVector.at(i+1)+dataVector.at(i);
    }
    dataVector.clear();
    hisVector.clear();
    for(int i=0;i<hisTemp.size();i++)
    {
        hisVector[i] = hisTemp.at(i);
        //logWin->addListData("历史阈值：["+QString::number(i)+"]:"+hisTemp.at(i));
    }
}
//历史数据--阈值读取
void BatteryManage::history_ThresholdVlaueRead()
{    
    int n = 0;
    int row = 0;
    int col = 0;
    //1.历史最大充电电压及其所在循环数
    for(int i=0;i<10;i++)
    {        
        ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(hisVector[n++].toUShort(nullptr,16))));
    }
    //2.历史最小放电电压及其所在循环数
    col = 0;
    row++;
    for(int i=0;i<10;i++)
    {        
        ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(hisVector[n++].toUShort(nullptr,16))));
    }
    //3.历史最大压差及其所在循环数
    col = 0;
    row++;
    for(int i=0;i<10;i++)
    {        
        ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(hisVector[n++].toUShort(nullptr,16))));
    }
    //4.历史最大放电电流及其所在循环数
    col = 0;
    row++;
    for(int i=0;i<5;i++)
    {
        if(hisVector[n].toUShort(nullptr,16) > 0x8000){
            ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(static_cast<double>(~(0xFFFF-hisVector[n].toInt(nullptr,16)))/10,'f',1)));
        }else{
            ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(static_cast<double>(hisVector[n].toUShort(nullptr,16))/10,'f',1)));
        }
        n++;
        ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(hisVector[n++].toUShort(nullptr,16))));
    }
    //5.历史最大充电电流及其所在循环数
    col = 0;
    row++;
    for(int i=0;i<5;i++)
    {        
        if(hisVector[n].toUShort(nullptr,16) > 0x8000){
            ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(static_cast<double>(~(0xFFFF-hisVector[n].toInt(nullptr,16)))/10,'f',1)));
        }else{
            ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(static_cast<double>(hisVector[n].toUShort(nullptr,16))/10,'f',1)));
        }
        n++;
        ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(hisVector[n++].toUShort(nullptr,16))));
    }
    //6.历史最大充电温度及其所在循环数
    col = 0;
    row++;
    for(int i=0;i<5;i++)
    {        
        if(hisVector[n].toUShort(nullptr,16) > 0x8000){
            ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(~(0xFFFF-hisVector[n].toInt(nullptr,16)))));
        }else{
            ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(hisVector[n].toUShort(nullptr,16))));
        }
        n++;
        ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(hisVector[n++].toUShort(nullptr,16))));
    }
    //7.历史最小充电温度及其所在循环数
    col = 0;
    row++;
    for(int i=0;i<5;i++)
    {
        if(hisVector[n].toUShort(nullptr,16) > 0x8000){
            ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(~(0xFFFF-hisVector[n].toInt(nullptr,16)))));
        }else{
            ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(hisVector[n].toUShort(nullptr,16))));
        }
        n++;
        ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(hisVector[n++].toUShort(nullptr,16))));
    }
    //8.历史最大放电温度及其所在循环数
    col = 0;
    row++;
    for(int i=0;i<5;i++)
    {        
        if(hisVector[n].toUShort(nullptr,16) > 0x8000){
            ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(~(0xFFFF-hisVector[n].toInt(nullptr,16)))));
        }else{
            ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(hisVector[n].toUShort(nullptr,16))));
        }
        n++;
        ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(hisVector[n++].toUShort(nullptr,16))));
    }
    //9.历史最小放电温度及其所在循环数
    col = 0;
    row++;
    for(int i=0;i<5;i++)
    {        
        if(hisVector[n].toUShort(nullptr,16) > 0x8000){
            ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(~(0xFFFF-hisVector[n].toInt(nullptr,16)))));
        }else{
            ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(hisVector[n].toUShort(nullptr,16))));
        }
        n++;
        ui->tableWidget_his->setItem(row,col++,new QTableWidgetItem(QString::number(hisVector[n++].toUShort(nullptr,16))));
    }
}
//历史数据--读取历史数据
void BatteryManage::send_historyData_Readframe(quint8 cmd, quint16 addr)
{    
    QSerialPortInfo portInfo(*M_port);
    sendVector[6] = cmd;
    sendVector[7] = addr & 0xFF;
    sendVector[8] = addr >> 8;
    sendVector[9] = 0x00;
    QVector<quint8> temp(10);
    for(int i=0;i<10;i++)
    {
        temp[i] = sendVector[i];
    }
    temp.remove(0,6);
    quint16 crcCal = calCRC16(temp,4);
    quint8 crc1 = crcCal >> 8;
    quint8 crc2 = (crcCal & 0xFF);
    sendVector[10] = crc2;
    sendVector[11] = crc1;
    sendData.clear();
    for(int i=0;i<12;i++)
    {
        sendData[i] = static_cast<char>(sendVector[i]);
    }
    if(portInfo.isBusy())
    M_port->write(sendData);
}
//历史数据--历史数组转存
void BatteryManage::history_analyise()
{
    QList<QString> tempList;    //存储1个字节
    for(int i=0;i<dataVector.size();i++)
    {
        tempList << dataVector.at(i);
    }
    dataVector.clear();
    //清空链表
    hisList.clear();
    /*不管是一个温度还是两个温度，累加和计算都是固定64长度*/
    if((tempList.at(64).toUShort(nullptr,16) & 0xFF) != calNum(tempList,64))        //累加和校验计算并比较
    {
        hisList << "校验错误";
        if(hisfile.isOpen())                                                        //只有文件是打开状态才允许发送信号
        emit his_receFrameSignal();
        return;
    }
    //循环数(2byte)
    int temp = 0;
    hisList << QString::number((tempList.at(temp+1)+tempList.at(temp)).toUShort(nullptr,16) & 0xFF);
    //记录序号（4byte）
    temp += 2;
    hisList << QString::number((tempList.at(temp+3)+tempList.at(temp+2)+tempList.at(temp+1)+tempList.at(temp)).toUInt(nullptr,16));
    //BMS时间（6byte）
    temp += 4;   
    hisList << "20"+QString::number(tempList.at(temp).toUShort(nullptr,16))+"/"+
                QString::number(tempList.at(temp+1).toUShort(nullptr,16))+"/"+
                QString::number(tempList.at(temp+2).toUShort(nullptr,16))+" "+
                QString::number(tempList.at(temp+3).toUShort(nullptr,16))+":"+
                QString::number(tempList.at(temp+4).toUShort(nullptr,16))+":"+
                QString::number(tempList.at(temp+5).toUShort(nullptr,16));
    //工作模式（1byte）
    temp += 6;
    switch (tempList.at(temp).toUShort(nullptr,16)) {
        case 0:hisList << "复位";break;
        case 1:hisList << "充电";break;
        case 2:hisList << "放电";break;
        case 3:hisList << "静止";break;
        case 4:hisList << "均衡";break;
        case 5:hisList << "自放电";break;
        case 6:hisList << "休眠";break;
        case 7:hisList << "掉电";break;
    default:hisList << "模式错误";break;
    }
    //SOC(1byte)、SOH(1byte)
    temp += 1;
    hisList << QString::number(tempList.at(temp).toUShort(&ok,16));
    hisList << QString::number(tempList.at(temp+1).toUShort(&ok,16));
    //电流(2byte)
    temp += 2;
    if((tempList.at(temp+1)+tempList.at(temp)).toUShort(&ok,16) > 0x8000)
    {
        hisList << QString::number(static_cast<double>(~(0xFFFF-(tempList.at(temp+1)+tempList.at(temp)).toUShort(&ok,16)))/10,'f',1);
    }else{
        hisList << QString::number(static_cast<double>((tempList.at(temp+1)+tempList.at(temp)).toUShort(&ok,16))/10,'f',1);
    }
    //故障代码(8byte)
    temp += 2;
    hisList << "0x"+tempList.at(temp+7)+tempList.at(temp+6)+tempList.at(temp+5)+tempList.at(temp+4)
               +tempList.at(temp+3)+tempList.at(temp+2)+tempList.at(temp+1)+tempList.at(temp);
    QString temp1 = tempList.at(temp+1)+tempList.at(temp);
    QString temp2 = tempList.at(temp+3)+tempList.at(temp+2);
    QString temp3 = tempList.at(temp+4);
    //电池均衡状态(2byte)
    temp += 8;
    hisList << "0x"+tempList.at(temp+1)+tempList.at(temp);
    //电池模块电压（28byte）        
    temp += 2;
    for(int i=0;i<cellNum*2;i=i+2)
    {
        hisList << QString::number((tempList.at(temp+i+1)+tempList.at(temp+i)).toUShort(nullptr,16));
    }
    for(int i=0;i<28-cellNum*2;i=i+2)
    {
        hisList << "None";
    }
    //电池温度(2Byte)
    temp += cellNum*2;
    switch (TempNum) {
        case 1:hisList << ((tempList.at(temp).toUShort(nullptr,16)) > 0x80 ? QString::number(~(0xFF-(tempList.at(temp).toUShort(nullptr,16)))) :
                                                                             QString::number(tempList.at(temp).toUShort(nullptr,16)));
                hisList << "";
                hisList << "";
                hisList << "";
            break;
        case 2:hisList << ((tempList.at(temp).toUShort(nullptr,16)) > 0x80 ? QString::number(~(0xFF-(tempList.at(temp).toUShort(nullptr,16)))) :
                                                                             QString::number(tempList.at(temp).toUShort(nullptr,16)));
            hisList << ((tempList.at(temp+1).toUShort(nullptr,16)) > 0x80 ? QString::number(~(0xFF-(tempList.at(temp+1).toUShort(nullptr,16)))) :
                                                                                QString::number(tempList.at(temp+1).toUShort(nullptr,16)));
            hisList << "";
            hisList << "";
            break;
        case 3:hisList << ((tempList.at(temp).toUShort(nullptr,16)) > 0x80 ? QString::number(~(0xFF-(tempList.at(temp).toUShort(nullptr,16)))) :
                                                                             QString::number(tempList.at(temp).toUShort(nullptr,16)));
            hisList << ((tempList.at(temp+1).toUShort(nullptr,16)) > 0x80 ? QString::number(~(0xFF-(tempList.at(temp+1).toUShort(nullptr,16)))) :
                                                                                QString::number(tempList.at(temp+1).toUShort(nullptr,16)));
            hisList << ((tempList.at(temp+2).toUShort(nullptr,16)) > 0x80 ? QString::number(~(0xFF-(tempList.at(temp+2).toUShort(nullptr,16)))) :
                                                                            QString::number(tempList.at(temp+2).toUShort(nullptr,16)));
            hisList << "";
            break;
        case 4:hisList << ((tempList.at(temp).toUShort(nullptr,16)) > 0x80 ? QString::number(~(0xFF-(tempList.at(temp).toUShort(nullptr,16)))) :
                                                                             QString::number(tempList.at(temp).toUShort(nullptr,16)));
            hisList << ((tempList.at(temp+1).toUShort(nullptr,16)) > 0x80 ? QString::number(~(0xFF-(tempList.at(temp+1).toUShort(nullptr,16)))) :
                                                                                QString::number(tempList.at(temp+1).toUShort(nullptr,16)));
            hisList << ((tempList.at(temp+2).toUShort(nullptr,16)) > 0x80 ? QString::number(~(0xFF-(tempList.at(temp+2).toUShort(nullptr,16)))) :
                                                                            QString::number(tempList.at(temp+2).toUShort(nullptr,16)));
            hisList << ((tempList.at(temp+3).toUShort(nullptr,16)) > 0x80 ? QString::number(~(0xFF-(tempList.at(temp+3).toUShort(nullptr,16)))) :
                                                                            QString::number(tempList.at(temp+3).toUShort(nullptr,16)));
            break;
        default:
            break;
    }
    //故障代码(8byte)    
        //bit:0~36
    for(int i=0;i<16;i++)
    {
        if(temp1.toUShort(nullptr,16) & (1 << i))
        {
            hisList << "有故障";
        }else{
            hisList << "None";
        }
    }
    for(int i=0;i<16;i++)
    {
        if(temp2.toUShort(nullptr,16) & (1 << i))
        {
            hisList << "有故障";
        }else{
            hisList << "None";
        }
    }
    for(int i=0;i<5;i++)
    {
        if((temp3.toUShort(nullptr,16) & 0xFF) & (1 << i))
        {
            hisList << "有故障";
        }else{
            hisList << "None";
        }
    }

    if(hisfile.isOpen())                                    //只有文件是打开状态才允许发送信号
    emit his_receFrameSignal();

}
//历史数据--保存每一条接收到的历史数据
void BatteryManage::his_writeToExcel()
{
    if(finishFlag)          //如果收到历史数据读取完成标志
    {        
        ui->progressBar_hisdata->setValue(hisDataNum);      //设置历史数据进度条的值
        ui->pushButton_savehistory->setText("读取并保存");
        hisfile.close();
        ui->spinBox->setReadOnly(false);
//        if(QMessageBox::question(this,"完成","文件已导出，是否打开",QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
//        {
//            QDesktopServices::openUrl("file:///"+QDir::fromNativeSeparators(ui->lineEdit_savehistory->text()));
//        }
        if(!timer->isActive())
            timer->start();
        if(!timerProcessData->isActive())
            timerProcessData->start();
        return ;
    }
    /*设置数据区*/
    //设置进度条
    hisFrameCnt++;
    ui->progressBar_hisdata->setValue(hisFrameCnt);         //设置历史数据进度条的值

    QList<QString> tempList;
    for(int i=0;i<hisList.size();i++)
    {
        tempList << hisList.at(i);
    }
    for(int j=0;j<tempList.size();j++)
    {
        in << tempList.at(j) << ",";
    }
    in << endl;
    if(hisFrameCnt == hisDataNum)                           //未收到结束帧但数据帧已全部接收
    {
        finishFlag = true;
        emit his_receFrameSignal();        
    }

}
//历史数据--保存历史数据
void BatteryManage::on_pushButton_savehistory_clicked()
{
    quint16 cycle;
    if(upTimer->isActive())                                            //如果正在升级
    {
        QMessageBox::warning(this,"错误提示","请先停止升级");
        return ;
    }
    if(ui->pushButton_saveReal->text() == "停止保存")                   //如果实时数据仍在保存
    {
        QMessageBox::warning(this,"提示","请先停止保存实时数据");
        return ;
    }
    if(!M_port->isOpen())                                               //如果串口未打开
    {
        QMessageBox::warning(this,"错误提示","串口未打开");
        return ;
    }
    if(ui->pushButton_savehistory->text() == "停止保存")                //如果正在读取历史数据
    {        
        /*发送请求停止帧*/
        if(ui->spinBox->text().toUShort() == 0)
        {
            send_historyData_Readframe(0x01,0x4000);                    //读取全部数据
        }else{
            send_historyData_Readframe(0x01,0x4000+ui->spinBox->text().toUShort());
        }
        ui->pushButton_savehistory->setText("读取并保存");               //设置按钮控件文本显示
        hisfile.close();                                                //关闭文件
        ui->spinBox->setReadOnly(false);                                //设置读取循环数可改
        if(!timer->isActive())
            timer->start();
        if(!timerProcessData->isActive())
            timerProcessData->start();

        return ;
    }
    //历史数据列标题初始
    if(colheadFlag)                                 //设置历史数据列标题只初始化一次
    {
        colheadFlag = false;
        colHeadList << "循环数" << "记录序号" << "BMS时间" << "工作模式" << "SOC" << "SOH" << "电流" << "故障代码" <<"电池均衡状态" << "电池V1" << "电池V2"
                    << "电池V3" << "电池V4" << "电池V5" << "电池V6" << "电池V7" << "电池V8" << "电池V9" << "电池V10" << "电池V11"
                    << "电池V12" << "电池V13" << "电池V14" << "T1" << "T2" << "T3" << "T4" << "单体过压告警" << "单体过压保护" << "单体欠压告警"
                    << "单体欠压保护" << "压差过大告警H" << "压差过大保护H" << "压差过大告警L" << "压差过大保护L" << "充电过流告警" << "充电过流保护"
                    << "充电短路" << "放电过流告警" << "放电过流保护" << "放电短路" << "充电高温告警" << "充电高温保护" << "充电低温告警" << "充电低温保护"
                    << "充电温差过大" << "放电高温告警" << "放电高温保护" << "放电低温告警" << "放电低温保护" << "放电温差过大" << "充电MOS高温告警"
                    << "充电MOS高温保护" << "放电MOS高温告警" << "放电MOS高温保护" << "当前温度大于电芯最高保存温度" << "当前温度低于电芯最高保存温度"
                    << "SOC过低告警" << "电池高温导致永久失效" << "电池低温导致永久失效" << "电池过压导致永久失效" << "电池欠压导致永久失效"
                    << "压差过大导致永久失效" << "非原装充电器充电";
        //连接发送信号与槽函数
        connect(this,&BatteryManage::his_receFrameSignal,this,&BatteryManage::his_writeToExcel);
    }
    if(timer->isActive())
        timer->stop();                              //关闭发送问询帧的定时器
    if(timerProcessData->isActive())
        timerProcessData->stop();                   //实时数据显示定时器关闭
    hisDataNum = -1;                                //收到回复帧的标志初始化
    hisFrameCnt = 0;                                //历史数据帧数计数变量
    finishFlag = false;                             //读取完成标志复位
    ui->progressBar_hisdata->reset();               //复位进度条
    in.reset();

    //选择保存路径
    QString filepath = QFileDialog::getSaveFileName(this,"选择保存文件路径","/","文件(*.csv)");    
    ui->spinBox->setReadOnly(true);                 //读取循环只读不可修改状态
    if(filepath != nullptr)
    {
        //显示路径
        ui->lineEdit_savehistory->setText(filepath);
        //设置进度条与excel列标题
        hisfile.setFileName(filepath);
        if (hisfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            in.setDevice(&hisfile);
            for(int i=0;i<colHeadList.size();i++)
            {
                in << colHeadList.at(i) << ",";
            }
            in << endl;
        }
        //读取历史数据
        cycle = ui->spinBox->text().toUShort();     //获取需要读取多少个循环
        if(cycle == 0)
        {
            send_historyData_Readframe(0x00,0x4000);//读取全部数据
        }else{
            send_historyData_Readframe(0x00,0x4000+cycle);
        }
    }else{
        ui->spinBox->setReadOnly(false);
        if(!timer->isActive())
            timer->start();
        if(!timerProcessData->isActive())
            timerProcessData->start();
        return ;
    }

    //正在读取。。。
    ui->lineEdit_RNum->setText("正在读取。。。");
    /*检测电池是否收到*/
    QTimer::singleShot(60000,&loop,SLOT(quit()));
    loop.exec();
    ui->lineEdit_RNum->setText("");
    delay_ms(100);

    ui->lineEdit_RNum->setText(QString::number(hisDataNum)+"帧");

    if(hisDataNum == -1)
    {
        QMessageBox::warning(this,"错误提示","未收到回复帧,需重新读取");
        if(hisfile.isOpen())
            hisfile.close();        
        return;
    }
    //只有通讯上改变按钮文本
    ui->pushButton_savehistory->setText("停止保存");
}
//实时数据--实时链表的转入
void BatteryManage::history_realDataArrayOtherSave()
{    
    hisList.clear();
    //当前时间
    QDateTime dateTime = QDateTime::currentDateTime();
    hisList << dateTime.toString("yyyy-MM-dd hh:mm:ss dddd");
    //告警信息及状态
    hisList << QString::number(totalVector[addr("200A")].toUShort(&ok,16))+QString::number(totalVector[addr("200B")].toUShort(&ok,16))+
                    QString::number(totalVector[addr("200C")].toUShort(&ok,16))+QString::number(totalVector[addr("200D")].toUShort(&ok,16));
    //总电压
    hisList <<QString::number(ui->lineEdit_totalV->text().toDouble(),'f',1);
    //平均电压
    hisList << ui->lineEdit_averageV->text();
    //最小输出电压
    hisList << ui->lineEdit_minOutputV->text();
    //最大输出电压
    hisList << ui->lineEdit_maxOutputV->text();
    //最小输出电压通道（字节高位）
    hisList << ui->lineEdit_minOutputChannel->text();
    //最大输出电压通道（字节低位）
    hisList << ui->lineEdit_maxOutChannel->text();
    //输出压差
    hisList << ui->lineEdit_OutVdiffer->text();
    //当前电流值
    hisList << QString::number(ui->lineEdit_currectValue->text().toDouble(),'f',1);
    //当前允许的最大放电电流
    hisList << QString::number(ui->lineEdit_maxDisCur->text().toDouble(),'f',1);
    //当前允许的最大充电电流
    hisList << QString::number(ui->lineEdit_maxMaxCharge->text().toDouble(),'f',1);
    //平均温度（字节高位）
    hisList << ui->lineEdit_averTemp->text();
    //最低温度（字节低位）
    hisList << ui->lineEdit_minTemp->text();
    //最高温度（字节低位）
    hisList << ui->lineEdit_maxTemp->text();
    //最低温度通道数（字节高位）
    hisList << ui->lineEdit_minTempChannel->text();
    //最高温度通道数（字节低位）
    hisList << ui->lineEdit_maxTempChannel->text();
    //输出温差
    hisList << ui->lineEdit_outTcha->text();
    //最新满充容量
    hisList << QString::number(ui->lineEdit_captotal->text().toDouble(),'f',2);
    //剩余容量
    hisList << QString::number(ui->lineEdit_shengyucap->text().toDouble(),'f',2);
    //充电累计AH
    hisList << QString::number(ui->lineEdit_chargetotal->text().toDouble(),'f',2);
    //SOC值（字节高位）
    hisList << ui->lineEdit_soc->text();
    //SOH值（字节低位）
    hisList << ui->lineEdit_soh->text();
    //循环次数
    hisList << ui->lineEdit_crilTime->text();
    //输出均衡状态    
    QVector<int> VVector;
    if(cellNum > 0 && cellNum <= 16)
    {
        hisList << QString::number(totalVector[addr("2100")].toUShort(&ok,16));
        for(int i=0;i<cellNum;i++)      //根据电芯串数选定单体电压的起始地址
        {
            VVector << addr("2101")+i;
        }
    }else if(cellNum > 16 && cellNum <= 32)
    {
        hisList << QString::number(totalVector[addr("2100")].toUShort(&ok,16))+QString::number(totalVector[addr("2101")].toUShort(&ok,16));
        for(int i=0;i<cellNum;i++)      //根据电芯串数选定单体电压的起始地址
        {
            VVector << addr("2102")+i;
        }
    }
    //单体电压值
    for(int i=0;i<cellNum;i++)
    {
        hisList << QString::number(totalVector[VVector.at(i)].toUShort(&ok,16));
    }
    if(cellNum < 16)
    {
        for(int i=0;i<16-cellNum;i++)
            hisList << "无";
    }
    //电池温度
    quint16 temp1 = 0;
    quint16 temp2 = 0;
    if(cellNum > 0 && cellNum <= 16)
    {
        temp1 = totalVector[addr("2101")+cellNum].toUShort(&ok,16);
        temp2 = totalVector[addr("2101")+cellNum+1].toUShort(&ok,16);
    }else if(cellNum > 16 &&cellNum <= 32)
    {
        temp1 = totalVector[addr("2102")+cellNum].toUShort(&ok,16);
        temp2 = totalVector[addr("2102")+cellNum+1].toUShort(&ok,16);
    }
    qint8 t1 = static_cast<qint8>(temp1 & 0xFF);
    qint8 t2 = static_cast<qint8>(temp1 >> 8);
    qint8 t3 = static_cast<qint8>(temp2 & 0xFF);
    qint8 t4 = static_cast<qint8>(temp2 >> 8);
    if(t1 == 0)
    {
        qint8 temp;
        temp = t1;
        t1 = t2;
        t2 = temp;
    }else if(t3 == 0){
        qint8 temp;
        temp = t3;
        t3 = t4;
        t4 = temp;
    }
    hisList << QString::number(t1);
    hisList << QString::number(t2);
    hisList << QString::number(t3);
    hisList << QString::number(t4);   
}
//实时数据--开始实时数据定时器和选择保存路径
void BatteryManage::on_pushButton_saveReal_clicked()
{
    if(hisheadFlag)
    {
        hisheadFlag = false;
        realhead_list << "当前时间" << "告警信息及状态" << "总电压" << "平均电压" << "最小输出电压"
                      << "最大输出电压" << "最小输出电压通道" << "最大输出电压通道" << "输出压差" << "当前电流值"
                      << "允许的最大放电电流" << "允许的最大充电电流" << "平均温度" << "最低温度" << "最高温度"
                      << "最低温度通道数" << "最高温度通道数" << "输出温差"<< "最新满充容量" << "剩余容量" << "充电累计AH"
                      << "SOC值" << "SOH值" << "循环次数" << "输出均衡状态" << "单体电压1" << "单体电压2"
                      << "单体电压3" << "单体电压4" << "单体电压5" << "单体电压6" << "单体电压7" << "单体电压8"
                      << "单体电压9" << "单体电压10" << "单体电压11" << "单体电压12" << "单体电压13"
                      << "单体电压14" << "单体电压15" << "单体电压16" << "T1" << "T2" << "T3" << "T4";
    }
    if(!M_port->isOpen())
    {
        QMessageBox::warning(this,"错误提示","串口未打开");
        return ;
    }
    //判断当前保存状态
    if(ui->pushButton_saveReal->text() == "停止保存")
    {
        ui->pushButton_saveReal->setText("保存");       
        ui->lineEdit_saveCycle->setReadOnly(false);  //失能存储周期，保护
        ui->label_log->setStyleSheet("");     //清除"正在保存"的标志图
        ui->label_left->setStyleSheet("");
        ui->label_right->setStyleSheet("");
        if(realhisData_timer->isActive())
        {            
            realhisData_timer->stop();
            realfile.close();                                               //关闭文件
            QMessageBox::about(this,"提示","数据已保存至文件");
        }
        return ;
    }else{
        /****************初始化*****************/
        if(realfile.isOpen())
        {
            realfile.close();
        }
        logFlag = false;                                                    //状态图判断标志
        timer->start();        
        in.reset();
        ui->pushButton_saveReal->setText("停止保存");
        ui->lineEdit_saveCycle->setReadOnly(true);
        saveCycleCnt = 0;                                                   //初始化存储周期辅助标志
        ui->label_left->setStyleSheet("border-image: url(:/image/real.png);");
        ui->label_right->setStyleSheet("border-image: url(:/image/excel.png);");

        QString filepath = QFileDialog::getSaveFileName(this,"选择保存文件路径","/","文件(*.csv)");
        ui->lineEdit_saveReal->setText(filepath);
        realfile.setFileName(filepath);
        if(filepath != nullptr)
        {
            if(realfile.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                in.setDevice(&realfile);
                for(int i=0;i<realhead_list.size();i++)
                {
                    in << realhead_list.at(i) << ",";
                }
                in << endl;
            }
        }else{
            ui->pushButton_saveReal->setText("保存");
            ui->lineEdit_saveCycle->setReadOnly(false);
            ui->label_log->setStyleSheet("");     //清除"正在保存"的标志图
            ui->label_left->setStyleSheet("");
            ui->label_right->setStyleSheet("");
            return;
        }
        //开启定时器
        realhisData_timer->start(100);                                              //开启实时保存定时器
    }
}
//实时数据--实时数据保存
void BatteryManage::history_realShow()
{  
    if(!timer->isActive() || (flag > 6))                       //如果实时定时器与通讯断了则关闭
    {
        realhisData_timer->stop();
        if(realfile.isOpen())
        {
//            file.close();
            return ;
        }
    }   
    uint flag1 = static_cast<uint>(ui->lineEdit_saveCycle->text().toDouble()*1000) / 100; //flag == 5;
    if(saveCycleCnt == flag1)
    {
        saveCycleCnt = 0;
        //存入实时链表
        history_realDataArrayOtherSave();
        //设置实时数据Excel数据区
        for(int j=0;j<hisList.size();j++)
        {
            in << hisList.at(j) << ",";           
        }
        in << endl;        
        if(logFlag)                                         //“正在保存”的状态图
        {
            logFlag = false;
            ui->label_log->setStyleSheet("border-image: url(:/image/right.png);");
        }else{
            logFlag = true;
            ui->label_log->setStyleSheet("border-image: url(:/image/left.png);");
        }
    }
    saveCycleCnt++;
    QApplication::processEvents();
}
//升级--打开文件并将文件数据存入数组
void BatteryManage::on_open_Bt_clicked()
{
    //判断是否正在升级中，正在升级中不可读取文件
    if(upTimer->isActive())
    {
        ui->open_Bt->setEnabled(false);
        ui->label_upHint->setText("It's upgrading! The file cannot be opened");
        return ;
    }else{
        ui->open_Bt->setEnabled(true);
    }
    //判断实时数据文件是否未关闭
    if(realfile.isOpen() || hisfile.isOpen())
    {
        QMessageBox::warning(this,"提示","实时数据保存文件未关闭/正在读取历史数据");
        return;
    }
    //关闭定时器
    if(timer->isActive())
        timer->stop();
    if(timerProcessData->isActive())
        timerProcessData->stop();
    if(realhisData_timer->isActive())
        realhisData_timer->stop();

    QString filename = QFileDialog::getOpenFileName(nullptr,"open file","/","bin 文件(*.bin)");
    if(filename != nullptr)
    {
        ui->label_upHint->setText("File Opened Successfully!");
        ui->Bt_upgrade->setEnabled(true);       //使能升级按钮
        ui->Bt_endUpgrade->setEnabled(true);    //使能终止升级按钮键
        ui->lineEdit_filename->setText(filename);        
        QFile file(filename);
        quint8 data=0;
        File_Vector.clear();
        if(file.open(QIODevice::ReadOnly))
        {
            QDataStream out(&file);
            out.setVersion(QDataStream::Qt_5_12);
            while(!file.atEnd())
            {
                out >> data;
                File_Vector << data;
                data = 0;
            }
        }
        file.close();
        ui->label_upHint->setText("File Read Successfully!");
    }else{
        ui->label_upHint->setText("File Opened Failed!");
        return;
    }
}
//升级--打包发送帧
void BatteryManage::upgrade_packFrame(quint16 addr, quint8 len,quint16 num)
{
    /* len: 每帧发送的数据长度
     * num: File_Vector每帧的起始下标
    */
    upSend_Vector[7] = addr & 0xFF;
    upSend_Vector[8] = addr >> 8;
    upSend_Vector[9] = len;
    for(int i=0;i<len;i++)
    {
        upSend_Vector[10+i] = File_Vector[num+i];
    }
    QVector<quint8> temp(len+10);
    for(int i=0;i<len+10;i++)
    {
        temp[i] = upSend_Vector[i];
    }
    temp.remove(0,6);//移除帧头
    quint16 crcCal = calCRC16(temp,len+4);
    quint8 crc1 = crcCal >> 8;
    quint8 crc2 = (crcCal & 0xFF);
    upSend_Vector[len+10] = crc2;
    upSend_Vector[len+11] = crc1;   
    sendData.clear();
    for(int i=0;i<len+12;i++)
    {
        sendData[i] = static_cast<qint8>(upSend_Vector.at(i));        
    }    
    return ;
}
//升级--发送数据
void BatteryManage::on_Bt_upgrade_clicked()
{
    if(!ui->open_Bt->isEnabled())
        ui->open_Bt->setEnabled(true);
    //检查串口是否打开
    if(!M_port->isOpen())
    {
        QMessageBox::warning(this,"错误提示","串口未打开");
        ui->label_upHint->setText("Serial Port Opened Failed!");
        return ;
    }
    //判断实时数据文件是否未关闭
    if(realfile.isOpen() || hisfile.isOpen())
    {
        QMessageBox::warning(this,"提示","实时数据或历史数据保存文件未关闭");
        return;
    }
    //关闭定时器
    if(timer->isActive())
        timer->stop();
    if(timerProcessData->isActive())
        timerProcessData->stop();
    if(realhisData_timer->isActive())
        realhisData_timer->stop();
    //识别bootload文件
    bool cntflag = false;
    for(int i=0;i<File_Vector.size();i++)
    {
        if(File_Vector.at(i) == 0x9D && File_Vector.at(i+1) == 0x41 && File_Vector.at(i+2) == 0x00 && File_Vector.at(i+3) == 0x08)
        {
            File_Vector.remove(0,i-4);
            cntflag = true;
        }
        if(cntflag)         //后面若全是0xFF不下载，浪费时间
        {
            if(File_Vector.at(i) == 0xFF && File_Vector.at(i+1) == 0xFF && File_Vector.at(i+2) == 0xFF && File_Vector.at(i+3) == 0xFF
                    && File_Vector.at(i+4) == 0xFF && File_Vector.at(i+5) == 0xFF && File_Vector.at(i+6) == 0xFF && File_Vector.at(i+7) == 0xFF)
            {
                File_Vector.remove(i,File_Vector.size()-i);
            }
        }
    }
    //清空发送数组
    sendData.clear();
    dataStartAddr.clear(); //清空数据起始地址    
    ui->progressBar->setMaximum(File_Vector.size());
    ui->progressBar->setMinimum(0);
    switchFlag = 1;
    FileAddrCnt = 0;
    waitAckFlag = false;
    frameNum = 0;
    FileOneFrameTXFlag = false;
    k = 0;
    //0x8000
    upTimer->start(10);
    ui->label_upHint->setText("Start Burning");
}
//升级--定时器槽函数
void BatteryManage::upgrade_Timerslot()
{    
    QSerialPortInfo portInfo(*M_port);
    //关闭定时器
    if(timer->isActive())                       //防止用户在参数设置或校准界面跳入实时问询帧发送界面
        timer->stop();
    switch(switchFlag)
    {
        case 1:
            if(!waitAckFlag) //如果没收到回复帧，即dataStartAddr != "8000"
            {
                //发送帧
                pack_sendFrame(0x01,0x00,0x80,0x00);
                waitAckFlag = true;
                frameNum++;
                whileCnt = 0;
            }
            whileCnt++;
            if(whileCnt > 100)
            {                
                waitAckFlag = false;//为了再发送一帧               
                if(frameNum > 5) //第三次都没收到回复
                {                    
                    ui->label_upHint->setText("No Entering Reply-Frame Received!");
                    QMessageBox::warning(this,"错误提示","进入bootload超时");
                    upTimer->stop();
                    return;
                }
            }
            if(dataStartAddr == "8000")
            {
                dataStartAddr.clear();
                if(upgradeFlag)
                {
                    upgradeFlag = false;
                    waitAckFlag = false;//成功则是为了后面的帧能发，失败则是为了再发一帧                   
                    ui->label_upHint->setText("Enter Bootload Successfully!");
                    frameNum = 0;
                    switchFlag = 2;
                    return ;
                }
                else{
                    ui->label_upHint->setText("校验错误！！！！！！");
                    if(frameNum > 5) //第三次帧是错误
                    {
                        ui->label_upHint->setText("The Reply-Frame Return Failed!");
                        QMessageBox::warning(this,"错误提示","操作bootload失败");
                        upTimer->stop();
                        return;
                    }
                }
            }
        break;
        case 2:
        if(!waitAckFlag) //如果没收到回复帧，即dataStartAddr != "8000"
        {
            //发送帧
            pack_sendFrame(0x01,0x01,0x80,0x00);
            waitAckFlag = true;
            frameNum++;
            whileCnt = 0;
        }
        whileCnt++;
        if(whileCnt > 100)
        {
            waitAckFlag = false;
            if(frameNum > 5) //第三次都没收到回复
            {
                ui->label_upHint->setText("No Erasing Reply-Frame Received!");
                QMessageBox::warning(this,"错误提示","进入擦除FLASH失败");
                upTimer->stop();
                return;
            }
        }        
        if(dataStartAddr == "8001")
        {            
            dataStartAddr.clear();
            if(upgradeFlag)
            {
                waitAckFlag = false;              
                ui->label_upHint->setText("Earse Flash Successfully!");
                frameNum = 0;
                switchFlag = 3;
                return ;
            }
            else{
                if(frameNum > 5) //第三次帧是错误
                {
                    ui->label_upHint->setText("Earse Flash Operation Failed!");
                    QMessageBox::warning(this,"错误提示","操作擦除FLASH失败");
                    upTimer->stop();
                    return;
                }
            }
        }
        break;
    case 3:
    {
        if(File_Vector.size() < 128)
        {
            upTimer->stop();
            return;
        }
        int fileLen = File_Vector.size();        
        {
            if(k > fileLen)
            {
                return ;
            }
            if(!waitAckFlag) //如果没收到回复帧，即dataStartAddr != "8000"
            {
                //发送帧
                if(((fileLen-k)>0 && (fileLen-k)<128) || k+128 == fileLen )  //判断是否最后一帧
                {
                    upgrade_packFrame(0x9FFF,static_cast<quint8>(fileLen-k),static_cast<quint16>(k)); //最后一帧
                    if(portInfo.isBusy())
                    M_port->write(sendData);
                    ui->progressBar->setValue(fileLen-k);       //设置进度条值                    
                }else{
                    upgrade_packFrame(static_cast<quint16>(0x9001+FileAddrCnt),128,static_cast<quint16>(k));
                    if(portInfo.isBusy())
                    M_port->write(sendData);
                    ui->progressBar->setValue(k);       //设置进度条值
                }
                frameNum++;
                waitAckFlag = true;
                whileCnt = 0;
            }
            whileCnt++;
            if(whileCnt > 150)
            {
                waitAckFlag = false;
                if(frameNum > 5) //第三次都没收到回复
                {
                    ui->label_upHint->setText("No FileData Reply-Frame Received");
                    QMessageBox::warning(this,"错误提示","进入下载数据失败");
                    upTimer->stop();
                    return;
                }
            }
            if(dataStartAddr.toUShort(&ok,16) == 36865+FileAddrCnt || dataStartAddr == "9FFF")
            {
                dataStartAddr.clear();
                if(upgradeFlag)
                {                    
                    waitAckFlag = false;
                    FileOneFrameTXFlag = true;    //当前帧发送成功
                    FileAddrCnt++;
                    frameNum = 0;
                    ui->label_upHint->setText("Bin File Data Downloading");
                    if(((fileLen-k)>0 && (fileLen-k)<128) || k+128 == fileLen )  //判断是否最后一帧
                    {                       
                        ui->label_upHint->setText("Last FileData Frame Erase Operation Successfully!");
                        switchFlag = 4;

                        return;
                    }
                }
                else{
                    if(frameNum > 5) //第三次帧是错误
                    {
                        ui->label_upHint->setText("The FileData Frame Erase Operation Failed!");
                        QMessageBox::warning(this,"错误提示","操作下载数据失败");
                        upTimer->stop();
                        return;
                    }
                }
            }
            if(FileOneFrameTXFlag)   //只有该帧发送成功才+128
            {
                FileOneFrameTXFlag = false;
                k = k + 128;
            }
        }
    }
        break;
    case 4://发送32位校验码
    {
        /*************************************************************/
        if(!waitAckFlag) //如果没收到回复帧，即dataStartAddr != "8000"
        {
            //发送帧
            pack_sendFrame(0x01,0x02,0x80,0x00);            
            frameNum++;
            waitAckFlag = true;
            whileCnt = 0;
        }
        whileCnt++;
        if(whileCnt > 200)
        {
            waitAckFlag = false;
            if(frameNum > 5) //第三次都没收到回复
            {
                ui->label_upHint->setText("No 32-Bit Checkout Frame Received!");
                QMessageBox::warning(this,"错误提示","进行校验FLASH失败");
                upTimer->stop();
                return;
            }
        }
        if(dataStartAddr == "8002")
        {
            dataStartAddr.clear();
            if(upgradeFlag)
            {
                waitAckFlag = false;
                ui->label_upHint->setText("32-Bit Checkout Successfully!");
                switchFlag = 5;                
            }
            else{
                if(frameNum > 5) //第三次帧是错误
                {
                    ui->label_upHint->setText("32-Bit Checkout Failed!");
                    QMessageBox::warning(this,"错误提示","操作校验FLASH失败");
                    upTimer->stop();
                    return;
                }
            }
        }
    }
        break;
    case 5:
        ui->label_upHint->setText("Upgrade Successfully!!!");
        QMessageBox::about(this,"升级提示","升级成功！！！");
        if(!ui->open_Bt->isEnabled())
            ui->open_Bt->setEnabled(true);
        if(!timer->isActive() && (TabIndex==1 || TabIndex == 0 || TabIndex == 2 || TabIndex == 4))
            timer->start(timerT);
        if(!timerProcessData->isActive() && (TabIndex==1 || TabIndex == 0 || TabIndex == 2 || TabIndex == 4))
            timerProcessData->start(timerT);
        if(upTimer->isActive())
        {
            upTimer->stop();
        }
        return;
    }    
}
//升级--32位校验
uint32_t BatteryManage::upgrade_Cal32CRC(QVector<quint32> buff, int len)
{
    quint32 CRC32_INIT = 0xFFFFFF;
    quint32 CRC32_POLY = 0x04C11DB7;
    uint32_t  xbit;
    uint32_t  data;
    uint32_t  CRC32 = CRC32_INIT;
    for(int i = 0;i < len; i++)
    {
        xbit = 0x80000000;
        data = buff[i];
        for (uint8_t j = 0; j < 32; j++)
        {
            CRC32 = (CRC32 & 0x80000000) ? ((CRC32 << 1) ^ CRC32_POLY) : (CRC32 << 1);
        if (data & xbit)
        {
            CRC32 ^= CRC32_POLY;
        }
            xbit >>= 1;
        }
    }
    return CRC32;
}
//升级--读取文件
void BatteryManage::upgrade_FileRead()
{
    QString filepath = QFileDialog::getOpenFileName(this,"Open","/","文本文档(*.bin)");
    //ui->lineEdit->setText(filepath);
    if(filepath == nullptr)
    {
        return;
    }
    QFile file(filepath);
    QVector<quint8> tempVector;
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QDataStream out(&file);
        out.setVersion(QDataStream::Qt_5_12);
        quint8 data=0;
        File_Vector.clear();
        while(!file.atEnd())
        {
            out >> data;
            tempVector << data-48;
            data = 0;
            if(tempVector.size() == 8)
            {
                for(int i=0;i<8;i++)
                {
                    data += tempVector.at(i) << (7-i);
                }
                //File_Vector << (QString("%1").arg(data,4,16,QLatin1Char('0')).toUShort(&ok,16) & 0xFF);
                File_Vector << data;
                tempVector.clear();
            }
        }
    }
    file.close();
}
//升级--终止升级
void BatteryManage::on_Bt_endUpgrade_clicked()
{
    if(!timer->isActive() && (TabIndex==1 || TabIndex == 0 || TabIndex == 2 || TabIndex == 4))
        timer->start(timerT);
    if(!timerProcessData->isActive() && (TabIndex==1 || TabIndex == 0 || TabIndex == 2 || TabIndex == 4))
        timerProcessData->start(timerT);
    if(upTimer->isActive())
        upTimer->stop();
    ui->Bt_upgrade->setEnabled(false);
    ui->Bt_endUpgrade->setEnabled(false);
    ui->open_Bt->setEnabled(true);
    ui->progressBar->setValue(0);
}
//测试--写入
void BatteryManage::on_pushButton_write_clicked()
{
    QString text = ui->textEdit_in->toPlainText();
    if(M_port->isOpen())
    {
        text = text.remove(QChar(' '),Qt::CaseInsensitive);
        QByteArray array = text.toLatin1();
        if(array.size() > 512)
        {
            array.remove(512,array.size()-512);
        }
        array = QByteArray::fromHex(array);
        array = array.toHex('/');
        QList<QByteArray> List = array.split('/');
        QString addr = ui->lineEdit_textAddr_2->text();
        if(addr.toUShort(nullptr,16) < 0x3000 || addr.toUShort(nullptr,16) > 0x30FF)
        {
            QMessageBox::warning(this,"错误提示","输入的地址范围不在0x3000~0x30FF");
            return ;
        }
        if(List.size() <=0 || List.size() > 256)
        {
            QMessageBox::warning(this,"错误提示","输入的长度范围不在0~256");
            return ;
        }
        if(addr.toUShort(nullptr,16)+List.size() > 0x30FF)
        {
            QMessageBox::warning(this,"错误提示","数据长度过长，地址越界");
            return ;
        }

        int k=6;
        sendVector[k++] = 0x01;
        sendVector[k++] = addr.toUShort(nullptr,16) & 0xFF;
        sendVector[k++] = addr.toUShort(nullptr,16) >> 8;
        sendVector[k++] = static_cast<quint8>(List.size());
        QString data1;
        for(int i=0;i<List.size();i++)
        {
            sendVector[k++] = static_cast<quint8>(List.at(i).toInt(nullptr,16));
            data1.append(List.at(i)+" ");
        }
        Pack_sendData(sendVector,List.size()+10);
        ui->textEdit_in->append("写入的有效数据:"+data1);
        QMessageBox::about(this,"提示","数据已写入");
    }else{
        QMessageBox::warning(this,"错误提示","串口未打开");
    }
}
//测试--读取
void BatteryManage::on_pushButton_read_clicked()
{
    QString addr = ui->lineEdit_textAddr->text();
    QString len = ui->lineEdit_testLen->text();
    if(addr.contains("0X") || addr.contains("0x"))
    {
        addr.remove(0,2);
    }
    if(addr.toUShort(nullptr,16) < 0x3000 || addr.toUShort(nullptr,16) > 0x30FF)
    {
        QMessageBox::warning(this,"错误提示","输入的地址范围不在0x3000~0x30FF");
        return ;
    }
    if(len.toInt() <=0 || len.toInt() > 256)
    {
        QMessageBox::warning(this,"错误提示","输入的长度范围不在0~256");
        return ;
    }
    pack_sendFrame(0x00,addr.toUShort(nullptr,16) & 0xFF,addr.toUShort(nullptr,16) >> 8,
                   static_cast<quint8>(len.toInt()));
    delay_ms(500);
    QString data;
    for(int i=0;i<TestList.size();i++)
    {
        data.append(TestList.at(i)+" ");
    }
    ui->plainTextEdit_out->appendPlainText(" + "+data);
}
//密码项
void BatteryManage::on_tabWidget_currentChanged(int index)
{
    TabIndex = index;               //传值

    logWin->flag = true;
    /*index: 0:主界面信息 1:单体电压值 2:告警信息 3:校准 4:历史数据 5:参数设置 6:测试*/
    if(index == 3 || index == 5 || index == 6)
    {
        timer->stop();              //关闭发送问询帧的定时器
        timerProcessData->stop();   //关闭显示数据定时器，因为要写入新数据

#if 0       //客户版
        if(index == 5)
        {
            //设置写入数据按钮不可用
            ui->pushButton_4->setEnabled(false);
            parmaterShow();
        }else{
            ui->tabWidget->setCurrentIndex(0);
        }

#else       //调试版
        if(index == 5)
        {
            parmaterShow();
        }       
#endif
//        QString text = QInputDialog::getText(this,"输入对话框","请输入密码",QLineEdit::Password,"111111",&ok);
//        if(ok && text=="666666")
//        {
//            ui->tab_3->setEnabled(true);
//            ui->tab_5->setEnabled(true);
//            ui->pushButton_zeroCalib->setEnabled(true);
//            ui->buttonBox->setEnabled(true);
//            ui->pushButton_clearForever->setEnabled(true);
//            ui->pushButton_timeCalib->setEnabled(true);
//            ui->pushButton_bigCCalib->setEnabled(true);
//            ui->pushButton_smallCCalib->setEnabled(true);
//            ui->pushButton_6->setEnabled(true);
//            ui->pushButton_bigVCalib->setEnabled(true);
//            ui->pushButton_smallVCalib->setEnabled(true);
//            ui->pushButton_balanceCalib->setEnabled(true);
//        }else if(ok && text == "123456"){
//            ui->tab_5->setEnabled(false);
//            ui->tab_3->setEnabled(true);
//            ui->lineEdit_tiaoxingma->setEnabled(true);
//            ui->lineEdit_BMSCode->setEnabled(true);
//            ui->buttonBox->setEnabled(false);
//            ui->pushButton_zeroCalib->setEnabled(false);
//            ui->pushButton_clearForever->setEnabled(false);
//            ui->pushButton_timeCalib->setEnabled(false);
//            ui->pushButton_bigCCalib->setEnabled(false);
//            ui->pushButton_smallCCalib->setEnabled(false);
//            ui->pushButton_6->setEnabled(false);
//            ui->pushButton_bigVCalib->setEnabled(false);
//            ui->pushButton_smallVCalib->setEnabled(false);
//            ui->pushButton_balanceCalib->setEnabled(false);
//        }else{
//            QMessageBox::warning(this,"错误提示","密码错误");
//           ui->tabWidget->setCurrentIndex(0);
//        }
    }else{
        if(M_port->isOpen())
        {
            if(!timer->isActive())
                timer->start(timerT);
            if(!timerProcessData->isActive())
                timerProcessData->start(timerT);

            if(realfile.isOpen())   //如果进入“历史数据”页面，且文件未关闭，则继续保存实时数据
            {
                if(!realhisData_timer->isActive())
                    realhisData_timer->start();
            }
            if(hisfile.isOpen()) //如果进入“历史数据”界面，但历史数据仍在保存中，则
            {
                if(timer->isActive())
                    timer->stop();
                if(timerProcessData->isActive())
                    timerProcessData->stop();
                if(realhisData_timer->isActive())
                    realhisData_timer->stop();
            }
        }
    }
}













