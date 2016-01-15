#pragma once
#ifndef UI_LIB
#define UI_LIB

#include "detail.h"
#include "texture_loader.h"
#include <vector>

namespace ui {
enum VAlignType { TOP, MIDDLE, BOTTOM };
enum HAlignType { LEFT, CENTER, RIGHT };
enum OrientationType { HORIZONTAL, VERTICAL, STACK };
enum DimensionType { PERCENT, PIXELS, AUTO };
enum OverflowConstraintType { WIDTH, HEIGHT, BOTH, NONE };

struct DimensionInfo {
  // This is the numerical portion of the xml spec.
  int value;
  int minPossible;
  DimensionType type;

  // Set the default to auto so that controls take up space on the screen.
  DimensionInfo() : value(-1), type(AUTO) {}

  void parse(const std::string &v) {
    // Figure out the rules for parsing this value here.
    if (v.empty())
      return; // Should throw.

    if (v == "auto") {
      type = AUTO;
      value = -1;
      return;
    }

    if (v.find_first_of('%') != std::string::npos) {
      std::string numeric = v.substr(0, v.find_first_of('%'));
      type = PERCENT;
      value = atoi(numeric.c_str());
      return;
    }

    if (v.find_first_of('px') != std::string::npos) {
      std::string numeric = v.substr(0, v.find_first_of('px'));
      type = PIXELS;
      value = atoi(numeric.c_str());
    }
  }

  // This needs some more work - especially when the dimensions are set to -1.
  int getValue(int from) {
    if (type == AUTO)
      return minPossible;
    else if (type == PIXELS)
      return value;
    else if (type == PERCENT) {
      // get the percentage from the given value
      float fvalue =
          (static_cast<float>(value) / 100.0f) * static_cast<float>(from);
      return (static_cast<int>(fvalue) > minPossible
                  ? minPossible
                  : static_cast<int>(fvalue));
    }
  }
};

// Controls introduce the concept of a heirarchy.
class Control : public detail::IRenderable, public detail::Interactive {
protected:
  detail::Texture *texture;
  std::string name;
  std::vector<Control *> children;
  DimensionInfo width;
  DimensionInfo height;
  VAlignType vAlign;
  HAlignType hAlign;
  OrientationType orientation;
  OverflowConstraintType overflowConstraint;

public:
  Control()
      : texture(nullptr), vAlign(TOP), hAlign(LEFT), orientation(HORIZONTAL),
        overflowConstraint(NONE) {}

  // Attributes
  virtual void SetTexture(const detail::Texture &tex) {
    texture = const_cast<detail::Texture *>(&tex);
    width.minPossible = texture->getW();
    height.minPossible = texture->getH();
  }

  virtual void SetName(const std::string &value) { name = value; }
  const std::string &getName() const { return name; }

  virtual void SetWidth(const std::string &value) {
    // Parse out the data here.
    width.parse(value);
  }
  DimensionInfo &getWidth() { return width; }

  virtual void SetHeight(const std::string &value) {
    // Parse out the height here.
    height.parse(value);
  }
  DimensionInfo &getHeight() { return height; }

  virtual void SetVAlign(const std::string &value) {
    // Parse out the v align.
    if (value.empty())
      return;

    if (value[0] == 'm')
      vAlign = MIDDLE;
    else if (value[0] == 'b')
      vAlign = BOTTOM;
  }
  VAlignType getVAlign() const { return vAlign; }

  virtual void SetHAlign(const std::string &value) {
    // Parse out the h align.
    if (value.empty())
      return;

    if (value[0] == 'c')
      hAlign = CENTER;
    else if (value[0] == 'r')
      hAlign = RIGHT;
  }
  HAlignType getHAlign() const { return hAlign; }

  virtual void SetOrientation(const std::string &value) {
    // Parse out the orientation here.
    if (value.empty())
      return;

    if (value[0] == 'v')
      orientation = VERTICAL;
    else if (value[0] == 's')
      orientation = STACK;
  }

  OrientationType getOrientation() const { return orientation; }

  virtual void SetOverflowConstraint(const std::string &value) {
    if (value.empty())
      return;

    if (value[0] == 'w')
      overflowConstraint = WIDTH;
    else if (value[0] == 'h')
      overflowConstraint = HEIGHT;
    else if (value[0] == 'b')
      overflowConstraint = BOTH;
  }

  // Construction / DOM / Position / Events.
  virtual ~Control() {
    for (auto child : children)
      delete child;
    children.clear();
  }

  virtual void AddChild(Control *control) { children.push_back(control); }
  std::vector<Control *> &getChildren() { return children; }

  virtual void setX(int x) override {
    IRenderable::setX(x);

    if (texture != nullptr)
      texture->setX(x);
  }
  virtual void setY(int y) override {
    IRenderable::setY(y);
    if (texture != nullptr)
      texture->setY(y);
  }

  detail::Texture *getTexture() { return texture; }

  virtual void render(detail::RenderBuffer &target) override {
    // Render ourselves.
    if (texture != nullptr)
      texture->render(target);

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

  // Let derived classes override this safely to add their own attributes.
  virtual void SetAttribute(const std::string &name, const std::string &value) {
    // Set all attributes here - they can be used by the derived classes.
    if (name == "background") {
      detail::TextureLoader *loader = detail::TextureLoader::getInstance();
      SetTexture(loader->getTexture(value));
    } else if (name == "name") {
      SetName(value);
    } else if (name == "width") {
      SetWidth(value);
    } else if (name == "height") {
      SetHeight(value);
    } else if (name == "valign") {
      SetVAlign(value);
    } else if (name == "halign") {
      SetHAlign(value);
    } else if (name == "orientation") {
      SetOrientation(value);
    } else if (name == "overflow_constraint") {
      SetOverflowConstraint(value);
    }
  }
};

} // namespace ui

#endif // UI_LIB