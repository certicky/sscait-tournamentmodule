#include "CommentaryModule.h"
#include <thread>
#include <iostream>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")

using namespace BWAPI;

int commentaryRetrievalSeconds = 3; // allow the module to start downloading a new commentary N seconds before the previous commentary has finished, so the pauses are shorter

time_t endOfLastCommentary = time(NULL);
bool isRequestingCommentaryNow = false;

// logs a unit death into 'deaths' list
void CommentaryModule::logDeath(BWAPI::Unit unit)
{
	bool skip = false;

	// when a Drone morphs into Extractor, onUnitDestroy is called on it. however, we shouldn't add that Drone into 'deaths' array
	if (unit->getType() == BWAPI::UnitTypes::Zerg_Drone) {
		// my drone
		if (unit->getPlayer() == Broodwar->self()) {
			for each (BWAPI::Unit nu in BWAPI::Broodwar->self()->getUnits())
			{
				if (nu->getType() == BWAPI::UnitTypes::Zerg_Extractor && nu->getPosition().x == unit->getPosition().x && nu->getPosition().y == unit->getPosition().y) {
					skip = true;
					break;
				}
			}
		}
		else {
			// enemy drone
			for each (BWAPI::Unit nu in BWAPI::Broodwar->enemy()->getUnits()) 
			{
				if (nu->getType() == BWAPI::UnitTypes::Zerg_Extractor && nu->getPosition().x == unit->getPosition().x && nu->getPosition().y == unit->getPosition().y) {
					skip = true;
					break;
				}
			}
		}
	}

	if (!skip) {
		Death death = {
			Broodwar->getFrameCount(),
			unit->getPlayer(),
			unit->getType()
		};
		deaths.push_back(death);
	}
}

// returns the situation description based on the events list
std::string CommentaryModule::getSituationString()
{	
	std::string situation = "";
	if (events.empty()) return "[" + situation + "]";
	
	for (auto it = events.begin(); it != events.end();)
	{
		situation += "\"" + *it + "\",";
		it = events.erase(it);
	}

	if (situation.length() >= 1) {
		situation = situation.substr(0, situation.length() - 1);
	}

	// NOTE: uncomment this while developing
	// Broodwar->printf(situation.c_str());

	return "[" + situation + "]";
}

void downloadAndPlayCommentary(const std::string &url)
{
	isRequestingCommentaryNow = true;

	// download the sound from the server
	HRESULT hr = URLDownloadToFile(NULL, url.c_str(), "commentary.wav", 0, NULL);

	if (hr == S_OK)
	{
		// approximate the duration of the downloaded .WAV file based on its size
		// WARNING: for this duration approximation, we MUST generate .WAV files with sample rate of 22050 and 1 channel
		int duration = 30; // 30s is just a fallback
		HANDLE hFile = CreateFile("commentary.wav", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			DWORD size = GetFileSize(hFile, NULL);
			if (size != INVALID_FILE_SIZE) {
				duration = (int)size / (22050 * 1 * 2); // 2 is the size of each sample in bytes
				
			}
			CloseHandle(hFile);
		}
		
		// update the 'endOfLastCommentary' to NOW + duration of just downloaded sound, so we don't try to speak over it
		endOfLastCommentary = time(NULL) + duration;
		
		// actually play the sound
		PlaySound(TEXT("commentary.wav"), NULL, SND_FILENAME); //PlaySound(TEXT("commentary.wav"), NULL, SND_FILENAME | SND_ASYNC);
	}

	isRequestingCommentaryNow = false;
}

// if we have some relevant events logged, speak about them when this is called
void CommentaryModule::speak()
{	
	time_t now = time(NULL);

	// make sure we only speak if we've finished speaking the previous part of the commentary
	if ((now + commentaryRetrievalSeconds >= endOfLastCommentary) && (!isRequestingCommentaryNow)) {
		// add data from deaths log into events as a "casualties" event
		if (!deaths.empty()) {
			// remove everything older than 640 frames from deaths first. that's no longer relevant.
			deaths.erase(std::remove_if(deaths.begin(), deaths.end(), [](const Death& d) { return d.frame < Broodwar->getFrameCount() - 640; }), deaths.end());
			if (!deaths.empty()) {
				std::set<BWAPI::Player> playersWithDeaths;
				for (auto death : deaths) {
					playersWithDeaths.insert(death.player);
				}

				for (auto player : playersWithDeaths) {
					std::string deathSummary = getDeathSummary(player);
					events.push_back(deathSummary);
				}
				
				if (!playersWithDeaths.empty()) deaths.clear();
			}
		}

		std::string situationString = getSituationString();
			
		// download the WAV file with the commentary and play it - all in a separate detached thread, so we don't block the game or the bot
		std::thread t(downloadAndPlayCommentary, generatorURL + "?gameId=" + gameId.str() + "&situation=" + situationString);
		t.detach();
	}
}

