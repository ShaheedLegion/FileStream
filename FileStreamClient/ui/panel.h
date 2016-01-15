#pragma once
#ifndef UI_LIB_PANEL
#define UI_LIB_PANEL

#include "ui.h"

namespace ui {

class PanelLabel : public Control {
public:
  PanelLabel() : Control() {}
  virtual ~PanelLabel() {}
};

// Panels are fairly generic.
class Panel : public Control {
  PanelLabel *myLabel;

  void EnsureLabel() {
    if (myLabel == nullptr) {
      myLabel = new PanelLabel();
      AddChild(myLabel);
    }
  }

public:
  Panel() : Control(), myLabel(nullptr) {}

  virtual ~Panel() {}

  virtual void SetAttribute(const std::string &name,
                            const std::string &value) override {
    Control::SetAttribute(name, value);

    if (name == "label") {
      EnsureLabel();
      myLabel->SetAttribute("name", value);
      myLabel->setX(0);
      myLabel->setY(0);
    } else if (name == "label_background") {
      EnsureLabel();
      myLabel->SetAttribute("background", value);
    } else if (name == "label_width") {
      EnsureLabel();
      myLabel->SetAttribute("width", value);
    } else if (name == "label_height") {
      EnsureLabel();
      myLabel->SetAttribute("height", value);
    }
  }
};

} // namespace ui

#endif // UI_LIB_PANEL