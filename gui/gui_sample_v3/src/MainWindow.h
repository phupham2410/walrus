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
#include <QScrollBar>

#include "StorageApi.h"

typedef void (*tWorkerFunc)(void*);

class MainWindow : public QDialog
{
    Q_OBJECT
public:
    struct sControl {
        QListWidget* DrvList;
        QPlainTextEdit* Console;
        QPushButton* ScanBtn;
        QPushButton* InfoBtn;
        QPushButton* SmartBtn;
        QPushButton* TestBtn;

        QPushButton* UpdateBtn;
        QPushButton* SetopBtn;
        QPushButton* CloneBtn;
        QPushButton* TrimBtn;
        QPushButton* EraseBtn;

        QProgressBar* ProgBar;
        QPushButton* StopBtn;

        QPushButton* DebugBtn;
        QPushButton* ReadBtn;
    };

    sControl ctrl;

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

public:
    void initWindow();

public:
    void showDriveList();
    void setLog(const QString& str);
    void appendLog(const QString& str);
    void enableGui(bool status);
    void resetProgress();
    void setProgress(unsigned int val);
    void setProgress(unsigned int min, unsigned int max);

public:
    void startWorker(tWorkerFunc func, void* param);
    void waitProcessing(volatile StorageApi::sProgress& prog);

public slots:
    void handleDebug();
    void handleReadDrive();

    void handleStop();
    void handleViewDrive();
    void handleViewSmart();
    void handleSelfTest();
    void handleScanDrive();

    void handleSetOverProvision();
    void handleCloneDrive();
    void handleTrimDrive();
    void handleEraseDrive();
    void handleUpdateFirmware();
};

#endif // MAINWINDOW_H
