#include "ScriptManager.h"
#include <iostream>
#include "dukglue/dukglue.h"
#include <fstream>
#include <cstdarg>

ScriptManager::ScriptManager()
{
	this->ctx = duk_create_heap_default();
	this->scriptLoaded = false;
	this->scriptName = "";
}

ScriptManager::~ScriptManager()
{
	duk_destroy_heap(this->ctx);
}

ScriptManager::ScriptManager(const ScriptManager& obj)
{
	std::cout << "ERROR: Cannot copy construct ScriptManager objects!" << std::endl;
	this->ctx = duk_create_heap_default();
	this->scriptLoaded = obj.scriptLoaded;
	this->scriptLastWriteTime = obj.scriptLastWriteTime;
	this->scriptName = obj.scriptName;
}

ScriptManager& ScriptManager::operator=(const ScriptManager& obj)
{
	std::cout << "ERROR: Cannot copy assign ScriptManager objects!" << std::endl;
	this->scriptLoaded = obj.scriptLoaded;
	this->scriptLastWriteTime = obj.scriptLastWriteTime;
	this->scriptName = obj.scriptName;
	return *this;
}

duk_context* ScriptManager::getContext()
{
	return this->ctx;
}

bool ScriptManager::loadScript(const std::string fileName)
{
	//get handle and file time for new script
	HANDLE newScriptHandle = CreateFile(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (newScriptHandle == INVALID_HANDLE_VALUE)
	{
		std::cerr << "Unable to create file" << std::endl;
		std::cerr << std::to_string(GetLastError()) << std::endl;
		CloseHandle(newScriptHandle);
		return false;
	}

	FILETIME newFileTime;
	if (!GetFileTime(newScriptHandle, NULL, NULL, &newFileTime))
	{
		std::cerr << "Unable to retrieve file time" << std::endl;
		std::cerr << std::to_string(GetLastError()) << std::endl;
		CloseHandle(newScriptHandle);
		return false;
	}

	CloseHandle(newScriptHandle);

	// If script is already loaded and is a different file than one we're trying to load, get rid of it
	if (this->scriptLoaded && fileName != scriptName)
	{
		std::cout << "Script is different than previous" << std::endl;

		//get rid of old script
		duk_pop(ctx);

		//set new time and name
		this->scriptLastWriteTime = newFileTime;
		this->scriptName = fileName;
	}
	//otherwise, if script is loaded and is same file as one we're trying to load, see if it needs to be reloaded
	else if (this->scriptLoaded && fileName == scriptName)
	{
		//if write times don't match, we need to reload
		if (scriptLastWriteTime.dwLowDateTime != newFileTime.dwLowDateTime
			|| scriptLastWriteTime.dwHighDateTime != newFileTime.dwHighDateTime)
		{
			std::cout << "Write times don't match, reloading script" << std::endl;

			//get rid of old script
			duk_pop(ctx);

			//set time
			this->scriptLastWriteTime = newFileTime;
		}
		//otherwise, no need to load script again
		else
		{
			return false;
		}
	}
	//no script loaded, so just set new time and name
	else
	{
		std::cout << "No script loaded" << std::endl;

		//set new time and name
		this->scriptLastWriteTime = newFileTime;
		this->scriptName = fileName;
	}

	// This piece of code simply reads the entire contents of the file and puts it in a string
	std::ifstream script_file = std::ifstream(fileName);
	std::stringstream script_contents_stream;
	script_contents_stream << script_file.rdbuf();
	std::string script_contents = script_contents_stream.str();

	// This pushes the contents of the file onto the stack
	duk_push_lstring(ctx, script_contents.c_str(), (duk_size_t)script_contents.length());
	// This ensures the contents was pushed correctly
	if (duk_peval(ctx) != 0) {
		printf("Error: %s\n", duk_safe_to_string(ctx, -1));
		exit(1);
	}

	// Clears the stack after the previous call
	duk_pop(ctx);
	duk_push_global_object(ctx);
	this->scriptLoaded = true;

	return true;
}

void ScriptManager::runScript(std::string functionName, std::vector<ScriptManager::ArgumentType> argTypes, ...)
{
	if (this->scriptLoaded)
	{
		// This indicates the name of the function within the script file that you wish to run
		duk_get_prop_string(ctx, -1, functionName.c_str());
		// This pushes all the passed in arguments onto the stack
		int argsNum = argTypes.size();
		va_list args;
		va_start(args, argTypes);
		for (int i = 0; i < argsNum; i++)
		{
			// pass in arguments according to their corresponding type
			switch (argTypes[i])
			{
				case ScriptManager::ArgumentType::TYPE_INTEGER:
					dukglue_push(this->ctx, va_arg(args, int));
					break;
				case ScriptManager::ArgumentType::TYPE_FLOAT:
					dukglue_push(this->ctx, va_arg(args, float));
					break;
				case ScriptManager::ArgumentType::TYPE_LOCATION_IN_SPACE:
					dukglue_push(this->ctx, va_arg(args, LocationInSpace*));
					break;
				case ScriptManager::ArgumentType::TYPE_CHAINED_DIRECTIONAL_MOVEMENT:
					dukglue_push(this->ctx, va_arg(args, ChainedDirectionalMovement*));
					break;
				case ScriptManager::ArgumentType::TYPE_SNAKE_COLLISION_HANDLER:
					dukglue_push(this->ctx, va_arg(args, SnakeCollisionHandler*));
					break;
				default:
					std::cerr << "Script argument has invalid type" << std::endl;
			}
		}
		va_end(args);

		//If you want your script function to return non-string values, or operate on/do something with return values, you will have to modify this
		if (duk_pcall(ctx, argsNum) != 0)
		{
			std::cout << "Error: ";
		}
		std::cout << duk_safe_to_string(ctx, -1) << std::endl;
		duk_pop(this->ctx);
	}
	else
	{
		std::cout << "ERROR: Cannot run script unless script is loaded, function is chosen, and arguments are given" << std::endl;
		exit(1);
	}
}
