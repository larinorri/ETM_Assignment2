#include "pch.h"
#include "NetworkEvents.h"

// access singleton
NetworkEvents& NetworkEvents::GetInstance()
{
	static NetworkEvents instance;
	return instance;
}

// Handle network events safely across threads

void NetworkEvents::PushIncomingEvent(EVENT_DATA *_receive) // used by network thread
{
	if (_receive == nullptr) return;
	std::unique_lock<std::mutex> block(lockIncoming);
	incomingEvents.push(*_receive);
}
bool NetworkEvents::PopIncomingEvent(EVENT_DATA *_receive) // used by game
{
	if (_receive == nullptr) return false;
	std::unique_lock<std::mutex> block(lockIncoming);
	if (incomingEvents.empty()) return false;
	*_receive = incomingEvents.back();
	incomingEvents.pop();
	return true;
}
void NetworkEvents::PushOutgoingEvent(EVENT_DATA *_send) // used by game
{
	if (_send == nullptr) return;
	std::unique_lock<std::mutex> block(lockOutgoing);
	outgoingEvents.push(*_send);
}
bool NetworkEvents::PopOutgoingEvent(EVENT_DATA *_send) // used by network thread
{
	if (_send == nullptr) return false;
	std::unique_lock<std::mutex> block(lockOutgoing);
	if (outgoingEvents.empty()) return false;
	*_send = outgoingEvents.back();
	outgoingEvents.pop();
	return true;
}