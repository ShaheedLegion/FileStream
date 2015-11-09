#pragma once
#ifndef RENDITION_PANEL_HPP
#define RENDITION_PANEL_HPP

#include "../ui/ui.hpp"
#include "../impl/impl.hpp"
#include "../impl/process_client.hpp"

namespace impl {

class RenditionPanel : public ui::InputPanel, public game::Output {
public:
  RenditionPanel(detail::RendererSurface &surface, const std::string &texName,
                 int x, int y)
      : InputPanel(surface, texName, x, y), m_processor(*this),
        m_minBtn("btn_min.graw"), m_dragBtn("btn_drag.graw", true),
        m_attachBtn("btn_attach.graw"), m_clsBtn("btn_exit.graw") {
    addText("Welcome to NetChatmium: by Shaheed Abdol. 2015");
    addText("This is a general c++ chat application.");
    addText("Press [Esc] to quit.");
    addText("Type -help [Enter] to get a hint.");
    addText("-------------------------------------------");

    surface.RegisterObserver(&m_minBtn);
    surface.RegisterObserver(&m_dragBtn);
    surface.RegisterObserver(&m_attachBtn);
    surface.RegisterObserver(&m_clsBtn);
  }

  RenditionPanel(detail::RendererSurface &surface)
      : InputPanel(surface, "bg.graw"), m_processor(*this),
        m_minBtn("btn_min.graw"), m_dragBtn("btn_drag.graw", true),
        m_attachBtn("btn_attach.graw"), m_clsBtn("btn_exit.graw") {

    addText("Welcome to NetChatmium: by Shaheed Abdol. 2015");
    addText("This is a general c++ chat application.");
    addText("Press [Esc] to quit.");
    addText("Type -help [Enter] to get a hint.");
    addText("-------------------------------------------");

    surface.RegisterObserver(&m_minBtn);
    surface.RegisterObserver(&m_dragBtn);
    surface.RegisterObserver(&m_attachBtn);
    surface.RegisterObserver(&m_clsBtn);
  }

  ~RenditionPanel() {}

  virtual void process() override {
    // try to get the text for the 'input' button for the next update.
    if (m_scratch.empty())
      return;

    // Else process a command or a send request
    m_exiting = m_processor.process_command(this);
  }

  virtual void update() override {
    // Button input processing.
    processButtonInput();

    // Display output processing.
    m_processor.update(this);

    // Render the main interface (fullscreen)
    ui::InputPanel::update();

    // Render elements on top of interface.
    m_minBtn.draw(m_screen, 760, 10);
    m_dragBtn.draw(m_screen, 760, 60);
    m_attachBtn.draw(m_screen, 760, 110);
    m_clsBtn.draw(m_screen, 760, 160);
  }

  virtual void sendOutput(const std::string &data) override {
    if (data.empty())
      return;

    addText(data);
  }

  virtual void sendOutput(const std::vector<std::string> &text) override {
    if (text.empty())
      return;

    addText(text);
  }

  virtual void sendOutput(const print::printQueue &text) override {
    if (text.empty())
      return;

    addText(text);
  }

  virtual void minimizeOutput() override { m_screen.Minimize(true); }
  virtual void flashOutput() override { m_screen.FlashWindow(true); }

  void clearOutput() override { clearText(); }

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
      InputPanel::openDir(m_processor.getAttachDir());
    } else if (m_clsBtn.wasClicked()) {
      m_exiting = m_processor.process_command(this, "-quit");
    }
  }
};

} // namespace impl
#endif // RENDITION_PANEL_HPP