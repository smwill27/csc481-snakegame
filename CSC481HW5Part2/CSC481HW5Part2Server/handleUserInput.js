function handleUserInput(id, keyType, chainedDirectionalMovement)
{
	if (keyType == 1)
	{
		setDirection(id, 1, chainedDirectionalMovement);
	}
	else if (keyType == 2)
	{
		setDirection(id, 2, chainedDirectionalMovement);
	}
	else if (keyType == 3)
	{
		setDirection(id, 3, chainedDirectionalMovement);
	}
	else if (keyType == 4)
	{
		setDirection(id, 4, chainedDirectionalMovement);
	}

	return "User input handled";
}