#pragma once
#ifndef UI_LIB
#define UI_LIB

#include "detail.h"
#include <vector>

namespace ui {

// Controls introduce the concept of a heirarchy.
class Control : public detail::IRenderable, public detail::Interactive {
  detail::Texture texture;
  std::vector<Control *> children;

public:
  Control(const detail::Texture &texture) : texture(texture) {}

  virtual ~Control() {
    for (auto child : children)
      delete child;
    children.clear();
  }

  void AddChild(Control *control) { children.push_back(control); }

  virtual void setX(int x) override {
    IRenderable::setX(x);
    texture.setX(x);
  }
  virtual void setY(int y) override {
    IRenderable::setY(y);
    texture.setY(y);
  }

  detail::Texture &getTexture() { return texture; }

  virtual void render(detail::RenderBuffer &target) override {
    // Render ourselves.
    texture.render(target);

    // Render child controls.
    for (auto child : children)
      child->render(target);
  }

  virtual void HandleKey(int keyCode, bool pressed) override {
    for (auto child : children)
      child->HandleKey(keyCode, pressed);
  }

  // We can accomplish all other events with this one - drag, move, capture.
  virtual void HandleMouse(int mx, int my, bool l, bool m, bool r) override {
    for (auto child : children)
      child->HandleMouse(mx, my, l, m, r);
  }
};
} // namespace ui

#endif // UI_LIB