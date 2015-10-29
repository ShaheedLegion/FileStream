#ifndef _CHAT_COMMON_HPP_
#define _CHAT_COMMON_HPP_
#pragma once

#include <ws2tcpip.h>
#include <WinSock2.h>
#include <iostream>
#include <string>
#include <vector>

#include "print_structs.hpp"

#include "zlib.h"
#pragma comment(lib, "../zlibstatic.lib")

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

#define SALT 0x16540000

#define CHATMIUM_PORT_ST "54547"
#define CHATMIUM_PORT_NR 54547
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

// The client requests the user list from the server.
#define PKT_LST 0x00010
#define PKT_LST_ACK 0x00011

// Client sending file to the server.
#define PKT_FILE_OUT 0x00012
#define PKT_FILE_OUT_ACK 0x00013

// Server sending file to the client.
#define PKT_FILE_IN 0x00014
#define PKT_FILE_IN_ACK 0x00015

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
  case PKT_LST:
    return "list";
  case PKT_LST_ACK:
    return "list_ack";
  case PKT_FILE_OUT:
    return "file_out";
  case PKT_FILE_OUT_ACK:
    return "file_out_ack";
  case PKT_FILE_IN:
    return "file_in";
  case PKT_FILE_IN_ACK:
    return "file_in_ack";
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
  if (type == "list")
    return PKT_LST;
  if (type == "list_ack")
    return PKT_LST_ACK;
  if (type == "file_out")
    return PKT_FILE_OUT;
  if (type == "file_out_ack")
    return PKT_FILE_OUT_ACK;
  if (type == "file_in")
    return PKT_FILE_IN;
  if (type == "file_in_ack")
    return PKT_FILE_IN_ACK;

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
  unsigned int id;       // similar to flags, but used for files.
};

const int HeaderSize = sizeof(Header);

// Larger values make file transfers more stable.
const int TransferSize = 18366;

#pragma pack(1)
struct Packet {
  Header hdr;
  std::string data;
};

struct OpenFileData {
  unsigned int id;
  FILE *file;
  std::string path;
  std::string displayName;
};

struct PacketInfo {
  Packet packet;
  bool sent;
  int skips;
};

typedef std::vector<PacketInfo> packetQueue;

