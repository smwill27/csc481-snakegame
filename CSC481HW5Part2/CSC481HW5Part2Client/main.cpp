#include <zmq.hpp>
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include "ClientServerConsts.h"
#include "Property.h"
#include "LocationInSpace.h"
#include "Rendering.h"
#include <windows.h>
#include "dukglue/dukglue.h"
#include "ScriptManager.h"
#define sleep(n)    Sleep(n)

/* context for all sockets */
zmq::context_t* context;

/* socket used for sending connection/disconnection requests to server */
zmq::socket_t* connectDisconnectReqRepSocket;

/* socket used for receiving connection/disconnection updates from server */
zmq::socket_t* connectDisconnectPubSubSocket;

/* socket used for receiving moving platform updates from server */
zmq::socket_t* movementUpdatePubSubSocket;

/* socket used for receiving object creation messages from server upon connecting */
zmq::socket_t* objectCreationReqRepSocket;

/* socket used for raising events on server */
zmq::socket_t* eventRaisingReqRepSocket;

/* id corresponding to snake head controlled by this client */
int snakeHeadId;

/* map with all properties */
std::map<int, Property*> propertyMap;

/* list of rendering property ids to use in drawing objects to screen */
std::vector<int> renderingIds;

/* value indicating whether the window is selected (and so input should be registered) */
bool windowInFocus;

/*manager for handling running and loading of customization scripts*/
ScriptManager* customizationScriptManager;

/* color to use in drawing window */
sf::Color backgroundColor;

/*id corresponding to LocationInSpace for borders*/
int borderLocationInSpaceId;

/*id corresponding to LocationInSpace for snake blocks*/
int snakeBlockLocationInSpaceId;

/*id corresponding to LocationInSpace for food pieces*/
int foodPieceLocationInSpaceId;

/*id corresponding to rendering for borders*/
int borderRenderingId;

/*id corresponding to rendering for food pieces*/
int foodPieceRenderingId;

/*id corresponding to Rendering for snake blocks*/
int snakeBlockRenderingId;

/* keys used by user to change direction of snake (can be set by script to custom ones) */
sf::Keyboard::Key northKey;
sf::Keyboard::Key southKey;
sf::Keyboard::Key westKey;
sf::Keyboard::Key eastKey;

/* name of script used to customize game elements such as colors and keys */
const std::string CUSTOMIZATION_SCRIPT_NAME = "customization.js";

/*value indicating whether a snake block was added this iteration (used to help determine whether to run color script)*/
bool snakeBlockAdded;

/*client's current score*/
int currentScore;

/*client's current highest score (not persistent)*/
int highScore;

/*points the client earns for each food piece consumed*/
int pointsPerFoodPiece;

/*whether the font for the scores was successfully loaded*/
bool fontLoaded;

/*font to use for the scores*/
sf::Font scoreFont;

/*text objects used to display high and current scores*/
sf::Text currentScoreLabelText;
sf::Text highScoreLabelText;
sf::Text currentScoreText;
sf::Text highScoreText;

/*
* Sets the background color to the color defined by the given int. Meant to allow a script to adjust the background color.
* 
* color: int representing color to set
*/
void setBackgroundColor(unsigned int color)
{
	backgroundColor = sf::Color(color);
}

/*
* Sets the specified key type to the specified key. Meant to allow a script to set a custom key for a particular function.
* 
* keyType: type of key being adjusted
* newKey: int representing new key to use for this function (defined by sf::Keyboard::Key)
*/
void setKeyUsed(int keyType, int newKey)
{
	if (keyType == ClientServerConsts::NORTH_KEY)
	{
		northKey = sf::Keyboard::Key(newKey);
	}
	else if (keyType == ClientServerConsts::SOUTH_KEY)
	{
		southKey = sf::Keyboard::Key(newKey);
	}
	else if (keyType == ClientServerConsts::WEST_KEY)
	{
		westKey = sf::Keyboard::Key(newKey);
	}
	else if (keyType == ClientServerConsts::EAST_KEY)
	{
		eastKey = sf::Keyboard::Key(newKey);
	}
}

