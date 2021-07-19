// Copyright (c) - 2015, Shaheed Abdol.
// Use this code as you wish, but don't blame me.
#include "Renderer.hpp"
#include "impl\main_panel.hpp"
#include "xml\src\pugixml.hpp"
#include "util\util.hpp"

namespace adapter {
class Observer : public detail::ScreenObserver {
  detail::Interactive *m_interactive;

public:
  Observer(detail::Interactive *interactive) : m_interactive(interactive) {}
  virtual ~Observer() {}

  virtual void HandleMouse(int x, int y, bool l, bool m, bool r) {
    if (m_interactive)
      m_interactive->HandleMouse(x, y, l, m, r);
  }
  virtual bool HandleDrag(int x, int y, bool l) {
    if (m_interactive)
      m_interactive->HandleMouse(x, y, l, false, false);

    return false;
  }
};
} // namespace adapter

// This is the guts of the renderer, without this it will do nothing.
DWORD WINAPI Update(LPVOID lpParameter) {
  Renderer *g_renderer = static_cast<Renderer *>(lpParameter);

  // We create the UI here based on a simple xml file that describes the panels.
  std::vector<std::string> paths;
  util::Paths::getInstance()->getPaths(paths, g_renderer->getUIName());
  pugi::xml_document uiTree;
  bool loaded = false;

  for (auto path : paths) {
    pugi::xml_parse_result result = uiTree.load_file(path.c_str());
    if (result.status == pugi::xml_parse_status::status_ok) {
      loaded = true;
      break;
    }
  }

  if (!loaded) {
    std::cout << "Could not load main interface definition." << std::endl;
    return -1;
  }

  impl::MainPanel &panel(impl::MainPanel::ConstructUI(uiTree));
  adapter::Observer fakeObserver(&panel);

  g_renderer->screen.RegisterObserver(&fakeObserver);

  while (g_renderer->IsRunning()) {
    panel.update(g_renderer->screen.GetPixels(), g_renderer->screen.GetWidth(),
                 g_renderer->screen.GetHeight());

    g_renderer->screen.Flip();
    // if (panel.exiting())
    // g_renderer->SetRunning(false);
    g_renderer->updateThread.Delay(100);
  }

  return 0;
}

int main(int argc, char *argv[]) {
  Renderer renderer("Rendition", "main.xml", &Update, nullptr, true);
  return 0;
}
