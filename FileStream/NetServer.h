#include "../NetCommon.hpp"

#define SERVER_PING_COVALENT 0

namespace net {
	// Server thread functions.
	DWORD WINAPI ServerAcceptConnections(LPVOID param);
	DWORD WINAPI ServerCommsConnections(LPVOID param);
#if SERVER_PING_COVALENT
	DWORD WINAPI ServerPingCovalent(LPVOID param);
#endif
	class NetServer : public NetCommon {
	private:
		HANDLE acceptThread;
		HANDLE commsThread;
#if SERVER_PING_COVALENT
		HANDLE covalentThread;
#endif
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
		void PushConnection(SOCKET client, const std::string &ip);

		void DropConnection(SOCKET client);

		void GetConnections(std::vector<SocketData> &clients);

		void SetAlias(SOCKET s, std::string &alias);

		void ProcessMessages();

		void SendMessages();

		// Create the server - initialize common WinSock things.
		// Create the accept and comms threads.
		NetServer();

		~NetServer();

	}; // NetServer

}; // namespace net