#include <zmq.hpp>
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <iostream>
#include "Timeline.h"
#include <thread>
#include <chrono>
#include "ClientServerConsts.h"
#include "Property.h"
#include "LocationInSpace.h"
#include "Collision.h"
#include "dukglue/dukglue.h"
#include "ChainedDirectionalMovement.h"
#include <windows.h>
#define sleep(n)    Sleep(n)
#include "RealTimeline.h"
#include "GameTimeline.h"
#include <mutex>
#include "Event.h"
#include "EventHandler.h"
#include "EventManager.h"
#include "Respawning.h"
#include "ScriptManager.h"
#include "PositionalUpdateHandler.h"
#include "UserInputHandler.h"
#include "SnakeCollisionHandler.h"

/*counter providing next GUID to be assigned*/
int idCounter;

/* context for all sockets */
zmq::context_t* context;

/* socket used for receiving connection/disconnection requests from clients */
zmq::socket_t* connectDisconnectReqRepSocket;

/* socket used for sending connection/disconnection updates to all clients */
zmq::socket_t* connectDisconnectPubSubSocket;

/* socket used for sending movement updates to all clients */
zmq::socket_t* movementUpdatePubSubSocket;

/* socket used for sending object creation messages to connecting client */
zmq::socket_t* objectCreationReqRepSocket;

/* map with all properties */
std::map<int, Property*> propertyMap;

/* snake block properties */
LocationInSpace* snakeBlockLocationsInSpace;
Collision* snakeBlockCollisions;
int snakeBlockRendering;
ChainedDirectionalMovement* snakeBlockChainedDirectionalMovements;
Respawning* snakeHeadRespawning;

/*snake head spawn point properties*/
LocationInSpace* snakeHeadSpawnPointLocationsInSpace;

/* border properties */
LocationInSpace* borderLocationsInSpace;
Collision* borderCollisions;
int borderRendering;

/* food properties */
LocationInSpace* foodLocationsInSpace;
Collision* foodCollisions;
int foodRendering;
Respawning* foodRespawning;

/*food spawn point properties*/
LocationInSpace* foodSpawnPointLocationsInSpace;

/*border ids*/
int northBorderId;
int southBorderId;
int westBorderId;
int eastBorderId;

/*food ids*/
int food1Id;

/*list of food spawn point ids*/
std::vector<int> foodSpawnPointIds;

/*id of snake head spawn point*/
int snakeHeadSpawnPoint1Id;

/* manager for events */
EventManager* eventManager;

/* modifier for individual client event port numbers */
int eventPortModifier;

/* mutex controlling access to event queue */
std::mutex queueLock;

/* whether a client is currently connected (used to restrict game to only one client connection due to not being
fully implemented for multiplayer)*/
bool clientConnected;

/*
* Returns the next id for assignment and then updates the counter.
*
* returns: id to use for next assignment
*/
int getNextId()
{
	int nextId = idCounter;
	idCounter++;
	return nextId;
}

/*
* Preps a message to be sent to the client for creating a particular property or object.
*
* objectOrPropertyType: type of property or object being created
* objectOrPropertyId: id of property or object being created
* locationInSpacePropertyId: if object, id of corresponding locationInSpace property (filler if property)
* renderingPropertyId: if object, id of corresponding rendering property (filler if property)
* color: if object, color to give it (filler if property)
* x: if object, x position (filler of property)
* y: if object, y position (filler if property)
*
* returns: pointer to message to send to client
*/
zmq::message_t* prepCreationMessage(int objectOrPropertyType, int objectOrPropertyId, int locationInSpacePropertyId,
	int renderingPropertyId, int color, float x, float y)
{
	std::string replyString = std::to_string(objectOrPropertyType) + " " + std::to_string(objectOrPropertyId) + " "
		+ std::to_string(locationInSpacePropertyId) + " " + std::to_string(renderingPropertyId) + " " +
		std::to_string(color) + " " + std::to_string(x) + " " + std::to_string(y);
	zmq::message_t* msg = new zmq::message_t(replyString.length() + 1);
	const char* replyChars = replyString.c_str();
	memcpy(msg->data(), replyChars, replyString.length() + 1);

	return msg;
}

