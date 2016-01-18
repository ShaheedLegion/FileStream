#pragma once
#ifndef UI_LIB_PANEL
#define UI_LIB_PANEL

#include "ui.h"

namespace ui {

// For now we simply try to render the text (name) directly into the thing that
// needs it .... we'll clean this up later.

class PanelLabel : public Control {
  detail::Texture *text;

public:
  PanelLabel() : Control(), text(nullptr) {}
  virtual ~PanelLabel() {}

  virtual void render(detail::RenderBuffer &target) override {
    // First, we render ourselves ...
    Control::render(target);

    // Then we render the text.
    // The reason this can be done safely is because we'll never add
    // children to this control.
    // Buttons, etc, will have to be 'stacked' on top of this control.
    if (text == nullptr) {
      // Then get a text of the correct width and height
      int pw = (getName().length() * 20);
      int ph = getH();
      text = new detail::Texture(getName(), pw, ph, nullptr);
      TextRenderer::getInstance()->drawText(text, getName(), 10, 30, pw, ph);
      // We have successfully renderered the text into our surface.
      text->setX(0);
      text->setY(0);
    } else
      text->render(target);
  }
};

// Panels are fairly generic.
class Panel : public Control {
  PanelLabel *myLabel;

  void EnsureLabel() {
    if (myLabel == nullptr) {
      myLabel = new PanelLabel();
    }
  }

public:
  Panel() : Control(), myLabel(nullptr) {}

  virtual ~Panel() {}

  virtual void render(detail::RenderBuffer &target) override {
    // First, we render ourselves ...
    Control::render(target);

    if (myLabel != nullptr) {
      DimensionInfo &height{myLabel->getHeight()};
      myLabel->setX(0);
      myLabel->setY(0);
      myLabel->setH(height.getValue(target.height));
      myLabel->render(target);
    }
  }

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