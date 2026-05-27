#pragma once

#include <vector>
#include <complex>
#include <set>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Settings;

class ATVSamplesHandler {
private:
    struct Option {
        int start_index;
        double carrier;
        double htop_avg;
        double image_avg;
    };

    int SAMPLE_RATE = 16000000;
    const int LINES_NUMBER = 625;
    const int FPS = 25;

    const double line_duration = 1.0 / (static_cast<double>(LINES_NUMBER) * static_cast<double>(FPS));
    int samples_per_line = static_cast<int>(line_duration * static_cast<double>(SAMPLE_RATE));

    std::vector<std::complex<double>> complex_samples;
    std::vector<std::complex<double>>::iterator start;

    std::vector<Option> get_options(std::vector<std::complex<double>>::iterator it, int number_of_lines = 1);
    std::vector<double> fm_demod(std::vector<std::complex<double>>& x, double df = 1.0, double fc = 0.0);
    void delete_anomalies(std::vector<double>& samples);
    void fir_filter(std::vector<double>& samples, int number_of_taps = 4);
    void normalize(std::vector<double>& samples);
    std::vector<Option> find_line(std::vector<double>& samples, double carrier);

public:
    void set_settings(const Settings& settings);
    std::vector<double> get_line();
    int get_index();
};
