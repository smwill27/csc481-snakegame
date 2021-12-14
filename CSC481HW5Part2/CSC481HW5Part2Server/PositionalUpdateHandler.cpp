#include "PositionalUpdateHandler.h"
#include "LocationInSpace.h"
#include <iostream>

PositionalUpdateHandler::PositionalUpdateHandler(std::map<int, Property*>* propertyMap, bool notifyWhileReplaying, zmq::socket_t* socket)
	: EventHandler(propertyMap, notifyWhileReplaying)
{
	this->socket = socket;
}

void PositionalUpdateHandler::onEvent(Event* e)
{
	if (e->getType() == ClientServerConsts::SNAKE_BLOCK_MOVED_EVENT 
		|| e->getType() == ClientServerConsts::FOOD_SPAWN_EVENT
		|| e->getType() == ClientServerConsts::SNAKE_BLOCK_ADDED_EVENT
		|| e->getType() == ClientServerConsts::SNAKE_HEAD_SPAWN_EVENT)
	{
		//get ids of moved object and its location in space, as well as its new absolute position
		int objectId = e->getArgument(0).argValue.argAsInt;
		int locationInSpaceId = e->getArgument(1).argValue.argAsInt;
		float absoluteX = e->getArgument(2).argValue.argAsFloat;
		float absoluteY = e->getArgument(3).argValue.argAsFloat;

		//construct message to send to clients
		std::string msgString = std::to_string(objectId) + " " + std::to_string(locationInSpaceId)
			+ " " + std::to_string(absoluteX) + " " + std::to_string(absoluteY);
		zmq::message_t msg(msgString.length() + 1);
		const char* msgChars = msgString.c_str();
		memcpy(msg.data(), msgChars, msgString.length() + 1);

		//std::cout << "Sending positional update" << std::endl;
		//send update to clients
		socket->send(msg, zmq::send_flags::none);

		//if snake head respawned, send additional message to allow client to remove old snake blocks
		if (e->getType() == ClientServerConsts::SNAKE_HEAD_SPAWN_EVENT)
		{
			//construct message to send to clients
			std::string headRespawnMsgString = std::to_string(ClientServerConsts::POSITIONAL_UPDATE_HEAD_RESPAWN_CODE) + " " 
				+ std::to_string(ClientServerConsts::POSITIONAL_UPDATE_FILLER)
				+ " " + std::to_string(ClientServerConsts::POSITIONAL_UPDATE_FILLER) + " " 
				+ std::to_string(ClientServerConsts::POSITIONAL_UPDATE_FILLER);
			zmq::message_t headRespawnMsg(headRespawnMsgString.length() + 1);
			const char* headRespawnMsgChars = headRespawnMsgString.c_str();
			memcpy(headRespawnMsg.data(), headRespawnMsgChars, headRespawnMsgString.length() + 1);

			//std::cout << "Sending positional update" << std::endl;
			//send update to clients
			socket->send(headRespawnMsg, zmq::send_flags::none);
		}
	}
}