// logsome game start related events when the game starts
void CommentaryModule::onStart(std::string commentaryGeneratorURL)
{
	generatorURL = commentaryGeneratorURL;
	gameId << std::time(nullptr);

	events.push_back("game is starting");
	events.push_back("map name is " + sanitizeMapName(Broodwar->mapFileName()));
	events.push_back("player 1 is called " + Broodwar->self()->getName() + " and plays as " + Broodwar->self()->getRace().toString());
	events.push_back("player 2	is called " + Broodwar->enemy()->getName() + " and plays as " + Broodwar->enemy()->getRace().toString());
	events.push_back(Broodwar->self()->getName() + " spawns " + getSegment(BWAPI::Position( Broodwar->self()->getStartLocation().x, Broodwar->self()->getStartLocation().y)));
}

// when a unit appears
void CommentaryModule::onUnitShow(BWAPI::Unit unit)
{
	// when any player creates a base at a new TilePosition
	if (Broodwar->getFrameCount() > 10 && unit->getType().isResourceDepot()) {
		// opponent
		if (unit->getPlayer()->isEnemy(Broodwar->self())) {
			if (knownEnemyBases.find(unit->getPosition()) == knownEnemyBases.end()) {
				knownEnemyBases.insert(unit->getPosition());
				if (knownEnemyBases.size() >= 2) events.push_back(unit->getPlayer()->getName() + " seems to have a built a new base.");
			}
		} else {
			// me
			if (knownMyBases.find(unit->getPosition()) == knownMyBases.end()) {
				knownMyBases.insert(unit->getPosition());
				events.push_back(unit->getPlayer()->getName() + " builds a new base.");
			}
		}
	}

	// when any player creates a first unit of a given type
	bool ignored = ( // ignore some units
  		   unit->getPlayer()->isNeutral()
		|| unit->getType().isWorker()
		|| unit->getType().isSpell()
		|| unit->getType().isResourceDepot()
		|| unit->getType().isRefinery()
		|| unit->getType() == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine
		|| unit->getType() == BWAPI::UnitTypes::Zerg_Creep_Colony
		|| unit->getType() == BWAPI::UnitTypes::Zerg_Overlord
		|| unit->getType() == BWAPI::UnitTypes::Zerg_Lurker_Egg
		|| unit->getType() == BWAPI::UnitTypes::Zerg_Egg
		|| unit->getType() == BWAPI::UnitTypes::Zerg_Larva);
	if (!ignored) {
		// opponent
		if (unit->getPlayer()->isEnemy(Broodwar->self())) {
			if (seenEnemyUnitTypes.find(unit->getType().toString()) == seenEnemyUnitTypes.end()) {
				seenEnemyUnitTypes.insert(unit->getType().toString());
				events.push_back(unit->getPlayer()->getName() + " seems to have been building " + sanitizeUnitType(unit->getType().toString()) + "s");
			}
		}
		else {
			// me
			if (seenMyUnitTypes.find(unit->getType().toString()) == seenMyUnitTypes.end()) {
				seenMyUnitTypes.insert(unit->getType().toString());
				events.push_back(unit->getPlayer()->getName() + " gets his first " + sanitizeUnitType(unit->getType().toString()));
			}
		}
	}
}

// when a unit gets destroyed, log that into the events
void CommentaryModule::onUnitDestroy(BWAPI::Unit unit)
{
	// when any player loses a base at a TilePosition
	if (unit->getType().isResourceDepot()) {
		// opponent
		if (unit->getPlayer()->isEnemy(Broodwar->self())) {
			if (knownEnemyBases.find(unit->getPosition()) == knownEnemyBases.end()) {
				knownEnemyBases.erase(unit->getPosition());
				events.push_back(unit->getPlayer()->getName() + " lost a base.");
				if (knownMyBases.size() <= knownEnemyBases.size()) {
					events.push_back(unit->getPlayer()->getName() + " now has at least " + std::to_string((int)knownEnemyBases.size()) + " bases while his opponent has at least " + std::to_string((int)knownMyBases.size()));
				}
			}
		}
		else {
			// me
			if (knownMyBases.find(unit->getPosition()) == knownMyBases.end()) {
				knownMyBases.erase(unit->getPosition());
				events.push_back(unit->getPlayer()->getName() + " lost a base.");
				if (knownMyBases.size() <= knownEnemyBases.size()) {
					events.push_back(unit->getPlayer()->getName() + " now has " + std::to_string((int)knownMyBases.size()) + " bases while his opponent has at least " + std::to_string((int)knownEnemyBases.size()));
				}
			}
		}
	}

	// ignore some units
	bool ignored = (unit->getPlayer()->isNeutral()
		|| unit->getType().isSpell() 
		|| unit->isHallucination()
		|| unit->getType() == BWAPI::UnitTypes::Zerg_Lurker_Egg
		|| unit->getType() == BWAPI::UnitTypes::Zerg_Egg
		|| unit->getType() == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine
		|| unit->getType() == BWAPI::UnitTypes::Zerg_Larva);

	if (!ignored) {
		logDeath(unit);
	}

}

