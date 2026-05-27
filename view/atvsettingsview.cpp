#include "atvsettingsview.h"
#include "../model/settings.h"
#include <QFileDialog>
#include <QDir>

ATVSettingsView::ATVSettingsView(QWidget *parent)
    : QDialog(parent), fileLineEdit(new QLineEdit(this)), formatCombo(new QComboBox(this)),
    sampleRateBox(new QSpinBox(this)), invertBox(new QCheckBox(this)),
    blackLevelSlider(new QSlider(Qt::Horizontal, this)), okBtn(new QPushButton("Применить", this))
{
    setWindowTitle("Настройки");
    setMinimumWidth(480);
    setupUI();
}

ATVSettingsView::~ATVSettingsView() = default;

void ATVSettingsView::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    QHBoxLayout *fileLayout = new QHBoxLayout();
    fileLayout->addWidget(new QLabel("Файл:", this));
    fileLayout->addWidget(fileLineEdit, 1);
    QPushButton *browseBtn = new QPushButton("Обзор...", this);
    fileLayout->addWidget(browseBtn);
    mainLayout->addLayout(fileLayout);

    QFormLayout *formLayout = new QFormLayout();
    formatCombo->addItems({"Complex 8-bit signed", "Complex 16-bit signed", "Complex 32-bit float"});
    formLayout->addRow("Формат:", formatCombo);

    sampleRateBox->setRange(1, 2000000000);
    sampleRateBox->setValue(16000000);
    formLayout->addRow("Частота дискретизации:", sampleRateBox);

    formLayout->addRow("Инвертировать видео:", invertBox);

    blackLevelSlider->setRange(0, 999);
    blackLevelSlider->setValue(300);
    formLayout->addRow("Уровень черного:", blackLevelSlider);

    mainLayout->addLayout(formLayout);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton *cancelBtn = new QPushButton("Отмена", this);
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    connect(browseBtn, &QPushButton::clicked, this, &ATVSettingsView::browseFile);
    connect(fileLineEdit, &QLineEdit::textChanged, this, &ATVSettingsView::updateOkButton);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    okBtn->setEnabled(false);
}

void ATVSettingsView::browseFile() {
    QString path = QFileDialog::getOpenFileName(this, "Выберите видео файл", QDir::homePath(), "Все файлы (*)");
    if (!path.isEmpty()) fileLineEdit->setText(path);
}

void ATVSettingsView::updateOkButton() {
    okBtn->setEnabled(!fileLineEdit->text().trimmed().isEmpty());
}

Settings ATVSettingsView::settings() const {
    return Settings(
        fileLineEdit->text(),
        formatCombo->currentText(),
        sampleRateBox->value(),
        invertBox->isChecked(),
        blackLevelSlider->value()
        );
}
