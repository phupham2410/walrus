#include <sstream>
#include "Worker.h"
#include "StorageApi.h"
#include "MainWindow.h"

#include "HexFrmt.h"

// ----------------------------------------------------------------------------
// Data structure to control StorageAPI

using namespace std;
using namespace StorageApi;

#define DEF_APP_DATA() \
    sAppData& data = gData; (void) data; \
    sAppData::sScanDriveData& scan = data.scan; (void) scan; \
    sAppData::sCommonData& cmn = data.cmn; (void) cmn

struct sAppData {
    // Data for ScanDrive, ViewInfo, ViewSmart functions
    struct sScanDriveData {
        StorageApi::tDriveArray darr;
        volatile StorageApi::sProgress progress;

        void init() {
            darr.clear();
            progress.reset();
        }

        std::string getProgressInfo() const {
            return *const_cast<std::string*>(&progress.info);
        }
    } scan;

    // Data for functions: SelfTest, SetOP, Erase, Optimize, Clone
    struct sCommonData {
        StorageApi::STR drvname;
        volatile StorageApi::sProgress progress;

        // Param for SetOP function
        StorageApi::U32 factor; // OP factor, 0 -> 100

        // Param for DiskClone function
        StorageApi::STR dstname;        // name of target drive
        StorageApi::tAddrArray partarr; // list of partition's address

        // Param for Firmware Update function
        StorageApi::STR fwname;       // firmware file, if any

        // Interchange buffer info (used in Read/Write/FwUpdate)
        StorageApi::U8* buffer = NULL;  // pointer to buffer
        StorageApi::U64 bufsize;        // buffer size
        StorageApi::U64 bufsecs;        // buffer-size in sector unit

        // init functions

        void initCommonParam(StorageApi::CSTR& drive) {
            drvname = drive;
            progress.reset();
        }

        void initSetopParam(StorageApi::CSTR& drive, StorageApi::U32 fOP) {
            factor = fOP;
            drvname = drive;
            progress.reset();
        }

        void initCloneParam(StorageApi::CSTR& drive,
                const StorageApi::tAddrArray& parr, StorageApi::CSTR& target) {
            drvname = drive;
            partarr = parr;
            dstname = target;
            progress.reset();
        }

        void initFwUpdateParam(StorageApi::CSTR& drive, StorageApi::CSTR& fwfile) {
            drvname = drive;
            fwname = fwfile;
            progress.reset();
        }

        void initBufferParam(StorageApi::U8* ptr, StorageApi::U64 size) {
            if (buffer) free(buffer);
            buffer = ptr; bufsize = size;
            if (!buffer) buffer = (uint8_t*) malloc(bufsize);
            memset(buffer, 0x00, bufsize);
            bufsecs = (bufsize + SECTOR_SIZE - 1) / SECTOR_SIZE;
        }
    } cmn;

    MainWindow* parent;

    void init(void* mw) {
        scan.init();
        cmn.initCommonParam("");
        parent = (MainWindow*) mw;
    }

    bool getDriveInfo(StorageApi::CSTR& name, StorageApi::sDriveInfo& ref) {
        for (uint32_t i = 0; i < scan.darr.size(); i++) {
            StorageApi::sDriveInfo& di = scan.darr[i];
            if (di.name == name) { ref = di; return true; }
        }
        return false;
    }
} gData;

// ----------------------------------------------------------------------------

MainWindow::MainWindow(QWidget *parent)
    : QDialog(parent)
{
    initWindow();
    gData.init(this);
}

MainWindow::~MainWindow()
{

}

// ----------------------------------------------------------------------------

#define MODEL_MAX_LEN 30

void MainWindow::showDriveList()
{
    DEF_APP_DATA();
    QListWidget* lst = ctrl.DrvList;
    const StorageApi::tDriveArray& darr = scan.darr;
    lst->clear();
    for(int i = 0, maxi = darr.size(); i < maxi; i++) {
        const StorageApi::sDriveInfo& drv = darr[i];
        std::stringstream sstr;
        sstr << (drv.id.cap >> 21) << " GB" << std::endl;
        sstr << drv.id.model << std::endl
             << std::string(MODEL_MAX_LEN, '-');
        lst->addItem(QString(sstr.str().c_str()));
    }
    QCoreApplication::processEvents();
}

void MainWindow::setLog(QPlainTextEdit* w, const QString &str)
{
    w->setPlainText(str);
    QCoreApplication::processEvents();
}

void MainWindow::appendLog(QPlainTextEdit* w, const QString &str)
{
    w->appendPlainText(str);
    QScrollBar *vbar = w->verticalScrollBar();
    vbar->setValue(vbar->maximum());
    QCoreApplication::processEvents();
}

// ----------------------------------------------------------------------------

void MainWindow::enableGui(bool status) {
    QWidget* ena[] = {
        ctrl.DrvList,
        ctrl.ScanBtn, ctrl.InfoBtn,
        ctrl.SmartBtn, ctrl.TestBtn,
        ctrl.TrimBtn, ctrl.EraseBtn,
        ctrl.CloneBtn,
        ctrl.UpdateBtn,
    };

    QWidget* dis[] = {

    };

    int i, count;

    count = sizeof(ena) / sizeof(ena[0]);
    for (i = 0; i < count; i++)
        ena[i]->setEnabled(status);

    count = sizeof(dis) / sizeof(dis[0]);
    for (i = 0; i < count; i++)
        dis[i]->setEnabled(!status);

    QCoreApplication::processEvents();
}

void MainWindow::resetProgress() {
    ctrl.ProgBar->reset();
}

void MainWindow::setProgress(unsigned int val) {
    ctrl.ProgBar->setValue(val);
}

void MainWindow::setProgress(unsigned int min, unsigned int max) {
    if (!min && !max) ctrl.ProgBar->reset();
    else {
        ctrl.ProgBar->setRange(min, max);
        ctrl.ProgBar->setValue(0);
    }
}

// ----------------------------------------------------------------------------

#include <chrono>
#include <ctime>
#include <sys/time.h>

static uint64_t gStartTime, gStopTime;