/*
* Sets the text for the scores to the color represented by the given int. Meant to allow script to adjust the text color.
* 
* color: int representing color to set for score text
*/
void setTextColor(unsigned int color)
{
	if (fontLoaded)
	{
		currentScoreLabelText.setFillColor(sf::Color(color));
		highScoreLabelText.setFillColor(sf::Color(color));
		currentScoreText.setFillColor(sf::Color(color));
		highScoreText.setFillColor(sf::Color(color));
	}
}

int main()
{
	/*PREPARE STATE*/
	backgroundColor = sf::Color::Black;
	borderLocationInSpaceId = -1;
	snakeBlockLocationInSpaceId = -1;
	foodPieceLocationInSpaceId = -1;
	snakeBlockRenderingId = -1;
	borderRenderingId = -1;
	foodPieceRenderingId = -1;
	currentScore = 0;
	highScore = 0;
	pointsPerFoodPiece = 1;

	// Prepare our context and sockets
	context = new zmq::context_t(1);
	connectDisconnectReqRepSocket = new zmq::socket_t(*context, ZMQ_REQ);
	connectDisconnectPubSubSocket = new zmq::socket_t(*context, ZMQ_SUB);
	movementUpdatePubSubSocket = new zmq::socket_t(*context, ZMQ_SUB);
	objectCreationReqRepSocket = new zmq::socket_t(*context, ZMQ_REQ);
	const char* filter1 = "";
	const char* filter2 = "";
	connectDisconnectPubSubSocket->set(zmq::sockopt::subscribe, filter1);
	movementUpdatePubSubSocket->set(zmq::sockopt::subscribe, filter2);

	//std::cout << "prepared context and sockets" << std::endl;

	//connect req/rep socket
	connectDisconnectReqRepSocket->connect(ClientServerConsts::CONNECT_DISCONNECT_REQ_REP_CLIENT_PORT);
	objectCreationReqRepSocket->connect(ClientServerConsts::OBJECT_CREATION_REQ_REP_CLIENT_PORT);

	//std::cout << "connected req/rep socket" << std::endl;

	//create an 800x600 window with a titlebar that can be resized and closed
	sf::RenderWindow window(sf::VideoMode(ClientServerConsts::WINDOW_WIDTH, ClientServerConsts::WINDOW_HEIGHT),
		ClientServerConsts::WINDOW_TITLE, sf::Style::Default);
	windowInFocus = true;

	/*SEND CONNECTION REQUEST*/
	//send request to server to initialize connection
	std::string connectMsgString = std::to_string(ClientServerConsts::CONNECT_CODE);
	zmq::message_t connectMsg(connectMsgString.length() + 1);
	const char* connectMsgChars = connectMsgString.c_str();
	memcpy(connectMsg.data(), connectMsgChars, connectMsgString.length() + 1);
	connectDisconnectReqRepSocket->send(connectMsg, zmq::send_flags::none);

	//std::cout << "sent request to server for connection" << std::endl;

	//get response from server confirming connection
	zmq::message_t connectReply;
	connectDisconnectReqRepSocket->recv(connectReply, zmq::recv_flags::none);
	int connectionId = -1;
	int portNum = -1;
	int scan = sscanf_s(connectReply.to_string().c_str(), "%d %d", &connectionId, &portNum);
	//std::cout << "Received from server: " + connectReply.to_string() << std::endl;

	//if connection failed, close window and exit
	if (scan != 2 || connectionId == ClientServerConsts::CONNECT_ERROR_CODE)
	{
		/*TODO: CLOSE WINDOW AND CLEAN UP*/
		window.close();
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
		std::cerr << "Failed to connect to server" << std::endl;
		return 0;
	}

	//if connection didn't fail, create objects as dictated by server
	//set id of snake head being controlled by this client
	snakeHeadId = connectionId;

	/* repeatedly check for object creation messages and create properties/objects indicated */
	bool moreToCreate = true;
	while (moreToCreate)
	{
		//send filler message
		std::string creationMsgString = std::to_string(ClientServerConsts::CREATION_FILLER);
		zmq::message_t creationMsg(creationMsgString.length() + 1);
		const char* creationMsgChars = creationMsgString.c_str();
		memcpy(creationMsg.data(), creationMsgChars, creationMsgString.length() + 1);
		objectCreationReqRepSocket->send(creationMsg, zmq::send_flags::none);

		//receive and parse out response
		zmq::message_t creationReply;
		objectCreationReqRepSocket->recv(creationReply, zmq::recv_flags::none);
		int objectOrPropertyType = -1;
		int objectOrPropertyId = -1;
		int locationInSpaceId = -1;
		int renderingId = -1;
		int colorInt = -1;
		float x = 0.f;
		float y = 0.f;
		int scan = sscanf_s(creationReply.to_string().c_str(), "%d %d %d %d %d %f %f", &objectOrPropertyType, &objectOrPropertyId,
			&locationInSpaceId, &renderingId, &colorInt, &x, &y);
		//std::cout << "Received from server: " + creationReply.to_string() << std::endl;

		//if end of property/object creation, don't send another message
		if (objectOrPropertyType == ClientServerConsts::CREATION_END)
		{
			moreToCreate = false;
		}
		//if location in space property, create it
		else if (objectOrPropertyType == ClientServerConsts::PROPERTY_LOCATION_IN_SPACE)
		{
			LocationInSpace* newLocationInSpace = new LocationInSpace(objectOrPropertyId, &propertyMap);
			propertyMap.insert(std::pair<int, Property*>(newLocationInSpace->getId(), newLocationInSpace));

			if (borderLocationInSpaceId == -1)
			{
				borderLocationInSpaceId = objectOrPropertyId;
			}
			else if (foodPieceLocationInSpaceId == -1)
			{
				foodPieceLocationInSpaceId = objectOrPropertyId;
			}
			else if (snakeBlockLocationInSpaceId == -1)
			{
				snakeBlockLocationInSpaceId = objectOrPropertyId;
			}
		}
		//if rendering property, create it
		else if (objectOrPropertyType == ClientServerConsts::PROPERTY_RENDERING)
		{
			Rendering* newRendering = new Rendering(objectOrPropertyId, &propertyMap, &window);
			propertyMap.insert(std::pair<int, Property*>(newRendering->getId(), newRendering));
			renderingIds.push_back(objectOrPropertyId);

			if (borderRenderingId == -1)
			{
				borderRenderingId = objectOrPropertyId;
			}
			else if (foodPieceRenderingId == -1)
			{
				foodPieceRenderingId = objectOrPropertyId;
			}
			else if (snakeBlockRenderingId == -1)
			{
				snakeBlockRenderingId = objectOrPropertyId;
			}
		}
		//if horizontal border, create it
		else if (objectOrPropertyType == ClientServerConsts::HORIZONTAL_BORDER)
		{
			sf::RectangleShape* newHorizontalBorderShape = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::HORIZONTAL_BORDER_WIDTH,
				ClientServerConsts::HORIZONTAL_BORDER_HEIGHT));
			newHorizontalBorderShape->setFillColor(sf::Color(colorInt));
			newHorizontalBorderShape->setPosition(sf::Vector2f(x, y));
			LocationInSpace* borderLocationInSpace = (LocationInSpace*)propertyMap.at(locationInSpaceId);
			borderLocationInSpace->addObject(objectOrPropertyId, newHorizontalBorderShape);
			Rendering* borderRendering = (Rendering*)propertyMap.at(renderingId);
			borderRendering->addObject(objectOrPropertyId, locationInSpaceId);
		}
		//if vertical border, create it
		else if (objectOrPropertyType == ClientServerConsts::VERTICAL_BORDER)
		{
			sf::RectangleShape* newVerticalBorderShape = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::VERTICAL_BORDER_WIDTH,
				ClientServerConsts::VERTICAL_BORDER_HEIGHT));
			newVerticalBorderShape->setFillColor(sf::Color(colorInt));
			newVerticalBorderShape->setPosition(sf::Vector2f(x, y));
			LocationInSpace* borderLocationInSpace = (LocationInSpace*)propertyMap.at(locationInSpaceId);
			borderLocationInSpace->addObject(objectOrPropertyId, newVerticalBorderShape);
			Rendering* borderRendering = (Rendering*)propertyMap.at(renderingId);
			borderRendering->addObject(objectOrPropertyId, locationInSpaceId);
		}
		//if snake block, create it
		else if (objectOrPropertyType == ClientServerConsts::SNAKE_BLOCK)
		{
			sf::RectangleShape* newSnakeBlockShape = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::SNAKE_BLOCK_WIDTH,
				ClientServerConsts::SNAKE_BLOCK_HEIGHT));
			newSnakeBlockShape->setFillColor(sf::Color(colorInt));
			newSnakeBlockShape->setPosition(sf::Vector2f(x, y));
			LocationInSpace* snakeBlockLocationInSpace = (LocationInSpace*)propertyMap.at(locationInSpaceId);
			snakeBlockLocationInSpace->addObject(objectOrPropertyId, newSnakeBlockShape);
			Rendering* snakeBlockRendering = (Rendering*)propertyMap.at(renderingId);
			snakeBlockRendering->addObject(objectOrPropertyId, locationInSpaceId);
		}
		//if food piece, create it
		else if (objectOrPropertyType == ClientServerConsts::FOOD_PIECE)
		{
			sf::RectangleShape* newFoodPieceShape = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::FOOD_WIDTH,
				ClientServerConsts::FOOD_HEIGHT));
			newFoodPieceShape->setFillColor(sf::Color(colorInt));
			newFoodPieceShape->setPosition(sf::Vector2f(x, y));
			LocationInSpace* foodLocationInSpace = (LocationInSpace*)propertyMap.at(locationInSpaceId);
			foodLocationInSpace->addObject(objectOrPropertyId, newFoodPieceShape);
			Rendering* foodRendering = (Rendering*)propertyMap.at(renderingId);
			foodRendering->addObject(objectOrPropertyId, locationInSpaceId);
		}
	}

	//create and connect event raising socket with given port number
	eventRaisingReqRepSocket = new zmq::socket_t(*context, ZMQ_REQ);
	eventRaisingReqRepSocket->connect(ClientServerConsts::EVENT_RAISING_REQ_REP_CLIENT_PORT_START + std::to_string(portNum));

	//subscribe for updates
	connectDisconnectPubSubSocket->connect(ClientServerConsts::CONNECT_DISCONNECT_PUB_SUB_CLIENT_PORT);
	movementUpdatePubSubSocket->connect(ClientServerConsts::MOVEMENT_UPDATE_PUB_SUB_CLIENT_PORT);

	//create script manager and set it up with method for changing color of everything as well as function for changing background
	//also function for changing key type and changing color of score text
	customizationScriptManager = new ScriptManager();
	dukglue_register_method(customizationScriptManager->getContext(), &LocationInSpace::setColorOfAll,
		"setColor");
	dukglue_register_function(customizationScriptManager->getContext(), &setBackgroundColor, "setBackground");
	dukglue_register_function(customizationScriptManager->getContext(), &setKeyUsed, "setKey");
	dukglue_register_function(customizationScriptManager->getContext(), &setTextColor, "setText");

	//set default values for direction keys
	northKey = sf::Keyboard::Key::Up;
	southKey = sf::Keyboard::Key::Down;
	westKey = sf::Keyboard::Key::Left;
	eastKey = sf::Keyboard::Key::Right;

	//values indicating whether a button was just pressed (used to support only one event being sent per key press)
	bool northKeyPressed = false;
	bool southKeyPressed = false;
	bool westKeyPressed = false;
	bool eastKeyPressed = false;

	//value indicating whether a direction key press has been sent this iteration (used to limit to one direction change per iteration)
	bool directionKeyPressSent;

	snakeBlockAdded = false;

	//load font for score text and create text objects
	fontLoaded = true;
	if (scoreFont.loadFromFile("arial.ttf"))
	{
		currentScoreLabelText.setFont(scoreFont);
		highScoreLabelText.setFont(scoreFont);
		currentScoreText.setFont(scoreFont);
		highScoreText.setFont(scoreFont);

		currentScoreLabelText.setString("Current Score:");
		highScoreLabelText.setString("High Score:");
		currentScoreText.setString(std::to_string(currentScore));
		highScoreText.setString(std::to_string(highScore));

		currentScoreLabelText.setCharacterSize(18);
		highScoreLabelText.setCharacterSize(18);
		currentScoreText.setCharacterSize(18);
		highScoreText.setCharacterSize(18);

		currentScoreLabelText.setFillColor(sf::Color::Cyan);
		highScoreLabelText.setFillColor(sf::Color::Cyan);
		currentScoreText.setFillColor(sf::Color::Cyan);
		highScoreText.setFillColor(sf::Color::Cyan);

		currentScoreLabelText.setPosition(sf::Vector2f(550.f, 5.f));
		highScoreLabelText.setPosition(sf::Vector2f(35.f, 5.f));
		currentScoreText.setPosition(sf::Vector2f(675.f, 5.f));
		highScoreText.setPosition(sf::Vector2f(135.f, 5.f));
	}
	else
	{
		fontLoaded = false;
		std::cerr << "Unable to load score font" << std::endl;
	}

	//initially load script and run to ensure colors and keys get initially set
	if (customizationScriptManager->loadScript(CUSTOMIZATION_SCRIPT_NAME))
	{
		LocationInSpace* borderLocationInSpace = (LocationInSpace*)propertyMap.at(borderLocationInSpaceId);
		LocationInSpace* snakeBlockLocationInSpace = (LocationInSpace*)propertyMap.at(snakeBlockLocationInSpaceId);
		LocationInSpace* foodPieceLocationInSpace = (LocationInSpace*)propertyMap.at(foodPieceLocationInSpaceId);
		customizationScriptManager->runScript("customizeColors", { ScriptManager::ArgumentType::TYPE_LOCATION_IN_SPACE,
			ScriptManager::ArgumentType::TYPE_LOCATION_IN_SPACE, ScriptManager::ArgumentType::TYPE_LOCATION_IN_SPACE }, 
			borderLocationInSpace, snakeBlockLocationInSpace, foodPieceLocationInSpace);
		customizationScriptManager->runScript("customizeKeys", {});
	}

	/***MAIN LOOP START***/
	while (true)
	{
		/*CHECK FOR WINDOW EVENTS AND SEND ANY NECESSARY UPDATES*/
		sf::Event event;
		//check for window events
		while (window.pollEvent(event))
		{
			//if a window close is requested, send disconnect request to server and close the window
			if (event.type == sf::Event::Closed)
			{
				//std::cout << "Sending disconnection request" << std::endl;
				//send request to server to disconnect
				std::string connectMsgString = std::to_string(snakeHeadId);
				zmq::message_t connectMsg(connectMsgString.length() + 1);
				const char* sChars = connectMsgString.c_str();
				memcpy(connectMsg.data(), sChars, connectMsgString.length() + 1);
				connectDisconnectReqRepSocket->send(connectMsg, zmq::send_flags::none);

				//get response from server confirming disconnection
				zmq::message_t disconnectReply;
				connectDisconnectReqRepSocket->recv(disconnectReply, zmq::recv_flags::none);
				//std::cout << "Received from server: " + disconnectReply.to_string() << std::endl;

				//send disconnect message to event port to let that thread stop
				std::string disconnectEventMsgString = ClientServerConsts::CLIENT_DISCONNECT_EVENT;
				zmq::message_t disconnectEventMsg(disconnectEventMsgString.length() + 1);
				const char* disconnectEventChars = disconnectEventMsgString.c_str();
				memcpy(disconnectEventMsg.data(), disconnectEventChars, disconnectEventMsgString.length() + 1);
				eventRaisingReqRepSocket->send(disconnectEventMsg, zmq::send_flags::none);

				//receive response
				zmq::message_t eventDisconnectReply;
				eventRaisingReqRepSocket->recv(eventDisconnectReply, zmq::recv_flags::none);

				//close window and destroy state
				window.close();
				//std::cout << "Deleting properties" << std::endl;
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

			//if window is no longer active, stop handling keyboard input
			if (event.type == sf::Event::LostFocus)
			{
				windowInFocus = false;
			}

			//if window is now active, resume handling keyboard input
			if (event.type == sf::Event::GainedFocus)
			{
				windowInFocus = true;
			}

			//if a window resize is requested, resize the window
			if (event.type == sf::Event::Resized)
			{
				//set the window to the given size
				window.setSize(sf::Vector2u(event.size.width, event.size.height));
			}
		}

		//see if we need to customize keys and colors before checking for user input
		if (customizationScriptManager->loadScript(CUSTOMIZATION_SCRIPT_NAME))
		{
			customizationScriptManager->runScript("customizeKeys", {});
			LocationInSpace* borderLocationInSpace = (LocationInSpace*)propertyMap.at(borderLocationInSpaceId);
			LocationInSpace* snakeBlockLocationInSpace = (LocationInSpace*)propertyMap.at(snakeBlockLocationInSpaceId);
			LocationInSpace* foodPieceLocationInSpace = (LocationInSpace*)propertyMap.at(foodPieceLocationInSpaceId);
			customizationScriptManager->runScript("customizeColors", { ScriptManager::ArgumentType::TYPE_LOCATION_IN_SPACE,
				ScriptManager::ArgumentType::TYPE_LOCATION_IN_SPACE, ScriptManager::ArgumentType::TYPE_LOCATION_IN_SPACE },
				borderLocationInSpace, snakeBlockLocationInSpace, foodPieceLocationInSpace);
		}

		/* check for user input and send appropriate events */
		if (windowInFocus && sf::Keyboard::isKeyPressed(northKey))
		{
			if (!northKeyPressed)
			{
				northKeyPressed = true;

				//std::cout << "Sending user input event to server" << std::endl;
				//send message to server to raise event
				std::string userInputString = std::to_string(ClientServerConsts::USER_INPUT_EVENT_CODE) + " "
					+ std::to_string(snakeHeadId) + " " + std::to_string(ClientServerConsts::NORTH_KEY);
				zmq::message_t userInputMsg(userInputString.length() + 1);
				const char* userInputChars = userInputString.c_str();
				memcpy(userInputMsg.data(), userInputChars, userInputString.length() + 1);
				eventRaisingReqRepSocket->send(userInputMsg, zmq::send_flags::none);

				//receive response
				zmq::message_t userInputReply;
				eventRaisingReqRepSocket->recv(userInputReply, zmq::recv_flags::none);
				//std::cout << "Received from server: " + userInputReply.to_string() << std::endl;
			}
		}
		else
		{
			northKeyPressed = false;
		}

		if (windowInFocus && sf::Keyboard::isKeyPressed(southKey))
		{
			if (!southKeyPressed)
			{
				southKeyPressed = true;

				//std::cout << "Sending user input event to server" << std::endl;
				//send message to server to raise event
				std::string userInputString = std::to_string(ClientServerConsts::USER_INPUT_EVENT_CODE) + " "
					+ std::to_string(snakeHeadId) + " " + std::to_string(ClientServerConsts::SOUTH_KEY);
				zmq::message_t userInputMsg(userInputString.length() + 1);
				const char* userInputChars = userInputString.c_str();
				memcpy(userInputMsg.data(), userInputChars, userInputString.length() + 1);
				eventRaisingReqRepSocket->send(userInputMsg, zmq::send_flags::none);

				//receive response
				zmq::message_t userInputReply;
				eventRaisingReqRepSocket->recv(userInputReply, zmq::recv_flags::none);
				//std::cout << "Received from server: " + userInputReply.to_string() << std::endl;
			}
		}
		else
		{
			southKeyPressed = false;
		}

		if (windowInFocus && sf::Keyboard::isKeyPressed(westKey))
		{
			if (!westKeyPressed)
			{
				westKeyPressed = true;

				//std::cout << "Sending user input event to server" << std::endl;
				//send message to server to raise event
				std::string userInputString = std::to_string(ClientServerConsts::USER_INPUT_EVENT_CODE) + " "
					+ std::to_string(snakeHeadId) + " " + std::to_string(ClientServerConsts::WEST_KEY);
				zmq::message_t userInputMsg(userInputString.length() + 1);
				const char* userInputChars = userInputString.c_str();
				memcpy(userInputMsg.data(), userInputChars, userInputString.length() + 1);
				eventRaisingReqRepSocket->send(userInputMsg, zmq::send_flags::none);

				//receive response
				zmq::message_t userInputReply;
				eventRaisingReqRepSocket->recv(userInputReply, zmq::recv_flags::none);
				//std::cout << "Received from server: " + userInputReply.to_string() << std::endl;
			}
		}
		else
		{
			westKeyPressed = false;
		}

		if (windowInFocus && sf::Keyboard::isKeyPressed(eastKey))
		{
			if (!eastKeyPressed)
			{
				eastKeyPressed = true;

				//std::cout << "Sending user input event to server" << std::endl;
				//send message to server to raise event
				std::string userInputString = std::to_string(ClientServerConsts::USER_INPUT_EVENT_CODE) + " "
					+ std::to_string(snakeHeadId) + " " + std::to_string(ClientServerConsts::EAST_KEY);
				zmq::message_t userInputMsg(userInputString.length() + 1);
				const char* userInputChars = userInputString.c_str();
				memcpy(userInputMsg.data(), userInputChars, userInputString.length() + 1);
				eventRaisingReqRepSocket->send(userInputMsg, zmq::send_flags::none);

				//receive response
				zmq::message_t userInputReply;
				eventRaisingReqRepSocket->recv(userInputReply, zmq::recv_flags::none);
				//std::cout << "Received from server: " + userInputReply.to_string() << std::endl;
			}
		}
		else
		{
			eastKeyPressed = false;
		}

		/*CHECK FOR SEVERAL CONNECTION UPDATES FROM SERVER AND PROCESS*/
		for (int i = 0; i < 5; i++)
		{
			zmq::message_t connectionUpdate;
			if (connectDisconnectPubSubSocket->recv(connectionUpdate, zmq::recv_flags::dontwait))
			{
				//parse out update
				int connectUpdateCode = 0;
				int connectUpdateId = 0;
				int locationInSpaceId = 0;
				int renderingId = 0;
				float x = 0.f;
				float y = 0.f;
				int scan = sscanf_s(connectionUpdate.to_string().c_str(), "%d %d %d %d %f %f", &connectUpdateCode, &connectUpdateId,
					&locationInSpaceId, &renderingId, &x, &y);
				//std::cout << "Received from server: " + connectionUpdate.to_string() << std::endl;

				//if message not as expected, print error message
				if (scan != 6
					|| (connectUpdateCode != ClientServerConsts::CONNECT_CODE && connectUpdateCode != ClientServerConsts::DISCONNECT_CODE))
				{
					std::cerr << "Error in communication when processing connection update from server" << std::endl;
				}
				//if update is connection, add character with given id
				else if (connectUpdateCode == ClientServerConsts::CONNECT_CODE && connectUpdateId != snakeHeadId)
				{
					//std::cout << "*********ADDING CHARACTER FOR NEW CLIENT*********" << std::endl;

					//create snake head shape
					sf::RectangleShape* nextSnakeHeadShape = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::SNAKE_BLOCK_WIDTH, 
						ClientServerConsts::SNAKE_BLOCK_HEIGHT));
					nextSnakeHeadShape->setFillColor(sf::Color::Green);
					nextSnakeHeadShape->setPosition(sf::Vector2f(x, y));

					//add snake head to properties
					LocationInSpace* snakeBlockLocationInSpace = (LocationInSpace*)propertyMap.at(locationInSpaceId);
					snakeBlockLocationInSpace->addObject(connectUpdateId, nextSnakeHeadShape);
					Rendering* snakeBlockRendering = (Rendering*)propertyMap.at(renderingId);
					snakeBlockRendering->addObject(connectUpdateId, locationInSpaceId);
				}
				//if update is disconnection, remove snake head with given id from all properties where it exists
				else
				{
					//std::cout << "******REMOVING CHARACTER FOR DISCONNECTING CLIENT*********" << std::endl;
					LocationInSpace* snakeBlockLocationInSpace = (LocationInSpace*)propertyMap.at(locationInSpaceId);
					snakeBlockLocationInSpace->removeObject(connectUpdateId);
					Rendering* snakeBlockRendering = (Rendering*)propertyMap.at(renderingId);
					snakeBlockRendering->removeObject(connectUpdateId);
				}
			}
		}

		/*CHECK FOR SEVERAL POSITION UPDATES FROM SERVER AND PROCESS*/
		for (int i = 0; i < 20; i++)
		{
			zmq::message_t positionUpdate;
			if (movementUpdatePubSubSocket->recv(positionUpdate, zmq::recv_flags::dontwait))
			{
				//parse out update
				int objectId = 0;
				int locationInSpaceId = 0;
				float x = 0.f;
				float y = 0.f;
				int scan = sscanf_s(positionUpdate.to_string().c_str(), "%d %d %f %f", &objectId, &locationInSpaceId, &x, &y);
				//std::cout << "Received from server: " + positionUpdate.to_string() << std::endl;

				//if message not as expected, print error message
				if (scan != 4)
				{
					std::cerr << "Error in communication when processing position update from server" << std::endl;
				}
				//otherwise, if snake head was respawned, remove all other snake blocks
				else if (objectId == ClientServerConsts::POSITIONAL_UPDATE_HEAD_RESPAWN_CODE)
				{
					LocationInSpace* snakeLocationInSpace = (LocationInSpace*)propertyMap.at(snakeBlockLocationInSpaceId);
					Rendering* snakeRendering = (Rendering*)propertyMap.at(snakeBlockRenderingId);

					std::vector<int> snakeBlocks = snakeLocationInSpace->getObjects();
					int snakeBlocksNum = snakeBlocks.size();
					for (int i = 0; i < snakeBlocksNum; i++)
					{
						if (snakeBlocks[i] != snakeHeadId)
						{
							snakeLocationInSpace->removeObject(snakeBlocks[i]);
							snakeRendering->removeObject(snakeBlocks[i]);
						}
					}

					//reset current score and potentially set high score
					if (currentScore > highScore)
					{
						highScore = currentScore;
					}

					currentScore = 0;
					
					if (fontLoaded)
					{
						currentScoreText.setString(std::to_string(currentScore));
						highScoreText.setString(std::to_string(highScore));
					}
				}
				//otherwise, process position update
				else
				{
					LocationInSpace* locationInSpace = (LocationInSpace*)propertyMap.at(locationInSpaceId);
					//make sure it has object (could be lingering update from removed character)
					if (locationInSpace->hasObject(objectId))
					{
						sf::Shape* objectShape = locationInSpace->getObjectShape(objectId);
						objectShape->setPosition(sf::Vector2f(x, y));
					}
					//if snake block and not already in property, it's being added, so create it
					else if (locationInSpaceId == snakeBlockLocationInSpaceId)
					{
						//create snake block shape
						sf::RectangleShape* nextSnakeBlockShape = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::SNAKE_BLOCK_WIDTH,
							ClientServerConsts::SNAKE_BLOCK_HEIGHT));
						nextSnakeBlockShape->setFillColor(sf::Color::Green);
						nextSnakeBlockShape->setPosition(sf::Vector2f(x, y));

						//add snake block to properties
						locationInSpace->addObject(objectId, nextSnakeBlockShape);
						Rendering* snakeBlockRendering = (Rendering*)propertyMap.at(snakeBlockRenderingId);
						snakeBlockRendering->addObject(objectId, locationInSpaceId);
						snakeBlockAdded = true;

						//update current score
						currentScore += pointsPerFoodPiece;
						if (fontLoaded)
						{
							currentScoreText.setString(std::to_string(currentScore));
						}
					}
				}
			}
		}

		//see if we need to run script to customize colors after adding a snake block
		if (snakeBlockAdded)
		{
			LocationInSpace* borderLocationInSpace = (LocationInSpace*)propertyMap.at(borderLocationInSpaceId);
			LocationInSpace* snakeBlockLocationInSpace = (LocationInSpace*)propertyMap.at(snakeBlockLocationInSpaceId);
			LocationInSpace* foodPieceLocationInSpace = (LocationInSpace*)propertyMap.at(foodPieceLocationInSpaceId);
			customizationScriptManager->runScript("customizeColors", { ScriptManager::ArgumentType::TYPE_LOCATION_IN_SPACE,
				ScriptManager::ArgumentType::TYPE_LOCATION_IN_SPACE, ScriptManager::ArgumentType::TYPE_LOCATION_IN_SPACE },
				borderLocationInSpace, snakeBlockLocationInSpace, foodPieceLocationInSpace);
			snakeBlockAdded = false;
		}

		/*DRAW TO WINDOW*/
		//clear the window with designated color
		window.clear(backgroundColor);

		//draw all objects
		int renderingNum = renderingIds.size();
		for (int i = 0; i < renderingNum; i++)
		{
			Rendering* nextRendering = (Rendering*)propertyMap.at(renderingIds[i]);
			nextRendering->drawAllObjects();
		}

		//draw score text
		if (fontLoaded)
		{
			window.draw(currentScoreLabelText);
			window.draw(highScoreLabelText);
			window.draw(currentScoreText);
			window.draw(highScoreText);
		}

		//end current frame
		window.display();


	}
	/***MAIN LOOP END***/

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