// Copyright (c) - 2015, Shaheed Abdol.
// Use this code as you wish, but don't blame me.

#ifndef PROCESS_CLIENT
#define PROCESS_CLIENT

#include "../chat_common.hpp"
#include <sstream>
#include <commdlg.h>

namespace game {

class Output {
public:
  virtual ~Output() {}
  virtual void sendOutput(const std::string &data) = 0;
  virtual void sendOutput(const std::vector<std::string> &text) = 0;
  virtual void sendOutput(const print::printQueue &text) = 0;
  virtual void minimizeOutput() = 0;
  virtual void flashOutput() = 0;
  virtual void clearOutput() = 0;
};

class ProcessClient {
private:
  Output &m_out;
  net::NetClient m_client;

  bool GetFileName(std::string &name, std::string &path) {
    char buf[MAX_PATH];
    char nbuf[MAX_PATH];
    ZeroMemory(buf, MAX_PATH);
    ZeroMemory(nbuf, MAX_PATH);

    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(OPENFILENAME));

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.lpstrFilter = "All Files\0*.*\0\0";
    ofn.lpstrFile = buf;
    ofn.lpstrFileTitle = nbuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.nMaxFileTitle = MAX_PATH;
    ofn.Flags = OFN_DONTADDTORECENT | OFN_EXPLORER | OFN_FILEMUSTEXIST |
                OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
      // Send the path on to the comms handler.
      int plen = strlen(buf);
      path.assign(buf, plen);

      int nlen = strlen(nbuf);
      name.assign(nbuf, nlen);
      return true;
    }

    // The user cancelled file sendind.
    return false;
  }

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
      m_out.sendOutput("Typed messages are sent to all connected users.");
      m_out.sendOutput("Unless you use the -pvt command.");
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

  void listUsers() { m_client.GetUserList(); }

  void sendPrivate(const std::string &command) {
    // Parse out the user name .... we will need to send this to the server for
    // processing.
  }

  void sendFileGeneral() {
    // First prompt the user for a file ...
    // Perform the upload and display the file.
    std::string name;
    std::string path;
    if (GetFileName(name, path)) {
      // Do something here to send the file.
      m_client.SendFile(name, path);
      return;
    }

    m_out.sendOutput("Private file sending cancelled.");
  }

  void sendFileUser(const std::string &command) {
    // Send the private message - preprocess for malformed user names.
    std::string user{util::split(command, "-pfil", true)};
    std::string name;
    std::string path;
    if (GetFileName(name, path)) {
      // Do something here to send the file.
      m_client.SendFileUser(name, path, user);
      return;
    }

    m_out.sendOutput("File sending cancelled.");
  }

  void helpUser(game::Output &out) const {}

  Output &getOutput() const { return m_out; }
  void sendOutput(const std::string &data) { m_out.sendOutput(data); }

  void update() {
    // Check if we can get any data from the net client.
    print::printQueue text;
    m_client.GetMessages(text);
    if (!text.empty())
      m_out.flashOutput();
    m_out.sendOutput(text);
  }
};

} // namespace game

#endif // PROCESS_CLIENT