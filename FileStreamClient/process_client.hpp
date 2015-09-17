// Copyright (c) - 2015, Shaheed Abdol.
// Use this code as you wish, but don't blame me.

#ifndef PROCESS_CLIENT
#define PROCESS_CLIENT

#include <sstream>
#include "../chat_common.hpp"

namespace game {

class Output {
public:
  virtual ~Output() {}
  virtual void sendOutput(const std::string &data) = 0;
  virtual void sendOutput(const std::vector<std::string> &text) = 0;
  virtual void minimizeOutput() = 0;
  virtual void flashOutput() = 0;
  virtual void clearOutput() = 0;
};

class ProcessClient {
private:
  Output &m_out;
  net::NetClient m_client;

public:
  ProcessClient(Output &out) : m_out(out) {}

  void processCommand(const std::string &command) {
    if (util::icompare(command, "-help")) {
      m_out.sendOutput("  -name [alias]: Set your alias for this session.");
      m_out.sendOutput("        You must do this first.");
      m_out.sendOutput("  -con  [alias]: Connect to server.");
      m_out.sendOutput("        You must connect to chat.");
      m_out.sendOutput("  -ls:  List the users in this session.");
      m_out.sendOutput("  -pvt  [user] [msg]: Send private message to user.");
      m_out.sendOutput("        We'll start supporting conversations soon.");
      m_out.sendOutput("  -file [choose]: Send a file to all users.");
      m_out.sendOutput("  -pfil [user] [choose]: Send file to user.");

      // Things that are not handled by the player class.
      m_out.sendOutput(" - - - - - - - - - - - - - - - - - - - - - - - - - -");
      m_out.sendOutput("  -min: Minimize the client.");
      m_out.sendOutput("  -cls: Clear the console.");
      m_out.sendOutput("  -quit End the session.");
      m_out.sendOutput("    ");
      m_out.sendOutput("Typing a message will send to all users be default.");
      return;
    } else if (util::icompare(command, "-name", true)) {
      setAlias(command);
      return;
    } else if (util::icompare(command, "-con", true)) {
      connectServer(command);
      return;
    } else if (util::icompare(command, "-ls")) {
      listUsers(); // working
      return;
    } else if (util::icompare(command, "-pvt", true)) {
      sendPrivate(command);
      return;
    } else if (util::icompare(command, "-file")) {
      sendFileGeneral(); // works
      return;
    } else if (util::icompare(command, "-pfil", true)) {
      sendFileUser(command); // works
      return;
    } else if (util::icompare(command, "-min")) {
      m_out.minimizeOutput();
      return;
    } else if (util::icompare(command, "-cls")) {
      m_out.clearOutput(); // works
      return;
    }

    // If the message could not be parsed, then it's a general chat message.
    m_client.AddMessage(command);
  }

  void setAlias(const std::string &command) {
    // Parse out the user alias and set it on the net client.
    std::string alias{util::split(command, "-name", true)};
    if (alias.empty()) {
      m_out.sendOutput("Please supply a valid name");
      return;
    } else if (alias.length() > 10) {
      m_out.sendOutput("Please choose a name that's only 10 characters long.");
      return;
    }

    m_client.SetAlias(alias);
  }

  void connectServer(const std::string &command) {
    // Parse out the IP address and connect to the server.
    std::string ip{util::split(command, "-con", true)};
    if (ip.empty()) {
      m_out.sendOutput("Please provide a valid IP address.");
      return;
    }

    m_client.Connect(ip);
  }

  void listUsers() {
    // Ok, so we need some kind of singleton that can process this command.
    m_out.sendOutput("Fetching list from server....");
  }
  void sendPrivate(const std::string &command) {
    // Send the private message - preprocess for malformed user names.
  }
  void sendFileGeneral() {
    // First prompt the user for a file ...
    // Perform the upload and display the file.
  }
  void sendFileUser(const std::string &command) {
    // First preprocess the user to check for malformed input.
    // Prompt the user for the file...
    // Perform the upload and display the file.
    // out.sendOutput("Put item where?");
  }

  void helpUser(game::Output &out) const {}

  Output &getOutput() const { return m_out; }
  void sendOutput(const std::string &data) { m_out.sendOutput(data); }

  void update() {
    // Check if we can get any data from the net client.
    std::vector<std::string> text;
    m_client.GetMessages(text);
    if (!text.empty())
      m_out.flashOutput();
    m_out.sendOutput(text);
  }
};

} // namespace game

#endif // PROCESS_CLIENT