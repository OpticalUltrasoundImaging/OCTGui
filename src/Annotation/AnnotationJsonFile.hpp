#pragma once

#include "Annotation.hpp"
#include <QList>
#include <QObject>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace annotation {
namespace fs = std::filesystem;

/**
Represents the json annotation file for a binfile
 */
class AnnotationJsonFile : public QObject {
  Q_OBJECT

public:
  AnnotationJsonFile();

  void toFile(const fs::path &path) const;
  bool fromFile(const fs::path &path);

  void init();

  [[nodiscard]] QList<Annotation> getAnnotationForFrame(int frameNum);
  void setAnnotationForFrame(int frameNum,
                             const QList<Annotation> &annotations);

private:
  using json = nlohmann::json;
  json m_doc;
};

} // namespace annotation