#ifndef _CHAT_COMMON_HPP_
#define _CHAT_COMMON_HPP_
#pragma once

#include <ws2tcpip.h>
#include <WinSock2.h>
#include <iostream>
#include <string>
#include <vector>

/*
Sequence of events.

Server is running.
Client connects.

State 1:
  Client sends alias.
  Server acks the interrogation with a message.

State 2:
  Client sends query packet.
  Server acks the query with a response.


State 3:
  Client sends message.
  Server acks message with a response.

State 4:
  Client sends a private message.
  Server acks the private message with a response.
*/

#if 0
So, the client now responds to the server ack messages.
Great.

The servers ProcessMessages function should now change to be able to push
outbound messages into a special queue for each client.

There should also be a global queue for clients.
#endif // 0
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

namespace comms {

#define CHATMIUM_PORT_ST "54546"
#define CHATMIUM_PORT_NR 54546
// Alias is the first thing sent to the server.
#define PKT_ALIAS 0x00002
#define PKT_ALIAS_ACK 0x00003

// The client periodically sends a query to the server.
#define PKT_QRY 0x00004
#define PKT_QRY_ACK 0x00005

// The client sends a message to the server.
#define PKT_MSG 0x00006
#define PKT_MSG_ACK 0x00007

// The client sends a private message to the server.
#define PKT_PVT 0x00008
#define PKT_PVT_ACK 0x00009

std::string CharToMessageType(unsigned short msgtype) {
  switch (msgtype) {
  case PKT_ALIAS:
    return "alias";
  case PKT_ALIAS_ACK:
    return "alias_ack";
  case PKT_QRY:
    return "query";
  case PKT_QRY_ACK:
    return "query_ack";
  case PKT_MSG:
    return "message";
  case PKT_MSG_ACK:
    return "message_ack";
  case PKT_PVT:
    return "private";
  case PKT_PVT_ACK:
    return "private_ack";
  }
  return "unk";
}

unsigned short MessageTypeToChar(const std::string &type) {
  if (type == "alias")
    return PKT_ALIAS;
  if (type == "alias_ack")
    return PKT_ALIAS_ACK;
  if (type == "query")
    return PKT_QRY;
  if (type == "query_ack")
    return PKT_QRY_ACK;
  if (type == "message")
    return PKT_MSG;
  if (type == "message_ack")
    return PKT_MSG_ACK;
  if (type == "private")
    return PKT_PVT;
  if (type == "private_ack")
    return PKT_PVT_ACK;

  return 0;
}

#pragma pack(1)
struct Header {
  unsigned int type;     // PacketType.
  unsigned int flags;    // packet flags.
  unsigned int parts;    // parts in a complete transaction.
  unsigned int current;  // current part.
  unsigned int len;      // length of packet data.
  unsigned int sequence; // sequence tracker for packets.
};

const int HeaderSize = sizeof(Header);

#pragma pack(1)
struct Packet {
  Header hdr;
  std::string data;
};

// Try to assign the data to the packet message.
// This is due to network ordering operatons (Endianness)
void AssignMessage(char *stack, Header &header, std::string &data) {
  if (header.len == 0)
    return; // sanity.

  char *start = stack; // advance the stack pointer.
  start += HeaderSize;
  unsigned int *buf = reinterpret_cast<unsigned int *>(start);

  std::vector<char> temp;

  for (int i = 0; i < header.len; ++i) {
    unsigned int info = ntohl(buf[i]);
    unsigned char ch = static_cast<unsigned char>(info);
    temp.push_back(ch);
  }
  data.assign(reinterpret_cast<char *>(&temp[0]), header.len);
}

void AssignHeader(Header &header, char *data) {
  unsigned int *out = reinterpret_cast<unsigned int *>(&header);
  unsigned int *in = reinterpret_cast<unsigned int *>(data);
  for (int i = 0; i < sizeof(header) / sizeof(int); ++i) {
    out[i] = ntohl(in[i]);
  }
}

}; // namespace comms

