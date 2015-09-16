// Copyright (c) - 2015, Shaheed Abdol.
// Use this code as you wish, but don't blame me.
#include "impl.hpp"

// This is the guts of the renderer, without this it will do nothing.
DWORD WINAPI Update(LPVOID lpParameter) {
  Renderer *g_renderer = static_cast<Renderer *>(lpParameter);

  impl::RenditionPanel panel(g_renderer->screen);

  while (g_renderer->IsRunning()) {
    panel.update();
    if (panel.exiting())
      g_renderer->SetRunning(false);
    g_renderer->updateThread.Delay(100);
  }

  return 0;
}

int main(int argc, char *argv[]) {
  Renderer renderer("Rendition", &Update, nullptr);
  return 0;
}
