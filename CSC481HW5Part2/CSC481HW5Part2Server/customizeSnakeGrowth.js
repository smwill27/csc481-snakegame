function customizeSnakeGrowth(snakeCollisionHandler)
{
	snakeCollisionHandler.setSnakeLengthGrowth(1); //default number of blocks
	//snakeCollisionHandler.setSnakeLengthGrowth(2); //two blocks
	//snakeCollisionHandler.setSnakeLengthGrowth(3); //3 blocks
	//1-3 blocks seem to work fairly well, but 4 or higher will break the game

	return "Snake growth length successfully set";
}