/*
* Should be run in a separate thread for each client to allow checking for client event requests in parallel.
*
* eventPortNum: port number to use in creating socket
*/
void handleClientEventRequests(int eventPortNum)
{
	//std::cout << "Client thread starting" << std::endl;

	//prepare and bind socket
	zmq::socket_t eventRaisingReqRepSocket(*context, ZMQ_REP);
	//std::cout << "Created thread socket" << std::endl;

	eventRaisingReqRepSocket.bind(ClientServerConsts::EVENT_RAISING_REQ_REP_SERVER_PORT_START + std::to_string(eventPortNum));
	//std::cout << "Bound thread socket" << std::endl;

	bool clientStillConnected = true;

	while (clientStillConnected)
	{
		//sleep for a bit
		sleep(5);

		//see if there's a request from client, and if so handle it
		try
		{
			std::lock_guard<std::mutex> lock(queueLock);
			zmq::message_t clientRequest;

			if (eventRaisingReqRepSocket.recv(clientRequest, zmq::recv_flags::dontwait))
			{
				//determine event type
				int eventType = -1;
				int scan = sscanf_s(clientRequest.to_string().c_str(), "%d", &eventType);

				if (eventType == ClientServerConsts::USER_INPUT_EVENT_CODE)
				{
					//retrieve arguments
					int characterId = -1;
					int keyType = -1;

					int nextScan = sscanf_s(clientRequest.to_string().c_str(), "%d %d %d", &eventType, &characterId, &keyType);
					//std::cout << "Key Type: " + std::to_string(keyType) << std::endl;

					//create and raise event
					struct Event::ArgumentVariant characterIdArg = { Event::ArgumentType::TYPE_INTEGER, characterId };
					struct Event::ArgumentVariant keyTypeArg = { Event::ArgumentType::TYPE_INTEGER, keyType };
					float eventTime = 0.f;
					eventTime = EventManager::getManager()->getCurrentTime();

					Event* userInputEvent = new Event(eventTime,
						ClientServerConsts::USER_INPUT_EVENT,
						{ characterIdArg, keyTypeArg });
					EventManager::getManager()->raise(userInputEvent);
				}
				else if (eventType == ClientServerConsts::CLIENT_DISCONNECT_EVENT_CODE)
				{
					clientStillConnected = false;
				}

				//send back response
				std::string replyString = "Event Received";
				zmq::message_t* msg = new zmq::message_t(replyString.length() + 1);
				const char* replyChars = replyString.c_str();
				memcpy(msg->data(), replyChars, replyString.length() + 1);
				eventRaisingReqRepSocket.send(*msg, zmq::send_flags::none);
			}
		}
		catch (...)
		{
			std::cerr << "Error while checking for/handling client event request" << std::endl;
		}
	}

	//std::cout << "Client thread ending" << std::endl;
}

