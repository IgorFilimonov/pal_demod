#include "atvvideogenerator.h"
#include "settings.h"
#include <iostream>
#include <QString>
#include <QThread>

using namespace std;

ATVVideoGenerator::ATVVideoGenerator(const Settings& settings, QObject *parent)
    : QObject(parent), handler()
{
    SAMPLE_RATE = settings.sampleRate();
    INVERT_VIDEO = settings.invert();
    LEVEL_BLACK = settings.blackLevel() / 1000.0;

    line_duration = 1.0 / (static_cast<double>(LINES_NUMBER) * static_cast<double>(FPS));
    samples_per_line = static_cast<int>(line_duration * static_cast<double>(SAMPLE_RATE));
    width = samples_per_line;
    sample_range_correction = 255.0 / (1.0 - LEVEL_BLACK);

    frame = vector<uint8_t>(height * width, 0);
    handler.set_settings(settings);
}

void ATVVideoGenerator::find_vsync() {
    bool is_vsync_found = false;
    while (!is_vsync_found) {
        vector<double> line = handler.get_line();
        if (line.empty()) {
            is_vsync_found = true;
        }
    }
}

void ATVVideoGenerator::getFrame() {
    bool is_vsync_found = false;

    for (int i = 0; i < height; i += 2) {
        int current_index = handler.get_index();
        if (469000 < current_index && current_index < 470000) {
            cout << "boop" << endl;
        }

        vector<double> line = handler.get_line();
        if (line.empty()) {
            is_vsync_found = true;
            break;
        }

        for (size_t j = 0; j < line.size(); ++j) {
            double sample = line[j];
            if (INVERT_VIDEO) sample = 1.0 - sample;
            if (sample < 0.0) sample = 0.0;
            if (sample > 1.0) sample = 1.0;

            int sample_to_write = static_cast<int>((sample - LEVEL_BLACK) * sample_range_correction);
            if (sample_to_write < 0) sample_to_write = 0;
            if (sample_to_write > 255) sample_to_write = 255;

            frame[i * width + static_cast<int>(j)] = static_cast<uint8_t>(sample_to_write);
        }
    }

    cout << handler.get_index() << endl;
    if (!is_vsync_found) {
        find_vsync();
    }
    is_vsync_found = false;

    for (int i = 1; i < height; i += 2) {
        vector<double> line = handler.get_line();
        if (line.empty()) {
            is_vsync_found = true;
            break;
        }

        for (size_t j = 0; j < line.size(); ++j) {
            double sample = line[j];
            if (INVERT_VIDEO) sample = 1.0 - sample;
            if (sample < 0.0) sample = 0.0;
            if (sample > 1.0) sample = 1.0;

            int sample_to_write = static_cast<int>((sample - LEVEL_BLACK) * sample_range_correction);
            if (sample_to_write < 0) sample_to_write = 0;
            if (sample_to_write > 255) sample_to_write = 255;

            frame[i * width + static_cast<int>(j)] = static_cast<uint8_t>(sample_to_write);
        }
    }

    if (!is_vsync_found) {
        find_vsync();
    }
}

void ATVVideoGenerator::generate() {
    string filename = "C:/Users/Igor/Documents/repos/areyoureallyfine/output.mp4";

    cv::VideoWriter writer(filename, cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
                           FPS, cv::Size(width, height), false);

    if (!writer.isOpened()) {
        emit errorOccurred("Не удалось открыть файл для записи видео");
        return;
    }

    try {
        find_vsync();
        for (int i = 0; i < 225; ++i) {
            if (QThread::currentThread()->isInterruptionRequested()) {
                writer.release();
                emit finished();
                return;
            }

            getFrame();
            cv::Mat cvframe(height, width, CV_8UC1, frame.data());
            writer.write(cvframe);

            int progress = static_cast<int>((i + 1) * 100.0 / 225.0);
            emit progressChanged(progress);
        }
        writer.release();
        emit finished();
    } catch (const std::exception &e) {
        emit errorOccurred(QString("Ошибка генерации: %1").arg(e.what()));
    } catch (...) {
        emit errorOccurred("Неизвестная ошибка при генерации видео");
    }
}
