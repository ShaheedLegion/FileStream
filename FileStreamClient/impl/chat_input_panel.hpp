#pragma once
#ifndef CHAT_INPUT_PANEL_HPP
#define CHAT_INPUT_PANEL_HPP

#include "../ui/ui.hpp"
#include "../impl/impl.hpp"
#include "../impl/process_client.hpp"

namespace impl {

class ChatInputPanel : public ui::InputPanel {
public:
  ChatInputPanel(detail::RendererSurface &surface, const std::string &texName,
                 int x, int y)
      : InputPanel(surface, texName, x, y){
  }

  ChatInputPanel(detail::RendererSurface &surface)
      : InputPanel(surface, "") {
  }

  ~ChatInputPanel() {}

  virtual void process() override {
  }

};

} // namespace impl
#endif // CHAT_INPUT_PANEL_HPP