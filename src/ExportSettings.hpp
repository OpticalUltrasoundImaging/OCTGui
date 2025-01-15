#pragma once

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QWidget>
#include <functional>
#include <vector>

namespace OCT {

struct ExportSettings {
  bool saveImages{false};
};

class ExportSettingsWidget : public QWidget {

public:
  ExportSettingsWidget() : m_menu(new QMenu("&Export")) {
    // UI
    // --
    auto *layout = new QGridLayout;
    setLayout(layout);

    const auto makeLabeledCheckbox = [this](QGridLayout *layout, int row,
                                            const QString &name,
                                            const QString &desc, bool &value) {
      // Create checkbox widget
      // ----------------------
      layout->addWidget(new QLabel(name), row, 0);

      auto *checkbox = new QCheckBox();
      layout->addWidget(checkbox, row, 1);
      using Qt::CheckState;

      connect(checkbox, &QCheckBox::checkStateChanged, this,
              [this, &value](CheckState state) {
                value = state == CheckState::Checked;
              });

      updateGuiFromParamsCallbacks.emplace_back([this, checkbox, &value] {
        QSignalBlocker blocker(checkbox);
        checkbox->setCheckState(value ? Qt::CheckState::Checked
                                      : Qt::CheckState::Unchecked);
      });

      // Create action to add to menu
      m_menu->addActions(checkbox->actions());
    };

    // makeLabeledCheckbox(layout, 0, "Save images", "Should images be
    // exported",
    //                     m_settings.saveImages);

    {
      auto *act = new QAction("Save images");
      act->setCheckable(true);
      act->setChecked(m_settings.saveImages);
      m_menu->addAction(act);
      connect(act, &QAction::changed,
              [this, act]() { m_settings.saveImages = act->isChecked(); });
    }
  }

  QMenu *menu() { return m_menu; }
  const auto &settings() { return m_settings; }

Q_SIGNALS:
  void updated(ExportSettings m_settings);

private:
  ExportSettings m_settings;
  std::vector<std::function<void()>> updateGuiFromParamsCallbacks;
  QMenu *m_menu;

  void updateGuiFromParams();
  inline void paramsUpdatedInternal() {}
};

}; // namespace OCT