/*
* Checks for any connection/disconnection requests and processes them. Returns true if a connection was established to allow
* adding a new thread to handle it.
*/
bool checkForConnectionUpdate()
{
	zmq::message_t connectDisconnectRequest;
	if (connectDisconnectReqRepSocket->recv(connectDisconnectRequest, zmq::recv_flags::dontwait))
	{
		//parse out request
		int connectionObjectId = 0;
		int scan = sscanf_s(connectDisconnectRequest.to_string().c_str(), "%d", &connectionObjectId);
		//std::cout << "Received from client: " + std::to_string(connectionObjectId) << std::endl;

		//if incorrect request, print error and send back error message
		if (scan != 1)
		{
			std::cerr << "Error in communication while processing connection request" << std::endl;
			std::string replyString = std::to_string(ClientServerConsts::CONNECT_ERROR_CODE) + " " + std::to_string(-1);
			zmq::message_t replyMsg(replyString.length() + 1);
			const char* replyChars = replyString.c_str();
			memcpy(replyMsg.data(), replyChars, replyString.length() + 1);
			connectDisconnectReqRepSocket->send(replyMsg, zmq::send_flags::none);
			return false;
		}
		//if client is connecting, add snake head for them, send back message with snake head id
		//and event port number, and publish connection update to all clients
		else if (connectionObjectId == ClientServerConsts::CONNECT_CODE)
		{
			//if a client is already connected, don't allow another to connect since game isn't fully designed/implemented for multiplayer
			if (clientConnected)
			{
				std::string replyString = std::to_string(ClientServerConsts::CONNECT_ERROR_CODE) + " " + std::to_string(-1);
				zmq::message_t replyMsg(replyString.length() + 1);
				const char* replyChars = replyString.c_str();
				memcpy(replyMsg.data(), replyChars, replyString.length() + 1);
				connectDisconnectReqRepSocket->send(replyMsg, zmq::send_flags::none);
				return false;
			}

			//create snake head shape
			sf::RectangleShape* snakeHeadShape = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::SNAKE_BLOCK_WIDTH,
				ClientServerConsts::SNAKE_BLOCK_HEIGHT));
			snakeHeadShape->setFillColor(sf::Color::Green);
			snakeHeadShape->setPosition(sf::Vector2f(390.f, 290.f));
			int snakeHeadId = getNextId();

			//add snake head to properties
			snakeBlockLocationsInSpace->addObject(snakeHeadId, snakeHeadShape);
			snakeBlockCollisions->addObject(snakeHeadId, snakeBlockLocationsInSpace->getId());
			snakeBlockChainedDirectionalMovements->addHead(snakeHeadId, snakeBlockLocationsInSpace->getId(),
				ClientServerConsts::SNAKE_BLOCK_VELOCITY, ChainedDirectionalMovement::Direction::NO_DIRECTION);
			snakeHeadRespawning->addObject(snakeHeadId, snakeBlockLocationsInSpace->getId(),
				Respawning::SpawnPoint(snakeHeadSpawnPoint1Id, snakeHeadSpawnPointLocationsInSpace->getId()));

			//send message with snake head id and event port number
			int newEventPortNum = ClientServerConsts::EVENT_RAISING_REQ_REP_NUM_START + eventPortModifier;
			std::string replyString = std::to_string(snakeHeadId) + " " + std::to_string(newEventPortNum);
			zmq::message_t replyMsg(replyString.length() + 1);
			const char* replyChars = replyString.c_str();
			memcpy(replyMsg.data(), replyChars, replyString.length() + 1);
			connectDisconnectReqRepSocket->send(replyMsg, zmq::send_flags::none);

			//send messages to client to create location in space properties
			objectCreationReqRepSocket->recv(connectDisconnectRequest, zmq::recv_flags::none);
			zmq::message_t* borderLocationInSpaceMsg = prepCreationMessage(ClientServerConsts::PROPERTY_LOCATION_IN_SPACE,
				borderLocationsInSpace->getId(), ClientServerConsts::CREATION_FILLER,
				ClientServerConsts::CREATION_FILLER, ClientServerConsts::CREATION_FILLER,
				ClientServerConsts::CREATION_FILLER, ClientServerConsts::CREATION_FILLER);
			objectCreationReqRepSocket->send(*borderLocationInSpaceMsg, zmq::send_flags::none);
			objectCreationReqRepSocket->recv(connectDisconnectRequest, zmq::recv_flags::none);
			zmq::message_t* foodLocationInSpaceMsg = prepCreationMessage(ClientServerConsts::PROPERTY_LOCATION_IN_SPACE,
				foodLocationsInSpace->getId(), ClientServerConsts::CREATION_FILLER,
				ClientServerConsts::CREATION_FILLER, ClientServerConsts::CREATION_FILLER,
				ClientServerConsts::CREATION_FILLER, ClientServerConsts::CREATION_FILLER);
			objectCreationReqRepSocket->send(*foodLocationInSpaceMsg, zmq::send_flags::none);
			objectCreationReqRepSocket->recv(connectDisconnectRequest, zmq::recv_flags::none);
			zmq::message_t* snakeBlockLocationInSpaceMsg = prepCreationMessage(ClientServerConsts::PROPERTY_LOCATION_IN_SPACE,
				snakeBlockLocationsInSpace->getId(), ClientServerConsts::CREATION_FILLER,
				ClientServerConsts::CREATION_FILLER, ClientServerConsts::CREATION_FILLER,
				ClientServerConsts::CREATION_FILLER, ClientServerConsts::CREATION_FILLER);
			objectCreationReqRepSocket->send(*snakeBlockLocationInSpaceMsg, zmq::send_flags::none);

			//send messages to client to create rendering properties
			objectCreationReqRepSocket->recv(connectDisconnectRequest, zmq::recv_flags::none);
			zmq::message_t* borderRenderingMsg = prepCreationMessage(ClientServerConsts::PROPERTY_RENDERING,
				borderRendering, ClientServerConsts::CREATION_FILLER,
				ClientServerConsts::CREATION_FILLER, ClientServerConsts::CREATION_FILLER,
				ClientServerConsts::CREATION_FILLER, ClientServerConsts::CREATION_FILLER);
			objectCreationReqRepSocket->send(*borderRenderingMsg, zmq::send_flags::none);
			objectCreationReqRepSocket->recv(connectDisconnectRequest, zmq::recv_flags::none);
			zmq::message_t* foodRenderingMsg = prepCreationMessage(ClientServerConsts::PROPERTY_RENDERING,
				foodRendering, ClientServerConsts::CREATION_FILLER,
				ClientServerConsts::CREATION_FILLER, ClientServerConsts::CREATION_FILLER,
				ClientServerConsts::CREATION_FILLER, ClientServerConsts::CREATION_FILLER);
			objectCreationReqRepSocket->send(*foodRenderingMsg, zmq::send_flags::none);
			objectCreationReqRepSocket->recv(connectDisconnectRequest, zmq::recv_flags::none);
			zmq::message_t* snakeBlockRenderingMsg = prepCreationMessage(ClientServerConsts::PROPERTY_RENDERING,
				snakeBlockRendering, ClientServerConsts::CREATION_FILLER,
				ClientServerConsts::CREATION_FILLER, ClientServerConsts::CREATION_FILLER,
				ClientServerConsts::CREATION_FILLER, ClientServerConsts::CREATION_FILLER);
			objectCreationReqRepSocket->send(*snakeBlockRenderingMsg, zmq::send_flags::none);

			//send messages to client to create borders
			std::vector<int> borders = borderLocationsInSpace->getObjects();
			int bordersNum = borders.size();
			for (int i = 0; i < bordersNum; i++)
			{
				objectCreationReqRepSocket->recv(connectDisconnectRequest, zmq::recv_flags::none);
				sf::Shape* borderShape = borderLocationsInSpace->getObjectShape(borders[i]);
				sf::RectangleShape* borderRectangle = (sf::RectangleShape*) borderLocationsInSpace->getObjectShape(borders[i]);
				int borderType;

				//need to know if border is horizontal or vertical
				if (borderRectangle->getSize().x > borderRectangle->getSize().y)
				{
					borderType = ClientServerConsts::HORIZONTAL_BORDER;
				}
				else
				{
					borderType = ClientServerConsts::VERTICAL_BORDER;
				}

				zmq::message_t* borderCreationMsg = prepCreationMessage(borderType,
					borders[i], borderLocationsInSpace->getId(),
					borderRendering, borderShape->getFillColor().toInteger(),
					borderShape->getPosition().x, borderShape->getPosition().y);
				objectCreationReqRepSocket->send(*borderCreationMsg, zmq::send_flags::none);
			}

			//send messages to client to create food
			std::vector<int> foodPieces = foodLocationsInSpace->getObjects();
			int foodPiecesNum = foodPieces.size();
			for (int i = 0; i < foodPiecesNum; i++)
			{
				objectCreationReqRepSocket->recv(connectDisconnectRequest, zmq::recv_flags::none);
				sf::Shape* foodPieceShape = foodLocationsInSpace->getObjectShape(foodPieces[i]);
				zmq::message_t* foodPieceCreationMsg = prepCreationMessage(ClientServerConsts::FOOD_PIECE,
					foodPieces[i], foodLocationsInSpace->getId(),
					foodRendering, foodPieceShape->getFillColor().toInteger(),
					foodPieceShape->getPosition().x, foodPieceShape->getPosition().y);
				objectCreationReqRepSocket->send(*foodPieceCreationMsg, zmq::send_flags::none);
			}

			//send messages to client to create snake blocks
			std::vector<int> snakeBlocks = snakeBlockLocationsInSpace->getObjects();
			int snakeBlocksNum = snakeBlocks.size();
			for (int i = 0; i < snakeBlocksNum; i++)
			{
				objectCreationReqRepSocket->recv(connectDisconnectRequest, zmq::recv_flags::none);
				sf::Shape* snakeBlockShape = snakeBlockLocationsInSpace->getObjectShape(snakeBlocks[i]);
				zmq::message_t* snakeBlockCreationMsg = prepCreationMessage(ClientServerConsts::SNAKE_BLOCK,
					snakeBlocks[i], snakeBlockLocationsInSpace->getId(),
					snakeBlockRendering, snakeBlockShape->getFillColor().toInteger(),
					snakeBlockShape->getPosition().x, snakeBlockShape->getPosition().y);
				objectCreationReqRepSocket->send(*snakeBlockCreationMsg, zmq::send_flags::none);
			}

			//send message to client indicating end of object/property creation
			objectCreationReqRepSocket->recv(connectDisconnectRequest, zmq::recv_flags::none);
			zmq::message_t* endCreationMsg = prepCreationMessage(ClientServerConsts::CREATION_END, ClientServerConsts::CREATION_FILLER,
				ClientServerConsts::CREATION_FILLER, ClientServerConsts::CREATION_FILLER, ClientServerConsts::CREATION_FILLER,
				ClientServerConsts::CREATION_FILLER, ClientServerConsts::CREATION_FILLER);
			objectCreationReqRepSocket->send(*endCreationMsg, zmq::send_flags::none);

			//publish connection message to all clients so they can add new snake head
			std::string connectMsgString = std::to_string(ClientServerConsts::CONNECT_CODE) + " " + std::to_string(snakeHeadId) + " "
				+ std::to_string(snakeBlockLocationsInSpace->getId()) + " " + std::to_string(snakeBlockRendering)
				+ " " + std::to_string(snakeHeadShape->getPosition().x) + " " + std::to_string(snakeHeadShape->getPosition().y);
			zmq::message_t connectMsg(connectMsgString.length() + 1);
			const char* connectChars = connectMsgString.c_str();
			memcpy(connectMsg.data(), connectChars, connectMsgString.length() + 1);
			connectDisconnectPubSubSocket->send(connectMsg, zmq::send_flags::none);

			clientConnected = true;
			return true;
		}
		//if client is disconnecting, remove client's snake from all properties in which it exists, inform thread to stop running,
		//send back message with success code, and publish disconnection update to all clients
		else
		{
			std::vector<int> snakeBlocks = snakeBlockLocationsInSpace->getObjects();
			int snakeBlocksNum = snakeBlocks.size();
			for (int i = 0; i < snakeBlocksNum; i++)
			{
				snakeBlockLocationsInSpace->removeObject(snakeBlocks[i]);
				snakeBlockCollisions->removeObject(snakeBlocks[i]);
			}

			snakeBlockChainedDirectionalMovements->removeChain(connectionObjectId);
			snakeHeadRespawning->removeObject(connectionObjectId);

			//send message with disconnect success code
			std::string replyString = std::to_string(ClientServerConsts::DISCONNECT_SUCCESS_CODE);
			zmq::message_t replyMsg(replyString.length() + 1);
			const char* replyChars = replyString.c_str();
			memcpy(replyMsg.data(), replyChars, replyString.length() + 1);
			connectDisconnectReqRepSocket->send(replyMsg, zmq::send_flags::none);

			//publish disconnection update to all clients
			std::string updateString = std::to_string(ClientServerConsts::DISCONNECT_CODE) + " " + std::to_string(connectionObjectId)
				+ " " + std::to_string(snakeBlockLocationsInSpace->getId()) + " " + std::to_string(snakeBlockRendering)
				+ " " + std::to_string(ClientServerConsts::CONNECTION_FILLER) + " "
				+ std::to_string(ClientServerConsts::CONNECTION_FILLER);
			zmq::message_t updateMsg(updateString.length() + 1);
			const char* updateChars = updateString.c_str();
			memcpy(updateMsg.data(), updateChars, updateString.length() + 1);
			connectDisconnectPubSubSocket->send(updateMsg, zmq::send_flags::none);

			clientConnected = false;
			return false;
		}
	}

	return false;
}

