*** Post-compile copy .dll to bwapi-data ***

1. Rename 'example_cp_SSCAITournamentModule.bat' to 'cp_SSCAITournamentModule.bat' and place it in the tournamentmodule folder (one step up in the filesystem).
2. Change the second parameter of the copy command (in the .bat file) to the correct location to copy the generated dll file as a post-compile action. Alternatively, remove the copy command (empty .bat file) to remove this automation.