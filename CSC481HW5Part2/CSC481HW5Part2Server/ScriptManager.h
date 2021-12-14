#pragma once
#include "duktape.h"
#include <string>
#include <vector>
#include "LocationInSpace.h"
#include "ChainedDirectionalMovement.h"
#include "SnakeCollisionHandler.h"
#include <Windows.h>

/*
* Class that manages loading and running arbitrary scripts used to modify game behavior.
* 
* Citation: Moodle example code used as starting point
*/
class ScriptManager
{
public:
	/*
	* Enum representing the possible types for script arguments.
	*/
	enum ArgumentType
	{
		TYPE_INTEGER,
		TYPE_FLOAT,
		TYPE_LOCATION_IN_SPACE,
		TYPE_CHAINED_DIRECTIONAL_MOVEMENT,
		TYPE_SNAKE_COLLISION_HANDLER
	};

private:
	/* context to handle scripts */
	duk_context* ctx;

	/* whether a script is currently loaded in the manager */
	bool scriptLoaded;

	/* time of last modification for currently loaded script */
	FILETIME scriptLastWriteTime;

	/* file name of currently loaded script */
	std::string scriptName;

public:
	/*
	* Constructs a ScriptManager by dynamically allocating its context.
	*/
	ScriptManager();

	/*
	* Destructs the ScriptManager by deallocating the memory for its context.
	*/
	~ScriptManager();

	/*
	* Copy constructor for ScriptManager to satisfy Rule of Three.
	* 
	* obj: ScriptManager to copy from
	*/
	ScriptManager(const ScriptManager& obj);

	/*
	* Copy operator for ScriptManager to satisfy Rule of Three.
	* 
	* obj: ScriptManager to copy from
	* 
	* returns: copied ScriptManager instance
	*/
	ScriptManager& operator=(const ScriptManager& obj);

	/*
	* Returns this ScriptManager's context.
	* 
	* returns: context
	*/
	duk_context* getContext();

	/*
	* Reads in the file with the given name, and puts its contents on the stack. If script was already loaded, only loads
	* script if it is a different file or same file but has been modified since last being loaded.
	* 
	* fileName: name of file containing script
	* 
	* returns: true if new script is loaded and false otherwise
	*/
	bool loadScript(const std::string fileName);

	/*
	* Runs the indicated function from the loaded script, passing in the indicated arguments.
	* 
	* functionName: name of function in loaded script to run
	* argTypes: types of arguments being passed in (in order)
	* ...: arguments being passed in
	* 
	* returns: string with message indicating results of function or set of arguments for event to be raised
	*/
	void runScript(std::string functionName, std::vector<ArgumentType>argTypes, ...);
};

