#include <sstream>
#include "StorageApi.h"
#include "PartWidget.h"

// ----------------------------------------------------------------------------
// Data structure to control StorageAPI

using namespace std;
using namespace StorageApi;

PartWidget::PartWidget(QWidget* parent) : QWidget(parent) {
    initWidget();
}

PartWidget::~PartWidget() {

}

#define FORMAT_WIDGET(wgt, w, h) \
    if (w) wgt->setFixedSize(w, h); \
    else wgt->setFixedHeight(h); \
    wgt->setFont(font); wgt->setStyleSheet("border: 0px solid")

void PartWidget::initWidget() {
    QFont font("Courier New", 11); QFontMetrics frmt(font);
    int fw = frmt.maxWidth(), fh = frmt.height();
    int h3 = fh * 3, h2 = fh * 2;

    QCheckBox* cbox = new QCheckBox;
    FORMAT_WIDGET(cbox, fw * 2, h2);
    cbox->setChecked(true);

    QLabel* ibox = new QLabel("Partition");
    FORMAT_WIDGET(ibox, 0, h3);

    QDoubleSpinBox* sbox = new QDoubleSpinBox;
    FORMAT_WIDGET(sbox, fw * 8, h2); sbox->setRange(0, 100);

    QHBoxLayout* b = new QHBoxLayout;
    b->setContentsMargins(1,1,1,1); b->setSpacing(1);
    b->addWidget(cbox); b->addWidget(ibox); b->addWidget(sbox);

    setLayout(b); ctrl.sel = cbox; ctrl.lbl = ibox; ctrl.val = sbox;
}

void PartWidget::setPart(const StorageApi::sPartition& p) {
    F64 mincap = (p.cap * 100 >> 21) / 100.0;
    F64 maxcap = mincap + 200; // enable to extend 200GB

    wstringstream sstr;
    sstr << mincap << " GB" << endl; sstr << p.name;

    ctrl.lbl->setText(QString::fromStdWString(sstr.str()));
    ctrl.sel->setChecked(true);
    ctrl.val->setRange(mincap, maxcap);
    part = p;
}

uint64_t PartWidget::getPartSize() const {
    return ctrl.sel->isChecked() ? ctrl.val->value() : 0;
}

bool PartWidget::getPartInfo(StorageApi::tPartAddr& pa) const {
    pa.first = pa.second = 0;
    if (!ctrl.sel->isChecked()) return false;

    U64 new_byte;
    double org_gb = ((part.cap * 100) >> 21) / 100.0;
    double new_gb = ctrl.val->value();
    if (org_gb >= new_gb) new_byte = part.cap << 9;
    else new_byte = new_gb * 1024 * 1024 * 1024;

    pa.first = part.addr.first;
    pa.second = new_byte;
    return true;
}