static void StartTimer() {
    struct timespec ct;
    clock_gettime(CLOCK_REALTIME, &ct);
    gStartTime = ct.tv_sec * 1000LL + ct.tv_nsec / 1000000;
    gStopTime = 0;
}

static void StopTimer() {
    struct timespec ct;
    clock_gettime(CLOCK_REALTIME, &ct);
    gStopTime = ct.tv_sec * 1000LL + ct.tv_nsec / 1000000;
}

static uint64_t GetTimeDiff() {
    return (gStopTime >= gStartTime) ? (gStopTime - gStartTime) : 0;
}

void MainWindow::startWorker(tWorkerFunc func, void *param) {
    Worker* wkr = new Worker();
    connect(wkr, &Worker::finished, wkr, &QObject::deleteLater);
    wkr->setFunc(func, param); wkr->start();
}

void MainWindow::waitProcessing(volatile StorageApi::sProgress& prog) {
    // wait for workload info
    while(!prog.ready)
        QCoreApplication::processEvents();

    // set workload info:
    setProgress(0, prog.workload);

    // polling status
    StorageApi::U32 curr, prev = prog.progress;
    while(!prog.done) {
        QCoreApplication::processEvents();
        curr = prog.progress;
        if (prev != curr) {
            prev = curr; setProgress(curr);
        }
    }
    resetProgress();
}

static void FormatWidget(QPlainTextEdit* w) {
    QFont font("Courier New", 11); QFontMetrics fmtr(font);
    w->setFont(font); w->setStyleSheet("border: 1px solid");
    w->setLineWrapMode(QPlainTextEdit::NoWrap); w->setReadOnly(true);
}

static void FormatWidget(QListWidget* w) {
    QFont font("Courier New", 11); QFontMetrics fmtr(font);
    w->setFont(font); w->setStyleSheet("border: 1px solid");
}

static void FormatWidget(QComboBox* w) {
    QFont font("Courier New", 11); QFontMetrics fmtr(font);
    int fw = fmtr.maxWidth(), fh = fmtr.height();
    int minw = fw * 40; w->setMinimumWidth(minw);
    int minh = 3 * fh; w->setMinimumHeight(minh);
    w->setFont(font); w->setStyleSheet("border: 1px solid");
}

static void FormatWidget(QPushButton* w) {
    QFont font("Courier New", 11); QFontMetrics fmtr(font);
    int fw = fmtr.maxWidth(), fh = fmtr.height(); w->setFixedSize(90, 3 * fh);
}

// ----------------------------------------------------------------------------
#define ALLOC_WIDGET(t, n) do { \
    t* __v = new t(this); FormatWidget(__v); n = __v; } while(0)

#define BUILD_WIDGET_0(p, t) \
    ALLOC_WIDGET(QPlainTextEdit, p.t); \
    QPushButton* barr[] = { }; \
    return buildWidget_Common(p.t, barr, 0)

#define BUILD_WIDGET_2(p, t, b0, b1) \
    ALLOC_WIDGET(QPlainTextEdit, p.t); \
    ALLOC_WIDGET(QPushButton, p.b0); \
    ALLOC_WIDGET(QPushButton, p.b1); \
    QPushButton* barr[] = { p.b0, p.b1 }; \
    return buildWidget_Common(p.t, barr, 2)

#define BUILD_WIDGET_3(p, t, b0, b1, b2) \
    ALLOC_WIDGET(QPlainTextEdit, p.t); \
    ALLOC_WIDGET(QPushButton, p.b0); \
    ALLOC_WIDGET(QPushButton, p.b1); \
    ALLOC_WIDGET(QPushButton, p.b2); \
    QPushButton* barr[] = { p.b0, p.b1, p.b2 }; \
    return buildWidget_Common(p.t, barr, 3)

#define BUILD_WIDGET_4(p, t, b0, b1, b2, b3) \
    ALLOC_WIDGET(QPlainTextEdit, p.t); \
    ALLOC_WIDGET(QPushButton, p.b0); \
    ALLOC_WIDGET(QPushButton, p.b1); \
    ALLOC_WIDGET(QPushButton, p.b2); \
    ALLOC_WIDGET(QPushButton, p.b3); \
    QPushButton* barr[] = { p.b0, p.b1, p.b2, p.b3 };\
    return buildWidget_Common(p.t, barr, 4)

QWidget* MainWindow::buildWidget_Common(
    QPlainTextEdit* text, QPushButton* btn[], int cnt) {
    QWidget* w = new QWidget(this);

    QVBoxLayout* b = new QVBoxLayout();
    b->setContentsMargins(1,1,1,1);
    b->setSpacing(1); b->addWidget(text);

    if (cnt) {
        QHBoxLayout* r = new QHBoxLayout();
        r->setContentsMargins(1,1,1,1);
        r->setSpacing(1); r->addStretch();
        for (int i = 0; i < cnt; i++) r->addWidget(btn[i]);
        r->addStretch();
        b->addLayout(r); b->addStretch();
    }

    w->setLayout(b);
    return w;
}

QWidget* MainWindow::buildWidget_ViewDrive() {
    BUILD_WIDGET_0(ctrl.di, Console);
}

QWidget* MainWindow::buildWidget_ViewSmart() {
    BUILD_WIDGET_0(ctrl.sm, Console);
}

