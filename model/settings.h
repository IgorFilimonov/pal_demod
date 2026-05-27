#pragma once
#include <QString>
#include <Qt>

class Settings {
public:
    Settings() = default;
    explicit Settings(const QString &filePath,
                      const QString &format,
                      int sampleRate,
                      bool invert,
                      int blackLevel);

    QString filePath() const;

    QString format() const;

    int sampleRate() const;

    bool invert() const;

    int blackLevel() const;

    bool isValid() const;

private:
    QString m_filePath;
    QString m_format;
    int m_sampleRate;
    bool m_invert;
    int m_blackLevel;
};
