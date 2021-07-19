#pragma once
#ifndef COMMAND_PANEL_HPP
#define COMMAND_PANEL_HPP

#include "../ui/ui.h"
#include "../impl/impl.hpp"
#include "../impl/process_client.hpp"

namespace impl {

/*
class CommandPanel : public ui::Panel {
public:
  CommandPanel(detail::RendererSurface &surface, const std::string &texName,
               int x, int y)
      : Panel(surface, texName, x, y), m_minBtn("btn_min.graw"),
        m_dragBtn("btn_drag.graw", true), m_attachBtn("btn_attach.graw"),
        m_clsBtn("btn_exit.graw") {

    surface.RegisterObserver(&m_minBtn);
    surface.RegisterObserver(&m_dragBtn);
    surface.RegisterObserver(&m_attachBtn);
    surface.RegisterObserver(&m_clsBtn);
  }

  CommandPanel(detail::RendererSurface &surface)
      : Panel(surface, ""), m_minBtn("btn_min.graw"),
        m_dragBtn("btn_drag.graw", true), m_attachBtn("btn_attach.graw"),
        m_clsBtn("btn_exit.graw") {
    surface.RegisterObserver(&m_minBtn);
    surface.RegisterObserver(&m_dragBtn);
    surface.RegisterObserver(&m_attachBtn);
    surface.RegisterObserver(&m_clsBtn);
  }

  ~CommandPanel() {}

  virtual void update() override {
    // Render the main interface (fullscreen)
    ui::Panel::update();

    // Render elements on top of interface.
    m_attachBtn.draw(m_screen, 610, 10);
    m_dragBtn.draw(m_screen, 660, 10);
    m_minBtn.draw(m_screen, 710, 10);
    m_clsBtn.draw(m_screen, 760, 10);
  }

  void processButtons(command_processor &processor) {
    // Process things the buttons should do.
    if (m_minBtn.wasClicked()) {
      processor.process_command(this, "-min");
    } else if (m_attachBtn.wasClicked()) {
      openDir(processor.getAttachDir());
    } else if (m_clsBtn.wasClicked()) {
      m_exiting = processor.process_command(this, "-quit");
    }
  }

protected:
  ui::Button m_minBtn;
  ui::Button m_dragBtn;
  ui::Button m_attachBtn;
  ui::Button m_clsBtn;
};
*/
} // namespace impl
#endif // COMMAND_PANEL_HPP