// Try to assign the data to the packet message.
// This is due to network ordering operatons (Endianness)
void AssignMessage(char *stack, Header &header, std::string &data) {
  if (header.len == 0)
    return; // sanity.

  char *start = stack; // advance the stack pointer.
  start += HeaderSize;
  unsigned int *buf = reinterpret_cast<unsigned int *>(start);

  std::vector<char> temp;

  for (unsigned int i = 0; i < header.len; ++i) {
    unsigned int info = ntohl(buf[i]);
    unsigned int infoShift = (info - SALT);
    unsigned char ch = static_cast<unsigned char>(infoShift);
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

void ReadSocketFully(SOCKET s, char *stack, std::vector<char> &data) {
  // Read fully from the client.
  int bytesRead{0};
  do {
    bytesRead = recv(s, stack, TransferSize, 0);
    if (bytesRead > 0) {
      const char *start = stack;
      const char *end = stack + bytesRead;
      data.insert(data.end(), start, end);
    }
  } while (bytesRead > 0);
}

void QueueCompletePackets(std::vector<char> &data, packetQueue &out) {
  // Now we have an entire list of packets.
  int packetDataSize = data.size();
  while (packetDataSize >= HeaderSize) {
    Header hdr;
    AssignHeader(hdr, &data[0]);
    std::cout << "Got packet - type[" << hdr.type << "] len[" << hdr.len
              << "] seq[" << hdr.sequence << "]" << std::endl;
    if (packetDataSize >=
        static_cast<int>(HeaderSize + (hdr.len * sizeof(int)))) {
      // We have a valid packet/s.
      PacketInfo info{{hdr, ""}, false, 0};
      AssignMessage(reinterpret_cast<char *>(&data[0]), hdr, info.packet.data);

      out.push_back(info);

      // Now that we have read a packet from the stream, we remove the
      // chunk we just read.
      int dataProcessed = (HeaderSize + hdr.len * sizeof(int));
      data.erase(data.begin(), data.begin() + dataProcessed);
      packetDataSize = data.size();
      std::cout << "Processed data [" << packetDataSize << "]" << std::endl;
    } else {
      // Need to keep this, it's a partial packet.
      std::cout << "Had partial packet" << std::endl;
      break; // Could not process this.
    }
  }
}

}; // namespace comms

namespace flate {
struct FlateResult {
  int inDataSize;
  int outDataSize;
  Bytef *inData;
  Bytef *outData;

  FlateResult(Bytef *in, int inSize, int outSize)
      : inData(in), inDataSize(inSize), outDataSize(outSize) {
    outData = new Bytef[outDataSize];
  }

  FlateResult(const char *in, int inSize, int outSize)
      : inData(reinterpret_cast<Bytef *>(const_cast<char *>(in))),
        inDataSize(inSize), outDataSize(outSize) {
    outData = new Bytef[outDataSize];
  }

  ~FlateResult() { delete[] outData; }
};

void InflateData(FlateResult &result) {
  z_stream zInfo = {Z_NULL};
  zInfo.total_in = zInfo.avail_in = result.inDataSize;
  zInfo.total_out = zInfo.avail_out = result.outDataSize;
  zInfo.next_in = result.inData;
  zInfo.next_out = result.outData;

  int nErr = inflateInit2(&zInfo, MAX_WBITS | 32);
  if (nErr == Z_OK) {
    nErr = inflate(&zInfo, Z_FINISH);
    if (nErr == Z_STREAM_END)
      result.outDataSize = zInfo.total_out;
  }
  inflateEnd(&zInfo);
}

void DeflateData(FlateResult &result) {
  z_stream zInfo = {Z_NULL};
  zInfo.total_in = zInfo.avail_in = result.inDataSize;
  zInfo.total_out = zInfo.avail_out = result.outDataSize;
  zInfo.next_in = result.inData;
  zInfo.next_out = result.outData;

  int nErr = deflateInit2(&zInfo, Z_BEST_SPEED, Z_DEFLATED, MAX_WBITS | 16, 8,
                          Z_DEFAULT_STRATEGY);
  if (nErr == Z_OK) {
    nErr = deflate(&zInfo, Z_FINISH);
    if (nErr == Z_STREAM_END)
      result.outDataSize = zInfo.total_out;
  }
  deflateEnd(&zInfo);
}

}; // namespace flate

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
  comms::packetQueue inboundMessages;

  // Messages we are sending to the client.
  comms::packetQueue outboundMessages;

  // Data coming in on the socket.
  std::vector<char> packetData;
};

class NetCommon {
  std::string m_ipAddress;
  std::vector<SOCKET> m_closedSockets;
  std::string m_attachmentsDir;

public:
  // Get the path to the attachments folder.
  const std::string &GetAttachmentsDirectory() const {
    return m_attachmentsDir;
  }

  void OpenAttachmentsFile(const std::string &name, unsigned int parse,
                           comms::OpenFileData &data) {
    std::string fullpath(GetAttachmentsDirectory());
    if (parse) {
      // Get the username out of the string.
      std::string::size_type pos = name.find_last_of('|');
      if (pos != std::string::npos) {
        fullpath.append(name.substr(0, pos));
        data.displayName = name.substr(0, pos);
      }
    } else {
      fullpath.append(name);
      data.displayName = name;
    }

    data.file = fopen(fullpath.c_str(), "wb");
    data.path = fullpath;
  }

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
    upscaledData.push_back(htonl(packet.hdr.id));