namespace net {

// Server thread functions.
DWORD WINAPI ServerAcceptConnections(LPVOID param);
DWORD WINAPI ServerCommsConnections(LPVOID param);

// Client thread functions.
DWORD WINAPI ClientCommsConnection(LPVOID param);

struct AutoLocker {
protected:
  CRITICAL_SECTION &m_mutex;

public:
  AutoLocker(CRITICAL_SECTION &mutex) : m_mutex(mutex) {
    EnterCriticalSection(&m_mutex);
  }
  ~AutoLocker() { LeaveCriticalSection(&m_mutex); }
};

struct SocketData {
  SOCKET socket;
  std::string ip;
  std::string alias;

  // Messages we got from the client.
  std::vector<comms::Packet> inboundMessages;

  // Messages we are sending to the client.
  std::vector<comms::Packet> outboundMessages;
};

class NetCommon {
  std::string m_ipAddress;
  std::vector<SOCKET> m_closedSockets;
  std::string m_attachmentsDir;

public:
  // Get the path to the attachments folder.
  std::string GetAttachmentsDirectory() { return m_attachmentsDir; }

  // The NetCommon constructor initializes WinSock and fetches our IP address.
  NetCommon() {

    // Get the attachments path irrespective of whether the startup succeeds.
    {
      char folder[MAX_PATH];
      DWORD nChars = GetModuleFileName(NULL, folder, MAX_PATH);

      std::string path(folder, nChars);
      std::string::size_type pos = path.find_last_of('\\');
      std::string path_prefix = path.substr(0, pos + 1);
      path_prefix.append("attachments\\");
      m_attachmentsDir = path_prefix;
    }

    WSADATA w;

    if (WSAStartup(0x0202, &w)) { // there was an error
      std::cout << "Could not initialize Winsock." << std::endl;
      throw "Could not initialize WinSock";
      return;
    }

    if (w.wVersion != 0x0202) { // wrong WinSock version!
      WSACleanup();             // unload ws2_32.dll
      std::cout << "Wrong Winsock version." << std::endl;
      throw "Incorrect WinSock version.";
      return;
    }

    std::cout << "WSAStartup" << std::endl
              << "Version: " << w.wVersion << std::endl
              << "Description: " << w.szDescription << std::endl
              << "Status: " << w.szSystemStatus << std::endl
              << "Attachments: " << m_attachmentsDir.c_str() << std::endl;

    // Get local IP's - useful for phones.
    char ac[80];
    if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR) {
      std::cout << "Error " << WSAGetLastError()
                << " when getting local host name." << std::endl;
      throw "Could not get local host name.";
      return;
    } else {
      struct hostent *phe = gethostbyname(ac);
      if (phe == 0) {
        std::cout << "Bad host lookup." << std::endl;
        throw "Bad host lookup.";
        return;
      } else {
        // Print only the first address.
        for (int i = 0; phe->h_addr_list[i] != 0; ++i) {
          struct in_addr addr;
          memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
          std::cout << "IP Address " << i << ": " << inet_ntoa(addr)
                    << std::endl;

          m_ipAddress = inet_ntoa(addr);
          break;
        }
      }
    }
  }

  ~NetCommon() {}

  void SendPacket(SOCKET s, const comms::Packet &packet) {
    // Push the entire packet header + data to the vector.
    std::vector<unsigned int> upscaledData;
    upscaledData.push_back(htonl(packet.hdr.type));
    upscaledData.push_back(htonl(packet.hdr.flags));
    upscaledData.push_back(htonl(packet.hdr.parts));
    upscaledData.push_back(htonl(packet.hdr.current));
    upscaledData.push_back(htonl(packet.hdr.len));
    upscaledData.push_back(htonl(packet.hdr.sequence));

    for (int i = 0; i < packet.hdr.len; ++i)
      upscaledData.push_back(htonl(static_cast<unsigned int>(packet.data[i])));

    int bytes = send(s, reinterpret_cast<char *>(&upscaledData[0]),
                     upscaledData.size() * sizeof(unsigned int), 0);
    if (bytes <= 0) {
      // Error occurred, we need to mark this socket as closed.
      m_closedSockets.push_back(s);
      return;
    }
  }

