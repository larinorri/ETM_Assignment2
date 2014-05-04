// WinsockListener.cpp
#include "pch.h"
#include "GameServer.h"
#include "NetworkEvents.h"

using namespace PhoneDirect3DXamlAppComponent;

GameServer::GameServer()
{
}

void GameServer::SetClientCallBack(IClientConnectedCallback^ callbackObject)
{
	_callbackObject = callbackObject;
}

// Main code pulled form windows 8 networking tutorials
void GameServer::StartSocketServer(String^* UsingIpAddress, String^* errMessage)
{
	int listenSocket;  // listen on sock_fd, clients that are connected talk on clientConnectedSocket
	struct addrinfo hints, *servinfo;
	int yes = 1;
	const char* c = NULL;
	c = (char*)&yes;
	int rv;
	//used to callback to the managed layer when connections are received
	IClientConnectedCallback^ callbackObject = _callbackObject;

	//init winsock
	WSADATA wsaData;
	if ((rv = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
		*errMessage = "WSAStartup error:";
		*errMessage += WSAGetLastError();
		return;
	}

	memset(&hints, 0, sizeof hints);
	//using ipv4
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	//get our device servinfo for the ipv4 socket
	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		*errMessage = "getaddrinfo error:";
		*errMessage += WSAGetLastError();
		return;
	}

	//get the listener socket
	listenSocket = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		*errMessage = "socket failed with error:";
		*errMessage += WSAGetLastError();
		freeaddrinfo(servinfo);
		WSACleanup();
		return;
	}

	//// set socket options no delay & reuse socket
	BOOL bOptVal = TRUE;
	int bOptLen = sizeof (BOOL);
	int iResult;// = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&bOptVal, bOptLen);
	//if (iResult == SOCKET_ERROR) {
	//	*errMessage = "Failed to set socket options with error:";
	//	*errMessage += WSAGetLastError();
	//	freeaddrinfo(servinfo);
	//	closesocket(listenSocket);
	//	WSACleanup();
	//	return;
	//}
	//// No delay
	//iResult = setsockopt(listenSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&bOptVal, bOptLen);
	//if (iResult == SOCKET_ERROR) {
	//	*errMessage = "Failed to set socket options with error:";
	//	*errMessage += WSAGetLastError();
	//	freeaddrinfo(servinfo);
	//	closesocket(listenSocket);
	//	WSACleanup();
	//	return;
	//}

	// bind
	iResult = bind(listenSocket, servinfo->ai_addr, (int)servinfo->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		*errMessage = "Bind failed with error:";
		*errMessage += WSAGetLastError();
		freeaddrinfo(servinfo);
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	//get ip of the device so we can show it in the UI 
	hostent* localHost;
	char* localIP;
	localHost = gethostbyname("");
	localIP = inet_ntoa(*(struct in_addr *)*localHost->h_addr_list);
	wchar_t serverIP[255];
	MultiByteToWideChar(CP_UTF8, 0, localIP, -1, serverIP, 255);
	*UsingIpAddress = ref new String(serverIP);

	//free servinfo; don't need it anymore
	freeaddrinfo(servinfo);

	if (listen(listenSocket, BACKLOG) == -1) {
		*errMessage = "Listen error:";
		*errMessage += WSAGetLastError();
		return;
	}

	//this handler executes in the background forever listening for 
	//incoming connections and calling back to the UI thread when it has one come through.
	using namespace Windows::System::Threading;
	ThreadPool::RunAsync(ref new WorkItemHandler([listenSocket, callbackObject](Windows::Foundation::IAsyncAction^ operation)
	{
		while (1) {

			struct sockaddr_storage their_addr;
			int clientConnectedSocket;
			socklen_t sin_size = sizeof their_addr;

			//accept connections from clients
			clientConnectedSocket = accept(listenSocket, (struct sockaddr *)&their_addr, &sin_size);
			if (clientConnectedSocket == -1) {
				perror("accept");
				continue;
			}

			//if you receive a connection from a client you'd likely spin it off on another 
			//thread we're doing very little here so we just execute on this one
			struct sockaddr_in* inaddr = (struct sockaddr_in*) &their_addr;
			char* s = inet_ntoa(inaddr->sin_addr);

			wchar_t ws[255];
			MultiByteToWideChar(CP_UTF8, 0, s, -1, ws, 255);
			String^ clientIpAddress = ref new String(ws);//

			//tell the UI thread we got a connection
			callbackObject->ClientConnectedServer(clientIpAddress);

			// this lets a socket be non-blocking (recv will not wait)
			u_long iMode = 1;
			ioctlsocket(clientConnectedSocket, FIONBIO, &iMode);

			// here we will loop sending any availbale server events to our client
			NetworkEvents::EVENT_DATA sendEvent, receiveEvent;
			memset(&sendEvent, 0x0000000, sizeof(NetworkEvents::EVENT_DATA));
			memset(&receiveEvent, 0x0000000, sizeof(NetworkEvents::EVENT_DATA));
			// loop until told otherwise
			while (sendEvent.ID != GAME_EVENT::NETWORK_KILL)
			{
				// if any data is waiting to be sent, send it to the client
				if (NetworkEvents::GetInstance().PopOutgoingEvent(&sendEvent))
				{
					if (send(clientConnectedSocket, (const char*)&sendEvent, sizeof(NetworkEvents::EVENT_DATA), 0) == -1)
						perror("could not send to client, event dropped");
				}
				// attempt to receive data from client (only look for events)
				if (recv(clientConnectedSocket, (char*)&receiveEvent, sizeof(NetworkEvents::EVENT_DATA), 0) == sizeof(NetworkEvents::EVENT_DATA))
				{
					// looks like we got somethin...
					NetworkEvents::GetInstance().PushIncomingEvent(&receiveEvent);
				}
			}
			//send any remaining outgoing events
			while (NetworkEvents::GetInstance().PopOutgoingEvent(&sendEvent))
			{
				if (send(clientConnectedSocket, (const char*)&sendEvent, sizeof(NetworkEvents::EVENT_DATA), 0) == -1)
					perror("could not send to client, event dropped");
			}
			
			closesocket(clientConnectedSocket);
			clientConnectedSocket = NULL;
			break; // leave this connection
		}

		closesocket(listenSocket);
		WSACleanup();
		//assert(0);

	}));

	return;
}