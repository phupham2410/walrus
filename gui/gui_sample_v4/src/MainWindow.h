#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QFont>
#include <QLabel>
#include <QWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialog>
#include <QListWidget>
#include <QCoreApplication>
#include <QProgressBar>
#include <QThread>
#include <QComboBox>
#include <QScrollBar>
#include <QStackedWidget>

#include "StorageApi.h"

typedef void (*tWorkerFunc)(void*);

// Widget Positions
enum eViewCode {
    #define MAP_ITEM(name, code) code,
    #include "FuncList.def"
    #undef MAP_ITEM
};

class MainWindow : public QDialog
{
    Q_OBJECT
public:
    struct sControl {
        QListWidget* DrvList;
        QStackedWidget* Stack;

        QPushButton* ScanBtn;
        QPushButton* InfoBtn;
        QPushButton* SmartBtn;
        QPushButton* TestBtn;
        QPushButton* UpdateBtn;
        QPushButton* CloneBtn;
        QPushButton* TrimBtn;
        QPushButton* EraseBtn;
        QPushButton* DebugBtn;

        QProgressBar* ProgBar;

        struct sDriveControl {
            QPlainTextEdit* Console;
        } di;

        struct sSmartControl {
            QPlainTextEdit* Console;
        } sm;

        struct sTestControl {
            QPlainTextEdit* Console;
            QPushButton* Mode0Btn;
            QPushButton* Mode1Btn;
            QPushButton* Mode2Btn;
            QPushButton* StopBtn;
        } st;

        struct sUpdateControl {
            QPlainTextEdit* Console;
            QPushButton* StartBtn;
            QPushButton* StopBtn;
        } uf;

        struct sCloneControl {
            QListWidget* List;
            QComboBox* Target;
            QPlainTextEdit* Console;
            QPushButton* StartBtn;
            QPushButton* StopBtn;
            QPushButton* ReadBtn;
        } dc;

        struct sTrimControl {
            QPlainTextEdit* Console;
            QPushButton* StartBtn;
            QPushButton* StopBtn;
        } td;

        struct sEraseControl {
            QPlainTextEdit* Console;
            QPushButton* StartBtn;
            QPushButton* StopBtn;
        } sw;

        struct sDebugControl {
            QPlainTextEdit* Console;
            QPushButton* ReadBtn;
            QPushButton* FillBtn;
            QPushButton* StopBtn;
        } db;
    };

    sControl ctrl;

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

public:
    void initWindow();

public:
    void showDriveList();
    void setLog(QPlainTextEdit* w, const QString& str);
    void appendLog(QPlainTextEdit* w, const QString& str);

    void enableGui(bool status);
    void resetProgress();
    void setProgress(unsigned int val);
    void setProgress(unsigned int min, unsigned int max);

public:
    void startWorker(tWorkerFunc func, void* param);
    void waitProcessing(volatile StorageApi::sProgress& prog);

public slots:
    // Common Slots
    void handleScanDrive();   // Press ScanBtn
    void handleSelectDrive(); // Select one item in the list

    #define MAP_ITEM(name, index) void handle##name();
    MAP_ITEM(ViewDrive , VIEW_DRIVE_INFO  )
    MAP_ITEM(ViewSmart , VIEW_SMART_INFO  )
    MAP_ITEM(SelfTest  , VIEW_SELF_TEST   )
    MAP_ITEM(DiskClone , VIEW_DISK_CLONE  )
    MAP_ITEM(TrimDrive , VIEW_OPTIMIZATION)
    MAP_ITEM(SecureWipe, VIEW_SECURE_WIPE )
    MAP_ITEM(UpdateFw  , VIEW_FW_UPDATE   )
    MAP_ITEM(Debug     , VIEW_DEBUG       )
    #undef MAP_ITEM

    // Specific operation
    void handleSelfTestMode0();
    void handleSelfTestMode1();
    void handleSelfTestMode2();
    void handleSelfTestStop();
    void handleUpdateFwStart();
    void handleUpdateFwStop();
    void handleTrimDriveStart();
    void handleTrimDriveStop();
    void handleSecureWipeStart();
    void handleSecureWipeStop();
    void handleDiskCloneRead();
    void handleDiskCloneStart();
    void handleDiskCloneStop();
    void handleDebugRead();
    void handleDebugFill();
    void handleDebugStop();

    // Redundant (old code)
    void handleCommonStop();
    void handleReadDrive();
    void handleFillDrive();

public:
    // Build content of specific widget
    QWidget* buildWidget_Common(
        QPlainTextEdit* text, QPushButton* btn[], int cnt);

    #define MAP_ITEM(funcname, index) \
            QWidget* buildWidget_##funcname(); \
            void showWidget_##funcname();
    #include "FuncList.def"
    #undef MAP_ITEM
};

#endif // MAINWINDOW_H