  // Partially Lock Free
  void HandleClosedSockets(const std::vector<SocketData> &clients,
                           std::vector<comms::Packet> &out,
                           std::vector<SOCKET> &sockets) {
    for (auto i : m_closedSockets) {
      for (int j = 0; j < clients.size(); ++j) {
        if (clients[j].socket == i) {
          std::string alias = clients[j].alias;
          alias.append(" - has taken the blue pill.");
          comms::Packet goodbye{{PKT_MSG, 0, 0, 0, alias.length(), 0}, alias};
          out.push_back(goodbye);
          break;
        }
      }
    }

    sockets = m_closedSockets;
    m_closedSockets.clear();
  }

}; // NetCommon

// The server is special, it uses a listen thread and a comms thread.
class NetServer : public NetCommon {
private:
  HANDLE acceptThread;
  HANDLE commsThread;
  SOCKET m_acceptSocket;

  CRITICAL_SECTION m_mutex;
  std::vector<SocketData> m_clients;

public:
  // Lock Free:

  // Get the socket on which we are accepting connections.
  SOCKET GetAcceptSocket() const { return m_acceptSocket; }

  // Get a reference to the server mutex.
  CRITICAL_SECTION &GetMutex() { return m_mutex; }

  // Get a reference to the server client list.
  std::vector<SocketData> &GetClients() { return m_clients; }

  // Auto Locking:

  // Push a new connection to our list of clients.
  void PushConnection(SOCKET client, const std::string &ip) {
    AutoLocker locker(m_mutex);
    m_clients.push_back(SocketData{client, ip, ""});
  }

  void DropConnection(SOCKET client) {
    AutoLocker locker(m_mutex);

    auto it = m_clients.begin();
    auto eit = m_clients.end();
    for (; it != eit; ++it) {
      if (it->socket == client) {
        std::cout << "Dropping connection from: " << it->ip.c_str();
        std::cout << "Goodbye: " << it->alias.c_str();
        m_clients.erase(it);
        break;
      }
    }
  }

  void GetConnections(std::vector<SocketData> &clients) {
    AutoLocker locker(m_mutex);
    clients = m_clients;
  }

  void SetAlias(SOCKET s, std::string &alias) {
    AutoLocker locker(m_mutex);

    for (auto &i : m_clients) {
      if (s == i.socket) {
        i.alias = alias;
        break;
      }
    }
  }

  void ProcessMessages() {
    // These get pushed to the main queue of each socket.
    std::vector<comms::Packet> globalMessages;
    std::vector<comms::Packet> privateMessages;
    AutoLocker locker(m_mutex);

    for (auto &so : m_clients) {
      // Deal with each message and push the appropriate response to the client.
      // We deal with all messages by pushing them to the correct outbound
      // queues.
      for (auto msg : so.inboundMessages) {
        switch (msg.hdr.type) {
        case PKT_ALIAS: {
          so.alias = msg.data;
          std::string data = msg.data;
          data.append(" - has taken the red pill.");
          comms::Packet packet{{PKT_MSG, 0, 0, 0, data.length(), 1}, data};
          globalMessages.push_back(packet);

          comms::Packet ack{{PKT_ALIAS_ACK, 0, 0, 0, 0, msg.hdr.sequence}, ""};
          so.outboundMessages.push_back(ack);
        } break;
        case PKT_QRY: {
          comms::Packet ack{{PKT_QRY_ACK, 0, 0, 0, 0, msg.hdr.sequence}, ""};
          so.outboundMessages.push_back(ack);
        } break;
        case PKT_MSG: {
          comms::Packet ack{{PKT_MSG_ACK, 0, 0, 0, 0, msg.hdr.sequence}, ""};
          so.outboundMessages.push_back(ack);

          globalMessages.push_back(msg);
        } break;
        case PKT_PVT: {
          comms::Packet ack{{PKT_PVT_ACK, 0, 0, 0, 0, msg.hdr.sequence}, ""};
          so.outboundMessages.push_back(ack);

          privateMessages.push_back(msg);
        } break;
        }
      }
      so.inboundMessages.clear();
    }

    // Push all the global/private messages to the correct clients.
    for (auto msg : globalMessages) {
      for (auto &so : m_clients) {
        so.outboundMessages.push_back(msg);
      }
    }
  }

  void SendMessages() {
    // For each client we send only 1 message at a time, we don't wait for an
    // ack, but we do wait for the client to be able to process the message
    // before we send the next one.
    // Slowly but surely the queue will empty out.
    AutoLocker locker(m_mutex);
    for (auto &client : m_clients) {
      auto it = client.outboundMessages.begin();
      if (it != client.outboundMessages.end()) {
        // Send the message to the client and erase it from the queue.
        SendPacket(client.socket, *it);
        client.outboundMessages.erase(it);
      }
    }
  }

