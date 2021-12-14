#pragma once
#include "Property.h"
#include "Timeline.h"
#include "LocationInSpace.h"
#include <iostream>
#include "ClientServerConsts.h"
/*
* Property defining an object's ability to move continuously in a designated direction as part of a chain of objects controlled
* by a head object.
* 
* Property Dependencies: LocationInSpace
*/
class ChainedDirectionalMovement :
    public Property
{
    public:
        /*
        * Enum representing the possible directions in which an object can be moving.
        */
        enum class Direction
        {
            NORTH,
            SOUTH,
            WEST,
            EAST,
            NO_DIRECTION //if a head has this direction, the chain will not move (can be used as starting direction for head so that
                            //it will not move until user chooses a direction, or could be used to allow a rest state)
        };

    private:
        /* LocationInSpace corresponding to each object */
        std::map<int, int> locationsInSpace;

        /* timeline to use in timing movements */
        Timeline* timeline;

        /* list of head objects */
        std::vector<int> heads;

        /* map of head objects to corresponding tail objects (-1 if there is no tail) */
        std::map<int, int> headToTailMap;
    
        /* map of objects to the next objects in their respective chains (-1 if there is no next object) */
        std::map<int, int> nextInChainMap;

        /* amounts to move per time unit */
        std::map<int, float> velocities;

        /* direction that each object is heading */
        std::map<int, Direction> directions;

        /* last direction in which an object moved (used to direct movement of next in chain) */
        std::map<int, Direction> lastDirectionsMoved;

        /* amount last moved for each object */
        std::map<int, float> amountsLastMoved;

        /* time for each chain's last movement (not relative to tic size) */
        std::map<int, float> timesAtLastMovement;

    public:
        /*
        * Constructs a ChainedDirectionalMovement property with the given values.
        * 
        * id: id of the property
        * propertyMap: map of all properties
        * timeline: timeline to use in timing movements
        */
        ChainedDirectionalMovement(int id, std::map<int, Property*>* propertyMap, Timeline* timeline);

        /*
        * Adds a head object with the given values to start a new chain.
        * 
        * objectId: id of head object
        * locationInSpaceId: id of LocationInSpace corresponding to this object
        * velocity: amount to move per time unit
        * startingDirection: direction for head to initially be pointed in
        */
        void addHead(int objectId, int locationInSpaceId, float velocity, Direction startingDirection);

        /*
        * Extends an existing chain by adding a tail object with the given values.
        * 
        * objectId: id of tail object
        * locationInSpaceId: id of LocationInSpace corresponding to this object
        * headId: id of head object corresponding to chain that this object is being added to
        */
        void addTail(int objectId, int locationInSpaceId, int headId);

        /*
        * Removes the chain of objects corresponding to the given head object.
        * 
        * headId: id of head object
        */
        void removeChain(int headId);

        /*
        * Sets the direction of the given head object.
        * 
        * headId: id of head object
        * direction: direction to set
        */
        void setDirection(int headId, Direction direction);

        /*
        * Processes next set of movements for the chain with the given head object.
        * 
        * headId: id of head object
        */
        void processMovement(int headId);

        /*
        * Returns a list of head object ids.
        * 
        * returns: list of head object ids
        */
        std::vector<int> getHeads();

        /*
        * Returns the id of the object following after the given object in the chain.
        * 
        * returns: id of object after given one in chain (-1 if no object following)
        */
        int getNextInChain(int idOfPrevious);

        /*
        * Returns the id of the tail object for the chain with the given head.
        * 
        * headId: id of head in chain
        * 
        * returns: id of tail object in chain
        */
        int getTailId(int headId);

        /*
        * Returns the last direction moved for the given object.
        * 
        * returns: last direction moved for object
        */
        Direction getLastDirection(int objectId);

        /*
        * Returns the current direction for the given object.
        *
        * returns: current direction for object
        */
        Direction getDirection(int objectId);
};

