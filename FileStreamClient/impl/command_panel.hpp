#pragma once
#ifndef COMMAND_PANEL_HPP
#define COMMAND_PANEL_HPP

#include "../ui/ui.hpp"
#include "../impl/impl.hpp"
#include "../impl/process_client.hpp"

namespace impl {

class CommandPanel : public ui::Panel, public game::Output {
public:
  CommandPanel(detail::RendererSurface &surface, const std::string &texName,
               int x, int y)
      : Panel(surface, texName, x, y), m_processor(*this),
        m_minBtn("btn_min.graw"), m_dragBtn("btn_drag.graw", true),
        m_attachBtn("btn_attach.graw"), m_clsBtn("btn_exit.graw") {

    surface.RegisterObserver(&m_minBtn);
    surface.RegisterObserver(&m_dragBtn);
    surface.RegisterObserver(&m_attachBtn);
    surface.RegisterObserver(&m_clsBtn);
  }

  CommandPanel(detail::RendererSurface &surface)
      : Panel(surface, ""), m_processor(*this), m_minBtn("btn_min.graw"),
        m_dragBtn("btn_drag.graw", true), m_attachBtn("btn_attach.graw"),
        m_clsBtn("btn_exit.graw") {
    surface.RegisterObserver(&m_minBtn);
    surface.RegisterObserver(&m_dragBtn);
    surface.RegisterObserver(&m_attachBtn);
    surface.RegisterObserver(&m_clsBtn);
  }

  ~CommandPanel() {}

  virtual void update() override {
    // Button input processing.
    processButtonInput();

    // Display output processing.
    m_processor.update(this);

    // Render the main interface (fullscreen)
    ui::Panel::update();

    // Render elements on top of interface.
    m_attachBtn.draw(m_screen, 610, 10);    
    m_dragBtn.draw(m_screen, 660, 10);
    m_minBtn.draw(m_screen, 710, 10);
    m_clsBtn.draw(m_screen, 760, 10);
  }

  //virtual void process() override {}

  virtual void minimizeOutput() override { m_screen.Minimize(true); }
  virtual void flashOutput() override { m_screen.FlashWindow(true); }

  virtual void sendOutput(const std::string &data) override {
    if (data.empty())
      return;

    //addText(data);
  }

  virtual void sendOutput(const std::vector<std::string> &text) override {
    if (text.empty())
      return;

    //addText(text);
  }

  virtual void sendOutput(const print::printQueue &text) override {
    if (text.empty())
      return;

    //addText(text);
  }

  void clearOutput() override { /*clearText();*/ }

protected:
  command_processor m_processor;
  ui::Button m_minBtn;
  ui::Button m_dragBtn;
  ui::Button m_attachBtn;
  ui::Button m_clsBtn;

  void processButtonInput() {
    // Process things the buttons should do.
    if (m_minBtn.wasClicked()) {
      m_processor.process_command(this, "-min");
    } else if (m_attachBtn.wasClicked()) {
      openDir(m_processor.getAttachDir());
    } else if (m_clsBtn.wasClicked()) {
      m_exiting = m_processor.process_command(this, "-quit");
    }
  }
};

} // namespace impl
#endif // COMMAND_PANEL_HPP