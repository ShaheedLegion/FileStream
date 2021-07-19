#include "NetCommon.hpp"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")


std::string comms::CharToMessageType(unsigned short msgtype) {
	switch (msgtype) {
	case PKT::ALIAS:
	return "alias";
	case PKT::ALIAS_ACK:
	return "alias_ack";
	case PKT::QRY:
	return "query";
	case PKT::QRY_ACK:
	return "query_ack";
	case PKT::MSG:
	return "message";
	case PKT::MSG_ACK:
	return "message_ack";
	case PKT::PVT:
	return "private";
	case PKT::PVT_ACK:
	return "private_ack";
	case PKT::LST:
	return "list";
	case PKT::LST_ACK:
	return "list_ack";
	case PKT::FILE_OUT:
	return "file_out";
	case PKT::FILE_OUT_ACK:
	return "file_out_ack";
	case PKT::FILE_IN:
	return "file_in";
	case PKT::FILE_IN_ACK:
	return "file_in_ack";
	case PKT::MSG_JOIN:
	return "msg_join";
	case PKT::MSG_JOIN_ACK:
	return "msg_join_ack";
	case PKT::MSG_LEAVE:
	return "msg_leave";
	case PKT::MSG_LEAVE_ACK:
	return "msg_leave_ack";
	}
	return "unk";
}

unsigned short comms::MessageTypeToChar(const std::string &type) {
	if (type == "alias")
	return PKT::ALIAS;
	if (type == "alias_ack")
	return PKT::ALIAS_ACK;
	if (type == "query")
	return PKT::QRY;
	if (type == "query_ack")
	return PKT::QRY_ACK;
	if (type == "message")
	return PKT::MSG;
	if (type == "message_ack")
	return PKT::MSG_ACK;
	if (type == "private")
	return PKT::PVT;
	if (type == "private_ack")
	return PKT::PVT_ACK;
	if (type == "list")
	return PKT::LST;
	if (type == "list_ack")
	return PKT::LST_ACK;
	if (type == "file_out")
	return PKT::FILE_OUT;
	if (type == "file_out_ack")
	return PKT::FILE_OUT_ACK;
	if (type == "file_in")
	return PKT::FILE_IN;
	if (type == "file_in_ack")
	return PKT::FILE_IN_ACK;
	if (type == "msg_join")
	return PKT::MSG_JOIN;
	if (type == "msg_join_ack")
	return PKT::MSG_JOIN_ACK;
	if (type == "msg_leave")
	return PKT::MSG_LEAVE;
	if (type == "msg_leave_ack")
	return PKT::MSG_LEAVE_ACK;
	return 0;
}

// Try to assign the data to the packet message.
// This is due to network ordering operatons (Endianness)
void comms::AssignMessage(char *stack, Header &header, std::string &data) {
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

void comms::AssignHeader(Header &header, char *data) {
	unsigned int *out = reinterpret_cast<unsigned int *>(&header);
	unsigned int *in = reinterpret_cast<unsigned int *>(data);
	for (int i = 0; i < sizeof(header) / sizeof(int); ++i) {
	out[i] = ntohl(in[i]);
	}
}

bool comms::ReadSocketFully(SOCKET s, char *stack, std::vector<char> &data) {
	// Read fully from the client.
	int bytesRead{0};
	do {
	bytesRead = recv(s, stack, TransferSize, 0);
	if (bytesRead > 0) {
		const char *start = stack;
		const char *end = stack + bytesRead;
		data.insert(data.end(), start, end);
	} else if (bytesRead == SOCKET_ERROR) {
		return false;
	}
	} while (bytesRead > 0);
	return true;
}

void comms::QueueCompletePackets(std::vector<char> &data, packetQueue &out) {
	// Now we have an entire list of packets.
	int packetDataSize = data.size();
	while (packetDataSize >= HeaderSize) {
	Header hdr;
	AssignHeader(hdr, &data[0]);
#ifdef DEBUG_MODE
	std::cout << "Got packet - type[" << hdr.type << "] len[" << hdr.len
				<< "] seq[" << hdr.sequence << "]" << std::endl;
#endif
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
#ifdef DEBUG_MODE
		std::cout << "Processed data [" << packetDataSize << "]" << std::endl;
#endif
	} else {
// Need to keep this, it's a partial packet.
#ifdef DEBUG_MODE
		std::cout << "Had partial packet" << std::endl;
#endif
		break; // Could not process this.
	}
	}
}

net::AutoLocker::AutoLocker(CRITICAL_SECTION &mutex) : m_mutex(mutex) {
	EnterCriticalSection(&m_mutex);
}
net::AutoLocker::~AutoLocker() { LeaveCriticalSection(&m_mutex); }



// Get the path to the attachments folder.
const std::string &net::NetCommon::GetAttachmentsDirectory() const {
	return m_attachmentsDir;
}

