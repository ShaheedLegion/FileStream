// Copyright (c) - 2015, Shaheed Abdol.
// Use this code as you wish, but don't blame me.
#ifndef IMPL
#define IMPL

#include "ui.hpp"
#include "process_client.hpp"

namespace impl {

// The command processor will hold all the details.
struct command_processor {
  command_processor(game::Output &out) : m_client(out) {}

  bool process_command(ui::InputPanel *panel) {
    if (panel == nullptr)
      return true; // exit if we get a null panel.

    const std::string &scratch{panel->getScratch()};

    if (util::icompare(scratch, "-quit")) {
      m_client.getOutput().sendOutput("Exiting session, goodbye.");
      return true;
    } else {
      m_client.processCommand(scratch);
    }
    return false;
  }

  void update(ui::InputPanel *panel) {
    if (panel == nullptr)
      return;

    m_client.update();
  }

protected:
  game::ProcessClient m_client;
};

class RenditionPanel : public ui::InputPanel, public game::Output {
public:
  RenditionPanel(detail::RendererSurface &surface)
      : InputPanel(surface, "bg.graw"), m_processor(*this) {

    addText("Welcome to NetChatmium: by Shaheed Abdol. 2015");
    addText("This is a general c++ chat application.");
    addText("Press [Esc] to quit.");
    addText("Type -help [Enter] to get a hint.");
    addText("-------------------------------------------");
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
    m_processor.update(this);

    ui::InputPanel::update();
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

  virtual void minimizeOutput() override { m_screen.Minimize(true); }
  virtual void flashOutput() override { m_screen.FlashWindow(true); }

  void clearOutput() override { clearText(); }

protected:
  command_processor m_processor;
};

} // namespace impl

#endif // IMPL