int main()
{
	//seed random number generator
	srand(time(0));

	//initialize port modifier and GUID counter
	eventPortModifier = 0;
	idCounter = 0;
	clientConnected = false;

	// Prepare our context and sockets
	context = new zmq::context_t(1);
	connectDisconnectReqRepSocket = new zmq::socket_t(*context, ZMQ_REP);
	connectDisconnectPubSubSocket = new zmq::socket_t(*context, ZMQ_PUB);
	movementUpdatePubSubSocket = new zmq::socket_t(*context, ZMQ_PUB);
	objectCreationReqRepSocket = new zmq::socket_t(*context, ZMQ_REP);

	//bind sockets
	connectDisconnectReqRepSocket->bind(ClientServerConsts::CONNECT_DISCONNECT_REQ_REP_SERVER_PORT);
	connectDisconnectPubSubSocket->bind(ClientServerConsts::CONNECT_DISCONNECT_PUB_SUB_SERVER_PORT);
	connectDisconnectPubSubSocket->set(zmq::sockopt::sndhwm, 20);
	movementUpdatePubSubSocket->bind(ClientServerConsts::MOVEMENT_UPDATE_PUB_SUB_SERVER_PORT);
	movementUpdatePubSubSocket->set(zmq::sockopt::sndhwm, 20);
	objectCreationReqRepSocket->bind(ClientServerConsts::OBJECT_CREATION_REQ_REP_SERVER_PORT);

	/*create border shapes*/
	sf::RectangleShape* northBorderShape = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::HORIZONTAL_BORDER_WIDTH,
		ClientServerConsts::HORIZONTAL_BORDER_HEIGHT));
	northBorderShape->setFillColor(sf::Color::White);
	northBorderShape->setPosition(sf::Vector2f(0.f, 0.f));
	northBorderId = getNextId();
	sf::RectangleShape* southBorderShape = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::HORIZONTAL_BORDER_WIDTH,
		ClientServerConsts::HORIZONTAL_BORDER_HEIGHT));
	southBorderShape->setFillColor(sf::Color::White);
	southBorderShape->setPosition(sf::Vector2f(0.f, 570.f));
	southBorderId = getNextId();
	sf::RectangleShape* westBorderShape = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::VERTICAL_BORDER_WIDTH,
		ClientServerConsts::VERTICAL_BORDER_HEIGHT));
	westBorderShape->setFillColor(sf::Color::White);
	westBorderShape->setPosition(sf::Vector2f(0.f, 0.f));
	westBorderId = getNextId();
	sf::RectangleShape* eastBorderShape = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::VERTICAL_BORDER_WIDTH,
		ClientServerConsts::VERTICAL_BORDER_HEIGHT));
	eastBorderShape->setFillColor(sf::Color::White);
	eastBorderShape->setPosition(sf::Vector2f(770.f, 0.f));
	eastBorderId = getNextId();

	//define border properties
	borderLocationsInSpace = new LocationInSpace(getNextId(), &propertyMap);
	propertyMap.insert(std::pair<int, Property*>(borderLocationsInSpace->getId(), borderLocationsInSpace));
	borderLocationsInSpace->addObject(northBorderId, northBorderShape);
	borderLocationsInSpace->addObject(southBorderId, southBorderShape);
	borderLocationsInSpace->addObject(westBorderId, westBorderShape);
	borderLocationsInSpace->addObject(eastBorderId, eastBorderShape);
	borderCollisions = new Collision(getNextId(), &propertyMap);
	propertyMap.insert(std::pair<int, Property*>(borderCollisions->getId(), borderCollisions));
	borderCollisions->addObject(northBorderId, borderLocationsInSpace->getId());
	borderCollisions->addObject(southBorderId, borderLocationsInSpace->getId());
	borderCollisions->addObject(westBorderId, borderLocationsInSpace->getId());
	borderCollisions->addObject(eastBorderId, borderLocationsInSpace->getId());
	//only need id of rendering to send to client
	borderRendering = getNextId();

	/*make food spawn point shapes*/
	sf::RectangleShape* foodSpawnPointShape1 = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::FOOD_WIDTH,
		ClientServerConsts::FOOD_HEIGHT));
	foodSpawnPointShape1->setPosition(sf::Vector2f(100.f, 100.f));
	foodSpawnPointIds.push_back(getNextId());
	sf::RectangleShape* foodSpawnPointShape2 = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::FOOD_WIDTH,
		ClientServerConsts::FOOD_HEIGHT));
	foodSpawnPointShape2->setPosition(sf::Vector2f(300.f, 320.f));
	foodSpawnPointIds.push_back(getNextId());
	sf::RectangleShape* foodSpawnPointShape3 = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::FOOD_WIDTH,
		ClientServerConsts::FOOD_HEIGHT));
	foodSpawnPointShape3->setPosition(sf::Vector2f(450.f, 200.f));
	foodSpawnPointIds.push_back(getNextId());
	sf::RectangleShape* foodSpawnPointShape4 = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::FOOD_WIDTH,
		ClientServerConsts::FOOD_HEIGHT));
	foodSpawnPointShape4->setPosition(sf::Vector2f(680.f, 60.f));
	foodSpawnPointIds.push_back(getNextId());
	sf::RectangleShape* foodSpawnPointShape5 = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::FOOD_WIDTH,
		ClientServerConsts::FOOD_HEIGHT));
	foodSpawnPointShape5->setPosition(sf::Vector2f(680.f, 500.f));
	foodSpawnPointIds.push_back(getNextId());
	sf::RectangleShape* foodSpawnPointShape6 = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::FOOD_WIDTH,
		ClientServerConsts::FOOD_HEIGHT));
	foodSpawnPointShape6->setPosition(sf::Vector2f(100.f, 400.f));
	foodSpawnPointIds.push_back(getNextId());
	sf::RectangleShape* foodSpawnPointShape7 = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::FOOD_WIDTH,
		ClientServerConsts::FOOD_HEIGHT));
	foodSpawnPointShape7->setPosition(sf::Vector2f(50.f, 530.f));
	foodSpawnPointIds.push_back(getNextId());
	sf::RectangleShape* foodSpawnPointShape8 = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::FOOD_WIDTH,
		ClientServerConsts::FOOD_HEIGHT));
	foodSpawnPointShape8->setPosition(sf::Vector2f(450.f, 50.f));
	foodSpawnPointIds.push_back(getNextId());
	sf::RectangleShape* foodSpawnPointShape9 = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::FOOD_WIDTH,
		ClientServerConsts::FOOD_HEIGHT));
	foodSpawnPointShape9->setPosition(sf::Vector2f(100.f, 320.f));
	foodSpawnPointIds.push_back(getNextId());
	sf::RectangleShape* foodSpawnPointShape10 = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::FOOD_WIDTH,
		ClientServerConsts::FOOD_HEIGHT));
	foodSpawnPointShape10->setPosition(sf::Vector2f(200.f, 550.f));
	foodSpawnPointIds.push_back(getNextId());

	/*define food spawn point properties*/
	foodSpawnPointLocationsInSpace = new LocationInSpace(getNextId(), &propertyMap);
	propertyMap.insert(std::pair<int, Property*>(foodSpawnPointLocationsInSpace->getId(), foodSpawnPointLocationsInSpace));
	foodSpawnPointLocationsInSpace->addObject(foodSpawnPointIds[0], foodSpawnPointShape1);
	foodSpawnPointLocationsInSpace->addObject(foodSpawnPointIds[1], foodSpawnPointShape2);
	foodSpawnPointLocationsInSpace->addObject(foodSpawnPointIds[2], foodSpawnPointShape3);
	foodSpawnPointLocationsInSpace->addObject(foodSpawnPointIds[3], foodSpawnPointShape4);
	foodSpawnPointLocationsInSpace->addObject(foodSpawnPointIds[4], foodSpawnPointShape5);
	foodSpawnPointLocationsInSpace->addObject(foodSpawnPointIds[5], foodSpawnPointShape6);
	foodSpawnPointLocationsInSpace->addObject(foodSpawnPointIds[6], foodSpawnPointShape7);
	foodSpawnPointLocationsInSpace->addObject(foodSpawnPointIds[7], foodSpawnPointShape8);
	foodSpawnPointLocationsInSpace->addObject(foodSpawnPointIds[8], foodSpawnPointShape9);
	foodSpawnPointLocationsInSpace->addObject(foodSpawnPointIds[9], foodSpawnPointShape10);

	/*make food piece shapes*/
	sf::RectangleShape* foodPieceShape1 = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::FOOD_WIDTH,
		ClientServerConsts::FOOD_HEIGHT));
	foodPieceShape1->setFillColor(sf::Color::Red);
	//determine initial spawn point for food piece randomly based on those available
	int initialSpawnPointNum = rand() % foodSpawnPointIds.size();
	sf::Shape* initialSpawnPointShape = foodSpawnPointLocationsInSpace->getObjectShape(foodSpawnPointIds[initialSpawnPointNum]);
	foodPieceShape1->setPosition(initialSpawnPointShape->getPosition());
	food1Id = getNextId();

	/*make food piece properties*/
	foodLocationsInSpace = new LocationInSpace(getNextId(), &propertyMap);
	propertyMap.insert(std::pair<int, Property*>(foodLocationsInSpace->getId(), foodLocationsInSpace));
	foodLocationsInSpace->addObject(food1Id, foodPieceShape1);
	foodCollisions = new Collision(getNextId(), &propertyMap);
	propertyMap.insert(std::pair<int, Property*>(foodCollisions->getId(), foodCollisions));
	foodCollisions->addObject(food1Id, foodLocationsInSpace->getId());
	//only need id of rendering to send to client
	foodRendering = getNextId();
	foodRespawning = new Respawning(getNextId(), &propertyMap);
	propertyMap.insert(std::pair<int, Property*>(foodRespawning->getId(), foodRespawning));
	foodRespawning->addObject(food1Id, foodLocationsInSpace->getId(), Respawning::SpawnPoint(foodSpawnPointIds[initialSpawnPointNum],
		foodSpawnPointLocationsInSpace->getId()));

	/*make snake spawn point shapes*/
	sf::RectangleShape* snakeHeadSpawnPoint1 = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::SNAKE_BLOCK_WIDTH,
		ClientServerConsts::SNAKE_BLOCK_HEIGHT));
	snakeHeadSpawnPoint1->setPosition(sf::Vector2f(390.f, 290.f));
	snakeHeadSpawnPoint1Id = getNextId();

	/*make snake spawn point properties*/
	snakeHeadSpawnPointLocationsInSpace = new LocationInSpace(getNextId(), &propertyMap);
	propertyMap.insert(std::pair<int, Property*>(snakeHeadSpawnPointLocationsInSpace->getId(),
		snakeHeadSpawnPointLocationsInSpace));
	snakeHeadSpawnPointLocationsInSpace->addObject(snakeHeadSpawnPoint1Id, snakeHeadSpawnPoint1);

	/*make snake block properties*/
	snakeBlockLocationsInSpace = new LocationInSpace(getNextId(), &propertyMap);
	propertyMap.insert(std::pair<int, Property*>(snakeBlockLocationsInSpace->getId(), snakeBlockLocationsInSpace));
	snakeBlockCollisions = new Collision(getNextId(), &propertyMap);
	propertyMap.insert(std::pair<int, Property*>(snakeBlockCollisions->getId(), snakeBlockCollisions));
	RealTimeline* msTimeline = new RealTimeline(0.001f);
	GameTimeline* msMovementTimeline = new GameTimeline(1.f, msTimeline);
	//only need id of rendering to send to client
	snakeBlockRendering = getNextId();
	snakeBlockChainedDirectionalMovements = new ChainedDirectionalMovement(getNextId(), &propertyMap, msMovementTimeline);
	propertyMap.insert(std::pair<int, Property*>(snakeBlockChainedDirectionalMovements->getId(),
		snakeBlockChainedDirectionalMovements));
	snakeHeadRespawning = new Respawning(getNextId(), &propertyMap);
	propertyMap.insert(std::pair<int, Property*>(snakeHeadRespawning->getId(), snakeHeadRespawning));

	//prepare event manager
	eventManager = EventManager::getManager();
	eventManager->setTimeline(msTimeline);
	eventManager->setEventTypes({ ClientServerConsts::USER_INPUT_EVENT, ClientServerConsts::SNAKE_BLOCK_MOVED_EVENT,
		ClientServerConsts::SNAKE_HEAD_MOVED_EVENT, ClientServerConsts::FOOD_SPAWN_EVENT,
		ClientServerConsts::SNAKE_BLOCK_ADDED_EVENT, ClientServerConsts::SNAKE_HEAD_SPAWN_EVENT});

	//create handler for positional updates to client
	PositionalUpdateHandler* positionalUpdateHandler = new PositionalUpdateHandler(&propertyMap, true, movementUpdatePubSubSocket);
	eventManager->registerForEvent(ClientServerConsts::SNAKE_BLOCK_MOVED_EVENT, positionalUpdateHandler);
	eventManager->registerForEvent(ClientServerConsts::FOOD_SPAWN_EVENT, positionalUpdateHandler);
	eventManager->registerForEvent(ClientServerConsts::SNAKE_BLOCK_ADDED_EVENT, positionalUpdateHandler);
	eventManager->registerForEvent(ClientServerConsts::SNAKE_HEAD_SPAWN_EVENT, positionalUpdateHandler);

	//create handler for user input
	ScriptManager* userInputScriptManager = new ScriptManager();
	UserInputHandler* userInputHandler = new UserInputHandler(&propertyMap, false, snakeBlockChainedDirectionalMovements->getId(),
		userInputScriptManager, "handleUserInput.js");
	eventManager->registerForEvent(ClientServerConsts::USER_INPUT_EVENT, userInputHandler);

	//create handler for snake collisions
	SnakeCollisionHandler* snakeCollisionHandler = new SnakeCollisionHandler(&propertyMap, false, snakeBlockCollisions->getId(),
		foodCollisions->getId(), foodRespawning->getId(), foodLocationsInSpace->getId(), foodSpawnPointLocationsInSpace->getId(),
		snakeBlockChainedDirectionalMovements->getId(), snakeBlockLocationsInSpace->getId(), 
		snakeHeadRespawning->getId(), borderCollisions->getId(), &idCounter);
	eventManager->registerForEvent(ClientServerConsts::SNAKE_HEAD_MOVED_EVENT, snakeCollisionHandler);

	//create script manager to handle running and loading script for setting snake length growth
	ScriptManager* snakeLengthScriptManager = new ScriptManager();
	dukglue_register_method(snakeLengthScriptManager->getContext(), &SnakeCollisionHandler::setSnakeLengthGrowth,
		"setSnakeLengthGrowth");

	/****MAIN LOOP START*****/
	while (true)
	{
		//run script to set snake length growth
		if (snakeLengthScriptManager->loadScript("customizeSnakeGrowth.js"))
		{
			snakeLengthScriptManager->runScript("customizeSnakeGrowth", { ScriptManager::ArgumentType::TYPE_SNAKE_COLLISION_HANDLER },
				snakeCollisionHandler);
		}

		/*CHECK FOR CONNECTIONS/DISCONNECTIONS*/
		//process several updates
		for (int i = 0; i < 5; i++)
		{
			//if client connected, create thread to handle their event requests and update port modifier
			if (checkForConnectionUpdate())
			{
				std::thread* clientEventThread = new std::thread(handleClientEventRequests,
					ClientServerConsts::EVENT_RAISING_REQ_REP_NUM_START + eventPortModifier);
				eventPortModifier++;
			}
		}

		//sleep for a bit
		sleep(10);

		//process movement for snake chains
		std::vector<int> chainHeads = snakeBlockChainedDirectionalMovements->getHeads();
		int chainHeadsNum = chainHeads.size();
		for (int i = 0; i < chainHeadsNum; i++)
		{
			snakeBlockChainedDirectionalMovements->processMovement(chainHeads[i]);
			int currentObject = chainHeads[i];

			//raise movement event for each object in chain
			do
			{
				sf::Shape* objectShape = snakeBlockLocationsInSpace->getObjectShape(currentObject);
				struct Event::ArgumentVariant objectIdArg = { Event::ArgumentType::TYPE_INTEGER, currentObject };
				struct Event::ArgumentVariant locationInSpaceIdArg = { Event::ArgumentType::TYPE_INTEGER,
					snakeBlockLocationsInSpace->getId() };
				struct Event::ArgumentVariant absoluteXArg;
				absoluteXArg.argType = Event::ArgumentType::TYPE_FLOAT;
				absoluteXArg.argValue.argAsFloat = objectShape->getPosition().x;
				struct Event::ArgumentVariant absoluteYArg;
				absoluteYArg.argType = Event::ArgumentType::TYPE_FLOAT;
				absoluteYArg.argValue.argAsFloat = objectShape->getPosition().y;
				Event* snakeBlockMovementEvent = new Event(eventManager->getCurrentTime(), ClientServerConsts::SNAKE_BLOCK_MOVED_EVENT,
					{ objectIdArg, locationInSpaceIdArg, absoluteXArg, absoluteYArg });

				try
				{
					std::lock_guard<std::mutex> lock(queueLock);
					eventManager->raise(snakeBlockMovementEvent);
				}
				catch (...)
				{
					std::cerr << "Error while raising snake block movement event" << std::endl;
				}

				//set current object to next in chain
				currentObject = snakeBlockChainedDirectionalMovements->getNextInChain(currentObject);
			} while (currentObject != -1);

			//raise movement event specific to snake head to allow checking for collisions
			struct Event::ArgumentVariant headIdArg = { Event::ArgumentType::TYPE_INTEGER, chainHeads[i] };
			Event* snakeHeadMovementEvent = new Event(eventManager->getCurrentTime(), ClientServerConsts::SNAKE_HEAD_MOVED_EVENT,
				{ headIdArg });
			eventManager->raise(snakeHeadMovementEvent);
		}

		//handle scheduled events
		try
		{
			std::lock_guard<std::mutex> lock(queueLock);
			eventManager->handleEvents();
		}
		catch (...)
		{
			std::cerr << "Error while handling events" << std::endl;
		}

		//sleep for a bit
		sleep(10);
	}
	/****MAIN LOOP END****/

	/*DESTROY STATE*/
	//delete sockets
	delete(connectDisconnectReqRepSocket);
	delete(connectDisconnectPubSubSocket);
	delete(movementUpdatePubSubSocket);
	delete(context);
	//delete properties
	while (propertyMap.size() > 0)
	{
		int currentPropertyId = propertyMap.begin()->first;
		Property* currentProperty = propertyMap.begin()->second;
		propertyMap.erase(currentPropertyId);
		delete(currentProperty);
	}
	return 0;
}