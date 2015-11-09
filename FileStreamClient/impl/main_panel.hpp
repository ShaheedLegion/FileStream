#pragma once
#ifndef _MAIN_PANEL_HPP
#define _MAIN_PANEL_HPP

#include "../ui/ui.hpp"
#include "rendition_panel.hpp"

namespace impl {

class MainPanel : public ui::InputPanel, public game::Output {
public:
  MainPanel(detail::RendererSurface &surface)
      : InputPanel(surface, ""), m_chatPanel(surface, "bg.graw", 200, 0) {}

  ~MainPanel() {}

  virtual void process() override {
    // try to get the text for the 'input' button for the next update.
    if (m_scratch.empty())
      return;

    m_exiting = m_chatPanel.exiting();
  }

  virtual void update() override {
    m_bg.draw(m_screen);

    // Cannot call base update here since that clears the keys queue.
    // ui::InputPanel::update();

    m_chatPanel.update();
    m_exiting = m_chatPanel.exiting();

    // Now commit those changes to the ui.
    ui::InputPanel::commit();
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
  RenditionPanel m_chatPanel;
};

} // namespace impl

#endif // _MAIN_PANEL_HPP