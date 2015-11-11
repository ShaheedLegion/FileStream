#pragma once
#ifndef _MAIN_PANEL_HPP
#define _MAIN_PANEL_HPP

#include "../ui/ui.hpp"
#include "rendition_panel.hpp"
#include "chat_input_panel.hpp"
#include "command_panel.hpp"

namespace impl {

class MainPanel : public ui::Panel, public game::Output {
public:
  MainPanel(detail::RendererSurface &surface)
      : Panel(surface, ""), m_processor(*this),
        m_chatPanel(surface, "bg.graw", 0, 0),
        m_dashPanel(surface, "dash_bg.graw", 600, 0),
        m_inputPanel(surface, "chat_bg.graw", 0, 500) {}

  ~MainPanel() {}

  virtual void update() override {
    m_bg.draw(m_screen);

    // Display output processing.
    m_processor.update(this);

    m_dashPanel.update();
    m_chatPanel.update();
    m_inputPanel.update();

    std::string text;
    m_inputPanel.getLastCommand(text);
    if (!text.empty()) {
	  m_processor.process_command(this, text);
      m_chatPanel.addText(text);
    }

    m_dashPanel.processButtons(m_processor);
    m_exiting = m_dashPanel.exiting();

    // Now commit those changes to the ui.
    commit();
  }

  virtual void sendOutput(const std::string &data) override {
    if (data.empty())
      return;

    m_chatPanel.addText(data);
  }

  virtual void sendOutput(const std::vector<std::string> &text) override {
    if (text.empty())
      return;

    m_chatPanel.addText(text);
  }

  virtual void sendOutput(const print::printQueue &text) override {
    if (text.empty())
      return;

    m_chatPanel.addText(text);
  }

  virtual void minimizeOutput() override { m_screen.Minimize(true); }
  virtual void flashOutput() override { m_screen.FlashWindow(true); }

  void clearOutput() override { m_chatPanel.clearText(); }

protected:
  command_processor m_processor;
  RenditionPanel m_chatPanel;
  CommandPanel m_dashPanel;
  ChatInputPanel m_inputPanel;
};

} // namespace impl

#endif // _MAIN_PANEL_HPP