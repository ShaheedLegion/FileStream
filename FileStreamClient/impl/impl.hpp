// Copyright (c) - 2015, Shaheed Abdol.
// Use this code as you wish, but don't blame me.
#ifndef IMPL
#define IMPL

#include "../ui/ui.h"
#include "process_client.hpp"

namespace impl {

// The command processor will hold all the details.
struct command_processor {
  command_processor(game::Output &out) : m_client(out) {}

  bool process_command(const std::string &overrideCommand) {
    return processInternal(overrideCommand);
  }

  void update() {
    m_client.update();
  }

  const std::string &getAttachDir() { return m_client.getAttachDir(); }

protected:
  game::ProcessClient m_client;

  bool processInternal(const std::string &command) {
	  /*
    if (util::icompare(command, "-quit")) {
      m_client.getOutput().sendOutput("Exiting session, goodbye.");
      return true;
    } else {
      m_client.processCommand(command);
    }
	*/
    return false;
  }
};

} // namespace impl

#endif // IMPL