#include <QSplitter>
QWidget* MainWindow::buildWidget_DiskClone() {
    ALLOC_WIDGET(QListWidget,    ctrl.dc.List    );
    ALLOC_WIDGET(QComboBox,      ctrl.dc.Target  );
    ALLOC_WIDGET(QPlainTextEdit, ctrl.dc.Console );
    ALLOC_WIDGET(QPushButton,    ctrl.dc.StartBtn);
    ALLOC_WIDGET(QPushButton,    ctrl.dc.StopBtn );
    ALLOC_WIDGET(QPushButton,    ctrl.dc.ReadBtn );
    // ctrl.dc.List->setFixedHeight(200);

    QSplitter* splt = new QSplitter(this);
    splt->setOrientation(Qt::Vertical);

    // QHBoxLayout* l = new QHBoxLayout();
    // l->setContentsMargins(1,1,1,1);
    // l->setSpacing(1);
    // l->addWidget(ctrl.dc.Console);

    splt->addWidget(ctrl.dc.Console);

    QVBoxLayout* b = new QVBoxLayout();
    b->setContentsMargins(1,1,1,1);
    b->setSpacing(1);
    b->addWidget(ctrl.dc.List);

    QHBoxLayout* t = new QHBoxLayout();
    t->setContentsMargins(1,1,1,1);
    t->setSpacing(1);
    t->addWidget(new QLabel("Target Drive: "));
    t->addWidget(ctrl.dc.Target);
    t->addStretch();
    t->addWidget(ctrl.dc.ReadBtn);
    t->addWidget(ctrl.dc.StartBtn);
    t->addWidget(ctrl.dc.StopBtn);
    b->addLayout(t);

    QWidget* w = new QWidget(this);
    w->setLayout(b);
    splt->addWidget(w);

    return splt;
}

QWidget* MainWindow::buildWidget_SelfTest() {
    BUILD_WIDGET_4(ctrl.st, Console, Mode0Btn, Mode1Btn, Mode2Btn, StopBtn);
}

QWidget* MainWindow::buildWidget_UpdateFw() {
    BUILD_WIDGET_2(ctrl.uf, Console, StartBtn, StopBtn);
}

QWidget* MainWindow::buildWidget_TrimDrive() {
    BUILD_WIDGET_2(ctrl.td, Console, StartBtn, StopBtn);
}

QWidget* MainWindow::buildWidget_SecureWipe() {
    BUILD_WIDGET_2(ctrl.sw, Console, StartBtn, StopBtn);
}

QWidget* MainWindow::buildWidget_Debug() {
    BUILD_WIDGET_3(ctrl.db, Console, ReadBtn, FillBtn, StopBtn);
}
// ----------------------------------------------------------------------------

#define SHOWBTN 1
#define HIDEBTN 0

#define FRMTBTN(btn, text) do { \
    btn->setText(text); btn->setFixedSize(90, 3 * fh); } while(0)

#define ADDBTN(btn, name, box, slot, showbtn) do { \
    btn = new QPushButton(this); FRMTBTN(btn, name); box->addWidget(btn); \
    connect (btn, SIGNAL(clicked()), this, SLOT(slot())); \
    if (!showbtn) btn->hide(); } while(0)

void MainWindow::initWindow()
{
    QFont font("Courier New", 11); QFontMetrics fmtr(font);
    int fw = fmtr.maxWidth(), fh = fmtr.height();

    QHBoxLayout* layout = new QHBoxLayout();
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(1);

    if (1) {
        QVBoxLayout* b = new QVBoxLayout();
        b->setContentsMargins(1,1,1,1);
        b->setSpacing(1);
        ADDBTN(ctrl.ScanBtn, "Scan\nDrive", b,        handleScanDrive, SHOWBTN);

        ADDBTN(ctrl.InfoBtn, "Drive\nInfo", b,        handleViewDrive, SHOWBTN);
        ADDBTN(ctrl.SmartBtn, "SMART\nView", b,       handleViewSmart, SHOWBTN);
        ADDBTN(ctrl.CloneBtn, "Disk\nClone", b,       handleDiskClone, SHOWBTN);
        ADDBTN(ctrl.TrimBtn, "Optimize\nDrive", b,    handleTrimDrive, HIDEBTN);
        ADDBTN(ctrl.EraseBtn, "Secure\nWipe", b,      handleSecureWipe,HIDEBTN);
        ADDBTN(ctrl.TestBtn, "Self\nTest", b,         handleSelfTest,  HIDEBTN);
        ADDBTN(ctrl.UpdateBtn, "Update\nFirmware", b, handleUpdateFw,  HIDEBTN);
        b->addStretch();
        ADDBTN(ctrl.DebugBtn, "Debug\n", b,           handleDebug,     SHOWBTN);
        layout->addLayout(b);
    }

    if(1) {
        QListWidget* w = new QListWidget(this);
        QVBoxLayout* b = new QVBoxLayout();
        w->setFixedWidth(MODEL_MAX_LEN * fw); w->setFont(font);
        b->setContentsMargins(0, 0, 0, 0);
        b->setSpacing(1); b->addWidget(w);
        layout->addLayout(b);
        ctrl.DrvList = w;
        connect(w, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(handleSelectDrive()));
    }

    if(1) {
        QStackedWidget* s = new QStackedWidget(this);
        #define MAP_ITEM(name, code) s->addWidget(buildWidget_##name());
        #include "FuncList.def"
        #undef MAP_ITEM
        s->setCurrentIndex(0);

        QVBoxLayout* b = new QVBoxLayout();
        b->setContentsMargins(0, 0, 0, 0);
        b->setSpacing(1); b->addWidget(s);

        if (1) {
            QProgressBar* p = new QProgressBar(this);
            b->addWidget(p); ctrl.ProgBar = p;
        }

        layout->addLayout(b);
        ctrl.Stack = s;
    }

    if (1) {
        // Connect signal handlers
        #define CONNECT_ITEM_1(prefix, name, func) \
            FRMTBTN(prefix.func##Btn, #func); \
            connect(prefix.func##Btn, SIGNAL(clicked()), this, SLOT(handle##name##func()))
        #define CONNECT_ITEM_2(p, n, f0, f1) \
            CONNECT_ITEM_1(p, n, f0); CONNECT_ITEM_1(p, n, f1)
        #define CONNECT_ITEM_3(p, n, f0, f1, f2) \
            CONNECT_ITEM_2(p, n, f0, f1); CONNECT_ITEM_1(p, n, f2)
        #define CONNECT_ITEM_4(p, n, f0, f1, f2, f3) \
            CONNECT_ITEM_3(p, n, f0, f1, f2); CONNECT_ITEM_1(p, n, f3)

        CONNECT_ITEM_3(ctrl.dc, DiskClone, Read, Start, Stop);
        CONNECT_ITEM_2(ctrl.uf, UpdateFw, Start, Stop);
        CONNECT_ITEM_2(ctrl.td, TrimDrive, Start, Stop);
        CONNECT_ITEM_2(ctrl.sw, SecureWipe, Start, Stop);
        CONNECT_ITEM_3(ctrl.db, Debug, Read, Fill, Stop);
        CONNECT_ITEM_4(ctrl.st, SelfTest, Mode0, Mode1, Mode2, Stop);
        #undef MAP_ITEM
    }

    setLayout(layout);

    int minw = ctrl.DrvList->width() * 3.5;
    int minh = fh * 3 * 4 * 3;
    setMinimumSize(minw, minh);
    setWindowTitle("StorageAPI Usage V4");
    showMaximized();
    enableGui(true);
}

// ----------------------------------------------------------------------------

void MainWindow::handleCommonStop()
{
    DEF_APP_DATA();

    volatile StorageApi::sProgress* cmnp = &cmn.progress;
    volatile StorageApi::sProgress* scnp = &scan.progress;

    // Hint to stop the current task
    cmnp->stop = true;
    scnp->stop = true;
    // appendLog("Stopping worker ...");
}

// ----------------------------------------------------------------------------

void MainWindow::handleSelectDrive() {
    int widx = ctrl.Stack->currentIndex();
    #define MAP_ITEM(func, code) case code: showWidget_##func(); return;
    switch(widx) {
        #include "FuncList.def"
    }
    #undef MAP_ITEM
}

#define MAP_ITEM(funcname, viewidx) \
void MainWindow::handle##funcname() { \
        ctrl.Stack->setCurrentIndex(viewidx); showWidget_##funcname(); }
#include "FuncList.def"
#undef MAP_ITEM

// ----------------------------------------------------------------------------

#define RETURN_NO_DRIVE_FOUND_IF(cond) if (cond) do { \
    setLog(cs, "No drive found. Please scan drive first."); return; } while(0)
#define RETURN_NO_SEL_DRIVE_IF(cond) if (cond) do { \
    setLog(cs, "Please select one drive."); return; } while(0)