  // Create the server - initialize common WinSock things.
  // Create the accept and comms threads.
  NetServer() : NetCommon() {
    // The server immediately starts listening.

    SOCKADDR_IN addr; // the address structure for a TCP socket

    addr.sin_family = AF_INET;                // Address family Internet
    addr.sin_port = htons(CHATMIUM_PORT_NR);  // Assign port to this socket
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // No destination

    m_acceptSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Create socket

    if (m_acceptSocket == INVALID_SOCKET) {
      std::cout << "Could not create socket." << std::endl;
      throw "Could not create socket";
      return;
    }

    // Not too stressed if we can't reuse the socket.
    const char reuse = 1;
    setsockopt(m_acceptSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));

    if (bind(m_acceptSocket, (LPSOCKADDR)&addr, sizeof(addr)) ==
        SOCKET_ERROR) // Try binding
    {                 // error
      std::cout << "Could not bind to IP." << std::endl;
      throw "Could not bind to IP";
      return;
    }

    listen(m_acceptSocket, 10); // Start listening
    unsigned long mode = 1;
    ioctlsocket(m_acceptSocket, FIONBIO, &mode); //  Non-blocking.

    std::cout << "Listening port " << CHATMIUM_PORT_ST << std::endl;

    InitializeCriticalSection(&m_mutex);
    // The server starts a separate set of threads. One to accept connections,
    // the other to read/write on those connections.
    DWORD acceptThreadId;
    acceptThread = CreateThread(NULL, 0, ServerAcceptConnections, this, 0,
                                &acceptThreadId);

    DWORD commsThreadId;
    commsThread =
        CreateThread(NULL, 0, ServerCommsConnections, this, 0, &commsThreadId);
  }

  ~NetServer() {
    // Should wait for the threads to stop
    // Then DeleteCriticalSection(&m_mutex);
  }
}; // NetServer

class NetClient : public NetCommon {
  HANDLE commsThread;
  std::string m_alias;
  std::string m_addy;
  SOCKET m_socket;
  bool m_connected;
  unsigned short m_sequence;

  HANDLE m_thread;
  CRITICAL_SECTION m_mutex;
  std::vector<comms::Packet> m_threadOutQueue;
  std::vector<comms::Packet> m_threadInQueue;
  std::vector<std::string> m_printQueue;

  // Lock-Free
  void PvtAddPrintQueueHelper(const std::string &data, bool &trigger) {
    if (!data.empty())
      m_printQueue.push_back(data);
    trigger = true;
  }

public:
  // Don't start up any threads.
  NetClient()
      : NetCommon(), m_connected(false), m_sequence(4),
        m_thread(INVALID_HANDLE_VALUE) {
    InitializeCriticalSection(&m_mutex);
  }

  SOCKET GetSocket() { return m_socket; }
  bool IsRunning() const { return m_connected; }
  std::vector<comms::Packet> &GetThreadOutQueue() { return m_threadOutQueue; }
  std::vector<comms::Packet> &GetThreadInQueue() { return m_threadInQueue; }
  unsigned short GetNextSequence() { return ++m_sequence; }

  void AddMessage(const std::string &text) {
    AutoLocker locker(m_mutex);
    std::string data(m_alias);
    data.append(" : ");
    data.append(text);
    comms::Packet packet{{PKT_MSG, 0, 0, 0, data.length(), GetNextSequence()},
                         data};
    m_threadOutQueue.push_back(packet);
  }

  void GetMessages(std::vector<std::string> &msg) {
    AutoLocker locker(m_mutex);
    msg.insert(msg.end(), m_printQueue.begin(), m_printQueue.end());
    m_printQueue.clear();
  }

  void SetAlias(const std::string &alias) {
    AutoLocker locker(m_mutex);

    if (m_connected) {
      m_printQueue.push_back("You cannot change your alias after connecting.");
      m_printQueue.push_back("You'll have to reconnect with a new alias.");
      return;
    }

    m_alias = alias;
    std::string message("Alias set to - ");
    message.append(alias);
    m_printQueue.push_back(message);
  }

