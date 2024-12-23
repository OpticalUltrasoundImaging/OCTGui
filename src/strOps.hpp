#pragma once

#include <QString>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

namespace OCT {
namespace fs = std::filesystem;

// Convert a QString to fs::path
inline fs::path toPath(const QString &str) {
  const auto rawPath = str.toLocal8Bit();
  return {rawPath.begin(), rawPath.end()};
}

// Convert a fs::path to a QString
inline QString toQString(const fs::path &path) {
  return QString::fromStdString(path.string());
}

// Convert a std::string to a QString
inline QString toQString(const std::string &str) {
  return QString::fromStdString(str);
}

// Lower all cases in-place
inline void toLower_(std::string &str) {
  std::ranges::transform(str, str.begin(), [](unsigned char c) { return std::tolower(c); });
}

// Returns a copy of str with all cases lowered
inline std::string toLower(const std::string &str) {
  std::string lower = str;
  toLower_(lower);
  return lower;
}

} // namespace OCT
