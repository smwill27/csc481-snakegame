#pragma once
#include "EventHandler.h"
#include "ClientServerConsts.h"
#include "ChainedDirectionalMovement.h"
#include "ClientServerConsts.h"
#include "ScriptManager.h"
#include "dukglue/dukglue.h"
/*
* EventHandler that handles input from the user.
*/
class UserInputHandler :
    public EventHandler
{
    private:
        /* id of ChainedDirectionalMovement corresponding to snake blocks */
        int chainedDirectionalMovementId;

        /*manager to use for loading and running user input script*/
        ScriptManager* scriptManager;

        /*name of user input script file*/
        std::string userInputScriptName;

    public:
        /*
        * Constructs a UserInputHandler with the given values.
        * 
        * propertyMap: map of all properties
        * notifyWhileReplaying: whether to notify this handler while replaying
        * chainedDirectionalMovementId: id of ChainedDirectionalMovement property pertaining to snake blocks
        * scriptManager: manager for user input script
        * userInputScriptName: name of file containing user input script
        */
        UserInputHandler(std::map<int, Property*>* propertyMap, bool notifyWhileReplaying, int chainedDirectionalMovementId,
            ScriptManager* scriptManager, std::string userInputScriptName);

        /*
        * Handles the user input appropriately.
        * 
        * e: event with user input details
        */
        void onEvent(Event* e);

        /*
        * Static helper function to allow scripts to change direction.
        *
        * headId: id of head to change direction of
        * direction: direction to set
        * chainedDirectionalMovement: property to set direction in
        */
        static void setDirection(int headId, int direction, ChainedDirectionalMovement* chainedDirectionalMovement);
};

