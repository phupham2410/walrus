#ifndef PartWidget_H
#define PartWidget_H

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
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>

#include "StorageApi.h"

class PartWidget : public QWidget
{
    Q_OBJECT
public:
    struct sControl {
        QCheckBox* sel;
        QLabel* lbl;
        QDoubleSpinBox* val;
    };

    sControl ctrl;
    StorageApi::sPartition part;

public:
    PartWidget(QWidget *parent = 0);
    ~PartWidget();

public:
    void initWidget();
    void setPart(const StorageApi::sPartition& p);

public:
    uint64_t getPartSize() const;
    bool getPartInfo(StorageApi::tPartAddr& pa) const;
};

#endif // PartWidget_H
