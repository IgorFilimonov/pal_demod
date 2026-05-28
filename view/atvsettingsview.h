#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QSlider>

class Settings;

class ATVSettingsView : public QDialog {
    Q_OBJECT
public:
    explicit ATVSettingsView(QWidget *parent = nullptr);
    ~ATVSettingsView() override;

    Settings getSettings() const;

private:
    void setupUI();
    void browseFile();
    void updateOkButton();

    QLineEdit *fileLineEdit;
    QComboBox *formatCombo;
    QSpinBox *sampleRateBox;
    QCheckBox *invertBox;
    QSlider *blackLevelSlider;
    QPushButton *okBtn;
};
