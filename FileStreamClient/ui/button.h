#pragma once
#ifndef UI_LIB_BUTTON
#define UI_LIB_BUTTON

#include "ui.h"

namespace ui {
class Button;
class ButtonHandler {
public:
  virtual void ButtonClicked(Button *btn) = 0;
};

// Buttons introduce the concept of a clicking.
class Button : public Control {

  ButtonHandler *handler;

  virtual void HandleMouse(int mx, int my, bool l, bool m, bool r) override {
    if (mx >= getX() && mx <= getX() + getW()) {
      if (my >= getY() && my <= getY() + getH()) {
        if (l) {
          if (getTexture())
            getTexture()->setCurrentFrame(2);
          if (handler)
            handler->ButtonClicked(this);
          return;
        } else {
          if (getTexture())
            getTexture()->setCurrentFrame(1);
          return;
        }
      }
    }
    if (getTexture())
      getTexture()->setCurrentFrame(0);
  }

public:
  Button(const detail::Texture &texture, ButtonHandler *handler)
      : Control(), handler(handler) {
    if (texture.getH() > 0) {
      SetTexture(texture);
      getTexture()->setHFrames(1);
      getTexture()->setVFrames(3);
      getTexture()->setCurrentFrame(0);

      setW(texture.getW());
      setH(texture.getH() / 3);
    }
  }

  virtual ~Button() { handler = nullptr; }
};

} // namespace ui

#endif // UI_LIB_BUTTON