#define RETURN_WRONG_INDEX_IF(cond) if (cond) do { \
    setLog(cs, "Something's wrong with drive index."); return; } while(0)
#define RETURN_NO_TARGET_DRIVE_IF(cond) if (cond) do { \
    setLog(cs, "No target drive found. Please insert new drive."); return; } while(0)
#define RETURN_NOT_SUPPORT_IF(cond) if (cond) do { \
    setLog(cs, "Feature not support on this drive."); return; } while(0)
// ----------------------------------------------------------------------------

void ScanDriveThreadFunc(void* param) {
    DEF_APP_DATA(); (void) param;
    StorageApi::ScanDrive(scan.darr, true, true, &scan.progress);
}

void MainWindow::handleScanDrive()
{
    DEF_APP_DATA();
    enableGui(false);
    // setLog(cs, "Scanning drive ...");
    scan.init(); showDriveList();
    startWorker(ScanDriveThreadFunc, &scan);
    waitProcessing(scan.progress);

    // std::stringstream sstr;
    // sstr << "Scanning Done! Return Code: "
    //      << StorageApi::ToString(scan.progress.rval) << std::endl
    //      << "Number of drives: " << scan.darr.size() << std::endl;
    // sstr << "Progress info: " << scan.getProgressInfo() << std::endl;
    // appendLog(cs, QString(sstr.str().c_str()));
    showDriveList(); enableGui(true);
}

// ----------------------------------------------------------------------------

void MainWindow::showWidget_ViewDrive()
{
    DEF_APP_DATA();
    QPlainTextEdit* cs = ctrl.di.Console;
    int idx = ctrl.DrvList->currentRow();
    int maxi = scan.darr.size();
    RETURN_NO_DRIVE_FOUND_IF(!maxi);
    RETURN_NO_SEL_DRIVE_IF(maxi && (idx < 0));
    RETURN_WRONG_INDEX_IF((idx < 0) || (idx >= maxi));
    const StorageApi::sDriveInfo& drv = scan.darr[idx];
    setLog(cs, QString(StorageApi::ToShortString(drv).c_str()));
    QString pstr = QString::fromStdWString(StorageApi::ToWideString(drv.pi));
    appendLog(cs, "Partitions: \n" + pstr);
}

// ----------------------------------------------------------------------------

void MainWindow::showWidget_ViewSmart()
{
    DEF_APP_DATA();
    QPlainTextEdit* cs = ctrl.sm.Console;
    int idx = ctrl.DrvList->currentRow();
    int maxi = scan.darr.size();
    RETURN_NO_DRIVE_FOUND_IF(!maxi);
    RETURN_NO_SEL_DRIVE_IF(maxi && (idx < 0));
    RETURN_WRONG_INDEX_IF((idx < 0) || (idx >= maxi));
    const StorageApi::sDriveInfo& drv = scan.darr[idx];
    RETURN_NOT_SUPPORT_IF(!drv.id.features.smart);
    setLog(cs, QString(StorageApi::ToString(drv.si, true, 2).c_str()));
}

// ----------------------------------------------------------------------------

void SelfTestThreadFunc(void* param) {
    DEF_APP_DATA(); (void) param;
    StorageApi::SelfTest(cmn.drvname, 0, &cmn.progress);
}

void MainWindow::handleSelfTestMode0() {}
void MainWindow::handleSelfTestMode1() {}
void MainWindow::handleSelfTestMode2()  {}
void MainWindow::handleSelfTestStop() {}
void MainWindow::handleUpdateFwStart() {}
void MainWindow::handleUpdateFwStop() {}
void MainWindow::handleTrimDriveStart() {}
void MainWindow::handleTrimDriveStop() {}
void MainWindow::handleSecureWipeStart() {}
void MainWindow::handleSecureWipeStop() {}
void MainWindow::handleDebugRead() {}
void MainWindow::handleDebugFill() {}
void MainWindow::handleDebugStop() {}

