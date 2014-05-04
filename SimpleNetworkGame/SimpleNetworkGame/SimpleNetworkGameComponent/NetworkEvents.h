// stores all game events transmited and received over the network
// used by both clienet and server machines
#pragma once
#include <directxmath.h>
#include <queue>
#include <mutex>
// the following is the list of possible events in the game

enum GAME_EVENT
{
	NETWORK_KILL = 666,	// network threads close down upon seeing this event 
	NON_EVENT = 0,		// dummy transmission (ignored)
	SEED_LEVEL,			// synchronizes the client cube to the server cube
	START_GAME,			// Clients are connected and intialized
	GAME_OVER,			// stops game play, contains win state
	PLAYER_LOC,			// Remote player has moved, preform visual update
	COMPASS_SEND,		// Allows you to sync with another players compass (for testing)
};

// Singleton which is used by the game server & client to send and receive events
class NetworkEvents
{
public:
	struct EVENT_DATA
	{
		unsigned int ID; // what kind of event is this
		// could be further optimized with an event size variable
		union // possible data stored in the event
		{
			// Removed message switched off nagle's algorithim
			///char			message[256];		// added this to hopefully speed up TCP, need to switch to UDP
			
			float			matrix[16];			// some kind of matrix
			double			floatingPoint64;	// precise floating point value	
			unsigned int	integerUnsigned;	// unsigned numerical value 
			int				integerSigned;		// signed numerical value 
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
	// clear all waiting messages from the queues
	void ClearOutgoingEvents();
	void ClearIncomingEvents();
};

