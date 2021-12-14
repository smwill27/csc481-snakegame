function customizeColors(borders, snakeBlocks, foodPieces)
{
	/*Some Colors to Note:
	Black = 255
	Green = 16711935
	Yellow = 4294902015
	Magenta = 4278255615
	Cyan = 16777215
	White = 4294967295
	Red = 4278190335
	Blue = 65535
	*/

	//color values based on sf::Color

	//uncomment these for color scheme 1 (dark background)
	borders.setColor(65535); //blue borders
	snakeBlocks.setColor(16711935); //green snake blocks
	foodPieces.setColor(4278190335); //red food pieces
	setBackground(255); //black background
	setText(16777215); //cyan text

	//uncomment these for color scheme 2 (light background)
	//borders.setColor(4278190335); //red borders
	//snakeBlocks.setColor(4278255615); //magenta snake blocks
	//foodPieces.setColor(65535); //blue food pieces
	//setBackground(4294967295); //white background
	//setText(255); //black text

	return "Color customization success";
}

function customizeKeys()
{
	/*Some Keys to Note:
	Up Arrow = 73
	Down Arrow = 74
	Left Arrow = 71
	Right Arrow = 72
	W = 22
	A = 0
	S = 18
	D = 3
	*/

	//change second value in function call to change key being used (based on sf::Keyboard::Key)
	
	//Uncomment these for arrow key controls
	setKey(1, 73); //sets key for turning north
	setKey(2, 74); //sets key for turning south
	setKey(3, 71); //sets key for turning west
	setKey(4, 72); //sets key for turning east

	//Uncomment these for WASD controls
	//setKey(1, 22); //sets key for turning north
	//setKey(2, 18); //sets key for turning south
	//setKey(3, 0); //sets key for turning west
	//setKey(4, 3); //sets key for turning east

	return "Key Customization Success";
}