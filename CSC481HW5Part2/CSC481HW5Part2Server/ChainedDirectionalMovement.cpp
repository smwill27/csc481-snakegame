#include "ChainedDirectionalMovement.h"

ChainedDirectionalMovement::ChainedDirectionalMovement(int id, std::map<int, Property*>* propertyMap, Timeline* timeline)
	: Property(id, propertyMap)
{
	this->timeline = timeline;
}

void ChainedDirectionalMovement::addHead(int objectId, int locationInSpaceId, float velocity, Direction startingDirection)
{
	if (!hasObject(objectId))
	{
		objects.push_back(objectId);
		heads.push_back(objectId);
		locationsInSpace.insert(std::pair<int, int>(objectId, locationInSpaceId));
		velocities.insert(std::pair<int, float>(objectId, velocity));
		directions.insert(std::pair<int, Direction>(objectId, startingDirection));
		amountsLastMoved.insert(std::pair<int, float>(objectId, 0.f));
		timesAtLastMovement.insert(std::pair<int, float>(objectId, -1.f));
		lastDirectionsMoved.insert(std::pair<int, Direction>(objectId, startingDirection));
		headToTailMap.insert(std::pair<int, int>(objectId, -1));
		nextInChainMap.insert(std::pair<int, int>(objectId, -1));
	}
	else
	{
		std::cerr << "Cannot add duplicate head object to ChainedDirectionalMovement" << std::endl;
	}
}

void ChainedDirectionalMovement::addTail(int objectId, int locationInSpaceId, int headId)
{
	if (!hasObject(objectId) && hasObject(headId))
	{
		int previousTailId = headToTailMap.at(headId);

		//if it's only head, use head as tail
		if (previousTailId == -1)
		{
			previousTailId = headId;
		}

		objects.push_back(objectId);
		locationsInSpace.insert(std::pair<int, int>(objectId, locationInSpaceId));
		directions.insert(std::pair<int, Direction>(objectId, lastDirectionsMoved.at(previousTailId)));
		lastDirectionsMoved.insert(std::pair<int, Direction>(objectId, lastDirectionsMoved.at(previousTailId)));
		nextInChainMap[previousTailId] = objectId;
		nextInChainMap.insert(std::pair<int, int>(objectId, -1));
		headToTailMap[headId] = objectId;
		amountsLastMoved.insert(std::pair<int, float>(objectId, 0.f));
	}
	else
	{
		std::cerr << "Cannot add duplicate tail object to ChainedDirectionalMovement" << std::endl;
	}
}

void ChainedDirectionalMovement::removeChain(int headId)
{
	if (!hasObject(headId))
	{
		std::cerr << "No such chain defined in ChainedDirectionalMovement" << std::endl;
		return;
	}

	int currentObject = headId;

	do
	{
		//save id of next object in chain
		int nextId = nextInChainMap.at(currentObject);

		//remove from objects list
		int objectsNum = objects.size();
		for (int i = 0; i < objectsNum; i++)
		{
			if (objects[i] == currentObject)
			{
				objects.erase(objects.begin() + i);
			}
		}

		//if object is head, remove from corresponding maps and list
		if (currentObject == headId)
		{
			headToTailMap.erase(currentObject);
			velocities.erase(currentObject);
			timesAtLastMovement.erase(currentObject);

			//remove from heads list
			int headsNum = heads.size();
			for (int i = 0; i < headsNum; i++)
			{
				if (heads[i] == currentObject)
				{
					heads.erase(heads.begin() + i);
				}
			}
		}

		//remove from maps
		locationsInSpace.erase(currentObject);
		nextInChainMap.erase(currentObject);
		directions.erase(currentObject);
		lastDirectionsMoved.erase(currentObject);
		amountsLastMoved.erase(currentObject);

		//set current to next
		currentObject = nextId;
	} while (currentObject != -1);
}

void ChainedDirectionalMovement::setDirection(int headId, Direction direction)
{
	if (hasObject(headId))
	{
		//cannot turn chain in opposite direction of that it's heading in (or it would turn on itself)
		if ((directions.at(headId) == Direction::NORTH && direction == Direction::SOUTH)
			|| (directions.at(headId) == Direction::SOUTH && direction == Direction::NORTH)
			|| (directions.at(headId) == Direction::WEST && direction == Direction::EAST)
			|| (directions.at(headId) == Direction::EAST && direction == Direction::WEST))
		{
			return;
		}

		directions[headId] = direction;
	}
	else
	{
		std::cerr << "No such object defined in ChainedDirectionalMovement" << std::endl;
	}
}