void MainWindow::showWidget_SelfTest()
{
    DEF_APP_DATA();
    QPlainTextEdit* cs = ctrl.st.Console;

    int idx = ctrl.DrvList->currentRow();
    int maxi = scan.darr.size();
    RETURN_NO_DRIVE_FOUND_IF(!maxi);
    RETURN_NO_SEL_DRIVE_IF(maxi && (idx < 0));
    RETURN_WRONG_INDEX_IF((idx < 0) || (idx >= maxi));
    const StorageApi::sDriveInfo& drv = scan.darr[idx];
    RETURN_NOT_SUPPORT_IF(!drv.id.features.test);
    setLog(cs, "Executing Self-Test on drive " + QString(drv.name.c_str()));

    enableGui(false);
    cmn.initCommonParam(drv.name);
    startWorker(SelfTestThreadFunc, &cmn);
    waitProcessing(cmn.progress);

    std::stringstream sstr;
    std::string info = *const_cast<std::string*>(&cmn.progress.info);
    sstr << "SelfTest Done! Return Code: "
         << StorageApi::ToString(cmn.progress.rval) << std::endl
         << "Report: " << std::endl << info;
    setLog(cs, QString(sstr.str().c_str()));
    enableGui(true);
}

// ----------------------------------------------------------------------------

void SetopThreadFunc(void* param) {
    DEF_APP_DATA(); (void) param;
    StorageApi::SetOverProvision(cmn.drvname, cmn.factor, &cmn.progress);
}

// ----------------------------------------------------------------------------

#define RAND(m) (rand() % m)

static bool GetRandomSet(uint32_t max, uint32_t count, std::vector<uint32_t>& v) {
    v.clear(); if (max < count) return false;
    for (uint32_t i = 0, r = RAND(max); i < count; i++, r = RAND(max)) {
        while(std::find(v.begin(), v.end(), r) != v.end()) r = RAND(max);
        v.push_back(r);
    }
    return true;
}

// Pick some partitions from current drive
static void GetRamdomParts(const StorageApi::sDriveInfo& drv, StorageApi::tAddrArray& sels)
{
    sels.clear();

    const StorageApi::tPartArray& parr = drv.pi.parr;
    uint32_t pcnt = parr.size(); if (!pcnt) return;

    // Generate mutual exclusive indexes in range [0,pcnt)
    std::vector<uint32_t> iset;
    uint32_t icnt = (rand() % pcnt) + 1;
    GetRandomSet(pcnt, icnt, iset);
    for(uint32_t i = 0; i < icnt; i++) {
        sels.push_back(parr[iset[i]].addr);
    }
}

void DiskCloneThreadFunc(void* param) {
    DEF_APP_DATA(); (void) param;
    StorageApi::CloneDrive(cmn.dstname, cmn.drvname, cmn.partarr, &cmn.progress);
}

static void FillTargetDrive(QComboBox* box, tDriveArray& darr, U32 cidx) {
    box->clear();
    for (U32 i = 0, maxi = darr.size(); i < maxi; i++) {
        if (i == cidx) continue;
        sDriveInfo& drv = darr[i];
        stringstream sstr;
        sstr << drv.id.model;
        box->addItem(QString(sstr.str().c_str()));
    }
}

#include "PartWidget.h"

static void ShowDrivePartition(QListWidget* box, const sDriveInfo& drv) {
    box->clear();
    for (int i = 0, maxi = drv.pi.parr.size(); i < maxi; i++) {
        const sPartition& p = drv.pi.parr[i];
        QListWidgetItem* item = new QListWidgetItem();
        PartWidget* pw = new PartWidget(); pw->setPart(p);
        item->setSizeHint(pw->sizeHint());
        box->addItem(item); box->setItemWidget(item, pw);
    }
}

static bool GetSourcePartArray(QListWidget* box, tAddrArray& pa) {
    pa.clear();
    for (int i = 0, maxi = box->count(); i < maxi; i++) {
        QListWidgetItem* item = box->item(i);
        PartWidget* pw = (PartWidget*) box->itemWidget(item);
        tPartAddr addr; if (!pw->getPartInfo(addr)) continue;
        pa.push_back(addr);
    }
    return pa.max_size() != 0;
}

static bool GetTargetDriveInfo(QComboBox* box, string& dst) {
    if (!box->count()) return false;
    dst = box->currentText().toStdString();
    return true;
}

void MainWindow::showWidget_DiskClone()
{
    // Show all partitions on selected drive

    DEF_APP_DATA();
    QPlainTextEdit* cs = ctrl.dc.Console;

    int idx = ctrl.DrvList->currentRow();
    int maxi = scan.darr.size();
    RETURN_NO_DRIVE_FOUND_IF(!maxi);
    RETURN_NO_SEL_DRIVE_IF(maxi && (idx < 0));
    RETURN_WRONG_INDEX_IF((idx < 0) || (idx >= maxi));
    RETURN_NO_TARGET_DRIVE_IF(maxi <= 1);

    const StorageApi::sDriveInfo& drv = scan.darr[idx];

    // Show list of partitions of current drive
    ShowDrivePartition(ctrl.dc.List, drv);

    // Fill Target with remaining drives
    FillTargetDrive(ctrl.dc.Target, scan.darr, idx);
}

#include <fileapi.h>
#include <errhandlingapi.h>

