#include "atvview.h"
#include "atvsettingsview.h"
#include "../model/settings.h"
#include "../model/atvvideogenerator.h"
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QDebug>
#include <QPointer>

static QString formatTime(qint64 ms) {
    qint64 seconds = ms / 1000;
    qint64 minutes = seconds / 60;
    seconds %= 60;
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}

ATVView::ATVView(QWidget *parent)
    : QMainWindow(parent)
    , player(new QMediaPlayer(this))
    , videoWidget(new QVideoWidget(this))
    , openBtn(new QPushButton(this))
    , playPauseBtn(new QPushButton(this))
    , stopBtn(new QPushButton(this))
    , seekSlider(new QSlider(Qt::Horizontal, this))
    , timeLabel(new QLabel("00:00 / 00:00", this))
{
    setWindowTitle("PAL Demod");
    resize(900, 600);

    openBtn->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));

    playPauseBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    playPauseBtn->setMaximumWidth(60);

    stopBtn->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    stopBtn->setMaximumWidth(50);

    setupUI();
    setupConnections();

    player->setVideoOutput(videoWidget);
}

ATVView::~ATVView() {
    if (m_genThread && m_genThread->isRunning()) {
        m_genThread->quit();
        m_genThread->wait();
    }
}

void ATVView::setupUI() {
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    mainLayout->addWidget(videoWidget, 1);

    QWidget *controlsWidget = new QWidget(central);
    QHBoxLayout *controlsLayout = new QHBoxLayout(controlsWidget);
    controlsLayout->setContentsMargins(10, 5, 10, 5);
    controlsLayout->setSpacing(10);

    controlsLayout->addWidget(openBtn);
    controlsLayout->addWidget(playPauseBtn);
    controlsLayout->addWidget(stopBtn);
    controlsLayout->addWidget(seekSlider);
    controlsLayout->addWidget(timeLabel);

    mainLayout->addWidget(controlsWidget);

    seekSlider->setRange(0, 0);
    playPauseBtn->setMaximumWidth(60);
    stopBtn->setMaximumWidth(50);
    seekSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void ATVView::setupConnections() {
    connect(openBtn, &QPushButton::clicked, this, &ATVView::openFile);
    connect(playPauseBtn, &QPushButton::clicked, this, &ATVView::togglePlayPause);
    connect(stopBtn, &QPushButton::clicked, this, &ATVView::stop);

    connect(seekSlider, &QSlider::sliderPressed, [this]() { isSeeking = true; });
    connect(seekSlider, &QSlider::sliderReleased, [this]() {
        isSeeking = false;
        seek(seekSlider->value());
    });

    connect(player, &QMediaPlayer::positionChanged, this, &ATVView::updatePosition);
    connect(player, &QMediaPlayer::durationChanged, this, &ATVView::updateDuration);
    connect(player, &QMediaPlayer::playbackStateChanged, this, &ATVView::updatePlayPauseButton);
    connect(player, &QMediaPlayer::errorOccurred, this, &ATVView::handleError);
}

void ATVView::openFile() {
    ATVSettingsView dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;

    Settings settings = dialog.getSettings();
    if (!settings.isValid()) return;

    startGeneration(settings);
}

void ATVView::startGeneration(const Settings &settings) {
    if (m_genThread && m_genThread->isRunning()) {
        QMessageBox::information(this, "Занято", "Генерация уже выполняется.");
        return;
    }

    m_genThread = new QThread(this);
    ATVVideoGenerator *generator = new ATVVideoGenerator(settings);
    generator->moveToThread(m_genThread);

    m_progressDialog = new QProgressDialog("Генерация видео...", "Отмена", 0, 100, this);
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setValue(0);
    m_progressDialog->setAutoClose(false);
    m_progressDialog->setAutoReset(false);

    QPointer<QProgressDialog> safeDialog(m_progressDialog);

    connect(m_progressDialog, &QProgressDialog::canceled, this, [this]() {
        if (m_genThread) {
            m_genThread->requestInterruption();
        }
    });
    m_progressDialog->show();

    connect(m_genThread, &QThread::started, generator, &ATVVideoGenerator::generate);

    connect(generator, &ATVVideoGenerator::progressChanged, this, [safeDialog](int value) {
        if (safeDialog && safeDialog->isVisible()) {
            safeDialog->setValue(value);
        }
    });

    connect(generator, &ATVVideoGenerator::finished, this, &ATVView::onGenerationFinished);
    connect(generator, &ATVVideoGenerator::errorOccurred, this, &ATVView::onGenerationError);

    connect(generator, &ATVVideoGenerator::finished, m_genThread, &QThread::quit);
    //connect(generator, &ATVVideoGenerator::errorOccurred, m_genThread, &QThread::quit);
    //connect(m_genThread, &QThread::finished, generator, &QObject::deleteLater);
    //connect(m_genThread, &QThread::finished, m_genThread, &QThread::deleteLater);

    connect(m_genThread, &QThread::finished, this, [this, safeDialog, generator]() {
        if (safeDialog) {
            safeDialog->close();
            safeDialog->deleteLater();
        }
        generator->deleteLater();
        m_genThread = nullptr;
        m_progressDialog = nullptr;
    });

    m_genThread->start();
}

void ATVView::onGenerationFinished() {
    if (m_progressDialog) { m_progressDialog->setValue(100); m_progressDialog->deleteLater(); m_progressDialog = nullptr; }
    QMessageBox::information(this, "Готово", "Видео успешно сгенерировано!");

    QDir dir = QDir::current();
    dir.cd("../../");
    QString path = dir.filePath("output.mp4");

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    player->setSource(QUrl::fromLocalFile(path));
#else
    player->setMedia(QUrl::fromLocalFile(path));
#endif
    player->play();
}

void ATVView::onGenerationError(const QString &message) {
    if (m_progressDialog) { m_progressDialog->deleteLater(); m_progressDialog = nullptr; }
    QMessageBox::critical(this, "Ошибка генерации", message);
}

void ATVView::togglePlayPause() {
    if (player->playbackState() == QMediaPlayer::PlayingState) {
        player->pause();
    } else {
        player->play();
    }
}

void ATVView::stop() {
    player->stop();
}

void ATVView::seek(qint64 position) {
    player->setPosition(position);
}

void ATVView::updatePosition() {
    if (!isSeeking && player->duration() > 0) {
        seekSlider->setValue(player->position());
        timeLabel->setText(QString("%1 / %2")
                               .arg(formatTime(player->position()))
                               .arg(formatTime(player->duration())));
    }
}

void ATVView::updateDuration(qint64 duration) {
    seekSlider->setRange(0, duration);
}

void ATVView::updatePlayPauseButton() {
    if (player->playbackState() == QMediaPlayer::PlayingState) {
        playPauseBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    } else {
        playPauseBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    }
}

void ATVView::handleError(QMediaPlayer::Error error, const QString &errorString) {
    Q_UNUSED(error);
    QMessageBox::warning(this, "Ошибка воспроизведения", errorString);
}
