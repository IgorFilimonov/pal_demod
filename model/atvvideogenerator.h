#pragma once
#include <QObject>
#include <string>
#include <vector>
#include <cstdint>
#include <opencv2/opencv.hpp>
#include "atvsampleshandler.h"

class Settings;

class ATVVideoGenerator : public QObject {
    Q_OBJECT
private:
    int SAMPLE_RATE = 16000000;
    const int LINES_NUMBER = 625;
    const int FPS = 25;
    bool INVERT_VIDEO = false;
    double LEVEL_BLACK = 0.3;

    double line_duration = 0.0;
    int samples_per_line = 0;
    const int height = LINES_NUMBER;
    int width = 0;
    double sample_range_correction = 1.0;

    ATVSamplesHandler handler;
    std::vector<uint8_t> frame;

    void getFrame();
    void find_vsync();

public:
    explicit ATVVideoGenerator(const Settings& settings, QObject *parent = nullptr);
    void generate();

signals:
    void progressChanged(int percent);
    void finished();
    void errorOccurred(const QString &message);
};