// given a Position, returns a string representation of where on map that is (north, south-east, etc.)
std::string CommentaryModule::getSegment(BWAPI::Position position) 
{
	int segmentWidth = Broodwar->mapWidth() / 3;
	int segmentHeight = Broodwar->mapHeight() / 3;
	int locationX = position.x;
	int locationY = position.y;
	
	if (locationX < segmentWidth && locationY < segmentHeight) {
		return "north-west";
	}
	else if (locationX < segmentWidth && locationY >= segmentHeight && locationY < 2 * segmentHeight) {
		return "west";
	}
	else if (locationX < segmentWidth && locationY >= 2 * segmentHeight) {
		return "south-west";
	}
	else if (locationX >= segmentWidth && locationX < 2 * segmentWidth && locationY < segmentHeight) {
		return "north";
	}
	else if (locationX >= segmentWidth && locationX < 2 * segmentWidth && locationY >= segmentHeight && locationY < 2 * segmentHeight) {
		return "center";
	}
	else if (locationX >= segmentWidth && locationX < 2 * segmentWidth && locationY >= 2 * segmentHeight) {
		return "south";
	}
	else if (locationX >= 2 * segmentWidth && locationY < segmentHeight) {
		return "north-east";
	}
	else if (locationX >= 2 * segmentWidth && locationY >= segmentHeight && locationY < 2 * segmentHeight) {
		return "east";
	}
	else if (locationX >= 2 * segmentWidth && locationY >= 2 * segmentHeight) {
		return "south-east";
	}
}

// turns string like "Terran_Command_Center" to "Command Center"
std::string CommentaryModule::sanitizeUnitType(std::string utype) {
	std::string remove_strings[] = { "Terran", "Zerg", "Protoss", "Mode" };

	for (int i = 0; i < 4; i++) {
		size_t pos = utype.find(remove_strings[i]);
		while (pos != std::string::npos) {
			utype.erase(pos, remove_strings[i].length());
			pos = utype.find(remove_strings[i]);
		}
	}
	size_t pos = utype.find("_");
	while (pos != std::string::npos) {
		utype.replace(pos, 1, " ");
		pos = utype.find("_");
	}

	// remove leading whitespaces
	utype.erase(utype.begin(), std::find_if(utype.begin(), utype.end(), [](int ch) {
		return !std::isspace(ch);
	}));

	// remove trailing whitespaces
	utype.erase(std::find_if(utype.rbegin(), utype.rend(), [](int ch) {
		return !std::isspace(ch);
	}).base(), utype.end());

	// special case "ling" -> "Zerglings"
	if (utype == "ling") utype = "Zerglings";

	return utype;
}

std::string addSpacesAfterUppercase(const std::string& s) {
	std::string result;
	for (int i = 0; i < s.length(); ++i) {
		if (isupper(s[i]) && i > 0) {
			result += " ";
		}
		result += s[i];
	}
	return result;
}

// return a map name extracted from a passed map filename (strips (?) and .sc? from '(?)*.sc?'
std::string CommentaryModule::sanitizeMapName(std::string mapName) {
	std::size_t pos = mapName.find(".sc"); // find the position of '.sc' in the string
	mapName = mapName.substr(mapName.find(')') + 1, pos - mapName.find(')') - 1); // return the substring that starts after the first ')' and ends before '.sc'
	mapName = addSpacesAfterUppercase(mapName);
	return mapName;
}

// returns a string with a list of given player's casualties recorded currently in the deaths vector (e.g. "Dragon's casualties: at least 4x Marine, at least 2x Vulture")
std::string CommentaryModule::getDeathSummary(BWAPI::Player player) {
	std::string deathString = player->getName() + "'s casualties: at least ";
	std::map<BWAPI::UnitType, int> unitCount;

	for (auto death : deaths) {
		if (death.player == player) {
			unitCount[death.unitType]++;
		}
	}

	for (std::map<BWAPI::UnitType, int>::iterator it = unitCount.begin(); it != unitCount.end(); ++it) {
		deathString += std::to_string(it->second) + "x " + sanitizeUnitType(it->first.getName()) + ", ";
	}

	if (deathString.length() >= 2) {
		deathString = deathString.substr(0, deathString.length() - 2);
	}
	return deathString;
}

void CommentaryModule::onSendText(std::string text) {
	events.push_back(Broodwar->self()->getName() + " says \"" + text + "\"");
}

void CommentaryModule::onReceiveText(BWAPI::Player player, std::string text) {
	events.push_back(player->getName() + " says \"" + text + "\"");
}