# csc481-snakegame

**********Part 2 (Second Game: Snake)*****************
Location: Solution is CSC481HW5Part2, projects are CSC481HW5Part2Server and CSC481HW5Part2Client

Steps for Compilation and Execution:
1. Load CSC481HW5Part2.sln in Visual Studio 2019.
2. In the toolbar at the top, set the two dropdowns to the left of the green arrow (Solution Configurations
   and Solution Platforms) to the values Release and Win32.
3. Go into both project's properties and make sure that the configuration options are set to match those in the
   toolbar (Release and Win32).
4. In both project's properties, go to the General tab under the C/C++ tab and in the Additional Include Directories
   field add the path to your <libzmq-v141-4_3_2> directory and the path <sfml-install-path>\include, removing
   the existing paths for those.
5. In both project's properties, go to the General tab under the Linker tab and in the Additional Library
   Directories field add the path to your <libzmq-v141-4_3_2> directory and the path <sfml-install-path>\lib,
   removing the existing paths for those.
6. In both project's properties, go to the Input tab under the Linker tab and in the Additional Dependencies field
   add the dependencies libzmq-v141-mt-4_3_2.lib, sfml-graphics.lib, sfml-window.lib, and sfml-system.lib (they
   may already be there, in which case leave them).
7. In CSC481HW5Part2Client's properties, go to the General tab under the C/C++ tab and in the Additional Include
   Directories add the path to the CSC481HW5Part2Server folder, removing the existing path.
8. Right-click on the solution CSC481HW5Part2 in the solution explorer and select Build Solution or Rebuild Solution.
9. After the projects have finished building, right-click on the project CSC481HW5Part2Server in the solution explorer
   and select Debug -> Start New Instance to start the server.
10. To run the client, right-click on the project CSC481HW5Part2Client in the solution explorer
    and select Debug -> Start New Instance. The client will open an 800x600 window with four borders, a background,
    text for high score and current score, as well as a square in the middle of the screen representing the head
    of the snake and a smaller square somewhere on the screen representing a food piece.
11. The game is singleplayer, and only one client is allowed to connect at a time. Attempting to connect
    a second client will fail. However, after the single client disconnects by closing the window, another
    client will then be able to connect in their place.

Handling Snake Input:
-Keys used for movement can be customized by a script (detailed in the Scripting section below). By default,
 they will be the arrow keys.
-To move the snake head to the left, press the key corresponding to turning west (left arrow key by default).
-To move the snake head to the right, press the key corresponding to turning east (right arrow key by default).
-To move the snake head upward, press the key corresponding to turning north (up arrow key by default).
-To move the snake head downward, press the key corresponding to turning south (down arrow key by default).
-The snake head will not be able to turn in the opposite direction of that which it is moving (i.e. if the snake
 head is moving north, it cannot turn south).
-At the start of the game as well as whenever the snake respawns, the snake will not start moving until a key
 is first pressed to move it in a starting direction.
-Pressing multiple keys at once or spamming multiple keys in quick succession often leads the snake to bug out,
 glitching visually and then dying, so it is recommended to press only one key at a time.

Collision:
-If the snake collides with one of the four borders surrounding the edges of the window, it will die and then
 respawn back at its original position (once again just a head, with all additional blocks lost).
-If the snake collides with itself, it will die and respawn the same as hitting one of the borders. The snake's
 collision with itself can sometimes be a bit oversensitive, and there is a chance that you will occasionally
 experience an unexpected death, but this did not happen often in my experience.
-If the snake collides with one of the food pieces, it will consume the food piece, leading the piece to respawn
 at a different position on the map (randomized from ten potential spawn points). The snake will also grow
 by a specified number of blocks (1 by default, though this can be customized with a script as detailed in the
 Scripting section below).

Scoring:
-The user's high score and current score are displayed at the top of the window. The high score is not persistent,
 and will be reset each time a client connects.
-Consuming a food piece will cause the client's current score to increase by 1 point for each block that the
 snake grows (so 1 point per food piece by default). This is because the logic for increasing the score is tied
 to the addition of the snake blocks.
-Once the snake dies, their current score will reset to 0. The high score will be set to the previous score if
 applicable.

Scripting:
-The colors of the different objects can be set through a script. To customize them, navigate to the
 customization.js file in the CSC481HW5Part2Client folder. In the customizeColors function, you will see a list
 of a few unsigned int values corresponding to colors in sf::Color as well as lines for two different color
 schemes. Color scheme 1 is initially uncommented, and will be what you see when you first load up the game.
 Comment out those lines and uncomment the ones for color scheme 2, then save to see the colors of all objects
 in the window change to those indicated. You can also set any of the colors to ones you wish so long as you
 know which unsigned int corresponds to it in sf::Color (the list provides a few values to play with).
-The keys used to control the snake head can be set through a script. To customize them, navigate to the
 customization.js file in the CSC481HW5Part2Client folder. In the customizeKeys function, you will see a list of
 a few ints corresponding to keys in sf::Keyboard::Key as well as a set of lines for arrow key controls and a
 a set of lines for WASD controls. The arrow key lines will initially be uncommented, and will be what you
 use when first loading up the game. To switch to WASD controls, comment out the arrow key lines and uncomment
 the WASD lines, then save the file. You should then be able to use WASD to control the snake in place of the
 arrow keys. You can also set any of the keys to one of your choosing so long as you know the int that corresponds
 to it in sf::Keyboard::Key.
-The amount that the snake grows with each piece of food consumed can be set through a script. To customize it,
 navigate to the customizeSnakeGrowth.js file in the CSC481HW5Part2Server folder. You will see lines for setting
 the snake length to 1, 2, or 3 blocks. The line for 1 block will be initially uncommented and will be active
 when you first load up the game. To switch to one of the other values, comment out the line for 1 block,
 uncomment the line you want, and then save the file. As you play the game, you should then see the snake
 grow by further blocks with each piece of food consumed. Increasing the length can lead to some occasional
 glitchy behavior, such as the end of the tail glitching out visually. Increasing the number of blocks beyond 3
 is not recommended as it broke the game in my experience by causing the snake to glitch out and then die
 quickly after consuming a food piece.
-User input event handling is done partly through the use of the script handleUserInput.js in the
 CSC481HW5Part2 folder, but there are no specific modifications to be done to it.
-Whenever the program attempts to load a script, it will access the script file to check the write times to
 determine whether it needs to reload the script. This can potentially cause conflicts in terms of saving a script
 file. If you get a message about being unable to save due to an access violation while trying to save, simply
 try saving again until it goes through. This issue did not happen too often in my experience.

Quitting the Program:
-To disconnect the client, close the window by hitting the red x in the upper right corner.
-To quit all execution, click the stop button in the debug menu.

Code Sources Consulted:
-SFML documentation
-ZeroMQ guide/documentation and examples
-C++ standard library documentation
-Duktape/Dukglue documentation
-Moodle scripting example (also used as starting point for ScriptManager)
