#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <stdio.h>

#include <BWAPI.h>

#include "SSCAITournamentModule.h"

extern "C" __declspec(dllexport) void gameInit(BWAPI::Game* game) { BWAPI::BroodwarPtr = game; }

BOOL APIENTRY DllMain(HANDLE, DWORD, LPVOID)
{
	return TRUE;
}

extern "C" __declspec(dllexport) BWAPI::AIModule* newTournamentAI()
{
	return new SSCAITournamentAI();
}

 extern "C" __declspec(dllexport) BWAPI::TournamentModule* newTournamentModule()
 {
	 return new SSCAITournamentModule();
 }