void ChainedDirectionalMovement::processMovement(int headId)
{
	if (!hasObject(headId))
	{
		std::cerr << "No such chain defined in ChainedDirectionalMovement" << std::endl;
		return;
	}

	//don't move chain if head has no direction
	if (directions.at(headId) == Direction::NO_DIRECTION)
	{
		return;
	}

	//determine time between last movement and next
	float currentTime = timeline->getTime();

	//if chain hasn't moved before, just set time for last movement to current time and return
	if (timesAtLastMovement.at(headId) < 0.f)
	{
		timesAtLastMovement[headId] = currentTime * timeline->getTic();
		return;
	}

	//if chain has moved before, calculate next time for movement
	float timeForMovement = currentTime - (timesAtLastMovement.at(headId) / timeline->getTic());
	timesAtLastMovement[headId] = currentTime * timeline->getTic();

	//determine amount for head to move based on change in time
	float unitsToMove = velocities.at(headId) * timeForMovement;
	float extraUnitsToMove = 0.f;

	int currentObject = headId;
	Direction directionForNextToMoveIn = directions.at(headId);

	do
	{
		//get object shape and set current direction
		LocationInSpace* locationInSpace = (LocationInSpace*)propertyMap->at(locationsInSpace.at(currentObject));
		sf::Shape* objectShape = locationInSpace->getObjectShape(currentObject);
		//std::cout << std::to_string(currentObject) + " Initial X: " + std::to_string(objectShape->getPosition().x) +
			//" Initial Y: " + std::to_string(objectShape->getPosition().y) << std::endl;
		float xToMove = 0.f;
		float yToMove = 0.f;
		directions[currentObject] = directionForNextToMoveIn;

		//adjust movements to make based on direction to move in
		if (directionForNextToMoveIn == Direction::NORTH)
		{
			yToMove = -unitsToMove;
			xToMove = extraUnitsToMove;
		}
		else if (directionForNextToMoveIn == Direction::SOUTH)
		{
			yToMove = unitsToMove;
			xToMove = extraUnitsToMove;
		}
		else if (directionForNextToMoveIn == Direction::WEST)
		{
			xToMove = -unitsToMove;
			yToMove = extraUnitsToMove;
		}
		else
		{
			xToMove = unitsToMove;
			yToMove = extraUnitsToMove;
		}

		//adjust position
		objectShape->setPosition(objectShape->getPosition().x + xToMove, objectShape->getPosition().y + yToMove);

		//set direction for next object in chain to direction before current one and amount for next to move as previous movement
		directionForNextToMoveIn = lastDirectionsMoved.at(currentObject);
		float lastUnitsMoved = amountsLastMoved.at(currentObject);
		amountsLastMoved[currentObject] = unitsToMove;
		unitsToMove = lastUnitsMoved;

		int nextInChainId = nextInChainMap.at(currentObject);
		if (nextInChainId != -1)
		{
			Direction lastDirectionOfNextInChain = lastDirectionsMoved.at(nextInChainId);
			sf::Shape* nextInChainShape = locationInSpace->getObjectShape(nextInChainId);
			//if next in chain is already heading same direction as this object, see if we need to account for turning
			if (lastDirectionOfNextInChain == directions.at(currentObject))
			{
				if (lastDirectionOfNextInChain == Direction::NORTH || lastDirectionOfNextInChain == Direction::SOUTH)
				{
					extraUnitsToMove = objectShape->getPosition().x - nextInChainShape->getPosition().x;
				}
				else
				{
					extraUnitsToMove = objectShape->getPosition().y - nextInChainShape->getPosition().y;
				}
			}
		}

		//set new last direction moved and set next object
		lastDirectionsMoved[currentObject] = directions.at(currentObject);
		currentObject = nextInChainMap.at(currentObject);
	} while (currentObject != -1);
}

std::vector<int> ChainedDirectionalMovement::getHeads()
{
	return heads;
}

int ChainedDirectionalMovement::getNextInChain(int idOfPrevious)
{
	if (!hasObject(idOfPrevious))
	{
		std::cerr << "No such chain defined in ChainedDirectionalMovement" << std::endl;
		return -1;
	}

	return nextInChainMap.at(idOfPrevious);
}

int ChainedDirectionalMovement::getTailId(int headId)
{
	if (!hasObject(headId))
	{
		std::cerr << "No such chain defined in ChainedDirectionalMovement" << std::endl;
		return -1;
	}

	return headToTailMap.at(headId);
}

ChainedDirectionalMovement::Direction ChainedDirectionalMovement::getLastDirection(int objectId)
{
	if (!hasObject(objectId))
	{
		std::cerr << "No such object defined in ChainedDirectionalMovement" << std::endl;
		return ChainedDirectionalMovement::Direction::NO_DIRECTION;
	}

	return lastDirectionsMoved.at(objectId);
}

ChainedDirectionalMovement::Direction ChainedDirectionalMovement::getDirection(int objectId)
{
	if (!hasObject(objectId))
	{
		std::cerr << "No such object defined in ChainedDirectionalMovement" << std::endl;
		return ChainedDirectionalMovement::Direction::NO_DIRECTION;
	}

	return directions.at(objectId);
}
