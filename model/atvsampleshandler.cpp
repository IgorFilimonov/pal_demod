#include "atvsampleshandler.h"
#include "settings.h"
#include <fstream>
#include <iostream>
#include <Iir.h>

using namespace std;

vector<ATVSamplesHandler::Option> ATVSamplesHandler::get_options(
    vector<complex<double>>::iterator it, int number_of_lines)
{
    vector<Option> options;
    vector<complex<double>>::iterator local_start = it;

    for (double carrier = 0.0; carrier <= static_cast<double>(SAMPLE_RATE) / 2000000.0; carrier += 0.5) {
        vector<complex<double>> sliced(local_start, local_start + number_of_lines * samples_per_line);
        vector<double> demodulated_signal = fm_demod(sliced, 3.0/8.0, carrier / (static_cast<double>(SAMPLE_RATE) / 2000000.0));
        fir_filter(demodulated_signal);
        normalize(demodulated_signal);
        vector<Option> line_options = find_line(demodulated_signal, carrier);
        options.insert(options.end(), line_options.begin(), line_options.end());
    }

    sort(options.begin(), options.end(), [](const Option& a, const Option& b) {
        return (a.image_avg - a.htop_avg) < (b.image_avg - b.htop_avg);
    });
    return options;
}

vector<double> ATVSamplesHandler::fm_demod(
    vector<complex<double>>& x, double df, double fc)
{
    size_t n_len = x.size();
    vector<double> phi(n_len);

    for (size_t n = 0; n < n_len; ++n) {
        complex<double> carrier_rot = exp(complex<double>(0, -1.0 * 2.0 * M_PI * fc * static_cast<double>(n)));
        complex<double> rx = x[n] * carrier_rot;
        phi[n] = atan2(rx.imag(), rx.real());
    }

    for (size_t i = 1; i < n_len; ++i) {
        double diff = phi[i] - phi[i - 1];
        if (diff > M_PI) {
            phi[i] -= 2.0 * M_PI * floor((diff + M_PI) / (2.0 * M_PI));
        } else if (diff < -M_PI) {
            phi[i] += 2.0 * M_PI * floor((M_PI - diff) / (2.0 * M_PI));
        }
    }

    vector<double> y(n_len - 1);
    for (size_t i = 0; i < n_len - 1; ++i) {
        y[i] = static_cast<double>((phi[i + 1] - phi[i]) / (2.0 * M_PI * df));
    }
    y.push_back(y.back());
    return y;
}

void ATVSamplesHandler::delete_anomalies(vector<double>& samples) {
    vector<double> sorted_samples = samples;
    size_t n = sorted_samples.size();

    nth_element(sorted_samples.begin(), sorted_samples.begin() + n / 4, sorted_samples.end());
    double q1 = sorted_samples[n / 4];

    nth_element(sorted_samples.begin(), sorted_samples.begin() + 3 * n / 4, sorted_samples.end());
    double q3 = sorted_samples[3 * n / 4];

    double iqr = q3 - q1;
    double lower_bound = q1 - 1.5 * iqr;
    double upper_bound = q3 + 1.5 * iqr;

    for (size_t i = 0; i < samples.size(); ++i) {
        if (!(lower_bound <= samples[i] && samples[i] <= upper_bound)) {
            if (i == 0) {
                samples[i] = samples[i + 1];
            } else {
                samples[i] = samples[i - 1];
            }
        }
    }
}

void ATVSamplesHandler::fir_filter(vector<double>& samples, int number_of_taps) {
    for (size_t i = 0; i <= samples.size() - number_of_taps; ++i) {
        double sum = samples[i];
        for (int j = 1; j < number_of_taps; ++j) {
            sum += samples[i + j];
        }
        samples[i] = sum / static_cast<double>(number_of_taps);
    }
}

void ATVSamplesHandler::normalize(vector<double>& samples) {
    double min_sample = samples[0];
    double max_sample = samples[0];
    for (double s : samples) {
        if (s < min_sample) min_sample = s;
        if (s > max_sample) max_sample = s;
    }

    double add_const = 0.0 - min_sample;
    double multi_const = 1.0 / (max_sample + add_const);
    for (size_t i = 0; i < samples.size(); ++i) {
        samples[i] = (samples[i] + add_const) * multi_const;
    }
}

