#pragma once

#include <QString>
#include <cctype>
#include <filesystem>
#include <string>

namespace OCT {
namespace fs = std::filesystem;

// Convert a QString to fs::path
static fs::path QStringToStdPath(const QString &str) {
  const auto rawPath = str.toLocal8Bit();
  return {rawPath.begin(), rawPath.end()};
}

std::string toLower(const std::string &str) {
  std::string lower = str;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return lower;
}

} // namespace OCT