  // Possibly disconnect here.
  ~NetClient() {}

  void Connect(const std::string &ip) {
    // Perform the whole connection dance here. Then start up the comms thread.
    // holds address info for socket to connect to
    if (m_alias.empty()) {
      AddMessage(std::string("Please set your alias for this session."));
      return;
    }
    m_addy = ip;

    struct addrinfo *result = NULL, *ptr = NULL;
    struct addrinfo hints;

    // set address info
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP; // TCP connection!!!

    int iResult =
        ::getaddrinfo(m_addy.c_str(), CHATMIUM_PORT_ST, &hints, &result);
    if (iResult == 0) {
      // Attempt to connect to an address until one succeeds
      for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to server
        m_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if (m_socket != INVALID_SOCKET) {
          // Connect to server.
          iResult = connect(m_socket, ptr->ai_addr, (int)ptr->ai_addrlen);

          if (iResult != SOCKET_ERROR)
            break;
        }
      }

      // no longer need address info for server
      ::freeaddrinfo(result);

      // if connection failed
      if (m_socket == INVALID_SOCKET) {
        std::cout << "Unable to connect to server!" << std::endl;
        m_printQueue.push_back("Unable to connect to the server.");
      } else {

        // If iMode!=0, non-blocking mode is enabled.
        u_long iMode = 1;
        ioctlsocket(m_socket, FIONBIO, &iMode);

        m_connected = true;

        // Create the connect message with our alias.
        comms::Packet aliasMsg{{PKT_ALIAS, 0, 0, 0, m_alias.length(), 4},
                               m_alias};
        m_threadOutQueue.push_back(aliasMsg);

        if (m_thread == INVALID_HANDLE_VALUE)
          m_thread = CreateThread(NULL, 0, ClientCommsConnection, (LPVOID) this,
                                  0, NULL);
      }
    }
  }

  void RemoveOutboundPacket(char msgtype) {
    // Remove the item from the out queue.
    auto out_it = m_threadOutQueue.begin();
    auto out_eit = m_threadOutQueue.end();
    for (; out_it != out_eit; ++out_it) {
      if (out_it->hdr.type == msgtype) {
        m_threadOutQueue.erase(out_it);
        break;
      }
    }
  }

  void ProcessQueues() {
    // We run through the out queue and see if we got an ack in the in queue.
    // If we got an ack, then we can erase the message from our in queue.
    // Else we wait for a second cycle and send the message again.
    AutoLocker locker(m_mutex);
    // The first thing we send to the server is an ALIAS packet.
    auto in_it = m_threadInQueue.begin();
    auto in_eit = m_threadInQueue.end();
    for (; in_it != in_eit; ++in_it) {
      // Go through the in queue ... basically if it's an ack, then we go
      // search for the sequence number in the out queue and remove it.
      bool erasePacket = false;
      if (in_it->hdr.type == PKT_ALIAS_ACK) {
        std::cout << "Got alias Ack." << std::endl;
        RemoveOutboundPacket(PKT_ALIAS);
        PvtAddPrintQueueHelper(in_it->data, erasePacket);
      } else if (in_it->hdr.type == PKT_QRY_ACK) {
        std::cout << "Got query Ack." << std::endl;
        RemoveOutboundPacket(PKT_QRY);
        PvtAddPrintQueueHelper(in_it->data, erasePacket);
      } else if (in_it->hdr.type == PKT_MSG_ACK) {
        std::cout << "Got message Ack." << std::endl;
        RemoveOutboundPacket(PKT_MSG);
        PvtAddPrintQueueHelper(in_it->data, erasePacket);
      } else if (in_it->hdr.type == PKT_PVT_ACK) {
        std::cout << "Got private Ack." << std::endl;
        RemoveOutboundPacket(PKT_PVT);
        PvtAddPrintQueueHelper(in_it->data, erasePacket);
      } else if (in_it->hdr.type == PKT_MSG) {
        std::cout << "Got general message." << std::endl;
        RemoveOutboundPacket(PKT_QRY);
        PvtAddPrintQueueHelper(in_it->data, erasePacket);
      }

      // We might miss one packet like this, but it's cool.
      if (erasePacket) {
        m_threadInQueue.erase(in_it);
        in_it = m_threadInQueue.begin();
        in_eit = m_threadInQueue.end();

        if (in_it == in_eit)
          break;
      }
    }

    auto out_it = m_threadOutQueue.begin();
    auto out_eit = m_threadOutQueue.end();
    if (out_it != out_eit) {
      // Send only the 1 message to the server. Then we wait for the ack.
      SendPacket(m_socket, *out_it);
      // We don't immediately erase the message since we are waiting
      // for an ack.
    }
  }
}; // NetClient

