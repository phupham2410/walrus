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
    VIEW_DRIVE_INFO,
    VIEW_SMART_INFO,
    VIEW_OPTIMIZATION,
    VIEW_FW_UPDATE,
    VIEW_SECURE_WIPE,
    VIEW_DISK_CLONE,
    VIEW_SELF_TEST,
    VIEW_DEBUG,
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
    // Debug Slots
    void handleStop();
    void handleReadDrive();
    void handleFillDrive();

    // Common Slots
    void handleSelectDrive();

    void handleScanDrive();
    void handleViewDrive();
    void handleViewSmart();
    void handleSelfTest();
    void handleCloneDrive();
    void handleTrimDrive();
    void handleEraseDrive();
    void handleUpdateFw();
    void handleDebug();

public:
    void showWidget_ViewDrive();
    void showWidget_ViewSmart();
    void showWidget_SelfTest();
    void showWidget_CloneDrive();
    void showWidget_TrimDrive();
    void showWidget_EraseDrive();
    void showWidget_UpdateFw();
    void showWidget_Debug();

public:
    QWidget* buildWidget_Common(
        QPlainTextEdit* text, QPushButton* btn[], int cnt);

    QWidget* buildWidget_ViewDrive();
    QWidget* buildWidget_ViewSmart();
    QWidget* buildWidget_DiskClone();
    QWidget* buildWidget_UpdateFw();
    QWidget* buildWidget_TrimDrive();
    QWidget* buildWidget_SelfTest();
    QWidget* buildWidget_EraseDrive();
    QWidget* buildWidget_Debug();

public slots:
    void handleSelfTestMode0();
    void handleSelfTestMode1();
    void handleSelfTestMode2();
    void handleSelfTestStop();
};

#endif // MAINWINDOW_H
