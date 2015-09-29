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
    return processInternal(panel, panel->getScratch());
  }

  bool process_command(ui::InputPanel *panel,
                       const std::string &overrideCommand) {
    return processInternal(panel, overrideCommand);
  }

  void update(ui::InputPanel *panel) {
    if (panel == nullptr)
      return;

    m_client.update();
  }

  const std::string &getAttachDir() { return m_client.getAttachDir(); }

protected:
  game::ProcessClient m_client;

  bool processInternal(ui::InputPanel *panel, const std::string &command) {
    if (panel == nullptr)
      return true; // exit if we get a null panel.

    if (util::icompare(command, "-quit")) {
      m_client.getOutput().sendOutput("Exiting session, goodbye.");
      return true;
    } else {
      m_client.processCommand(command);
    }
    return false;
  }
};

class RenditionPanel : public ui::InputPanel, public game::Output {
public:
  RenditionPanel(detail::RendererSurface &surface)
      : InputPanel(surface, "bg.graw"), m_processor(*this),
        m_minBtn("btn_min.graw"), m_dragBtn("btn_drag.graw"),
        m_attachBtn("btn_attach.graw"), m_clsBtn("btn_exit.graw") {

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
  command_processor m_processor;
  ui::Button m_minBtn;
  ui::Button m_dragBtn;
  ui::Button m_attachBtn;
  ui::Button m_clsBtn;

  void processButtonInput() {
    // First make sure the screen drag area is initialized.
    m_screen.InitializeDragArea(m_dragBtn.getX(), m_dragBtn.getY(),
                                m_dragBtn.getW(), m_dragBtn.getH());

    // Process things the buttons should do.
    int mx, my;
    bool l, m, r;
    m_screen.QueryMouse(mx, my, l, m, r);
    if (m_minBtn.handleMouseInput(mx, my, l, m, r)) {
      m_screen.ClearMouse();
      m_processor.process_command(this, "-min");
    } else if (m_attachBtn.handleMouseInput(mx, my, l, m, r)) {
      m_screen.ClearMouse();
      InputPanel::openDir(m_processor.getAttachDir());
    } else if (m_clsBtn.handleMouseInput(mx, my, l, m, r)) {
      m_screen.ClearMouse();
      m_exiting = m_processor.process_command(this, "-quit");
    }
  }
};

} // namespace impl

#endif // IMPL