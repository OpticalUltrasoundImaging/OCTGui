#include "AnnotationJsonFile.hpp"
#include "Annotation.hpp"
#include "datetime.hpp"
#include <QDebug>
#include <fstream>
#include <nlohmann/json.hpp>

namespace annotation {

AnnotationJsonFile::AnnotationJsonFile() { init(); }

bool AnnotationJsonFile::fromFile(const fs::path &path) {
  std::ifstream ifs(path);
  ifs >> m_doc;
  return true;
}

void AnnotationJsonFile::toFile(const fs::path &path) const {
  std::ofstream ofs(path);
  ofs << std::setw(4) << m_doc;
}

void AnnotationJsonFile::init() {
  // Create
  const auto dateStr = datetime::datetimeISO8601();
  m_doc["date-created"] = dateStr;
  m_doc["date-modified"] = dateStr;
  m_doc["frames"] = json::array();
}

QList<Annotation> AnnotationJsonFile::getAnnotationForFrame(int frameNum) {
  const auto frameNumStr = std::to_string(frameNum);
  QList<Annotation> result;

  try {
    const auto &frames = m_doc.at("frames");
    const auto &frame = frames.at(frameNumStr);

    for (const auto &el : frame.items()) {
      Annotation anno;
      el.value().get_to(anno);
      result.append(anno);
    }
  } catch (std::exception &e) {
    qDebug() << "getAnnotationForFrame exception at frame " << frameNum
             << e.what();
  }

  return result;
}

void AnnotationJsonFile::setAnnotationForFrame(
    int frameNum, const QList<Annotation> &annotations) {

  // Update date-modified
  m_doc["date-modified"] = datetime::datetimeISO8601();

  auto &frames = m_doc["frames"];

  const auto frameNumStr = std::to_string(frameNum);

  auto frame = json::array();
  for (const auto &anno : annotations) {
    frame.emplace_back(anno);
  }

  // Replace in the doc
  frames.at(frameNumStr).update(frame);
}
} // namespace annotation
