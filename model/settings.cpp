#include "settings.h"

Settings::Settings(const QString &filePath,
                   const QString &format,
                   int sampleRate,
                   bool invert,
                   int blackLevel)
    : m_filePath(filePath), m_format(format),
    m_sampleRate(sampleRate), m_invert(invert), m_blackLevel(blackLevel) {}

QString Settings::filePath() const { return m_filePath; }

QString Settings::format() const { return m_format; }

int Settings::sampleRate() const { return m_sampleRate; }

bool Settings::invert() const { return m_invert; }

int Settings::blackLevel() const { return m_blackLevel; }

bool Settings::isValid() const { return !m_filePath.trimmed().isEmpty(); }
