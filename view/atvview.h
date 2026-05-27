#pragma once
#include <QMainWindow>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QThread>
#include <QProgressDialog>

class Settings;

class ATVView : public QMainWindow {
    Q_OBJECT
public:
    explicit ATVView(QWidget *parent = nullptr);
    ~ATVView() override;

private slots:
    void openFile();
    void togglePlayPause();
    void stop();
    void seek(qint64 position);
    void updatePosition();
    void updateDuration(qint64 duration);
    void updatePlayPauseButton();
    void handleError(QMediaPlayer::Error error, const QString &errorString);
    void onGenerationFinished();
    void onGenerationError(const QString &message);

private:
    void setupUI();
    void setupConnections();
    void startGeneration(const Settings &settings);

    QMediaPlayer *player;
    QVideoWidget *videoWidget;
    QPushButton *openBtn;
    QPushButton *playPauseBtn;
    QPushButton *stopBtn;
    QSlider *seekSlider;
    QLabel *timeLabel;
    bool isSeeking = false;
    QThread *m_genThread = nullptr;
    QProgressDialog *m_progressDialog = nullptr;
};
