#include "Annotation.hpp"

namespace annotation {

Annotation::Annotation(const QLineF &line, const QColor &color, QString name)
    : polygon({line.p1(), line.p2()}), color(color), name(std::move(name)) {}

Annotation::Annotation(const QRectF &rect, const QColor &color, QString name)
    : type(Rect), polygon({rect.topLeft(), rect.bottomRight()}), color(color),
      name(std::move(name)) {}

Annotation::Annotation(const Arc &arc, const QRectF &rect, const QColor &color,
                       QString name)
    : type(Fan), color(color), name(std::move(name)) {
  setArc(arc, rect);
}

void to_json(json &j, const Annotation &a) {
  json jpolygon = json::array();
  for (const auto &pt : a.polygon) {
    jpolygon.emplace_back(pt.x(), pt.y());
  }

  j = json{
      {"type", a.type},
      {"polygon", jpolygon},
      {"color",
       {a.color.red(), a.color.green(), a.color.blue(), a.color.alpha()}},
      {"name", a.name.toStdString()},
  };
}

Annotation from_json(const json &j, Annotation &a) {
  j.at("type").get_to(a.type);

  for (const auto &el : j.at("polygon").items()) {
    const auto &item = el.value();
    a.polygon.emplace_back(item[0], item[1]);
  }

  const auto &jcolor = j.at("color");
  a.color = QColor(jcolor[0], jcolor[1], jcolor[2], jcolor[3]);
  a.name = QString::fromStdString(j.at("name"));

  return a;
}

} // namespace annotation
