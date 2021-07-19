#ifndef _CHAT_COMMON_HPP_
#define _CHAT_COMMON_HPP_
#pragma once

#include <ws2tcpip.h>
#include <WinSock2.h>
#include <iostream>
#include <string>
#include <vector>

#include "print_structs.hpp"

//#define DEBUG_MODE 1
//#define USE_FLATE 1

#ifdef USE_FLATE
#include "zlib.h"
#pragma comment(lib, "../zlibstatic.lib")
#endif

#if 0
So, the client now responds to the server ack messages.
Great.

The servers ProcessMessages function should now change to be able to push
outbound messages into a special queue for each client.

There should also be a global queue for clients.

// App client id.
//558136167287-f1lesdcasa71hikh107b6dq7qto7nui8.apps.googleusercontent.com
#endif // 0

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

namespace comms {

	static const int SALT = 0x16540000;
	#define CHATMIUM_PORT_ST "54547"
	#define CHATMIUM_PORT_NR 54547

	enum PKT {
		ALIAS = 0x00002,
		ALIAS_ACK = 0x00003,
		QRY = 0x00004,
		QRY_ACK = 0x00005,
		MSG = 0x00006,
		MSG_ACK = 0x00007,
		PVT = 0x00008,
		PVT_ACK = 0x00009,
		LST = 0x00010,
		LST_ACK = 0x00011,
		FILE_OUT = 0x00012,
		FILE_OUT_ACK = 0x00013,
		FILE_IN = 0x00014,
		FILE_IN_ACK = 0x00015,
		MSG_JOIN = 0x00016,
		MSG_JOIN_ACK = 0x00017,
		MSG_LEAVE = 0x00018,
		MSG_LEAVE_ACK = 0x00019,
	};

	std::string CharToMessageType(unsigned short msgtype);

	unsigned short MessageTypeToChar(const std::string &type);

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
	// Based on what your TCP stack can handle.
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
	void AssignMessage(char *stack, Header &header, std::string &data);

	void AssignHeader(Header &header, char *data);

	bool ReadSocketFully(SOCKET s, char *stack, std::vector<char> &data);

	void QueueCompletePackets(std::vector<char> &data, packetQueue &out);

}; // namespace comms

#ifdef USE_FLATE
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
    // if (nErr == Z_STREAM_END)
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
#endif

namespace net {

// Client thread functions.
DWORD WINAPI ClientCommsConnection(LPVOID param);

struct AutoLocker {
protected:
  CRITICAL_SECTION &m_mutex;

public:
	AutoLocker(CRITICAL_SECTION &mutex);
	~AutoLocker();
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
	const std::string &GetAttachmentsDirectory() const;

	void OpenAttachmentsFile(const std::string &name, unsigned int parse,
		comms::OpenFileData &data);

  // The NetCommon constructor initializes WinSock and fetches our IP address.
	NetCommon();

	~NetCommon();

	void SendPacket(SOCKET s, const comms::Packet &packet);

  // Partially Lock Free
	void HandleClosedSockets(std::vector<SocketData> &clients);

}; // NetCommon

// The server is special, it uses a listen thread and a comms thread.


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
	  bool compressed);

  // Partially locked
  void transferFileInternal(const std::string &name, const std::string &path,
	  const std::string &user);

  // Lock-Free
  void HandleFile(comms::Packet &packet);

  // Lock-free
  void HandlePacketLockStepSend(comms::packetQueue::iterator &it,
	  comms::packetQueue::iterator &eit);

public:
  // Don't start up any threads.
	NetClient();

  SOCKET GetSocket() { return m_socket; }
  bool IsRunning() const { return m_connected; }
  comms::packetQueue &GetThreadOutQueue() { return m_threadOutQueue; }
  comms::packetQueue &GetThreadInQueue() { return m_threadInQueue; }
  unsigned short GetNextSequence() { return ++m_sequence; }

  void AddMessage(const std::string &text);

  void GetMessages(print::printQueue &msg);

  void SetAlias(const std::string &alias);

  void GetUserList();

  void SendFile(const std::string &name, const std::string &path);
  void SendFileUser(const std::string &name, const std::string &path,
	  const std::string &user);

  // Possibly disconnect here.
  ~NetClient();

  void Connect(const std::string &ip);

  void RemoveOutboundPacket(comms::packetQueue &queue, char msgtype,
	  unsigned int sequence = 4294967290);

  void ProcessQueues();
}; // NetClient

// Client thread functions.
DWORD WINAPI ClientCommsConnection(LPVOID param);

}; // namespace net

#endif // _CHAT_COMMON_HPP_