#pragma once
#include <string>
/*
* Namespace defining constant values used in server and clients.
*/
namespace ClientServerConsts
{

	/* connection/disconnection codes */
	static const int CONNECT_CODE = -1; //code used to indicate making a connection
	static const int DISCONNECT_CODE = -2; //code used to indicate disconnecting
	static const int CONNECT_ERROR_CODE = -3; //code used to indicate an error in connecting
	static const int DISCONNECT_SUCCESS_CODE = -4; //code used to indicate a successful disconnection
	static const int CONNECTION_FILLER = -5; //filler to fill up unused params

	/* server socket ports */
	static const char* CONNECT_DISCONNECT_REQ_REP_SERVER_PORT = "tcp://*:5555"; //port for receiving connection/disconnection requests
	static const char* CONNECT_DISCONNECT_PUB_SUB_SERVER_PORT = "tcp://*:5556"; //port for publishing connections/disconnections
	static const char* MOVEMENT_UPDATE_PUB_SUB_SERVER_PORT = "tcp://*:5557"; //port for publishing changes in positions of objects
	static const char* OBJECT_CREATION_REQ_REP_SERVER_PORT = "tcp://*:5560"; //port for sending object creation messages to connecting clients
	static const char* EVENT_RAISING_REQ_REP_SERVER_PORT_START = "tcp://*:"; //port start for receiving event raising messages from clients
	static const int EVENT_RAISING_REQ_REP_NUM_START = 5561; //starting number for event raising ports

	/* client socket ports */
	static const char* CONNECT_DISCONNECT_REQ_REP_CLIENT_PORT = "tcp://localhost:5555"; //port for sending connection/disconnection requests
	static const char* CONNECT_DISCONNECT_PUB_SUB_CLIENT_PORT = "tcp://localhost:5556"; //port for receiving published connections/disconnections
	static const char* MOVEMENT_UPDATE_PUB_SUB_CLIENT_PORT = "tcp://localhost:5557"; //port for receiving published positional updates
	static const char* OBJECT_CREATION_REQ_REP_CLIENT_PORT = "tcp://localhost:5560"; //port for receiving object creation messages from server while connecting
	static const char* EVENT_RAISING_REQ_REP_CLIENT_PORT_START = "tcp://localhost:"; //port start for sending event raising messages to server

	/* window attributes */
	static const int WINDOW_WIDTH = 800; //width of window
	static const int WINDOW_HEIGHT = 600; //height of window
	static const char* WINDOW_TITLE = "CSC 481 - HW5";

	/*snake block attributes*/
	static const float SNAKE_BLOCK_WIDTH = 20.f;
	static const float SNAKE_BLOCK_HEIGHT = 20.f;
	static const float SNAKE_BLOCK_VELOCITY = 0.3f;

	/*border attributes*/
	static const float HORIZONTAL_BORDER_WIDTH = 800.f;
	static const float HORIZONTAL_BORDER_HEIGHT = 30.f;
	static const float VERTICAL_BORDER_WIDTH = 30.f;
	static const float VERTICAL_BORDER_HEIGHT = 600.f;

	/*food attributes*/
	static const float FOOD_WIDTH = 15.f;
	static const float FOOD_HEIGHT = 15.f;

	/*property and object types*/
	static const int PROPERTY_LOCATION_IN_SPACE = 0;
	static const int PROPERTY_RENDERING = 1;
	static const int SNAKE_BLOCK = 2;
	static const int HORIZONTAL_BORDER = 3;
	static const int VERTICAL_BORDER = 4;
	static const int FOOD_PIECE = 5;

	/*filler code for object creation*/
	static const int CREATION_FILLER = 0;

	/*number of params in creation message*/
	static const int CREATION_PARAM_NUM = 7;

	/*code indicating end of object/property creation messages*/
	static const int CREATION_END = -1;

	/*positional update codes*/
	static const int POSITIONAL_UPDATE_FILLER = -1;
	static const int POSITIONAL_UPDATE_HEAD_RESPAWN_CODE = -2;

	/* event types */
	static const std::string CLIENT_DISCONNECT_EVENT = "ClientDisconnectEvent"; //no args; sent by client to stop thread for its event raising
	static const std::string USER_INPUT_EVENT = "UserInputEvent"; //args: snake head id and key type
	static const std::string SNAKE_COLLISION_EVENT = "SnakeCollisionEvent"; //args: snake head id, colliding object id
	static const std::string SNAKE_DEATH_EVENT = "SnakeDeathEvent"; //args: snake head id
	static const std::string SNAKE_BLOCK_MOVED_EVENT = "SnakeBlockMovedEvent"; /*args: snake block id, location in space id,
																			new absolute x, new absolute y*/
	static const std::string SNAKE_HEAD_MOVED_EVENT = "SnakeHeadMovedEvent"; //args: snake head id
	static const std::string FOOD_SPAWN_EVENT = "FoodSpawnEvent"; /*args: food piece id, location in space id,
																  new absolute x, new absolute y*/
	static const std::string SNAKE_BLOCK_ADDED_EVENT = "SnakeBlockAddedEvent"; /*args: snake block id, location in space id,
																			new absolute x, new absolute y*/
	static const std::string SNAKE_HEAD_SPAWN_EVENT = "SnakeHeadSpawnEvent"; /*args: snake head id, location in space id,
																			new absolute x, new absolute y*/

	/* event type codes (for client sending to server) */
	static const int CLIENT_DISCONNECT_EVENT_CODE = 1;
	static const int USER_INPUT_EVENT_CODE = 2;

	/* key types for user input */
	static const int NORTH_KEY = 1;
	static const int SOUTH_KEY = 2;
	static const int WEST_KEY = 3;
	static const int EAST_KEY = 4;

	/* script function names for particular events */
}