void MainWindow::handleDiskCloneRead()
{
    DEF_APP_DATA();
    QPlainTextEdit* cs = ctrl.dc.Console;

    int idx = ctrl.DrvList->currentRow();
    int maxi = scan.darr.size();
    RETURN_NO_DRIVE_FOUND_IF(!maxi);
    RETURN_NO_SEL_DRIVE_IF(maxi && (idx < 0));
    RETURN_WRONG_INDEX_IF((idx < 0) || (idx >= maxi));
    RETURN_NO_TARGET_DRIVE_IF(maxi <= 1);

    // Generate random params for DiskClone
    // In the real code, user will select these params from GUI
    const StorageApi::sDriveInfo& drv = scan.darr[idx];
    tAddrArray pa;  GetSourcePartArray(ctrl.dc.List, pa);
    string dstname; GetTargetDriveInfo(ctrl.dc.Target, dstname);

    // Now, clone partitions in parr to new target drive
    setLog(cs, "Reading Drive " + QString(drv.name.c_str()));

    enableGui(false);

    // read drive drv, from address in pa
    do {
        stringstream sstr;
        if (pa.size() > 1) {
            appendLog(cs, "Too many partitions"); break;
        }
        const tPartAddr& addr = pa[0];

        StorageApi::HDL hdl; eRetCode ret;
        ret = StorageApi::Open(drv.name, hdl);
        if (RET_OK != ret) {
            sstr << "Cannot open device " << drv.name << endl
                 << "Ret code: " << StorageApi::ToString(ret) << endl;
            appendLog(cs, sstr.str().c_str()); break;
        }

        DWORD sc = 256, bufsize = sc * 512, rsize, rval;
        LONG lopos = addr.first & 0xFFFFFFFF;
        LONG hipos = (addr.first >> 32) & 0xFFFFFFFF;

        if (SetFilePointer((HANDLE)hdl, lopos, &hipos, 0) == INVALID_SET_FILE_POINTER) {
            rval = GetLastError();
            sstr << "Cannot seek file pointer. Error code: " << rval << endl;
            appendLog(cs, sstr.str().c_str()); break;
        }

        U8* bufptr = (U8*) malloc(bufsize); memset(bufptr, 0x00, bufsize);
        if (ReadFile((HANDLE) hdl, bufptr, bufsize, &rsize, NULL)) {
            stringstream rstr; U8* ptr = bufptr;
            for (U32 i = 0; i < sc; i++, ptr += 512) {
                U64 lba = (addr.first >> 9) + i;
                rstr << "LBA: " << lba << ": " << endl;
                rstr << HexFrmt::ToHexString(ptr, 512) << endl;
            }

            appendLog(cs, rstr.str().c_str());
        }

        StorageApi::Close(hdl);
    } while(0);

    enableGui(true);
}

void MainWindow::handleDiskCloneStart()
{
    DEF_APP_DATA();
    QPlainTextEdit* cs = ctrl.dc.Console;

    int idx = ctrl.DrvList->currentRow();
    int maxi = scan.darr.size();
    RETURN_NO_DRIVE_FOUND_IF(!maxi);
    RETURN_NO_SEL_DRIVE_IF(maxi && (idx < 0));
    RETURN_WRONG_INDEX_IF((idx < 0) || (idx >= maxi));
    RETURN_NO_TARGET_DRIVE_IF(maxi <= 1);

    // Generate random params for DiskClone
    // In the real code, user will select these params from GUI
    const StorageApi::sDriveInfo& drv = scan.darr[idx];
    tAddrArray pa;  GetSourcePartArray(ctrl.dc.List, pa);
    string dstname; GetTargetDriveInfo(ctrl.dc.Target, dstname);

    // Now, clone partitions in parr to new target drive
    setLog(cs, "Cloning Drive " + QString(drv.name.c_str()));

    enableGui(false);
    cmn.initCloneParam(drv.name, pa, dstname);

    if (1) {
        // Should not dump message in worker thread, so put it here
        std::stringstream sstr;
        sstr << "Cloning data from source(" << cmn.drvname
             << ") to target(" << cmn.dstname << ")" << std::endl;
        const StorageApi::tAddrArray& arr = cmn.partarr;
        sstr << "List of selected partitions: " << std::endl;
        for (uint32_t i = 0, maxi = arr.size(); i < maxi; i++) {
            const StorageApi::tPartAddr& item = arr[i];
            sstr << "Section(" << item.first << "," << item.second << ")" << std::endl;
        }
        appendLog(cs, sstr.str().c_str());
    }

    startWorker(DiskCloneThreadFunc, &cmn);
    waitProcessing(cmn.progress);

    std::stringstream sstr;
    std::string info = *const_cast<std::string*>(&cmn.progress.info);
    sstr << "Clone Drive Done! Return Code: "
         << StorageApi::ToString(cmn.progress.rval) << std::endl
         << "Report: " << std::endl << info;
    appendLog(cs, QString(sstr.str().c_str()));
    enableGui(true);
}

void MainWindow::handleDiskCloneStop()
{
    handleCommonStop();
}

// ----------------------------------------------------------------------------

void TrimDriveThreadFunc(void* param) {
    DEF_APP_DATA(); (void) param;
    StorageApi::OptimizeDrive(cmn.drvname, &cmn.progress);
}

void MainWindow::showWidget_TrimDrive()
{
    DEF_APP_DATA();
    QPlainTextEdit* cs = ctrl.td.Console;

    int idx = ctrl.DrvList->currentRow();
    int maxi = scan.darr.size();
    RETURN_NO_DRIVE_FOUND_IF(!maxi);
    RETURN_NO_SEL_DRIVE_IF(maxi && (idx < 0));
    RETURN_WRONG_INDEX_IF((idx < 0) || (idx >= maxi));
    const StorageApi::sDriveInfo& drv = scan.darr[idx];
    RETURN_NOT_SUPPORT_IF(!drv.id.features.trim);
    setLog(cs, "Optimizing Drive " + QString(drv.name.c_str()));

    enableGui(false);
    cmn.initCommonParam(drv.name);
    startWorker(TrimDriveThreadFunc, &cmn);
    waitProcessing(cmn.progress);

    std::stringstream sstr;
    std::string info = *const_cast<std::string*>(&cmn.progress.info);
    sstr << "Optimizing Drive Done! Return Code: "
         << StorageApi::ToString(cmn.progress.rval) << std::endl
         << "Report: " << std::endl << info;
    setLog(cs, QString(sstr.str().c_str()));
    enableGui(true);
}

// ----------------------------------------------------------------------------

void SecureEraseThreadFunc(void* param) {
    DEF_APP_DATA(); (void) param;
    StorageApi::SecureErase(cmn.drvname, &cmn.progress);
}

