#include "cipherwidget.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSpacerItem>
#include <QStringListModel>
#include <QVBoxLayout>

#include <common.hpp>
#include <cscrypto/cscrypto.hpp>
#include <passwordlineedit.hpp>
#include <utils.hpp>

namespace cscrypto {
namespace gui {

CipherWidget::CipherWidget(QStringListModel& encryptionKeys,
                           QStringListModel& decryptionKeys,
                           QStatusBar& sb, QWidget* parent)
        : QWidget(parent),
          statusBar_(sb),
          sourceFileLbl_(new QLabel(tr("No source file loaded"), this)),
          targetFileLbl_(new QLabel(tr("No target file loaded"), this)),
          sourceFileLoaded_(false),
          targetFileLoaded_(false),
          encryptionMode_(false),
          decryptionMode_(false),
          startBtn_(new QPushButton(tr("Start"), this)),
          encryptionKeys_(encryptionKeys),
          decryptionKeys_(decryptionKeys) {
    tuneLayouts();
    startBtn_->setEnabled(false);
}

void CipherWidget::tuneLayouts() {
    QVBoxLayout* mainLayout = new QVBoxLayout;
    QVBoxLayout* modeLayout = new QVBoxLayout;
    QVBoxLayout* middleLayout = new QVBoxLayout;
    QVBoxLayout* pswdLayout = new QVBoxLayout;
    QHBoxLayout* lowLayout = new QHBoxLayout;

    fillModeLayout(modeLayout);
    fillMiddleLayout(middleLayout);
    fillPswdLayout(pswdLayout);
    fillLowLayout(lowLayout);

    mainLayout->addLayout(modeLayout);
    mainLayout->addLayout(middleLayout);
    mainLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    mainLayout->addLayout(pswdLayout);
    mainLayout->addLayout(lowLayout);
    setLayout(mainLayout);
}

void CipherWidget::fillModeLayout(QVBoxLayout* l) {
    QHBoxLayout* modeSelectionLayout = new QHBoxLayout;
    modeSelectionLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
    QRadioButton* encBtn = new QRadioButton(tr("Encryption"), this);
    QRadioButton* decBtn = new QRadioButton(tr("Decryption"), this);
    modeSelectionLayout->addWidget(encBtn);
    modeSelectionLayout->addWidget(decBtn);

    QHBoxLayout* useKeysLayout = new QHBoxLayout;
    useKeysLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
    QCheckBox* useKeysCheckBox = new QCheckBox(tr("Use available keys"), this);
    useKeysLayout->addWidget(useKeysCheckBox);
    useKeysCheckBox->setEnabled(false);

    l->addLayout(modeSelectionLayout);
    l->addLayout(useKeysLayout);

    connect(encBtn, &QRadioButton::clicked, this, &CipherWidget::activateEncryptionMode);
    connect(decBtn, &QRadioButton::clicked, this, &CipherWidget::activateDecryptionMode);
}

void CipherWidget::fillMiddleLayout(QVBoxLayout* l) {
    QPushButton* loadSrcBtn = new QPushButton(tr("Load source file"), this);
    QHBoxLayout* l1 = new QHBoxLayout;
    l1->addWidget(loadSrcBtn);
    l1->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

    l->addLayout(l1);
    l->addWidget(sourceFileLbl_);

    QPushButton* loadTrgBtn = new QPushButton(tr("Load target file"), this);
    QHBoxLayout* l2 = new QHBoxLayout;
    l2->addWidget(loadTrgBtn);
    l2->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

    l->addLayout(l2);
    l->addWidget(targetFileLbl_);

    connect(loadSrcBtn, &QPushButton::clicked, this, &CipherWidget::getSrcFileName);
    connect(loadTrgBtn, &QPushButton::clicked, this, &CipherWidget::getTrgFileName);
}

void CipherWidget::fillLowLayout(QLayout* l) {
    l->addWidget(startBtn_);
    l->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

    connect(this, &CipherWidget::canStart, startBtn_, &QPushButton::setEnabled);
    connect(startBtn_, &QPushButton::clicked, this, &CipherWidget::start);
}

void CipherWidget::fillPswdLayout(QLayout* l) {
    QLabel* encLbl = new QLabel(tr("Available encryption keys:"), this);
    l->addWidget(encLbl);
    encKeysComboBox_ = new QComboBox(this);
    l->addWidget(encKeysComboBox_);
    encKeysComboBox_->setModel(&encryptionKeys_);

    QLabel* decLbl = new QLabel(tr("Available decryption keys:"), this);
    l->addWidget(decLbl);
    decKeysComboBox_ = new QComboBox(this);
    l->addWidget(decKeysComboBox_);
    decKeysComboBox_->setModel(&decryptionKeys_);

    QLabel* lbl = new QLabel(tr("Password:"), this);
    l->addWidget(lbl);
    pswdLineEdit_ = new PasswordLineEdit(this);
    l->addWidget(pswdLineEdit_);
}

void CipherWidget::start() {
    auto pswd = pswdLineEdit_->text().toStdString();
    if (pswd.empty()) {
        toStatusBar(statusBar_, tr("Password empty! Fill it to encrypt / decrypt."));
        return;
    }
    if (pswd.size() < kMinPswdSize) {
        toStatusBar(statusBar_, tr("Password is too short!"));
        return;
    }
    auto cipherKey = cscrypto::cipher::getCipherKeyFromPassword(pswd.c_str(), pswd.size());
    if (encryptionMode_ && cscrypto::cipher::encryptFile(targetFileLbl_->text().toStdString().c_str(),
                                                         sourceFileLbl_->text().toStdString().c_str(),
                                                         cipherKey)) {
        toStatusBar(statusBar_, tr("Source file successfully encrypted."));
    }
    else if (decryptionMode_ && cscrypto::cipher::decryptFile(targetFileLbl_->text().toStdString().c_str(),
                                                              sourceFileLbl_->text().toStdString().c_str(),
                                                              cipherKey)) {
        toStatusBar(statusBar_, tr("Source file successfully decrypted."));
    }
    else {
        toStatusBar(statusBar_, tr("Unsuccessfull operation!"));
    }
}

void CipherWidget::getSrcFileName() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Choose source file"));
    if (fileName.isEmpty()) {
        sourceFileLbl_->setText(tr("No source file loaded"));
        toStatusBar(statusBar_, tr("No file selected!"));
        sourceFileLoaded_ = false;
        emit canStart(false);
        return;
    }
    sourceFileLbl_->setText(fileName);
    sourceFileLoaded_ = true;

