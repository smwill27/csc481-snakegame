#include "UserInputHandler.h"

UserInputHandler::UserInputHandler(std::map<int, Property*>* propertyMap, bool notifyWhileReplaying, int chainedDirectionalMovementId,
	ScriptManager* scriptManager, std::string userInputScriptName)
	: EventHandler(propertyMap, notifyWhileReplaying)
{
	this->chainedDirectionalMovementId = chainedDirectionalMovementId;
	this->scriptManager = scriptManager;
	this->userInputScriptName = userInputScriptName;
	dukglue_register_function(scriptManager->getContext(), &UserInputHandler::setDirection , "setDirection");
}

void UserInputHandler::onEvent(Event* e)
{
	//get event parameters and ChainedDirectionalMovement property and let script handle based on key type
	if (e->getType() == ClientServerConsts::USER_INPUT_EVENT)
	{
		int snakeHeadId = e->getArgument(0).argValue.argAsInt;
		int keyType = e->getArgument(1).argValue.argAsInt;
		ChainedDirectionalMovement* chainedDirectionalMovement =
			(ChainedDirectionalMovement*)propertyMap->at(chainedDirectionalMovementId);

		if (chainedDirectionalMovement->hasObject(snakeHeadId))
		{
			scriptManager->loadScript(userInputScriptName);
			scriptManager->runScript("handleUserInput", { ScriptManager::ArgumentType::TYPE_INTEGER,
				ScriptManager::ArgumentType::TYPE_INTEGER, ScriptManager::ArgumentType::TYPE_CHAINED_DIRECTIONAL_MOVEMENT },
				snakeHeadId, keyType, chainedDirectionalMovement);
		}
	}
}

void UserInputHandler::setDirection(int headId, int direction, ChainedDirectionalMovement* chainedDirectionalMovement)
{
	switch (direction)
	{
		case 1:
			chainedDirectionalMovement->setDirection(headId, ChainedDirectionalMovement::Direction::NORTH);
			break;
		case 2:
			chainedDirectionalMovement->setDirection(headId, ChainedDirectionalMovement::Direction::SOUTH);
			break;
		case 3:
			chainedDirectionalMovement->setDirection(headId, ChainedDirectionalMovement::Direction::WEST);
			break;
		case 4:
			chainedDirectionalMovement->setDirection(headId, ChainedDirectionalMovement::Direction::EAST);
			break;
		case 5:
			chainedDirectionalMovement->setDirection(headId, ChainedDirectionalMovement::Direction::NO_DIRECTION);
			break;
		default:
			std::cerr << std::to_string(direction) + " is not a valid direction" << std::endl;
	}
}
