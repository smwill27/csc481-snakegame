#include "SnakeCollisionHandler.h"

SnakeCollisionHandler::SnakeCollisionHandler(std::map<int, Property*>* propertyMap, bool notifyWhileReplaying, int snakeCollisionId,
	int foodCollisionId, int foodRespawningId, int foodLocationInSpaceId, int foodSpawnPointLocationInSpaceId,
	int snakeChainedDirectionalMovementId, int snakeLocationInSpaceId, int snakeRespawningId, int borderCollisionId, int* idCounter)
	: EventHandler(propertyMap, notifyWhileReplaying)
{
	this->snakeCollisionId = snakeCollisionId;
	this->foodCollisionId = foodCollisionId;
	this->foodRespawningId = foodRespawningId;
	this->foodLocationInSpaceId = foodLocationInSpaceId;
	this->foodSpawnPointLocationInSpaceId = foodSpawnPointLocationInSpaceId;
	this->snakeChainedDirectionalMovementId = snakeChainedDirectionalMovementId;
	this->snakeLocationInSpaceId = snakeLocationInSpaceId;
	this->snakeRespawningId = snakeRespawningId;
	this->borderCollisionId = borderCollisionId;
	this->idCounter = idCounter;
	this->snakeLengthGrowth = 1;
}

void SnakeCollisionHandler::onEvent(Event* e)
{
	//if snake head has moved, check to see if there are any collisions and handle them accordingly
	if (e->getType() == ClientServerConsts::SNAKE_HEAD_MOVED_EVENT)
	{
		//std::cout << "Checking for food collision" << std::endl;

		int snakeHeadId = e->getArgument(0).argValue.argAsInt;
		Collision* snakeCollision = (Collision*)propertyMap->at(snakeCollisionId);

		if (snakeCollision->hasObject(snakeHeadId))
		{
			//check to see if snake is colliding with a food piece
			bool snakeConsumedFood = false;
			Collision* foodCollision = (Collision*)propertyMap->at(foodCollisionId);
			std::vector<int> foodObjects = foodCollision->getObjects();
			int foodObjectsNum = foodObjects.size();
			for (int i = 0; i < foodObjectsNum; i++)
			{
				//if snake is colliding with food piece, choose new spawn point for food piece and respawn it
				if (snakeCollision->isCollidingWith(snakeHeadId, foodObjects[i], foodCollisionId))
				{
					Respawning* foodRespawning = (Respawning*)propertyMap->at(foodRespawningId);
					LocationInSpace* foodSpawnPointLocationInSpace = (LocationInSpace*)propertyMap->at(foodSpawnPointLocationInSpaceId);

					std::vector<int> foodSpawnPoints = foodSpawnPointLocationInSpace->getObjects();
					int foodSpawnPointsNum = foodSpawnPoints.size();
					int originalFoodSpawnPoint = foodRespawning->getSpawnPointObjectId(foodObjects[i]);
					
					//continously select random spawn point and respawn food piece there until it is no longer colliding with snake
					//and is at a different point than it was previously
					do
					{
						int foodSpawnPointIndex = rand() % foodSpawnPointsNum;
						foodRespawning->setSpawnPoint(foodObjects[i], Respawning::SpawnPoint(foodSpawnPoints[foodSpawnPointIndex],
							foodSpawnPointLocationInSpaceId));
						foodRespawning->respawn(foodObjects[i]);
					} while (foodCollision->isCollidingWithAny(foodObjects[i], snakeCollisionId)
						|| foodRespawning->getSpawnPointObjectId(foodObjects[i]) == originalFoodSpawnPoint);

					//raise event indicating food has spawned in new location
					LocationInSpace* foodLocationInSpace = (LocationInSpace*)propertyMap->at(foodLocationInSpaceId);
					sf::Shape* objectShape = foodLocationInSpace->getObjectShape(foodObjects[i]);
					struct Event::ArgumentVariant objectIdArg = { Event::ArgumentType::TYPE_INTEGER, foodObjects[i] };
					struct Event::ArgumentVariant locationInSpaceIdArg = { Event::ArgumentType::TYPE_INTEGER,
						foodLocationInSpace->getId() };
					struct Event::ArgumentVariant absoluteXArg;
					absoluteXArg.argType = Event::ArgumentType::TYPE_FLOAT;
					absoluteXArg.argValue.argAsFloat = objectShape->getPosition().x;
					struct Event::ArgumentVariant absoluteYArg;
					absoluteYArg.argType = Event::ArgumentType::TYPE_FLOAT;
					absoluteYArg.argValue.argAsFloat = objectShape->getPosition().y;
					Event* foodSpawnEvent = new Event(EventManager::getManager()->getCurrentTime(), 
						ClientServerConsts::FOOD_SPAWN_EVENT,
						{ objectIdArg, locationInSpaceIdArg, absoluteXArg, absoluteYArg });
					EventManager::getManager()->raise(foodSpawnEvent);
					snakeConsumedFood = true;
					break;
				}
			}

			ChainedDirectionalMovement* snakeChainedDirectionalMovement =
				(ChainedDirectionalMovement*)propertyMap->at(snakeChainedDirectionalMovementId);
			LocationInSpace* snakeLocationInSpace = (LocationInSpace*)propertyMap->at(snakeLocationInSpaceId);

			//if snake consumed a food piece, increase length of snake by adding a new block(s)
			if (snakeConsumedFood)
			{
				for (int i = 0; i < snakeLengthGrowth; i++)
				{
					//get current tail's direction and position so we know where to add new tail
					int tailId = snakeChainedDirectionalMovement->getTailId(snakeHeadId);

					//if it's only head, use head as tail
					if (tailId == -1)
					{
						tailId = snakeHeadId;
					}

					ChainedDirectionalMovement::Direction lastTailDirection = snakeChainedDirectionalMovement->getLastDirection(tailId);
					sf::Shape* lastTailShape = snakeLocationInSpace->getObjectShape(tailId);
					float lastTailX = lastTailShape->getPosition().x;
					float lastTailY = lastTailShape->getPosition().y;
					//std::cout << "Retrieved information for old tail" << std::endl;

					//create new tail shape
					sf::RectangleShape* newTailShape = new sf::RectangleShape(sf::Vector2f(ClientServerConsts::SNAKE_BLOCK_WIDTH,
						ClientServerConsts::SNAKE_BLOCK_HEIGHT));
					newTailShape->setFillColor(sf::Color::Green);
					int newTailId = *idCounter;
					(*idCounter)++;

					//std::cout << "Created new tail shape" << std::endl;

					//set new tail's position based on direction of last tail
					if (lastTailDirection == ChainedDirectionalMovement::Direction::NORTH)
					{
						newTailShape->setPosition(sf::Vector2f(lastTailX, lastTailY + ClientServerConsts::SNAKE_BLOCK_HEIGHT));
					}
					else if (lastTailDirection == ChainedDirectionalMovement::Direction::SOUTH)
					{
						newTailShape->setPosition(sf::Vector2f(lastTailX, lastTailY - ClientServerConsts::SNAKE_BLOCK_HEIGHT));
					}
					else if (lastTailDirection == ChainedDirectionalMovement::Direction::WEST)
					{
						newTailShape->setPosition(sf::Vector2f(lastTailX + ClientServerConsts::SNAKE_BLOCK_WIDTH, lastTailY));
					}
					else
					{
						newTailShape->setPosition(sf::Vector2f(lastTailX - ClientServerConsts::SNAKE_BLOCK_WIDTH, lastTailY));
					}

					//std::cout << "Set tail's position" << std::endl;

					//add new tail to properties
					snakeLocationInSpace->addObject(newTailId, newTailShape);
					snakeCollision->addObject(newTailId, snakeLocationInSpaceId);
					//std::cout << "Added tail to location in space and collision" << std::endl;
					snakeChainedDirectionalMovement->addTail(newTailId, snakeLocationInSpaceId, snakeHeadId);

					//std::cout << "Added tail to properties" << std::endl;

					//raise event for adding new snake block
					struct Event::ArgumentVariant objectIdArg = { Event::ArgumentType::TYPE_INTEGER, newTailId };
					struct Event::ArgumentVariant locationInSpaceIdArg = { Event::ArgumentType::TYPE_INTEGER,
						snakeLocationInSpaceId };
					struct Event::ArgumentVariant absoluteXArg;
					absoluteXArg.argType = Event::ArgumentType::TYPE_FLOAT;
					absoluteXArg.argValue.argAsFloat = newTailShape->getPosition().x;
					struct Event::ArgumentVariant absoluteYArg;
					absoluteYArg.argType = Event::ArgumentType::TYPE_FLOAT;
					absoluteYArg.argValue.argAsFloat = newTailShape->getPosition().y;
					Event* snakeBlockAddedEvent = new Event(EventManager::getManager()->getCurrentTime(), ClientServerConsts::SNAKE_BLOCK_ADDED_EVENT,
						{ objectIdArg, locationInSpaceIdArg, absoluteXArg, absoluteYArg });
					EventManager::getManager()->raise(snakeBlockAddedEvent);
					//std::cout << "Raised snake block added event" << std::endl;
				}
			}

			//check to see if snake is colliding with a border or itself and so must die and respawn
			bool snakeDied = false;

			//check to see if snake collided with any borders
			if (snakeCollision->isCollidingWithAny(snakeHeadId, borderCollisionId))
			{
				snakeDied = true;
			}

			//if snake is not colliding with any borders, check to see if it is colliding with itself
			if (!snakeDied)
			{
				ChainedDirectionalMovement::Direction headDirection = snakeChainedDirectionalMovement->getLastDirection(snakeHeadId);
				sf::Shape* snakeHeadShape = snakeLocationInSpace->getObjectShape(snakeHeadId);
				float headX = snakeHeadShape->getPosition().x;
				float headY = snakeHeadShape->getPosition().y;
				std::vector<int> snakeBlocks = snakeCollision->getObjects();
				int snakeBlocksNum = snakeBlocks.size();
				for (int i = 0; i < snakeBlocksNum; i++)
				{
					sf::Shape* snakeBlockShape = snakeLocationInSpace->getObjectShape(snakeBlocks[i]);
					float snakeBlockX = snakeBlockShape->getPosition().x;
					float snakeBlockY = snakeBlockShape->getPosition().y;
					if (snakeBlocks[i] != snakeHeadId &&
						snakeCollision->isCollidingWith(snakeHeadId, snakeBlocks[i], snakeCollisionId)
						&& ((headDirection == ChainedDirectionalMovement::Direction::NORTH
							&& snakeBlockY < headY)
							|| (headDirection == ChainedDirectionalMovement::Direction::SOUTH
								&& snakeBlockY > headY)
							|| (headDirection == ChainedDirectionalMovement::Direction::WEST
								&& snakeBlockX < headX)
							|| (headDirection == ChainedDirectionalMovement::Direction::EAST
								&& snakeBlockX > headX)))
					{
						//std::cout << std::to_string(snakeHeadId) + " colliding with " + std::to_string(snakeBlocks[i]);
						snakeDied = true;
						break;
					}
				}
			}

			//if snake died, respawn head and get rid of additional blocks
			if (snakeDied)
			{
				//respawn snake head
				Respawning* snakeRespawning = (Respawning*)propertyMap->at(snakeRespawningId);
				snakeRespawning->respawn(snakeHeadId);

				//remove rest of snake blocks from properties
				std::vector<int> snakeBlocks = snakeCollision->getObjects();
				int snakeBlocksNum = snakeBlocks.size();
				for (int i = 0; i < snakeBlocksNum; i++)
				{
					if (snakeBlocks[i] != snakeHeadId)
					{
						snakeCollision->removeObject(snakeBlocks[i]);
						snakeLocationInSpace->removeObject(snakeBlocks[i]);
					}
				}

				//remove chain from movement and then just add head back in
				snakeChainedDirectionalMovement->removeChain(snakeHeadId);
				snakeChainedDirectionalMovement->addHead(snakeHeadId, snakeLocationInSpaceId, ClientServerConsts::SNAKE_BLOCK_VELOCITY,
					ChainedDirectionalMovement::Direction::NO_DIRECTION);

				//create and raise event for snake respawning
				sf::Shape* headShape = snakeLocationInSpace->getObjectShape(snakeHeadId);
				struct Event::ArgumentVariant objectIdArg = { Event::ArgumentType::TYPE_INTEGER, snakeHeadId };
				struct Event::ArgumentVariant locationInSpaceIdArg = { Event::ArgumentType::TYPE_INTEGER,
					snakeLocationInSpaceId };
				struct Event::ArgumentVariant absoluteXArg;
				absoluteXArg.argType = Event::ArgumentType::TYPE_FLOAT;
				absoluteXArg.argValue.argAsFloat = headShape->getPosition().x;
				struct Event::ArgumentVariant absoluteYArg;
				absoluteYArg.argType = Event::ArgumentType::TYPE_FLOAT;
				absoluteYArg.argValue.argAsFloat = headShape->getPosition().y;
				Event* snakeHeadSpawnEvent = new Event(EventManager::getManager()->getCurrentTime(), ClientServerConsts::SNAKE_HEAD_SPAWN_EVENT,
					{ objectIdArg, locationInSpaceIdArg, absoluteXArg, absoluteYArg });
				EventManager::getManager()->raise(snakeHeadSpawnEvent);

				std::vector<int> snakeBlocksNew = snakeCollision->getObjects();
				//std::cout << "Remaining snake blocks: " + std::to_string(snakeBlocksNew.size()) << std::endl;
			}
		}
	}
}

void SnakeCollisionHandler::setSnakeLengthGrowth(int snakeLengthGrowth)
{
	if (snakeLengthGrowth < 1)
	{
		std::cerr << "Can't set snake length growth lower than 1" << std::endl;
	}
	else
	{
		this->snakeLengthGrowth = snakeLengthGrowth;
	}
}
