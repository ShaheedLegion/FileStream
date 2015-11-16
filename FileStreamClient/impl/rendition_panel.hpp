#pragma once
#ifndef RENDITION_PANEL_HPP
#define RENDITION_PANEL_HPP

#include "../ui/ui.h"
#include "../impl/impl.hpp"
#include "../impl/process_client.hpp"

namespace impl {

/*
class RenditionPanel : public ui::OutputPanel {
public:
  RenditionPanel(detail::RendererSurface &surface, const std::string &texName,
                 int x, int y)
      : OutputPanel(surface, texName, x, y) {
    addText("Welcome to NetChatmium: by Shaheed Abdol. 2015");
    addText("This is a general c++ chat application.");
    addText("Press [Esc] to quit.");
    addText("Type -help [Enter] to get a hint.");
    addText("-------------------------------------------");
  }

  RenditionPanel(detail::RendererSurface &surface)
      : OutputPanel(surface, "bg.graw") {
    addText("Welcome to NetChatmium: by Shaheed Abdol. 2015");
    addText("This is a general c++ chat application.");
    addText("Press [Esc] to quit.");
    addText("Type -help [Enter] to get a hint.");
    addText("-------------------------------------------");
  }

  ~RenditionPanel() {}
};
*/
} // namespace impl
#endif // RENDITION_PANEL_HPP