    if (targetFileLoaded_ && (encryptionMode_ || decryptionMode_)) emit canStart(true);
}

void CipherWidget::getTrgFileName() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Choose target file"));
    if (fileName.isEmpty()) {
        targetFileLbl_->setText(tr("No target file loaded"));
        toStatusBar(statusBar_, tr("No file selected!"));
        targetFileLoaded_ = false;
        emit canStart(false);
        return;
    }
    targetFileLbl_->setText(fileName);
    targetFileLoaded_ = true;

    if (sourceFileLoaded_ && (encryptionMode_ || decryptionMode_)) emit canStart(true);
}

void CipherWidget::activateEncryptionMode() {
    emit encryptionModeActivated(true);
    emit decryptionModeActivated(false);
    toStatusBar(statusBar_, tr("Encryption mode activated."));
    encryptionMode_ = true;
    decryptionMode_ = false;
    if (targetFileLoaded_ && sourceFileLoaded_) emit canStart(true);
}

void CipherWidget::activateDecryptionMode() {
    emit decryptionModeActivated(true);
    emit encryptionModeActivated(false);
    toStatusBar(statusBar_, tr("Decryption mode activated."));
    decryptionMode_ = true;
    encryptionMode_ = false;
    if (targetFileLoaded_ && sourceFileLoaded_) emit canStart(true);
}

} // namespace gui
} // namespace cscrypto