vector<ATVSamplesHandler::Option> ATVSamplesHandler::find_line(
    vector<double>& samples, double carrier)
{
    double line_duration = 1.0 / (static_cast<double>(LINES_NUMBER) * static_cast<double>(FPS));
    int samples_per_line = static_cast<int>(line_duration * static_cast<double>(SAMPLE_RATE));
    int samples_per_htop = static_cast<int>(line_duration * static_cast<double>(SAMPLE_RATE) * 4.7 / 64.0);
    int samples_per_burst = static_cast<int>(line_duration * static_cast<double>(SAMPLE_RATE) * 5.8 / 64.0);
    int samples_per_image = samples_per_line - samples_per_htop - samples_per_burst;

    double sum_htop = 0, sum_burst = 0, sum_image = 0;
    int index = 0;

    while (index < samples_per_htop) sum_htop += samples[index++];
    while (index < samples_per_htop + samples_per_burst) sum_burst += samples[index++];

    multiset<double> image_list;
    while (index < samples_per_line) {
        sum_image += samples[index];
        image_list.insert(samples[index]);
        index++;
    }

    vector<Option> options;
    auto check_option = [&](int start) {
        double avg_htop = sum_htop / samples_per_htop;
        double avg_burst = sum_burst / samples_per_burst;
        double avg_image = sum_image / samples_per_image;

        if (avg_burst - avg_htop > 0.1 && avg_image - avg_burst > 0.1 && avg_htop < 0.5) {
            auto it = image_list.upper_bound(static_cast<double>(avg_burst));
            size_t count = distance(image_list.begin(), it);
            if (static_cast<double>(count) / samples_per_image < 0.10) {
                options.push_back({start, carrier, avg_htop, avg_image});
            }
        }
    };

    check_option(0);

    int start_index = 0;
    while (start_index + samples_per_line < static_cast<int>(samples.size())) {
        sum_htop -= samples[start_index];
        sum_burst -= samples[start_index + samples_per_htop];
        sum_image -= samples[start_index + samples_per_htop + samples_per_burst];

        auto it_rem = image_list.find(samples[start_index + samples_per_htop + samples_per_burst]);
        if (it_rem != image_list.end()) image_list.erase(it_rem);

        sum_htop += samples[start_index + samples_per_htop];
        sum_burst += samples[start_index + samples_per_htop + samples_per_burst];
        sum_image += samples[start_index + samples_per_line];
        image_list.insert(samples[start_index + samples_per_line]);

        start_index++;
        check_option(start_index);
    }
    return options;
}

void ATVSamplesHandler::set_settings(const Settings& settings) {
    SAMPLE_RATE = settings.sampleRate();
    samples_per_line = static_cast<int>(line_duration * static_cast<double>(SAMPLE_RATE));

    ifstream samples_iterator(settings.filePath().toStdString(), ios::binary);
    if (!samples_iterator.is_open()) {
        cerr << "Could not open file: " << endl;
        return;
    }

    complex_samples.reserve(20000000);

    int8_t sample_i, sample_q;
    int count = 0;
    while (samples_iterator.read(reinterpret_cast<char*>(&sample_i), sizeof(int8_t)) &&
           samples_iterator.read(reinterpret_cast<char*>(&sample_q), sizeof(int8_t))) {
        complex_samples.emplace_back(sample_i / 127.0, sample_q / 127.0);
        ++count;
        // if (count == 17000000) break;
    }

    vector<Option> options = get_options(complex_samples.begin(), 2);
    start = complex_samples.begin() + options.back().start_index;
}

vector<double> ATVSamplesHandler::get_line() {
    vector<Option> options = get_options(start);

    if (options.empty()) {
        bool new_field = true;

        for (int i = 0; i < 2; ++i) {
            options = get_options(start + i * samples_per_line, 2);
            if (!options.empty()) {
                new_field = false;
                start += i * samples_per_line + options.back().start_index;
                break;
            }
        }
        if (new_field) {
            start += 25600;
            return {};
        }
    } else {
        start += options.back().start_index;
    }

    vector<complex<double>> sliced(start, start + samples_per_line);
    vector<double> demodulated_signal = fm_demod(sliced, 3.0/8.0, options.back().carrier / (static_cast<double>(SAMPLE_RATE) / 2000000.0));
    fir_filter(demodulated_signal);
    normalize(demodulated_signal);
    start += samples_per_line;

    Iir::Butterworth::BandPass<2> bandpass;
    bandpass.setup(SAMPLE_RATE, options.back().carrier, 4433618.0);
    for (double& sample : demodulated_signal) {
        sample = bandpass.filter(sample);
    }

    return demodulated_signal;
}

int ATVSamplesHandler::get_index() {
    return static_cast<int>(distance(complex_samples.begin(), start));
}
