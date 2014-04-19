// stores all game events transmited and received over the network
// used by both clienet and server machines
#pragma once
#include <directxmath.h>
#include <queue>
#include <mutex>
// the following is the list of possible events in the game

enum GAME_EVENT
{
	NETWORK_KILL = -1, // network threads close down upon seeing this event 
	NON_EVENT = 0, // dummy transmission (ignored)
	SYNC_CUBE, // synchronizes the client cube to the server cube
};

// Singleton which is used by the game server & client to send and receive events
class NetworkEvents
{
public:
	struct EVENT_DATA
	{
		unsigned int ID; // what kind of event is this
		union // possible data stored in the event
		{
			float matrix[16]; // some kind of matrix
		};
	};
private:
	// data members which contain out going & incoming transmissions
	std::queue<EVENT_DATA> incomingEvents;
	std::queue<EVENT_DATA> outgoingEvents;
	// mutexes protecting queues
	std::mutex	lockIncoming;
	std::mutex	lockOutgoing;
public:
	// access to singleton
	static NetworkEvents& GetInstance();
	// functions used to send and retreive event data
	void PushIncomingEvent(EVENT_DATA *_receive); // used by network thread
	bool PopIncomingEvent(EVENT_DATA *_receive); // used by game
	void PushOutgoingEvent(EVENT_DATA *_send); // used by game
	bool PopOutgoingEvent(EVENT_DATA *_send); // used by network thread
};

