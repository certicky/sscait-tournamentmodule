#pragma once
#include <BWAPI.h>
#include <vector>
#include <set>
#include <windows.h>
#include <Shlwapi.h>
#include <iostream>
#include <fstream>
#include "Timer.h"
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <string>
#include <ctime>

class CommentaryModule
{
private:
	struct Death {
		int frame;
		BWAPI::Player player;
		BWAPI::UnitType unitType;
	};
	
	std::vector<std::string> events = {};	// log of events that commentator can mention next time it speaks
	std::vector<Death> deaths = {};			// log of unit deaths that is used to create an Event and emptied from time to time
	std::string generatorURL = "";          // URL of the server where commentary sound files are prepared
	std::stringstream gameId;				// unique ID of currently played game. can be anything, just make sure that two consecutive games have different IDs.

	std::string getSegment(BWAPI::Position position);
	std::string getDeathSummary(BWAPI::Player player);
	std::string sanitizeUnitType(std::string utype);
	std::string sanitizeMapName(std::string mapName);
	std::set<std::string> seenEnemyUnitTypes;
	std::set<std::string> seenMyUnitTypes;
	std::set <BWAPI::Position> knownEnemyBases = {};
	std::set <BWAPI::Position> knownMyBases = {};

public:
	std::string getSituationString();
	virtual void logDeath(BWAPI::Unit unit);
	virtual void speak();
	virtual void onStart(std::string commentaryGeneratorURL);
	virtual void onUnitShow(BWAPI::Unit unit);
	virtual void onUnitDestroy(BWAPI::Unit unit);
	virtual void onSendText(std::string text);
	virtual void onReceiveText(BWAPI::Player player, std::string text);
};
