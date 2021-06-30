/****************************************************************************
** Meta object code from reading C++ file 'batterymanage.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../batterymanage.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'batterymanage.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_BatteryManage_t {
    QByteArrayData data[44];
    char stringdata0[1026];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_BatteryManage_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_BatteryManage_t qt_meta_stringdata_BatteryManage = {
    {
QT_MOC_LITERAL(0, 0, 13), // "BatteryManage"
QT_MOC_LITERAL(1, 14, 19), // "his_receFrameSignal"
QT_MOC_LITERAL(2, 34, 0), // ""
QT_MOC_LITERAL(3, 35, 11), // "serial_read"
QT_MOC_LITERAL(4, 47, 18), // "on_open_Bt_clicked"
QT_MOC_LITERAL(5, 66, 21), // "on_pushButton_clicked"
QT_MOC_LITERAL(6, 88, 16), // "his_writeToExcel"
QT_MOC_LITERAL(7, 105, 10), // "Timer_Init"
QT_MOC_LITERAL(8, 116, 16), // "Timer_UpdatePort"
QT_MOC_LITERAL(9, 133, 10), // "Timer_slot"
QT_MOC_LITERAL(10, 144, 19), // "writeCorrespondData"
QT_MOC_LITERAL(11, 164, 16), // "history_realShow"
QT_MOC_LITERAL(12, 181, 17), // "upgrade_Timerslot"
QT_MOC_LITERAL(13, 199, 24), // "on_menu_exit_aboutToShow"
QT_MOC_LITERAL(14, 224, 31), // "on_pushButton_timeCalib_clicked"
QT_MOC_LITERAL(15, 256, 31), // "on_pushButton_bigCCalib_clicked"
QT_MOC_LITERAL(16, 288, 33), // "on_pushButton_smallCCalib_cli..."
QT_MOC_LITERAL(17, 322, 31), // "on_pushButton_bigVCalib_clicked"
QT_MOC_LITERAL(18, 354, 33), // "on_pushButton_smallVCalib_cli..."
QT_MOC_LITERAL(19, 388, 23), // "on_pushButton_6_clicked"
QT_MOC_LITERAL(20, 412, 23), // "on_pushButton_4_clicked"
QT_MOC_LITERAL(21, 436, 23), // "on_pushButton_3_clicked"
QT_MOC_LITERAL(22, 460, 23), // "on_pushButton_2_clicked"
QT_MOC_LITERAL(23, 484, 33), // "on_pushButton_readBarcode_cli..."
QT_MOC_LITERAL(24, 518, 34), // "on_pushButton_balanceCalib_cl..."
QT_MOC_LITERAL(25, 553, 33), // "on_pushButton_savehistory_cli..."
QT_MOC_LITERAL(26, 587, 30), // "on_pushButton_saveReal_clicked"
QT_MOC_LITERAL(27, 618, 23), // "on_pushButton_5_clicked"
QT_MOC_LITERAL(28, 642, 21), // "on_Bt_upgrade_clicked"
QT_MOC_LITERAL(29, 664, 24), // "on_Bt_endUpgrade_clicked"
QT_MOC_LITERAL(30, 689, 30), // "on_pushButton_BMSWrite_clicked"
QT_MOC_LITERAL(31, 720, 24), // "on_checkBox_stateChanged"
QT_MOC_LITERAL(32, 745, 31), // "on_pushButton_zeroCalib_clicked"
QT_MOC_LITERAL(33, 777, 34), // "on_pushButton_clearForever_cl..."
QT_MOC_LITERAL(34, 812, 20), // "on_buttonBox_clicked"
QT_MOC_LITERAL(35, 833, 16), // "QAbstractButton*"
QT_MOC_LITERAL(36, 850, 6), // "button"
QT_MOC_LITERAL(37, 857, 27), // "on_pushButton_write_clicked"
QT_MOC_LITERAL(38, 885, 26), // "on_pushButton_read_clicked"
QT_MOC_LITERAL(39, 912, 27), // "on_tabWidget_currentChanged"
QT_MOC_LITERAL(40, 940, 5), // "index"
QT_MOC_LITERAL(41, 946, 23), // "on_pushButton_8_clicked"
QT_MOC_LITERAL(42, 970, 23), // "on_pushButton_9_clicked"
QT_MOC_LITERAL(43, 994, 31) // "on_pushButton_frontChip_clicked"

    },
    "BatteryManage\0his_receFrameSignal\0\0"
    "serial_read\0on_open_Bt_clicked\0"
    "on_pushButton_clicked\0his_writeToExcel\0"
    "Timer_Init\0Timer_UpdatePort\0Timer_slot\0"
    "writeCorrespondData\0history_realShow\0"
    "upgrade_Timerslot\0on_menu_exit_aboutToShow\0"
    "on_pushButton_timeCalib_clicked\0"
    "on_pushButton_bigCCalib_clicked\0"
    "on_pushButton_smallCCalib_clicked\0"
    "on_pushButton_bigVCalib_clicked\0"
    "on_pushButton_smallVCalib_clicked\0"
    "on_pushButton_6_clicked\0on_pushButton_4_clicked\0"
    "on_pushButton_3_clicked\0on_pushButton_2_clicked\0"
    "on_pushButton_readBarcode_clicked\0"
    "on_pushButton_balanceCalib_clicked\0"
    "on_pushButton_savehistory_clicked\0"
    "on_pushButton_saveReal_clicked\0"
    "on_pushButton_5_clicked\0on_Bt_upgrade_clicked\0"
    "on_Bt_endUpgrade_clicked\0"
    "on_pushButton_BMSWrite_clicked\0"
    "on_checkBox_stateChanged\0"
    "on_pushButton_zeroCalib_clicked\0"
    "on_pushButton_clearForever_clicked\0"
    "on_buttonBox_clicked\0QAbstractButton*\0"
    "button\0on_pushButton_write_clicked\0"
    "on_pushButton_read_clicked\0"
    "on_tabWidget_currentChanged\0index\0"
    "on_pushButton_8_clicked\0on_pushButton_9_clicked\0"
    "on_pushButton_frontChip_clicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_BatteryManage[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      39,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,  209,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    0,  210,    2, 0x08 /* Private */,
       4,    0,  211,    2, 0x08 /* Private */,
       5,    0,  212,    2, 0x08 /* Private */,
       6,    0,  213,    2, 0x08 /* Private */,
       7,    0,  214,    2, 0x08 /* Private */,
       8,    0,  215,    2, 0x08 /* Private */,
       9,    0,  216,    2, 0x08 /* Private */,
      10,    0,  217,    2, 0x08 /* Private */,
      11,    0,  218,    2, 0x08 /* Private */,
      12,    0,  219,    2, 0x08 /* Private */,
      13,    0,  220,    2, 0x08 /* Private */,
      14,    0,  221,    2, 0x08 /* Private */,
      15,    0,  222,    2, 0x08 /* Private */,
      16,    0,  223,    2, 0x08 /* Private */,
      17,    0,  224,    2, 0x08 /* Private */,
      18,    0,  225,    2, 0x08 /* Private */,
      19,    0,  226,    2, 0x08 /* Private */,
      20,    0,  227,    2, 0x08 /* Private */,
      21,    0,  228,    2, 0x08 /* Private */,
      22,    0,  229,    2, 0x08 /* Private */,
      23,    0,  230,    2, 0x08 /* Private */,
      24,    0,  231,    2, 0x08 /* Private */,
      25,    0,  232,    2, 0x08 /* Private */,
      26,    0,  233,    2, 0x08 /* Private */,
      27,    0,  234,    2, 0x08 /* Private */,
      28,    0,  235,    2, 0x08 /* Private */,
      29,    0,  236,    2, 0x08 /* Private */,
      30,    0,  237,    2, 0x08 /* Private */,
      31,    1,  238,    2, 0x08 /* Private */,
      32,    0,  241,    2, 0x08 /* Private */,
      33,    0,  242,    2, 0x08 /* Private */,
      34,    1,  243,    2, 0x08 /* Private */,
      37,    0,  246,    2, 0x08 /* Private */,
      38,    0,  247,    2, 0x08 /* Private */,
      39,    1,  248,    2, 0x08 /* Private */,
      41,    0,  251,    2, 0x08 /* Private */,
      42,    0,  252,    2, 0x08 /* Private */,
      43,    0,  253,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 35,   36,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   40,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void BatteryManage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<BatteryManage *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->his_receFrameSignal(); break;
        case 1: _t->serial_read(); break;
        case 2: _t->on_open_Bt_clicked(); break;
        case 3: _t->on_pushButton_clicked(); break;
        case 4: _t->his_writeToExcel(); break;
        case 5: _t->Timer_Init(); break;
        case 6: _t->Timer_UpdatePort(); break;
        case 7: _t->Timer_slot(); break;
        case 8: _t->writeCorrespondData(); break;
        case 9: _t->history_realShow(); break;
        case 10: _t->upgrade_Timerslot(); break;
        case 11: _t->on_menu_exit_aboutToShow(); break;
        case 12: _t->on_pushButton_timeCalib_clicked(); break;
        case 13: _t->on_pushButton_bigCCalib_clicked(); break;
        case 14: _t->on_pushButton_smallCCalib_clicked(); break;
        case 15: _t->on_pushButton_bigVCalib_clicked(); break;
        case 16: _t->on_pushButton_smallVCalib_clicked(); break;
        case 17: _t->on_pushButton_6_clicked(); break;
        case 18: _t->on_pushButton_4_clicked(); break;
        case 19: _t->on_pushButton_3_clicked(); break;
        case 20: _t->on_pushButton_2_clicked(); break;
        case 21: _t->on_pushButton_readBarcode_clicked(); break;
        case 22: _t->on_pushButton_balanceCalib_clicked(); break;
        case 23: _t->on_pushButton_savehistory_clicked(); break;
        case 24: _t->on_pushButton_saveReal_clicked(); break;
        case 25: _t->on_pushButton_5_clicked(); break;
        case 26: _t->on_Bt_upgrade_clicked(); break;
        case 27: _t->on_Bt_endUpgrade_clicked(); break;
        case 28: _t->on_pushButton_BMSWrite_clicked(); break;
        case 29: _t->on_checkBox_stateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 30: _t->on_pushButton_zeroCalib_clicked(); break;
        case 31: _t->on_pushButton_clearForever_clicked(); break;
        case 32: _t->on_buttonBox_clicked((*reinterpret_cast< QAbstractButton*(*)>(_a[1]))); break;
        case 33: _t->on_pushButton_write_clicked(); break;
        case 34: _t->on_pushButton_read_clicked(); break;
        case 35: _t->on_tabWidget_currentChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 36: _t->on_pushButton_8_clicked(); break;
        case 37: _t->on_pushButton_9_clicked(); break;
        case 38: _t->on_pushButton_frontChip_clicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 32:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAbstractButton* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (BatteryManage::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BatteryManage::his_receFrameSignal)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject BatteryManage::staticMetaObject = { {
    &QMainWindow::staticMetaObject,
    qt_meta_stringdata_BatteryManage.data,
    qt_meta_data_BatteryManage,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *BatteryManage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BatteryManage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_BatteryManage.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int BatteryManage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 39)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 39;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 39)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 39;
    }
    return _id;
}

// SIGNAL 0
void BatteryManage::his_receFrameSignal()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
struct qt_meta_stringdata_MyFrame_t {
    QByteArrayData data[1];
    char stringdata0[8];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MyFrame_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MyFrame_t qt_meta_stringdata_MyFrame = {
    {
QT_MOC_LITERAL(0, 0, 7) // "MyFrame"

    },
    "MyFrame"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MyFrame[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void MyFrame::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject MyFrame::staticMetaObject = { {
    &QFrame::staticMetaObject,
    qt_meta_stringdata_MyFrame.data,
    qt_meta_data_MyFrame,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MyFrame::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MyFrame::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MyFrame.stringdata0))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int MyFrame::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
