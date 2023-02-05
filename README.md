## SSCAIT Tournament Module DLL

A BWAPI Tournament Module DLL used to run the SSCAI tournament (https://sscaitournament.com/). 

Its **basic functions** are:
* Sending game's results to the tournament manager software.
* Enforcing time limits and other tournament rules.
* Making sure bots don't cheat.
* Displaying bot names, time limits, and other data on screen during the game.

Its more **interesting functions** are:
* **Camera Module** that moves the camera where the action happens during the game.
* **Commentator Module** that speaks some vaguely game-related nonsense during the game.

### Developement Setup

#### Dependencies

I recommend using Visual Studio 2017 to develop and build the tournament module.

The bwapi dependencies must be linked via:

* Solution properties > Configuration Properties > C/C++ > General > Additional Include Directories. This field should contain e.g. "../BWAPI_4.1.2/include".
* Solution properties > Configuration Properties > Linker > Input > Additional Dependencies. This field should contain e.g. "../BWAPI_4.1.2/lib/BWAPI.lib".

#### Post-compile copy .dll to bwapi-data

1. Rename 'local_files_example\example_cp_SSCAITournamentModule.bat' to 'cp_SSCAITournamentModule.bat' and place it in the tournamentmodule root folder.
2. Change the second parameter of the copy command (in the .bat file) to the correct location to copy the generated dll file as a post-compile action. Alternatively, remove the copy command (empty .bat file) to remove this automation.
3. If the second argument of the copy command is a directory, remember to create it or the resulting dll file will be renamed to the name of the directory without any file extension.

#### Using the SSCAI Tournament settings

1. Copy 'tm_settings.ini' to the 'bwapi-data' folder (it should be placed in the same folder as the 'bwapi.ini' file).
2. When starting a game using the tournament module, the .ini file will be parsed and its settings used in the game.