void MainWindow::showWidget_SecureWipe()
{
    DEF_APP_DATA();
    QPlainTextEdit* cs = ctrl.sw.Console;

    int idx = ctrl.DrvList->currentRow();
    int maxi = scan.darr.size();
    RETURN_NO_DRIVE_FOUND_IF(!maxi);
    RETURN_NO_SEL_DRIVE_IF(maxi && (idx < 0));
    RETURN_WRONG_INDEX_IF((idx < 0) || (idx >= maxi));
    const StorageApi::sDriveInfo& drv = scan.darr[idx];
    RETURN_NOT_SUPPORT_IF(!drv.id.features.erase);
    setLog(cs, "Secure Wipe Drive " + QString(drv.name.c_str()));

    enableGui(false);
    cmn.initCommonParam(drv.name);
    startWorker(SecureEraseThreadFunc, &cmn);
    waitProcessing(cmn.progress);

    std::stringstream sstr;
    std::string info = *const_cast<std::string*>(&cmn.progress.info);
    sstr << "Secure Wipe Drive Done! Return Code: "
         << StorageApi::ToString(cmn.progress.rval) << std::endl
         << "Report: " << std::endl << info;
    setLog(cs, QString(sstr.str().c_str()));
    enableGui(true);
}

// ----------------------------------------------------------------------------

void UpdateFirmwareThreadFunc(void* param) {
    DEF_APP_DATA(); (void) param;
    StorageApi::UpdateFirmware(cmn.drvname, cmn.buffer, cmn.bufsize, &cmn.progress);
}

void MainWindow::showWidget_UpdateFw()
{
    DEF_APP_DATA();
    QPlainTextEdit* cs = ctrl.uf.Console;

    int idx = ctrl.DrvList->currentRow();
    int maxi = scan.darr.size();
    RETURN_NO_DRIVE_FOUND_IF(!maxi);
    RETURN_NO_SEL_DRIVE_IF(maxi && (idx < 0));
    RETURN_WRONG_INDEX_IF((idx < 0) || (idx >= maxi));
    const StorageApi::sDriveInfo& drv = scan.darr[idx];
    RETURN_NOT_SUPPORT_IF(!drv.id.features.dlcode);
    setLog(cs, "Update Firmware Drive " + QString(drv.name.c_str()));

    enableGui(false);
    cmn.initCommonParam(drv.name);
    startWorker(UpdateFirmwareThreadFunc, &cmn);
    waitProcessing(cmn.progress);

    std::stringstream sstr;
    std::string info = *const_cast<std::string*>(&cmn.progress.info);
    sstr << "Update Firmware on Drive Done! Return Code: "
         << StorageApi::ToString(cmn.progress.rval) << std::endl
         << "Report: " << std::endl << info;
    setLog(cs, QString(sstr.str().c_str()));
    enableGui(true);
}

// ----------------------------------------------------------------------------

void MainWindow::showWidget_Debug()
{
    DEF_APP_DATA();
    QPlainTextEdit* cs = ctrl.db.Console;

    // Try to read partition info on all drives
    std::stringstream sstr;

    for (uint32_t i = 0; i < 16; i++) {
        std::stringstream nstr;
        nstr << "\\\\.\\PhysicalDrive" << i;
        std::string drvname = nstr.str();


        StorageApi::sPartInfo pi;
        StorageApi::eRetCode ret = StorageApi::ScanPartition(drvname, pi);
        if (ret != StorageApi::RET_OK) continue;

        sstr << "Scan partitions on drive " << drvname << std::endl;
        sstr << StorageApi::ToString(pi) << std::endl;
    }

    setLog(cs, sstr.str().c_str());
}

// ----------------------------------------------------------------------------

static std::string ToSpeedString(uint64_t workload, uint64_t msec) {
    std::stringstream sstr;
    if (msec == 0) sstr << "Unknown I/O speed";
    else {
        struct item_t{ uint32_t shift; std::string unit; };
        item_t items[] = {
            {30, "GB/s"}, {20, "MB/s"}, {10, "KB/s"}, {0, "B/s"}
        };
        uint32_t icnt = sizeof(items) / sizeof(items[0]);
        uint64_t bps = workload / msec;
        for (uint32_t i = 0; i < icnt; i++) {
            const item_t& it = items[i];
            double vps = bps / (1 << it.shift); if (vps < 1) continue;
            sstr << vps << " " << it.unit << std::endl; break;
        }
    }
    return sstr.str();
}

static bool GetNextPartition(STR& drvname, sPartition& p) {
    DEF_APP_DATA();

    static std::map<std::string, uint32_t> pmap;

    StorageApi::sDriveInfo di;
    if (!gData.getDriveInfo(drvname, di)) return false;

    // select next partition on this drive
    if (pmap.find(drvname) == pmap.end()) pmap[drvname] = 0;

    uint32_t& idx = pmap[drvname];
    uint32_t maxidx = di.pi.parr.size();

    if (maxidx == 0) return false;

    if (idx >= maxidx) idx = 0;
    p = di.pi.parr[idx];
    idx = (idx + 1) % maxidx; return true;
}

