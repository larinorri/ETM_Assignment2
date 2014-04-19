#pragma once

#include <ws2tcpip.h>

#include <stdio.h>
#include <tchar.h>

using namespace Platform;

#define PORT "4533"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold

// classes based on windows 8 networking MSDN examples
namespace PhoneDirect3DXamlAppComponent
{
	// allows us to inform the UI the game has started
	public interface class IClientConnectedCallback
	{
	public:
		void ClientConnectedServer(String^ ipAddressOfClient);

	};
	// launches the game server
	public ref class GameServer sealed
	{
	public:
		GameServer();
		void SetClientCallBack(IClientConnectedCallback^ callbackObject);
		void StartSocketServer(String^* UsingIpAddress, String^* errMessage);

	private:
		// allows C# to know when the client has connected
		IClientConnectedCallback^ _callbackObject;

	};


}