void net::NetCommon::OpenAttachmentsFile(const std::string &name, unsigned int parse,
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
net::NetCommon::NetCommon() {

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

net::NetCommon::~NetCommon() {}

void net::NetCommon::SendPacket(SOCKET s, const comms::Packet &packet) {
// Push the entire packet header + data to the vector.
#ifdef DEBUG_MODE
	std::cout << "Sending packet [" << packet.hdr.type << "]["
				<< packet.data.length() << "][" << packet.data << "]"
				<< std::endl;
#endif
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
			htonl(static_cast<unsigned int>(packet.data[i]) + comms::SALT));

	int bytes = send(s, reinterpret_cast<char *>(&upscaledData[0]),
						upscaledData.size() * sizeof(unsigned int), 0);

// Next step is to print out the upscaled data as a sequency of bytes.
#ifdef DEBUG_MODE
	std::cout << "Sent packet response [";
	unsigned char *packetData =
		reinterpret_cast<unsigned char *>(&upscaledData[0]);
	for (int i = 0; i < (upscaledData.size() * sizeof(unsigned int)); ++i)
		std::cout << (int)packetData[i];
	std::cout << "]" << std::endl;
#endif

	if (bytes <= 0) {
		// Error occurred, we need to mark this socket as closed.
		m_closedSockets.push_back(s);
		return;
	}
}

// Partially Lock Free
void net::NetCommon::HandleClosedSockets(std::vector<SocketData> &clients) {
	std::vector<comms::PacketInfo> out;
	for (auto i : m_closedSockets) {
		for (unsigned int j = 0; j < clients.size(); ++j) {
		if (clients[j].socket == i) {
			std::string alias = clients[j].alias;
			alias.append("|_+_| - has taken the blue pill.");

			comms::PacketInfo bye{
				{{comms::PKT::MSG_LEAVE, 0, 0, 0, alias.length(), 0, 0}, alias},
				false,
				0};

			out.push_back(bye);
			break;
		}
		}
	}

	for (auto a = clients.begin(); a != clients.end(); ++a) {
		for (auto c = m_closedSockets.begin(); c != m_closedSockets.end(); ++c) {
		if (a->socket == *c) {
			a = clients.erase(a);
		}
		}
	}

	m_closedSockets.clear();

	if (out.size()) {
		for (auto o : out) {
		for (auto &i : clients) {
			i.outboundMessages.push_back(o);
		}
		}
	}
}



  // Lock-Free
  void net::NetClient::PvtAddPrintQueueHelper(const std::string &data, bool &trigger,
                              bool compressed) {
    if (!data.empty()) {
      if (!compressed) {
        m_printQueue.push_back(print::PrintInfo(data, "", false));
      } else {
#ifdef USE_FLATE
        // Decompress the data.
        std::string pData{data};
        flate::FlateResult decompressed(pData.c_str(), pData.length(),
                                        pData.length() * 2);
        InflateData(decompressed);
        std::string iData;
        iData.assign(reinterpret_cast<char *>(decompressed.outData),
                     decompressed.outDataSize);
        m_printQueue.push_back(print::PrintInfo(iData, "", false));
#else
        m_printQueue.push_back(print::PrintInfo(data, "", false));
#endif
      }
    }
    trigger = true;
  }

  // Partially locked
  void net::NetClient::transferFileInternal(const std::string &name, const std::string &path,
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
    comms::PacketInfo initial{{{comms::PKT::FILE_OUT, 0, 0, 0, 0, GetNextSequence(),
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

      comms::PacketInfo next{{{comms::PKT::FILE_OUT, 0, chunks, i + 1, bytesRead,
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
  void net::NetClient::HandleFile(comms::Packet &packet) {
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
  void net::NetClient::HandlePacketLockStepSend(comms::packetQueue::iterator &it,
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


  // Don't start up any threads.
  net::NetClient::NetClient()
      : NetCommon(), m_connected(false), m_sequence(4),
        m_thread(INVALID_HANDLE_VALUE) {
    InitializeCriticalSection(&m_mutex);
  }


  void net::NetClient::AddMessage(const std::string &text) {
    AutoLocker locker(m_mutex);
    std::string data(m_alias);
    data.append("|_+_|");
    data.append(text);

#ifdef USE_FLATE
    // Compress the data.
    flate::FlateResult compressed(data.c_str(), data.length(),
                                  data.length() * 2);
    flate::DeflateData(compressed);

    std::string cData;
    cData.assign(reinterpret_cast<char *>(compressed.outData),
                 compressed.outDataSize);

    comms::PacketInfo info{
        {{comms::PKT::MSG, 0, 0, 0, cData.length(), GetNextSequence(), 0}, cData},
        false,
        0};
#else
    comms::PacketInfo info{
        {{comms::PKT::MSG, 0, 0, 0, data.length(), GetNextSequence(), 0}, data},
        false,
        0};
#endif
    m_threadOutQueue.push_back(info);
  }

  void net::NetClient::GetMessages(print::printQueue &msg) {
    AutoLocker locker(m_mutex);
    msg.insert(msg.end(), m_printQueue.begin(), m_printQueue.end());
    m_printQueue.clear();
  }

  void net::NetClient::SetAlias(const std::string &alias) {
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

  void net::NetClient::GetUserList() {
    AutoLocker locker(m_mutex);
    if (!m_connected) {
      m_printQueue.push_back(
          print::PrintInfo("You need to connect first.", "", false));
      return;
    }

    m_printQueue.push_back(
        print::PrintInfo("Fetching list from server....", "", false));
    comms::PacketInfo info{
        {{comms::PKT::LST, 0, 0, 0, 0, GetNextSequence(), 0}, ""}, false, 0};
    m_threadOutQueue.push_back(info);
  }

  void net::NetClient::SendFile(const std::string &name, const std::string &path) {
    std::string user = "";
    transferFileInternal(name, path, user);
  }
  void net::NetClient::SendFileUser(const std::string &name, const std::string &path,
                    const std::string &user) {
    transferFileInternal(name, path, user);
  }

  // Possibly disconnect here.
  net::NetClient::~NetClient() {}

  void net::NetClient::Connect(const std::string &ip) {
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
            {{comms::PKT::ALIAS, 0, 0, 0, m_alias.length(), 4, 0}, m_alias}, false, 0};
        m_threadOutQueue.push_back(info);

        if (m_thread == INVALID_HANDLE_VALUE)
          m_thread = CreateThread(NULL, 0, ClientCommsConnection, (LPVOID) this,
                                  0, NULL);
      }
    }
  }

  void net::NetClient::RemoveOutboundPacket(comms::packetQueue &queue, char msgtype,
                            unsigned int sequence) {
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

  void net::NetClient::ProcessQueues() {
    AutoLocker locker(m_mutex);
    auto in_it = m_threadInQueue.begin();
    auto in_eit = m_threadInQueue.end();

    for (; in_it != in_eit; ++in_it) {
      bool erasePacket = false;
      if (in_it->packet.hdr.type == comms::PKT::ALIAS_ACK) {
        std::cout << "Got alias Ack." << std::endl;
        RemoveOutboundPacket(m_threadOutQueue, comms::PKT::ALIAS);
        PvtAddPrintQueueHelper(in_it->packet.data, erasePacket, false);
      } else if (in_it->packet.hdr.type == comms::PKT::QRY_ACK) {
        std::cout << "Got query Ack." << std::endl;
        RemoveOutboundPacket(m_threadOutQueue, comms::PKT::QRY);
        PvtAddPrintQueueHelper(in_it->packet.data, erasePacket, false);
      } else if (in_it->packet.hdr.type == comms::PKT::MSG_ACK) {
        std::cout << "Got message Ack." << std::endl;
        RemoveOutboundPacket(m_threadOutQueue, comms::PKT::MSG,
                             in_it->packet.hdr.sequence);
        PvtAddPrintQueueHelper(in_it->packet.data, erasePacket, false);
      } else if (in_it->packet.hdr.type == comms::PKT::PVT_ACK) {
        std::cout << "Got private Ack." << std::endl;
        RemoveOutboundPacket(m_threadOutQueue, comms::PKT::PVT);
        PvtAddPrintQueueHelper(in_it->packet.data, erasePacket, false);
      } else if (in_it->packet.hdr.type == comms::PKT::MSG) {
        std::cout << "Got general message." << std::endl;
        RemoveOutboundPacket(m_threadOutQueue, comms::PKT::QRY);
        PvtAddPrintQueueHelper(in_it->packet.data, erasePacket, true);
      } else if (in_it->packet.hdr.type == comms::PKT::LST_ACK) {
        std::cout << "Got list users Ack." << std::endl;
        RemoveOutboundPacket(m_threadOutQueue, comms::PKT::LST);
        PvtAddPrintQueueHelper(in_it->packet.data, erasePacket, false);
      } else if (in_it->packet.hdr.type == comms::PKT::FILE_OUT_ACK) {
        std::cout << "Got file_out Ack." << std::endl;
        RemoveOutboundPacket(m_threadFileOutQueue, comms::PKT::FILE_OUT,
                             in_it->packet.hdr.sequence);
        erasePacket = true;
      } else if (in_it->packet.hdr.type == comms::PKT::FILE_IN) {
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


// Client thread functions.
DWORD WINAPI net::ClientCommsConnection(LPVOID param) {
  net::NetClient *client{reinterpret_cast<net::NetClient *>(param)};
  char stack[comms::TransferSize];
  std::vector<char> packetData;

  while (client->IsRunning()) {
    comms::ReadSocketFully(client->GetSocket(), stack, packetData);
    comms::QueueCompletePackets(packetData, client->GetThreadInQueue());

    client->ProcessQueues();

    Sleep(5);
  }

  return 0;
}
