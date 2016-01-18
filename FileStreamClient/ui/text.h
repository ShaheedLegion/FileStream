#pragma once
#ifndef UI_LIB_TEXT
#define UI_LIB_TEXT

#include "ui.h"

namespace ui {

// For now we simply try to render the text (name) directly into the thing that
// needs it .... we'll clean this up later.

class TextLabel : public Control {
  detail::Texture *text;

public:
  TextLabel() : Control(), text(nullptr) {}
  virtual ~TextLabel() {}

  virtual void render(detail::RenderBuffer &target) {
    // First, we render ourselves ...
    Control::render(target);

    // Then we render the text.
    // The reason this can be done safely is because we'll never add
    // children to this control.
    // Buttons, etc, will have to be 'stacked' on top of this control.
    if (text == nullptr) {
      // Then get a text of the correct width and height
      int pw = getW();
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
class Text : public Control {
  TextLabel *myLabel;

  void EnsureLabel() {
    if (myLabel == nullptr) {
      myLabel = new TextLabel();
      AddChild(myLabel);
    }
  }

public:
  Text() : Control(), myLabel(nullptr) {}

  virtual ~Text() {}

  virtual void render(detail::RenderBuffer &target) {
    // First, we render ourselves ...
    Control::render(target);

    // Then we render the text.
    // The reason this can be done safely is because we'll never add
    // children to this control.
    // Buttons, etc, will have to be 'stacked' on top of this control.
    if (texture != nullptr) {
      // Get the color from the texture.
      detail::Uint32 color = texture->getBuffer()[0];
      target.drawRect(color, getX(), getY(), getW(), getH());
    }
      // Then get a text of the correct width and height
    //  int pw = getW();
    //  int ph = getH();
    //  text = new detail::Texture(getName(), pw, ph, nullptr);
    //  TextRenderer::getInstance()->drawText(text, getName(), 10, 30, pw, ph);
      // We have successfully renderered the text into our surface.
    //  text->setX(0);
    //  text->setY(0);
    //} else
    //  text->render(target);
  }

};

} // namespace ui

#endif // UI_LIB_TEXT