// Server thread functions.
DWORD WINAPI ServerAcceptConnections(LPVOID param) {
  net::NetServer *server = reinterpret_cast<net::NetServer *>(param);

  // Great, we have the netserver.
  bool running = true;
  while (running) {
    SOCKADDR_IN from;
    int fromlen = sizeof(SOCKADDR_IN);

    SOCKET clientSocket =
        accept(server->GetAcceptSocket(), (struct sockaddr *)&from, &fromlen);

    if (clientSocket != INVALID_SOCKET) {
      // Push the socket to the list of clients.
      char *ip{inet_ntoa(from.sin_addr)};
      size_t len{strlen(ip)};
      std::string ip_addy(ip, len);
      std::cout << "Connection from : " << ip_addy.c_str() << std::endl;

      unsigned long mode = 1;
      ioctlsocket(clientSocket, FIONBIO, &mode); //  Non-blocking.
      server->PushConnection(clientSocket, ip_addy);
    }

    // Try to accept a new connection every 10 ms.
    Sleep(10);
  }
  return 0;
}

DWORD WINAPI ServerCommsConnections(LPVOID param) {
  net::NetServer *server = reinterpret_cast<net::NetServer *>(param);

  bool running = true;
  char stack[2048];

  while (running) {
    {
      net::AutoLocker lock(server->GetMutex());
      std::vector<net::SocketData> &clients{server->GetClients()};
      for (auto &i : clients) {
        int bytesRead = recv(i.socket, stack, 2048, 0);
        if (bytesRead > comms::HeaderSize) {
          comms::Header hdr;
          AssignHeader(hdr, stack);
          if (bytesRead >= comms::HeaderSize + (hdr.len * sizeof(int))) {
            comms::Packet packet{hdr, ""};
            AssignMessage(stack, hdr, packet.data);
            i.inboundMessages.push_back(packet);
          }
        }
      }
    }

    server->ProcessMessages();
    server->SendMessages();

    Sleep(1);
  }

  return true;
}

// Client thread functions.
DWORD WINAPI ClientCommsConnection(LPVOID param) {
  net::NetClient *client{reinterpret_cast<net::NetClient *>(param)};
  char stack[2048];
  std::vector<char> packetData;

  while (client->IsRunning()) {
    std::vector<comms::Packet> &in_queue{client->GetThreadInQueue()};
    std::vector<comms::Packet> &out_queue{client->GetThreadOutQueue()};

    int bytesRead{0};
    do {
      bytesRead = recv(client->GetSocket(), stack, 2048, 0);
      if (bytesRead > 0) {
        const char *start = stack;
        const char *end = stack + bytesRead;
        packetData.insert(packetData.end(), start, end);
      }
    } while (bytesRead > 0);

    // Now we have an entire list of packets.
    int packetDataSize = packetData.size();
    while (packetDataSize >= comms::HeaderSize) {
      comms::Header hdr;
      AssignHeader(hdr, &packetData[0]);

      if (packetDataSize >= comms::HeaderSize + (hdr.len * sizeof(int))) {
        // We have a valid packet/s.
        comms::Packet pkt{hdr, ""};
        AssignMessage(reinterpret_cast<char *>(&packetData[0]), hdr, pkt.data);

        // std::cout << "Got Packet: " << pkt.data.c_str() << std::endl;
        in_queue.push_back(pkt);

        // Now that we have read a packet from the stream, we remove the chunk
        // we just read.
        int dataProcessed = (comms::HeaderSize + hdr.len * sizeof(int));
        packetData.erase(packetData.begin(),
                         packetData.begin() + dataProcessed);
        packetDataSize = packetData.size();
      }
    }

    client->ProcessQueues();

    Sleep(100);
  }

  return 0;
}

}; // namespace net

#endif // _CHAT_COMMON_HPP_