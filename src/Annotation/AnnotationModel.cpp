#include "Annotation/AnnotationModel.hpp"
#include <QAbstractItemModel>
#include <QByteArray>
#include <QMimeData>
#include <Qt>
#include <QtLogging>
#include <cassert>
#include <set>
#include <sstream>
#include <utility>

namespace annotation {

[[nodiscard]] int AnnotationModel::rowCount(const QModelIndex &parent) const {
  Q_UNUSED(parent);
  return static_cast<int>(m_annotations.size());
}

[[nodiscard]] int
AnnotationModel::columnCount(const QModelIndex &parent) const {
  return HEADER_DATA.size();
}

[[nodiscard]] QVariant AnnotationModel::headerData(int section,
                                                   Qt::Orientation orientation,
                                                   int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    assert(section < HEADER_DATA.size());
    return HEADER_DATA.at(section).header;
  }
  return QAbstractListModel::headerData(section, orientation, role);
}

[[nodiscard]] QVariant AnnotationModel::data(const QModelIndex &index,
                                             int role) const {
  if (!index.isValid() || index.row() >= m_annotations.size()) {
    return {};
  }

  const auto &annotation = m_annotations[index.row()];

  switch (role) {
  case Qt::DisplayRole:
    assert(index.column() < HEADER_DATA.size());
    return HEADER_DATA.at(index.column()).getter(annotation);

  case TypeRole:
    return annotation.type;
  case NameRole:
    return annotation.name;
  case ColorRole:
    return annotation.color;
  default:
    return {};
  }
}

bool AnnotationModel::setData(const QModelIndex &index, const QVariant &value,
                              int role) {
  if (!index.isValid() || index.row() >= m_annotations.size()) {
    return false;
  }

  auto &annotation = m_annotations[index.row()];
  if (role == Qt::EditRole) {
    HEADER_DATA.at(index.column()).setter(annotation, value);
    setDirty();
    Q_EMIT dataChanged(index, index, {});
    return true;
  }
  if (role == NameRole) {
    annotation.name = value.toString();
    setDirty();
    Q_EMIT dataChanged(index, index, {});
    return true;
  }

  return false;
}

Qt::ItemFlags AnnotationModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) {
    return Qt::NoItemFlags;
  }
  auto flags = QAbstractListModel::flags(index);
  if (HEADER_DATA.at(index.column()).editable) {
    flags |= Qt::ItemIsEditable;
  }

  return flags;
}

bool AnnotationModel::removeRows(int row, int count,
                                 const QModelIndex &parent) {
  if (row < 0 || count <= 0 || (row + count) > m_annotations.size()) {
    return false;
  }
  setDirty();
  beginRemoveRows(parent, row, row + count - 1);
  m_annotations.remove(row, count);
  endRemoveRows();
  return true;
}

void AnnotationModel::addAnnotation(const Annotation &annotation) {
  setDirty();
  beginInsertRows(QModelIndex(), static_cast<int>(m_annotations.size()),
                  static_cast<int>(m_annotations.size()));
  m_annotations.append(annotation);
  endInsertRows();
}

void AnnotationModel::clear() {
  setDirty(false);
  if (size() > 0) {
    beginRemoveRows(QModelIndex(), 0,
                    static_cast<int>(m_annotations.size()) - 1);
    m_annotations.clear();
    endRemoveRows();
  }
}

void AnnotationModel::setAnnotations(QList<Annotation> annotations) {
  clear();

  if (!annotations.empty()) {
    beginInsertRows(QModelIndex{}, 0, static_cast<int>(annotations.size()) - 1);
    m_annotations = std::move(annotations);
    endInsertRows();
  }
}

// NOLINTNEXTLINE(*-static)
QStringList AnnotationModel::mimeType() const {
  QStringList types;
  types << "application/json";
  return types;
}

QMimeData *AnnotationModel::mimeData(const QModelIndexList &indexes) const {
  // JSON impl

  // Find unique rows in selected annotations
  std::set<int> uniqueRows;
  for (const auto &index : indexes) {
    if (index.isValid()) {
      uniqueRows.insert(index.row());
    }
  }

  // Serialize annotations to JSON doc
  json j;
  for (const auto row : uniqueRows) {
    j.emplace_back(m_annotations[row]);
  }

  std::ostringstream ss;
  ss << j;

  auto *mimeData = new QMimeData;
  // Implicitly construct QByteArray from const char *
  mimeData->setData("application/json", ss.view().data());
  return mimeData;
}

bool AnnotationModel::canDropMimeData(const QMimeData *data,
                                      Qt::DropAction action, int row,
                                      int column,
                                      const QModelIndex &parent) const {
  return data->hasFormat("application/json");
}

bool AnnotationModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                   int row, int column,
                                   const QModelIndex &parent) {
  // JSON impl
  if (!data->hasFormat("application/json")) {
    return false;
  }

  const QByteArray encodedData = data->data("application/json");

  std::istringstream iss(encodedData.constData());
  json j;
  iss >> j;

  for (const auto &el : j.items()) {
    addAnnotation(el.value().get<Annotation>());
  }

  return true;
}

void to_json(json &j, const AnnotationModel &model) {
  for (const auto &a : model.annotations()) {
    j.emplace_back(a);
  }
}

void from_json(const json &j, AnnotationModel &model) {
  model.clear();
  QList<Annotation> annotations;
  for (const auto &el : j.items()) {
    annotations.append(el.value().get<Annotation>());
  }
  model.setAnnotations(annotations);
}

} // namespace annotation