    for (unsigned int i = 0; i < packet.hdr.len; ++i)
      upscaledData.push_back(
          htonl(static_cast<unsigned int>(packet.data[i]) + SALT));

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
      for (unsigned int j = 0; j < clients.size(); ++j) {
        if (clients[j].socket == i) {
          std::string alias = clients[j].alias;
          alias.append(" - has taken the blue pill.");
          comms::Packet goodbye{{PKT_MSG, 0, 0, 0, alias.length(), 0, 0},
                                alias};
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
    comms::packetQueue globalMessages;
    comms::packetQueue privateMessages;
    AutoLocker locker(m_mutex);

    for (auto &so : m_clients) {
      // Deal with each message and push the appropriate response to the client.
      // We deal with all messages by pushing them to the correct outbound
      // queues.
      for (auto msg : so.inboundMessages) {
        switch (msg.packet.hdr.type) {
        case PKT_ALIAS: {
          so.alias = msg.packet.data;
          std::string data = msg.packet.data;
          data.append(" - has taken the red pill.");

          // Compress the response.
          flate::FlateResult compressed(data.c_str(), data.length(),
                                        data.length() * 2);
          flate::DeflateData(compressed);

          std::string cData;
          cData.assign(reinterpret_cast<char *>(compressed.outData),
                       compressed.outDataSize);

          comms::PacketInfo info{
              {{PKT_MSG, 0, 0, 0, cData.length(), 1, 0}, cData}, false, 0};
          // Store the messages for global delivery.
          globalMessages.push_back(info);

          // Immediately ack.
          comms::Packet ack{
              {PKT_ALIAS_ACK, 0, 0, 0, 0, msg.packet.hdr.sequence, 0}, ""};
          SendPacket(so.socket, ack);
        } break;
        case PKT_QRY: {
          // Immediately ack.
          comms::Packet ack{
              {PKT_QRY_ACK, 0, 0, 0, 0, msg.packet.hdr.sequence, 0}, ""};
          SendPacket(so.socket, ack);
        } break;
        case PKT_MSG: {
          // Store the message for global delivery.
          globalMessages.push_back(msg);
          // Immediately ack.
          comms::Packet ack{
              {PKT_MSG_ACK, 0, 0, 0, 0, msg.packet.hdr.sequence, 0}, ""};
          SendPacket(so.socket, ack);

        } break;
        case PKT_PVT: {
          // Immediately ack.
          comms::Packet ack{
              {PKT_PVT_ACK, 0, 0, 0, 0, msg.packet.hdr.sequence, 0}, ""};
          SendPacket(so.socket, ack);

          // Store the message for private delivery.
          privateMessages.push_back(msg);
        } break;
        case PKT_LST: {
          std::string user_list("Users on this server:|");
          {
            auto lst_it = m_clients.begin();
            auto lst_eit = m_clients.end();
            for (; lst_it != lst_eit; ++lst_it) {
              user_list.append(lst_it->ip);
              user_list.append(" : ");
              user_list.append(lst_it->alias);
              user_list.append("|");
            }
            comms::Packet ack{{PKT_LST_ACK, 0, 0, 0, user_list.length(),
                               msg.packet.hdr.sequence, 0},
                              user_list};
            // Immediately ack.
            SendPacket(so.socket, ack);
          }
        } break;
        case PKT_FILE_OUT: {
          comms::Packet ack{
              {PKT_FILE_OUT_ACK, 0, 0, 0, 0, msg.packet.hdr.sequence, 0}, ""};
          SendPacket(so.socket, ack);
          std::cout << "Sending back file in ack[" << msg.packet.hdr.sequence
                    << "]" << std::endl;

          // This is one of the only messages which are mutated before being
          // sent back.
          comms::PacketInfo info{msg.packet, false, 0};
          info.packet.hdr.type = PKT_FILE_IN;
          globalMessages.push_back(info);
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
        SendPacket(client.socket, it->packet);
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
  comms::packetQueue m_threadOutQueue;
  comms::packetQueue m_threadInQueue;
  print::printQueue m_printQueue;

  std::vector<comms::OpenFileData> openFiles;
  comms::packetQueue m_threadFileOutQueue;

  // Lock-Free
  void PvtAddPrintQueueHelper(const std::string &data, bool &trigger,
                              bool compressed) {
    if (!data.empty()) {
      if (!compressed) {
        m_printQueue.push_back(print::PrintInfo(data, "", false));
      } else {
        // Decompress the data.
        std::string pData{data};
        flate::FlateResult decompressed(pData.c_str(), pData.length(),
                                        pData.length() * 2);
        InflateData(decompressed);
        std::string iData;
        iData.assign(reinterpret_cast<char *>(decompressed.outData),
                     decompressed.outDataSize);
        m_printQueue.push_back(print::PrintInfo(iData, "", false));
      }
    }
    trigger = true;
  }

  // Partially locked
  void transferFileInternal(const std::string &name, const std::string &path,
                            const std::string &user) {
    FILE *file{fopen(path.c_str(), "rb")};
    // We simply block until we can allocate the entire file into our buffer.
    if (!file) {
      AutoLocker locker(m_mutex);
      m_printQueue.push_back(
          print::PrintInfo("Could not open the file", "", false));
      return;
    }

    comms::packetQueue lFilePieces;
    comms::PacketInfo initial{{{PKT_FILE_OUT, 0, 0, 0, 0, GetNextSequence(),
                                reinterpret_cast<int>(&file)},
                               ""},
                              false,
                              0};

    std::string name_user{name};
    if (!user.empty()) {
      name_user.append("|");
      name_user.append(user);
      initial.packet.hdr.flags = 1; // set flags to 1 to indicate it's special.
    }

    initial.packet.data = name_user;
    initial.packet.hdr.len = name_user.length();

    // Loop and read the file in discrete chunks.
    int fileSize{0};
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Discover how many chunks we will send.
    int chunks = static_cast<int>(
        (static_cast<float>(fileSize) /
         static_cast<float>(comms::TransferSize - comms::HeaderSize)) +
        1);
    initial.packet.hdr.parts = chunks;
    lFilePieces.push_back(initial);

    char buf[comms::TransferSize - comms::HeaderSize];
    // Loop and push the read pieces into our local vector.
    for (int i = 0; i < chunks; ++i) {
      int bytesRead =
          fread(buf, 1, comms::TransferSize - comms::HeaderSize, file);

      comms::PacketInfo next{{{PKT_FILE_OUT, 0, chunks, i + 1, bytesRead,
                               GetNextSequence(), reinterpret_cast<int>(&file)},
                              ""},
                             false,
                             0};
      next.packet.data.assign(buf, bytesRead);
      lFilePieces.push_back(next);

      std::cout << "Stacking file chunk for sending [" << i + 1 << "]["
                << chunks << "]" << std::endl;
    }

    AutoLocker locker(m_mutex);
    m_threadFileOutQueue.insert(m_threadFileOutQueue.end(), lFilePieces.begin(),
                                lFilePieces.end());
    fclose(file);
  }

  // Lock-Free
  void HandleFile(comms::Packet &packet) {
    if (packet.hdr.current == 0) {
      comms::OpenFileData data;
      data.id = packet.hdr.id;
      OpenAttachmentsFile(packet.data, packet.hdr.flags, data);
      openFiles.push_back(data);
      return;
    }

    bool displayFile = false;
    std::string displayName;
    std::string path;
    // Next we simply write the data out if we have an open file.
    auto it = openFiles.begin();
    auto eit = openFiles.end();
    for (; it != eit; ++it) {
      if (it->id == packet.hdr.id) {
        // Then we can write some data to the file.
        fwrite(packet.data.data(), 1, packet.data.length(), it->file);
        std::cout << "Writing file data: " << packet.data.length() << std::endl;
        if (packet.hdr.current == packet.hdr.parts) {
          fclose(it->file);
          displayFile = true;
          displayName = it->displayName;
          path = it->path;
          openFiles.erase(it);
        }
        break;
      }
    }
    if (displayFile) {
      // Push the file to some kind of list for the user to see.
      std::string msg = "Got file: ";
      msg.append(displayName);

      print::PrintInfo info(msg, path, true);
      m_printQueue.push_back(info);
    }
  }

  // Lock-free
  void HandlePacketLockStepSend(comms::packetQueue::iterator &it,
                                comms::packetQueue::iterator &eit) {
    if (it != eit) {
      // Send only the 1 message to the server. Then we wait for the ack.
      // The way we wait for an ack is by specifying that the message was sent.
      // Along with a 'timeout' value, this allows us to essentially remain in
      // lock-step with the server.
      if (it->sent && --it->skips < 0) {
        it->skips = 500;
        SendPacket(m_socket, it->packet);
      } else if (!it->sent) {
        it->sent = true;
        it->skips = 500;
        SendPacket(m_socket, it->packet);
      }
      // We don't immediately erase the message since we are waiting
      // for an ack.
    }
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
  comms::packetQueue &GetThreadOutQueue() { return m_threadOutQueue; }
  comms::packetQueue &GetThreadInQueue() { return m_threadInQueue; }
  unsigned short GetNextSequence() { return ++m_sequence; }

  void AddMessage(const std::string &text) {
    AutoLocker locker(m_mutex);
    std::string data(m_alias);
    data.append(" : ");
    data.append(text);

    // Compress the data.
    flate::FlateResult compressed(data.c_str(), data.length(),
                                  data.length() * 2);
    flate::DeflateData(compressed);

    std::string cData;
    cData.assign(reinterpret_cast<char *>(compressed.outData),
                 compressed.outDataSize);

    comms::PacketInfo info{
        {{PKT_MSG, 0, 0, 0, cData.length(), GetNextSequence(), 0}, cData},
        false,
        0};
    m_threadOutQueue.push_back(info);
  }

  void GetMessages(print::printQueue &msg) {
    AutoLocker locker(m_mutex);
    msg.insert(msg.end(), m_printQueue.begin(), m_printQueue.end());
    m_printQueue.clear();
  }

  void SetAlias(const std::string &alias) {
    AutoLocker locker(m_mutex);

    if (m_connected) {
      m_printQueue.push_back(print::PrintInfo(
          "You cannot change your alias after connecting.", "", false));
      m_printQueue.push_back(print::PrintInfo(
          "You'll have to reconnect with a new alias.", "", false));
      return;
    }

    m_alias = alias;
    std::string message("Alias set to - ");
    message.append(alias);
    m_printQueue.push_back(print::PrintInfo(message, "", false));
  }

  void GetUserList() {
    AutoLocker locker(m_mutex);
    if (!m_connected) {
      m_printQueue.push_back(
          print::PrintInfo("You need to connect first.", "", false));
      return;
    }

    m_printQueue.push_back(
        print::PrintInfo("Fetching list from server....", "", false));
    comms::PacketInfo info{
        {{PKT_LST, 0, 0, 0, 0, GetNextSequence(), 0}, ""}, false, 0};
    m_threadOutQueue.push_back(info);
  }

  void SendFile(const std::string &name, const std::string &path) {
    std::string user = "";
    transferFileInternal(name, path, user);
  }
  void SendFileUser(const std::string &name, const std::string &path,
                    const std::string &user) {
    transferFileInternal(name, path, user);
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
      if (m_socket == INVALID_SOCKET || iResult == SOCKET_ERROR) {
        std::cout << "Unable to connect to server!" << std::endl;
        m_printQueue.push_back(
            print::PrintInfo("Unable to connect to the server.", "", false));
      } else {

        // If iMode!=0, non-blocking mode is enabled.
        u_long iMode = 1;
        ioctlsocket(m_socket, FIONBIO, &iMode);

        m_connected = true;

        // Create the connect message with our alias.
        comms::PacketInfo info{
            {{PKT_ALIAS, 0, 0, 0, m_alias.length(), 4, 0}, m_alias}, false, 0};
        m_threadOutQueue.push_back(info);

        if (m_thread == INVALID_HANDLE_VALUE)
          m_thread = CreateThread(NULL, 0, ClientCommsConnection, (LPVOID) this,
                                  0, NULL);
      }
    }
  }

  void RemoveOutboundPacket(comms::packetQueue &queue, char msgtype,
                            unsigned int sequence = 4294967290) {
    // Remove the item from the out queue.
    auto out_it = queue.begin();
    auto out_eit = queue.end();

    if (sequence != 4294967290) {
      // unsigned int reformedSequence = htonl(sequence);
      for (; out_it != out_eit; ++out_it) {
        if (out_it->packet.hdr.sequence == sequence) {
          queue.erase(out_it);
          break;
        }
      }
      return;
    }

    for (; out_it != out_eit; ++out_it) {
      if (out_it->packet.hdr.type == msgtype) {
        queue.erase(out_it);
        break;
      }
    }
  }

  void ProcessQueues() {
    AutoLocker locker(m_mutex);
    auto in_it = m_threadInQueue.begin();
    auto in_eit = m_threadInQueue.end();

    for (; in_it != in_eit; ++in_it) {
      bool erasePacket = false;
      if (in_it->packet.hdr.type == PKT_ALIAS_ACK) {
        std::cout << "Got alias Ack." << std::endl;
        RemoveOutboundPacket(m_threadOutQueue, PKT_ALIAS);
        PvtAddPrintQueueHelper(in_it->packet.data, erasePacket, false);
      } else if (in_it->packet.hdr.type == PKT_QRY_ACK) {
        std::cout << "Got query Ack." << std::endl;
        RemoveOutboundPacket(m_threadOutQueue, PKT_QRY);
        PvtAddPrintQueueHelper(in_it->packet.data, erasePacket, false);
      } else if (in_it->packet.hdr.type == PKT_MSG_ACK) {
        std::cout << "Got message Ack." << std::endl;
        RemoveOutboundPacket(m_threadOutQueue, PKT_MSG,
                             in_it->packet.hdr.sequence);
        PvtAddPrintQueueHelper(in_it->packet.data, erasePacket, false);
      } else if (in_it->packet.hdr.type == PKT_PVT_ACK) {
        std::cout << "Got private Ack." << std::endl;
        RemoveOutboundPacket(m_threadOutQueue, PKT_PVT);
        PvtAddPrintQueueHelper(in_it->packet.data, erasePacket, false);
      } else if (in_it->packet.hdr.type == PKT_MSG) {
        std::cout << "Got general message." << std::endl;
        RemoveOutboundPacket(m_threadOutQueue, PKT_QRY);
        PvtAddPrintQueueHelper(in_it->packet.data, erasePacket, true);
      } else if (in_it->packet.hdr.type == PKT_LST_ACK) {
        std::cout << "Got list users Ack." << std::endl;
        RemoveOutboundPacket(m_threadOutQueue, PKT_LST);
        PvtAddPrintQueueHelper(in_it->packet.data, erasePacket, false);
      } else if (in_it->packet.hdr.type == PKT_FILE_OUT_ACK) {
        std::cout << "Got file_out Ack." << std::endl;
        RemoveOutboundPacket(m_threadFileOutQueue, PKT_FILE_OUT,
                             in_it->packet.hdr.sequence);
        erasePacket = true;
      } else if (in_it->packet.hdr.type == PKT_FILE_IN) {
        std::cout << "Got file_in packet." << std::endl;
        HandleFile(in_it->packet);
        erasePacket = true;
      }

      if (erasePacket) {
        m_threadInQueue.erase(in_it);
        in_it = m_threadInQueue.begin();
        in_eit = m_threadInQueue.end();

        if (in_it == in_eit)
          break;
      }
    }

    HandlePacketLockStepSend(m_threadOutQueue.begin(), m_threadOutQueue.end());
    HandlePacketLockStepSend(m_threadFileOutQueue.begin(),
                             m_threadFileOutQueue.end());
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
  char stack[comms::TransferSize];

  while (running) {
    {
      net::AutoLocker lock(server->GetMutex());
      std::vector<net::SocketData> &clients{server->GetClients()};
      for (auto &i : clients) {
        comms::ReadSocketFully(i.socket, stack, i.packetData);
        comms::QueueCompletePackets(i.packetData, i.inboundMessages);
      }
    }

    server->ProcessMessages();
    server->SendMessages();

    Sleep(10);
  }

  return true;
}

// Client thread functions.
DWORD WINAPI ClientCommsConnection(LPVOID param) {
  net::NetClient *client{reinterpret_cast<net::NetClient *>(param)};
  char stack[comms::TransferSize];
  std::vector<char> packetData;

  while (client->IsRunning()) {
    comms::ReadSocketFully(client->GetSocket(), stack, packetData);
    comms::QueueCompletePackets(packetData, client->GetThreadInQueue());

    client->ProcessQueues();

    Sleep(10);
  }

  return 0;
}

}; // namespace net

#endif // _CHAT_COMMON_HPP_