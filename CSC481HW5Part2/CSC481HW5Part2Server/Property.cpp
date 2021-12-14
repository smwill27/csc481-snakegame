#include "Property.h"

Property::Property()
{
	this->id = -1;
	this->propertyMap = nullptr;
}

Property::Property(int id, std::map<int, Property*>* propertyMap)
{
	this->id = id;
	this->propertyMap = propertyMap;
}

bool Property::hasObject(int id)
{
	int objectsNum = objects.size();

	for (int i = 0; i < objectsNum; i++)
	{
		if (objects[i] == id)
		{
			return true;
		}
	}

	return false;
}

int Property::getId()
{
	return id;
}
