//#pragma once
#include "pch.h"
#include <ws2tcpip.h>
#include <stdio.h>
#include <tchar.h>

#define PORT "4533" // the port client will be connecting to 
#define MAXDATASIZE 64 // max number of bytes we can get at once 

using namespace Platform;


namespace PhoneDirect3DXamlAppComponent
{
	public ref class GameClient sealed
	{
	public:
		GameClient();
		int ConnectThroughSocket(String^ hostToConnect, String^* error);
	};
}