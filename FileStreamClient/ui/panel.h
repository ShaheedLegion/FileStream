#pragma once
#ifndef UI_LIB_PANEL
#define UI_LIB_PANEL

#include "ui.h"

namespace ui {

// Panels are fairly generic.
class Panel : public Control {

public:
  Panel(const detail::Texture &texture) : Control(texture) {}

  virtual ~Panel() {}
};

} // namespace ui

#endif // UI_LIB_PANEL