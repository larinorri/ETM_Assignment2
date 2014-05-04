// WinsockClient.cpp
#include "pch.h"
#include "GameClient.h"
#include "NetworkEvents.h"

// Need to link with Ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

using namespace PhoneDirect3DXamlAppComponent;
//using namespace Platform;

GameClient::GameClient()
{
}

int GameClient::ConnectThroughSocket(String^ hostForConn, String^* errMsg)
{
	int sockfd;  // socket to connect with
	char buf[MAXDATASIZE]; //receive buffer (unused for now)
	struct addrinfo hints, *servinfo, *p;  //socket setup
	int rv; //return value

	WSADATA wsaData;

	//init Winsock
	if ((rv = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
		*errMsg = "WSAStartup error: %s", gai_strerror(rv);
		return 1;
	}

	//configure the host we're going to connect to
	auto host = hostForConn->Data();
	int size = hostForConn->Length() + 1;
	char* hostToConnect = new char[size];
	WideCharToMultiByte(CP_UTF8, 0, host, -1, hostToConnect, size, NULL, NULL);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	//get adapters
	if ((rv = getaddrinfo((PCSTR)hostToConnect, PORT, &hints, &servinfo)) != 0) {
		*errMsg = "getaddrinfo: %s\n", gai_strerror(rv);
		delete[] hostToConnect;
		return 1;
	}

	// loop through all our adapters and connect to our target through the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			*errMsg = "client: socket";
			continue;
		}

		//Connect
		if ((rv = connect(sockfd, p->ai_addr, p->ai_addrlen)) == -1) {
			int err = WSAGetLastError();
			closesocket(sockfd);
			*errMsg = "client: connect";
			continue;
		}
		break;
	}

	if (p == NULL) {
		*errMsg = "client: failed to connect";
		delete[] hostToConnect;
		return 2;
	}

	//// disable nagle's algorithim on this socket
	//BOOL bOptVal = TRUE;
	//int bOptLen = sizeof (BOOL);
	//int iResult = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&bOptVal, bOptLen);
	//if (iResult == SOCKET_ERROR) {
	//	*errMsg = "Failed to set socket options with error:";
	//	*errMsg += WSAGetLastError();
	//	freeaddrinfo(servinfo);
	//	closesocket(sockfd);
	//	WSACleanup();
	//	return 2;
	//}

	//free servinfo struct
	freeaddrinfo(servinfo);

	// here we will spawn a thread receiving & sending events from the server
	// the only events we send will be input events & a quit event. Server will handle main game play logic
	using namespace Windows::System::Threading;
	ThreadPool::RunAsync(ref new WorkItemHandler([sockfd, hostToConnect](Windows::Foundation::IAsyncAction^ operation)
	{
		// this lets a socket be non-blocking (recv will not wait)
		u_long iMode = 1;
		ioctlsocket(sockfd, FIONBIO, &iMode);

		// here we will loop sending any availbale client events to our server & listen for server instructions
		NetworkEvents::EVENT_DATA sendEvent, receiveEvent;
		memset(&sendEvent, 0x0000000, sizeof(NetworkEvents::EVENT_DATA));
		memset(&receiveEvent, 0x0000000, sizeof(NetworkEvents::EVENT_DATA));
		while (sendEvent.ID != GAME_EVENT::NETWORK_KILL)
		{
			// attempt to receive data from server (only look for events)
			if (recv(sockfd, (char*)&receiveEvent, sizeof(NetworkEvents::EVENT_DATA), 0) == sizeof(NetworkEvents::EVENT_DATA))
			{
				// looks like we got somethin...
				NetworkEvents::GetInstance().PushIncomingEvent(&receiveEvent);
			}
			// if any data is waiting to be sent, send it to the server
			if (NetworkEvents::GetInstance().PopOutgoingEvent(&sendEvent))
			{
				if (send(sockfd, (const char*)&sendEvent, sizeof(NetworkEvents::EVENT_DATA), 0) == -1)
					perror("could not send to server, event dropped");
			}
			
		}
		//send any remaining outgoing events
		while (NetworkEvents::GetInstance().PopOutgoingEvent(&sendEvent))
		{
			if (send(sockfd, (const char*)&sendEvent, sizeof(NetworkEvents::EVENT_DATA), 0) == -1)
				perror("could not send to client, event dropped");
		}
		// shutdown listen socket
		closesocket(sockfd);
		//delete the character array holding our target listener
		delete[] hostToConnect;
		//assert(0);

	}));
	
	//Send or Receive data from the opened socket...
	//  if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	//*errMsg = "recv error";
	//delete[] hostToConnect;
	//      return -1; //exit(1);
	//  }
	//...


	// back to UI while thread runs...
	return 0;
}