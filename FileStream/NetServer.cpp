#include "NetServer.h"

#include "../NetCommon.hpp"

// Server thread functions.
DWORD WINAPI net::ServerAcceptConnections(LPVOID param) {
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
			char *ip{ inet_ntoa(from.sin_addr) };
			size_t len{ strlen(ip) };
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

DWORD WINAPI net::ServerCommsConnections(LPVOID param) {
	net::NetServer *server = reinterpret_cast<net::NetServer *>(param);
	bool running = true;
	char stack[comms::TransferSize];

	while (running) {
		{
			net::AutoLocker lock(server->GetMutex());
			std::vector<net::SocketData> &clients{ server->GetClients() };
			for (auto &i : clients) {
				comms::ReadSocketFully(i.socket, stack, i.packetData);
				comms::QueueCompletePackets(i.packetData, i.inboundMessages);
			}

			server->HandleClosedSockets(clients);
		}

		server->ProcessMessages();
		server->SendMessages();

		Sleep(10);
	}

	return true;
}

#if SERVER_PING_COVALENT
// Ping the covalent server to notify it of my ip address.
DWORD WINAPI net::ServerPingCovalent(LPVOID param) {
	// Perform the post to my website where it can record my ip address.
	while (true) {
		SOCKET Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		struct hostent *host;
		host = gethostbyname("www.shaheedabdol.co.za");
		if (host == NULL) {
			DWORD error = WSAGetLastError();
			std::cout << "Could not resolve host name " << error << std::endl;
		}
		else {
			SOCKADDR_IN SockAddr;
			SockAddr.sin_port = htons(80);
			SockAddr.sin_family = AF_INET;
			SockAddr.sin_addr.s_addr = *((unsigned long *)host->h_addr);
			std::cout << "Connecting...to covalent." << std::endl;

			if (connect(Socket, (SOCKADDR *)(&SockAddr), sizeof(SockAddr)) != 0) {
				std::cout << "Could not connect to covalent." << std::endl;
			}
			else {
				std::cout << "Connected to covalent, sending data.";
				// We connected to covalent, send our ping data.
				std::string requestHeader;
				requestHeader.append(
					"GET "
					"/covalent.php?register=fs_srv_2635_6253&port=54547&"
					"timeout=2 HTTP/1.1\r\n");
				requestHeader.append("Host: www.shaheedabdol.co.za\r\n");
				requestHeader.append("\r\n");

				std::cout << requestHeader << std::endl;

				send(Socket, requestHeader.data(), requestHeader.size(), NULL);
			}
		}
		closesocket(Socket);
		Sleep(30000);
	}

	return 0;
}
#endif

// Push a new connection to our list of clients.
void net::NetServer::PushConnection(SOCKET client, const std::string &ip) {
	AutoLocker locker(m_mutex);
	m_clients.push_back(SocketData{ client, ip, "" });
}

void net::NetServer::DropConnection(SOCKET client) {
	AutoLocker locker(m_mutex);

	auto it = m_clients.begin();
	auto eit = m_clients.end();
	for (; it != eit; ++it) {
		if (it->socket == client) {
			std::cout << "Dropping connection from: " << it->ip.c_str();
#ifdef DEBUG_MODE
			std::cout << "Goodbye: " << it->alias.c_str();
#endif
			m_clients.erase(it);
			break;
		}
	}
}

void net::NetServer::GetConnections(std::vector<SocketData> &clients) {
	AutoLocker locker(m_mutex);
	clients = m_clients;
}

void net::NetServer::SetAlias(SOCKET s, std::string &alias) {
	AutoLocker locker(m_mutex);

	for (auto &i : m_clients) {
		if (s == i.socket) {
			i.alias = alias;
			break;
		}
	}
}

void net::NetServer::ProcessMessages() {
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
			case comms::PKT::ALIAS: {
				so.alias = msg.packet.data;
				std::string data = msg.packet.data;
#ifdef DEBUG_MODE
				std::cout << "NB. " << data << std::endl;
#endif
#ifdef USE_FLATE
				// Compress the response.
				flate::FlateResult compressed(data.c_str(), data.length(),
					data.length() * 2);
				flate::DeflateData(compressed);

				std::string cData;
				cData.assign(reinterpret_cast<char *>(compressed.outData),
					compressed.outDataSize);

				comms::PacketInfo info{
					{{PKT_MSG_JOIN, 0, 0, 0, cData.length(), 1, 0}, cData}, false, 0 };
#else
				comms::PacketInfo info{
					{{comms::PKT::MSG_JOIN, 0, 0, 0, data.length(), 1, 0}, data}, false, 0 };
#endif
				// Store the messages for global delivery.
				globalMessages.push_back(info);

				// Immediately ack.
				comms::Packet ack{
					{comms::PKT::ALIAS_ACK, 0, 0, 0, 0, msg.packet.hdr.sequence, 0}, "" };
				SendPacket(so.socket, ack);
			} break;
			case comms::PKT::QRY: {
				// Immediately ack.
				comms::Packet ack{
					{comms::PKT::QRY_ACK, 0, 0, 0, 0, msg.packet.hdr.sequence, 0}, "" };
				SendPacket(so.socket, ack);
			} break;
			case comms::PKT::MSG: {
				// Store the message for global delivery.
				globalMessages.push_back(msg);
				// Immediately ack.
				comms::Packet ack{
					{comms::PKT::MSG_ACK, 0, 0, 0, 0, msg.packet.hdr.sequence, 0}, "" };
				SendPacket(so.socket, ack);

			} break;
			case comms::PKT::PVT: {
				// Immediately ack.
				comms::Packet ack{
					{comms::PKT::PVT_ACK, 0, 0, 0, 0, msg.packet.hdr.sequence, 0}, "" };
				SendPacket(so.socket, ack);

				// Store the message for private delivery.
				privateMessages.push_back(msg);
			} break;
			case comms::PKT::LST: {
				std::string user_list("Users on this server:|");
				{
					auto lst_it = m_clients.begin();
					auto lst_eit = m_clients.end();
					for (; lst_it != lst_eit; ++lst_it) {
						user_list.append(lst_it->ip);
						user_list.append(" : ");
						user_list.append(lst_it->alias);
						user_list.append("|_+_|");
					}
					comms::Packet ack{ {comms::PKT::LST_ACK, 0, 0, 0, user_list.length(),
										msg.packet.hdr.sequence, 0},
										user_list };
					// Immediately ack.
					SendPacket(so.socket, ack);
				}
			} break;
			case comms::PKT::FILE_OUT: {
				comms::Packet ack{
					{comms::PKT::FILE_OUT_ACK, 0, 0, 0, 0, msg.packet.hdr.sequence, 0}, "" };
				SendPacket(so.socket, ack);
				std::cout << "Sending back file in ack[" << msg.packet.hdr.sequence
					<< "]" << std::endl;

				// This is one of the only messages which are mutated before being
				// sent back.
				comms::PacketInfo info{ msg.packet, false, 0 };
				info.packet.hdr.type = comms::PKT::FILE_IN;
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

void net::NetServer::SendMessages() {
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
net::NetServer::NetServer() : NetCommon() {
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

#if SERVER_PING_COVALENT
	DWORD covalentThreadId;
	covalentThread =
		CreateThread(NULL, 0, ServerPingCovalent, this, 0, &covalentThreadId);
#endif
}

net::NetServer::~NetServer() {
	// Should wait for the threads to stop
	// Then DeleteCriticalSection(&m_mutex);
}