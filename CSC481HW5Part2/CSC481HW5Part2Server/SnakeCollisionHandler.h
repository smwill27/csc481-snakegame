#pragma once
#include "EventHandler.h"
#include "EventManager.h"
#include "ClientServerConsts.h"
#include "Collision.h"
#include "Respawning.h"
#include "LocationInSpace.h"
#include "ChainedDirectionalMovement.h"
#include <iostream>
/*
* An EventHandler that handles checking for collisions with the snake head and responding to them appropriately.
*/
class SnakeCollisionHandler :
    public EventHandler
{
    private:
        /*id corresponding to Collision property for snake blocks*/
        int snakeCollisionId;

        /*id corresponding to Collision property for food pieces*/
        int foodCollisionId;

        /*id corresponding to Respawning property for food pieces*/
        int foodRespawningId;

        /*id corresponding to LocationInSpace property for food pieces*/
        int foodLocationInSpaceId;

        /*id corresponding to LocationInSpace property for food spawn points*/
        int foodSpawnPointLocationInSpaceId;

        /*id corresponding to ChainedDirectionalMovement for snake blocks*/
        int snakeChainedDirectionalMovementId;

        /*id corresponding to LocationInSpace for snake*/
        int snakeLocationInSpaceId;

        /*id corresponding to Respawning for snake*/
        int snakeRespawningId;

        /* id corresponding to Collision for borders*/
        int borderCollisionId;

        /*pointer to GUID counter*/
        int* idCounter;

        /*number of snake blocks to add with each piece of food consumed; 1 by default*/
        int snakeLengthGrowth;

    public:
        /*
        * Constructs a SnakeCollisionHandler with the given values.
        * 
        * propertyMap: map of all properties
        * notifyWhileReplaying: whether to notify this handler while a replay is playing
        * snakeCollisionId: id of Collision property corresponding to snake blocks
        * foodCollisionId: id of Collision property corresponding to food pieces
        * foodRespawningId: id of Respawning property corresponding to food pieces
        * foodLocationInSpaceId: id of LocationInSpace property corresponding to food pieces
        * foodSpawnPointLocationInSpaceId: id of LocationInSpace property corresponding to food spawn points
        * snakeChainedDirectionalMovementId: id of ChainedDirectionalMovement property corresponding to snake blocks
        * snakeLocationInSpaceId: id of LocationInSpace property corresponding to snake blocks
        * snakeRespawningId: id of Respawning property corresponding to snake heads
        * borderCollisionId: id of Collision property corresponding to borders
        * idCounter: pointer to counter for GUIDs
        */
        SnakeCollisionHandler(std::map<int, Property*>* propertyMap, bool notifyWhileReplaying, int snakeCollisionId,
            int foodCollisionId, int foodRespawningId, int foodLocationInSpaceId, int foodSpawnPointLocationInSpaceId,
            int snakeChainedDirectionalMovementId, int snakeLocationInSpaceId, int snakeRespawningId, 
            int borderCollisionId, int* idCounter);

        /*
        * Handles the given movement event by checking for collisions and responding to them.
        */
        void onEvent(Event* e);

        /*
        * Sets the number of snake blocks to add with each food piece consumed to the given value.
        * 
        * snakeLengthGrowth: number of snake blocks to add with each food piece consumed
        */
        void setSnakeLengthGrowth(int snakeLengthGrowth);
};