void ReadDriveThreadFunc(void* param) {
    DEF_APP_DATA(); (void) param;

    // Read some sectors at teh beginning of next partition
    uint64_t count = 128, start = 0;
    StorageApi::sPartition part;
    if (GetNextPartition(cmn.drvname, part))
        start = part.addr.first / 512;

    start = (uint64_t) 100 * 1024 * 1024 * 1024 / 512;

    volatile StorageApi::sProgress* prog = &cmn.progress;

    prog->progress = 0;
    prog->workload = count;
    prog->ready = true;

    StorageApi::HDL handle;
    StorageApi::eRetCode ret = StorageApi::Open(cmn.drvname, handle);
    if (ret != StorageApi::RET_OK) {
        prog->rval = StorageApi::RET_FAIL; prog->done = true; return;
    }

    StorageApi::sAdapterInfo adapter;
    ret = StorageApi::GetDeviceInfo(handle, adapter);
    if (ret != StorageApi::RET_OK) {
        prog->rval = StorageApi::RET_FAIL; prog->done = true; return;
    }

    uint32_t sc = 1; // adapter.MaxTransferSector;
    uint32_t i, bufsize = SECTOR_SIZE * sc;
    uint8_t* buffer = (uint8_t*) malloc(bufsize);

    if (!buffer) {
        prog->rval = StorageApi::RET_OUT_OF_MEMORY; prog->done = true; return;
    }

    memset(buffer, 0x00, bufsize);

    uint64_t lba, end, fcnt = 0;
    std::stringstream sstr, fstr, dstr; // main, fail and data stream

    StartTimer();

    for (i = 0; i < count; i++) {
        if (prog->stop) {
            sstr << "User stop reading at LBA(" << end << ")" << std::endl; break;
        }
        // Read the next block
        lba = start + i * sc; end = lba + sc;
        ret = StorageApi::Read(handle, lba, sc, buffer, adapter.BusType);
        if (ret != StorageApi::RET_OK) {
            fcnt++;
            fstr << "Read Fail: "
                 << "LBA(" << lba << ") Count(" << sc << ")" << std::endl;
        }
        else {
            dstr << "LBA: " << lba << std::endl
                 << HexFrmt::ToHexString(buffer, sc * 512) << std::endl;
        }

        prog->progress++;
    }

    StopTimer();

    uint64_t secs = end - start;
    uint64_t tdiff = GetTimeDiff();

    sstr << "Reading drive " << cmn.drvname << ", "
         << "partition " << QString::fromStdWString(part.name).toStdString()
         << "(index " << part.index << ")" << std::endl
         << "Range: start_lba(" << start << ") -> end_lba(" << end << "), "
         << "total: " << secs << " sectors (" << secs * 512 << " bytes)" << std::endl
         << "Transfer size per command: " << sc * 512 << " bytes" << std::endl;

    if (fcnt) sstr << fstr.str() << std::endl;
    else sstr << "Speed: " << ToSpeedString(secs*512*1000, tdiff) << std::endl;

    sstr << dstr.str();
    *(const_cast<std::string*>(&prog->info)) += sstr.str();

    free(buffer);
    StorageApi::Close(handle);

    prog->rval = StorageApi::RET_OK; prog->done = true;

    start += count; return;
}

void MainWindow::handleReadDrive()
{
    DEF_APP_DATA();
    QPlainTextEdit* cs = ctrl.db.Console;

    int idx = ctrl.DrvList->currentRow();
    int maxi = scan.darr.size();
    RETURN_NO_DRIVE_FOUND_IF(!maxi);
    RETURN_NO_SEL_DRIVE_IF(maxi && (idx < 0));
    RETURN_WRONG_INDEX_IF((idx < 0) || (idx >= maxi));
    const StorageApi::sDriveInfo& drv = scan.darr[idx];
    RETURN_NOT_SUPPORT_IF(!drv.id.features.lbamode);
    setLog(cs, "Read (LBA48B) on drive " + QString(drv.name.c_str()));

    enableGui(false);
    cmn.initCommonParam(drv.name);
    startWorker(ReadDriveThreadFunc, &cmn);
    waitProcessing(cmn.progress);

    std::stringstream sstr;
    std::string info = *const_cast<std::string*>(&cmn.progress.info);
    sstr << "Debug Done! Return Code: "
         << StorageApi::ToString(cmn.progress.rval) << std::endl
         << "Report: " << std::endl << info;
    setLog(cs, QString(sstr.str().c_str()));
    enableGui(true);
}

void FillDriveThreadFunc(void* param) {
    DEF_APP_DATA(); (void) param;
    std::stringstream sstr;

    volatile StorageApi::sProgress* prog = &cmn.progress;
    prog->progress = 0; prog->workload = 100; prog->ready = true;

    // If filling on drive 0 --> skip it

    sDriveInfo di;
    if (!gData.getDriveInfo(cmn.drvname, di)) {
        prog->rval = StorageApi::RET_FAIL; prog->done = true; return;
    }

    // --------------------------------------------------
    // Step 1: Open device, get bustype & handle


    // --------------------------------------------------
    // Step 2: Read Capacity

    uint64_t size_in_byte = di.id.cap * 1024 * 1024 * 1024;
    uint64_t size_in_sector = size_in_byte / 512;

    sstr << "Size in gb: " << di.id.cap << std::endl;
    sstr << "Size in sector: " << size_in_sector << std::endl;
    sstr << "Size in byte: " << size_in_byte << std::endl;
    *(const_cast<std::string*>(&prog->info)) += sstr.str();

    // --------------------------------------------------
    // Step 3: Fill drive with pattern

    uint32_t sc = 1;
    uint8_t* buffer = (uint8_t*) malloc(sc * 512);
    if (!buffer) {
        prog->rval = StorageApi::RET_OUT_OF_MEMORY; prog->done = true; return;
    }

    for (uint64_t lba = 0; lba < size_in_sector; lba++) {
        // eRetCode ret = StorageApi::Write(hdl, lba, sc, buffer, bustype);
    }

    // Open device
    // Read capacity
    // Fill drive with pattern
    // Read each sector & check result
    // Report

    prog->rval = StorageApi::RET_NOT_IMPLEMENT; prog->done = true;
}

void MainWindow::handleFillDrive()
{
    DEF_APP_DATA();
    QPlainTextEdit* cs = ctrl.db.Console;

    int idx = ctrl.DrvList->currentRow();
    int maxi = scan.darr.size();
    RETURN_NO_DRIVE_FOUND_IF(!maxi);
    RETURN_NO_SEL_DRIVE_IF(maxi && (idx < 0));
    RETURN_WRONG_INDEX_IF((idx < 0) || (idx >= maxi));
    const StorageApi::sDriveInfo& drv = scan.darr[idx];
    RETURN_NOT_SUPPORT_IF(!drv.id.features.lbamode);
    setLog(cs, "Fill drive with pattern " + QString(drv.name.c_str()));

    enableGui(false);
    cmn.initCommonParam(drv.name);
    startWorker(FillDriveThreadFunc, &cmn);
    waitProcessing(cmn.progress);

    std::stringstream sstr;
    std::string info = *const_cast<std::string*>(&cmn.progress.info);
    sstr << "Debug Done! Return Code: "
         << StorageApi::ToString(cmn.progress.rval) << std::endl
         << "Report: " << std::endl << info;
    setLog(cs, QString(sstr.str().c_str()));
    enableGui(true);
}

// ----------------------------------------------------------------------------
