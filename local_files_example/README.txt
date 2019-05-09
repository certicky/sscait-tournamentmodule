*** Post-compile copy .dll to bwapi-data ***

1. Rename 'example_cp_SSCAITournamentModule.bat' to 'cp_SSCAITournamentModule.bat' and place it in the tournamentmodule folder (one step up in the filesystem).
2. Change the second parameter of the copy command (in the .bat file) to the correct location to copy the generated dll file as a post-compile action. Alternatively, remove the copy command (empty .bat file) to remove this automation.
3. If the second argument of the copy command is a directory, remember to create it or the resulting dll file will be renamed to the name of the directory without any file extension.


*** Using the SSCAI Tournament settings ***

1. Copy 'tm_settings.ini' to the 'bwapi-data' folder (it should be placed in the same folder as the 'bwapi.ini' file).
2. When starting a game using the tournament module, the .ini file will be parsed and its settings used in the game.
