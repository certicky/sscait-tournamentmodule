#include "SSCAITournamentModule.h"

using namespace BWAPI;

class TournamentModuleState
{

public:

	std::string selfName;
	std::string enemyName;
	std::string mapName;
	
	int frameCount;
	int selfScore;
	int enemyScore;
	int gameElapsedTime;
	int gameOver;
	int gameTimeUp;
	
	std::vector<int> timeOutExceeded;

	Timer gameTimer;

	TournamentModuleState()
	{
		selfName		= BWAPI::Broodwar->self()->getName();
		enemyName		= BWAPI::Broodwar->enemy()->getName();
		mapName			= BWAPI::Broodwar->mapName();

		frameCount		= BWAPI::Broodwar->getFrameCount();
		selfScore		= BWAPI::Broodwar->self()->getKillScore() + BWAPI::Broodwar->self()->getRazingScore();
						//+ BWAPI::Broodwar->self()->getBuildingScore() 
						//+ BWAPI::Broodwar->self()->gatheredMinerals()
						//+ BWAPI::Broodwar->self()->gatheredGas();

		enemyScore		= 0;

		gameOver = 0;
		gameTimeUp = 0;

		gameTimer.start();

		gameElapsedTime = 0;
	}

	void update(std::vector<int> & times)
	{
		frameCount		= BWAPI::Broodwar->getFrameCount();
		selfScore		= BWAPI::Broodwar->self()->getKillScore() + BWAPI::Broodwar->self()->getRazingScore();
						//+ BWAPI::Broodwar->self()->getBuildingScore() 
						//+ BWAPI::Broodwar->self()->gatheredMinerals()
						//+ BWAPI::Broodwar->self()->gatheredGas();

		timeOutExceeded = times;

		gameElapsedTime = (int)gameTimer.getElapsedTimeInMilliSec();
	}

	void ended()
	{
		gameOver = 1;
	}

	bool write(std::string filename)
	{
		gameTimeUp = Broodwar->getFrameCount() > 85714;

		std::ofstream outfile(filename.c_str(), std::ios::out);
		if (outfile.is_open())
		{
			outfile << selfName			<< std::endl;
			outfile << enemyName		<< std::endl;
			outfile << mapName			<< std::endl;
			outfile << frameCount		<< std::endl;
			outfile << selfScore		<< std::endl;
			outfile << enemyScore		<< std::endl;
			outfile << gameElapsedTime  << std::endl;
			outfile << BWAPI::Broodwar->self()->isDefeated()   << std::endl;
			outfile << BWAPI::Broodwar->self()->isVictorious() << std::endl;
			outfile << gameOver		    << std::endl;
			outfile << gameTimeUp       << std::endl;

			for (size_t i(0); i<timeOutExceeded.size(); ++i)
			{
				outfile << timeOutExceeded[i] << std::endl;
			}

			return true;
		}
		else
		{
			return false;
		}

		outfile.close();
	}	
};

std::vector<int> timerLimits;
std::vector<int> timerLimitsBound;
std::vector<int> timerLimitsExceeded;

//int cameraMoveTime = 150;
//int cameraMoveTimeMin = 50;
//int watchScoutWorkerUntil = 7000;
//int lastMoved = 0;
//int lastMovedPriority = 0;
//BWAPI::Position lastMovedPosition = BWAPI::Position(0, 0);
int localSpeed = 10; // this is overwritten when setting zero speed
int frameSkip = 0;
int gameTimeLimit = 85714;
int zeroSpeedTime = 85714;

bool drawBotNames = true;
bool drawUnitInfo = false;
bool drawTournamentInfo = true;

char buffer[MAX_PATH];

std::vector<int> frameTimes(100000,0);

void SSCAITournamentAI::onStart()
{
	
	GetModuleFileName(NULL, buffer, MAX_PATH);
	BWAPI::Broodwar->printf("Path is %s", buffer);

	// Set the command optimization level (reduces high APM, size of bloated replays, etc)
	Broodwar->setCommandOptimizationLevel(MINIMUM_COMMAND_OPTIMIZATION);

	timerLimits.push_back(55);
	timerLimitsBound.push_back(320);
	timerLimits.push_back(1000);
	timerLimitsBound.push_back(10);
	timerLimits.push_back(10000);
	timerLimitsBound.push_back(1);
	timerLimitsExceeded.push_back(0);
	timerLimitsExceeded.push_back(0);
	timerLimitsExceeded.push_back(0);
	
	camera = CameraModule();
	parseConfigFile("bwapi-data\\tm_settings.ini");
	
	Broodwar->setLocalSpeed(localSpeed);
	Broodwar->setFrameSkip(frameSkip);

	myStartLocation = Position(Broodwar->self()->getStartLocation().x()*TILE_SIZE, Broodwar->self()->getStartLocation().y()*TILE_SIZE);
	Broodwar->printf("Start position: %d %d", myStartLocation.x(), myStartLocation.y());

	camera.onStart(myStartLocation);
}


void SSCAITournamentAI::onEnd(bool isWinner)
{
	TournamentModuleState state = TournamentModuleState();
	state.ended();
	state.update(timerLimitsExceeded);
	state.write("gameState.txt");
}

void SSCAITournamentAI::onFrame()
{
	if ((int)frameTimes.size() < BWAPI::Broodwar->getFrameCount() + 10)
	{
		frameTimes.push_back(0);
	}

	camera.onFrame();

	if (Broodwar->getFrameCount() % 360 == 0)
	{
		TournamentModuleState state = TournamentModuleState();
		state.update(timerLimitsExceeded);
		state.write("gameState.txt");
	}

	if ((gameTimeLimit > 0) && (Broodwar->getFrameCount() > gameTimeLimit))
	{
		Broodwar->sendText("Game time limit of %d frames reached, exiting", gameTimeLimit);
		Broodwar->leaveGame();
	}
	
	int frame = BWAPI::Broodwar->getFrameCount();
	// check the sum of the times for the previous frame

	if (frame < 10)
	{
		return;
	}

	// add the framer times for this frame
	frameTimes[frame] += BWAPI::Broodwar->getLastEventTime();

	// the total time for the last frame
	int timeElapsed = frameTimes[frame-1];

	// check to see if the timer exceeded any frame time limits
	for (size_t t(0); t<timerLimits.size(); ++t)
	{
		if (timeElapsed > timerLimits[t])
		{
			timerLimitsExceeded[t]++;

			if (timerLimitsExceeded[t] >= timerLimitsBound[t])
			{
				Broodwar->sendText("TIMEOUT on %d ms", timerLimits[t]);
				Broodwar->leaveGame();
			}
		}
	}

	drawTournamentModuleSettings(10, 10);

	if (drawUnitInfo)
	{
		drawUnitInformation(440,6);
	}

	if (Broodwar->getFrameCount() >= zeroSpeedTime && Broodwar->getFrameCount() <= zeroSpeedTime+32) {
		Broodwar->setLocalSpeed(0);
		localSpeed = 0;
	}
}

void SSCAITournamentAI::drawTournamentModuleSettings(int x, int y)
{
	int drawX = x;
	int drawY = y;
	int width = 120;

	/*Broodwar->drawTextScreen(drawX, drawY, "\x04 Player Name:");
	Broodwar->drawTextScreen(drawX+width, drawY, "\x07 %s", BWAPI::Broodwar->self()->getName().c_str());
	drawY += 10;

	Broodwar->drawTextScreen(drawX, drawY, "\x04 Enemy Name:");
	Broodwar->drawTextScreen(drawX+width, drawY, "\x07 %s", BWAPI::Broodwar->enemy()->getName().c_str());
	drawY += 10;*/

	/*Broodwar->drawTextScreen(drawX, drawY, "\x04 Map Filename:");
	Broodwar->drawTextScreen(drawX+width, drawY, " %s", BWAPI::Broodwar->mapFileName().c_str());
	drawY += 20;*/

	if (drawTournamentInfo)
	{
		BWAPI::Broodwar->setTextSize();

		BWAPI::Broodwar->drawTextScreen(drawX, drawY, "\x04 Elapsed in-game time: ");
		BWAPI::Broodwar->drawTextScreen(drawX + width, drawY, " %d:%s%d (%d)", Broodwar->elapsedTime() / 60, (Broodwar->elapsedTime() % 60 < 10) ? "0" : "", Broodwar->elapsedTime() % 60, Broodwar->getFrameCount());
		drawY += 10;

		BWAPI::Broodwar->drawTextScreen(drawX, drawY, "\x04 Game time limit: ");
		BWAPI::Broodwar->drawTextScreen(drawX + width, drawY, " %d:%s%d (%d)", gameTimeLimit / 16 / 60, ((gameTimeLimit / 16) % 60 < 10) ? "0" : "", (gameTimeLimit / 16) % 60, gameTimeLimit);
		drawY += 10;

		BWAPI::Broodwar->drawTextScreen(drawX, drawY, "\x04 BWAPI local speed: ");
		BWAPI::Broodwar->drawTextScreen(drawX + width, drawY, " %d", localSpeed);
		drawY += 10;

		BWAPI::Broodwar->drawTextScreen(drawX, drawY, "\x04 BWAPI frame skip: ");
		BWAPI::Broodwar->drawTextScreen(drawX + width, drawY, " %d", frameSkip);
		drawY += 10;

		drawX = 420;
		drawY = y + 18;

		BWAPI::Broodwar->setTextSize(2);
		Broodwar->drawTextScreen(drawX, drawY, "\x03%s", BWAPI::Broodwar->mapFileName().c_str());
		BWAPI::Broodwar->setTextSize();

		drawX -= 5;
		drawY += 20;
		for (size_t t(0); t<timerLimits.size(); ++t)
		{
			BWAPI::Broodwar->drawTextScreen(drawX, drawY, "\x04 # Frames > %d ms ", timerLimits[t]);
			BWAPI::Broodwar->drawTextScreen(drawX + width, drawY, " %d   (Max %d)", timerLimitsExceeded[t], timerLimitsBound[t]);
			drawY += 10;
		}
	}

	if (drawBotNames)
	{
		int boxX = 200;
		int rowY = 310;
		
		Broodwar->drawBoxScreen(boxX, rowY, boxX + 17, rowY + 17, Colors::Green, true);
		Broodwar->drawBoxScreen(boxX + 1, rowY + 1, boxX + 16, rowY + 16, Broodwar->self()->getColor(), true);

		BWAPI::Broodwar->setTextSize(2);
		Broodwar->drawTextScreen(boxX + 25, rowY - 1, "\x07%s (%c)", BWAPI::Broodwar->self()->getName().c_str(), Broodwar->self()->getRace().getName().c_str()[0]);

		rowY = 335;

		Broodwar->drawBoxScreen(boxX, rowY, boxX + 17, rowY + 17, Colors::Green, true);
		Broodwar->drawBoxScreen(boxX + 1, rowY + 1, boxX + 16, rowY + 16, Broodwar->enemy()->getColor(), true);

		BWAPI::Broodwar->setTextSize(2);
		Broodwar->drawTextScreen(boxX + 25, rowY - 1, "\x07%s (%c)", BWAPI::Broodwar->enemy()->getName().c_str(), Broodwar->enemy()->getRace().getName().c_str()[0]);
	}	

	BWAPI::Broodwar->setTextSize();
}

void SSCAITournamentAI::drawUnitInformation(int x, int y) 
{
	
	std::string prefix = "\x04";

	//BWAPI::Broodwar->drawBoxScreen(x-5, y-4, x+200, y+200, BWAPI::Colors::Black, true);

	//BWAPI::Broodwar->drawTextScreen(x, y, "\x04 Unit Information: %s", BWAPI::Broodwar->self()->getRace().getName().c_str());
	BWAPI::Broodwar->drawTextScreen(x, y+20, "\x04%s's Units", BWAPI::Broodwar->self()->getName().c_str());
	BWAPI::Broodwar->drawTextScreen(x+160, y+20, "\x04#");
	BWAPI::Broodwar->drawTextScreen(x+180, y+20, "\x04X");

	int yspace = 0;

	// for each unit in the queue
	for each (BWAPI::UnitType t in BWAPI::UnitTypes::allUnitTypes()) 
	{
		int numUnits = BWAPI::Broodwar->self()->completedUnitCount(t) + BWAPI::Broodwar->self()->incompleteUnitCount(t);
		int numDeadUnits = BWAPI::Broodwar->self()->deadUnitCount(t);

		// if there exist units in the vector
		if (numUnits > 0) 
		{
			if (t.isWorker())			{ prefix = "\x0F"; }
			else if (t.isDetector())	{ prefix = "\x07"; }		
			else if (t.canAttack())		{ prefix = "\x08"; }		
			else if (t.isBuilding())	{ prefix = "\x03"; }
			else						{ prefix = "\x04"; }

			BWAPI::Broodwar->drawTextScreen(x, y+40+((yspace)*10), "%s%s", prefix.c_str(), t.getName().c_str());
			BWAPI::Broodwar->drawTextScreen(x+160, y+40+((yspace)*10), "%s%d", prefix.c_str(), numUnits);
			BWAPI::Broodwar->drawTextScreen(x+180, y+40+((yspace++)*10), "%s%d", prefix.c_str(), numDeadUnits);
		}
	}
}

//bool SSCAITournamentAI::isNearEnemyBuilding(BWAPI::Unit* unit, std::set<BWAPI::Unit*> &enemyUnits) {
//	for (std::set<BWAPI::Unit*>::iterator it = enemyUnits.begin(); it != enemyUnits.end(); it++) {
//		if ((*it)->getType().isBuilding() && unit->getDistance(*it) <= TILE_SIZE*20 && (Broodwar->self()->isEnemy((*it)->getPlayer()) || ((*it)->getPlayer()->isNeutral() && (*it)->getType().isAddon())) && (*it)->getType() != UnitTypes::Terran_Vulture_Spider_Mine/* && !(*it)->isLifted()*/) {
//			return true;
//		}
//	}
//
//	return false;
//}
//
//bool SSCAITournamentAI::isNearStartLocation(BWAPI::Position pos) {
//	int distance = 1000;
//	std::set<BWAPI::TilePosition> startLocations = Broodwar->getStartLocations();
//
//	for (std::set<BWAPI::TilePosition>::iterator it = startLocations.begin(); it != startLocations.end(); it++) {
//		Position startLocation = BWAPI::Position(*it);
//		
//		if (!isNearOwnStartLocation(startLocation) && startLocation.getDistance(pos) <= distance) {
//			return true;
//		}
//	}
//
//	return false;
//}
//
//bool SSCAITournamentAI::isNearOwnStartLocation(BWAPI::Position pos) {
//	int distance = 10 * TILE_SIZE;
//	return (myStartLocation.getDistance(pos) <= distance);
//}
//
//bool SSCAITournamentAI::isArmyUnit(BWAPI::Unit* unit) {
//	return !(unit->getType().isWorker()
//		|| unit->getType().isBuilding()
//		|| unit->getType() == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine
//		|| unit->getType() == BWAPI::UnitTypes::Zerg_Overlord
//		|| unit->getType() == BWAPI::UnitTypes::Zerg_Larva);
//}
//
//bool SSCAITournamentAI::shouldMoveCamera(int priority) {
//	return !(BWAPI::Broodwar->getFrameCount() - lastMoved < cameraMoveTime && (lastMovedPriority >= priority || BWAPI::Broodwar->getFrameCount() - lastMoved < cameraMoveTimeMin));
//}
//
//void SSCAITournamentAI::moveCameraIsUnderAttack()
//{
//	int prio = 3;
//	if (!shouldMoveCamera(prio))
//	{
//		return;
//	}
//
//	for each (BWAPI::Unit * unit in BWAPI::Broodwar->self()->getUnits())
//	{
//		if (unit->isUnderAttack())
//		{
//			moveCamera(unit->getPosition(), prio);
//		}
//	}
//}
//
//void SSCAITournamentAI::moveCameraIsAttacking()
//{
//	int prio = 3;
//	if (!shouldMoveCamera(prio))
//	{
//		return;
//	}
//
//	for each (BWAPI::Unit * unit in BWAPI::Broodwar->self()->getUnits())
//	{
//		if (unit->isAttacking())
//		{
//			moveCamera(unit->getPosition(), prio);
//		}
//	}
//}
//
//void SSCAITournamentAI::moveCameraScoutWorker() {
//	int highPrio = 2;
//	int lowPrio = 0;
//	if (!shouldMoveCamera(lowPrio))
//	{
//		return;
//	}
//
//	for each (BWAPI::Unit* unit in BWAPI::Broodwar->self()->getUnits()) {
//		if (!unit->getType().isWorker()) {
//			continue;
//		}
//		if (isNearStartLocation(unit->getPosition())) {
//			moveCamera(unit->getPosition(), highPrio);
//		} else if (!isNearOwnStartLocation(unit->getPosition())) {
//			moveCamera(unit->getPosition(), lowPrio);
//		}
//	}
//}
//
//void SSCAITournamentAI::moveCameraDrop() {
//	int prio = 2;
//	if (!shouldMoveCamera(prio))
//	{
//		return;
//	}
//	for each (BWAPI::Unit* unit in BWAPI::Broodwar->self()->getUnits()) {
//		if ((unit->getType() == UnitTypes::Zerg_Overlord || unit->getType() == UnitTypes::Terran_Dropship || unit->getType() == UnitTypes::Protoss_Shuttle)
//			&& isNearStartLocation(unit->getPosition()) && unit->getLoadedUnits().size() > 0) {
//			moveCamera(unit->getPosition(), prio);
//		}
//	}
//}
//
//void SSCAITournamentAI::moveCameraArmy() {
//	int prio = 1;
//	if (!shouldMoveCamera(prio))
//	{
//		return;
//	}
//	// Double loop, check if army units are close to each other
//	int radius = 50;
//
//	BWAPI::Position bestPos;
//	int mostUnitsNearby = 0;
//
//	for each (BWAPI::Unit* unit1 in BWAPI::Broodwar->getAllUnits()) {
//		if (!isArmyUnit(unit1)) {
//			continue;
//		}
//		BWAPI::Position uPos = unit1->getPosition();
//
//		int nrUnitsNearby = 0;
//		for each (BWAPI::Unit* unit2 in BWAPI::Broodwar->getUnitsInRadius(uPos, radius)) {
//			if (!isArmyUnit(unit2)) {
//				continue;
//			}
//			nrUnitsNearby++;
//		}
//
//		if (nrUnitsNearby > mostUnitsNearby) {
//			mostUnitsNearby = nrUnitsNearby;
//			bestPos = uPos;
//		}
//	}
//
//	if (mostUnitsNearby > 1) {
//		moveCamera(bestPos, prio);
//	}
//}
//
//void SSCAITournamentAI::moveCamera(BWAPI::Position pos, int priority) {
//	if (!shouldMoveCamera(priority))
//	{
//		return;
//	}
//
//	BWAPI::Position currentMovedPosition = pos - BWAPI::Position(320, 200);
//
//	if (currentMovedPosition.getDistance(lastMovedPosition) > 4.0f * TILE_SIZE && currentMovedPosition.isValid()) {
//		BWAPI::Broodwar->setScreenPosition(currentMovedPosition);
//		lastMovedPosition = currentMovedPosition;
//		lastMoved = BWAPI::Broodwar->getFrameCount();
//		lastMovedPriority = priority;
//	}
//}

void SSCAITournamentAI::onSendText(std::string text)
{
	frameTimes[BWAPI::Broodwar->getFrameCount()] += BWAPI::Broodwar->getLastEventTime();
}

void SSCAITournamentAI::onReceiveText(BWAPI::Player* player, std::string text)
{
	frameTimes[BWAPI::Broodwar->getFrameCount()] += BWAPI::Broodwar->getLastEventTime();
}

void SSCAITournamentAI::onPlayerLeft(BWAPI::Player* player)
{
	frameTimes[BWAPI::Broodwar->getFrameCount()] += BWAPI::Broodwar->getLastEventTime();
}

void SSCAITournamentAI::onPlayerDropped(BWAPI::Player* player)
{
	frameTimes[BWAPI::Broodwar->getFrameCount()] += BWAPI::Broodwar->getLastEventTime();
}

void SSCAITournamentAI::onNukeDetect(BWAPI::Position target)
{
	frameTimes[BWAPI::Broodwar->getFrameCount()] += BWAPI::Broodwar->getLastEventTime();
}

void SSCAITournamentAI::onUnitDiscover(BWAPI::Unit* unit)
{
	frameTimes[BWAPI::Broodwar->getFrameCount()] += BWAPI::Broodwar->getLastEventTime();
}

void SSCAITournamentAI::onUnitEvade(BWAPI::Unit* unit)
{
	frameTimes[BWAPI::Broodwar->getFrameCount()] += BWAPI::Broodwar->getLastEventTime();
}

void SSCAITournamentAI::onUnitShow(BWAPI::Unit* unit)
{
	frameTimes[BWAPI::Broodwar->getFrameCount()] += BWAPI::Broodwar->getLastEventTime();
}

void SSCAITournamentAI::onUnitHide(BWAPI::Unit* unit)
{
	frameTimes[BWAPI::Broodwar->getFrameCount()] += BWAPI::Broodwar->getLastEventTime();
}

void SSCAITournamentAI::onUnitCreate(BWAPI::Unit* unit)
{
	frameTimes[BWAPI::Broodwar->getFrameCount()] += BWAPI::Broodwar->getLastEventTime();
}

void SSCAITournamentAI::onUnitDestroy(BWAPI::Unit* unit)
{
	frameTimes[BWAPI::Broodwar->getFrameCount()] += BWAPI::Broodwar->getLastEventTime();
}

void SSCAITournamentAI::onUnitMorph(BWAPI::Unit* unit)
{
	frameTimes[BWAPI::Broodwar->getFrameCount()] += BWAPI::Broodwar->getLastEventTime();
}

void SSCAITournamentAI::onUnitComplete(BWAPI::Unit *unit)
{
	frameTimes[BWAPI::Broodwar->getFrameCount()] += BWAPI::Broodwar->getLastEventTime();
	
	camera.moveCameraUnitCreated(unit);
}

void SSCAITournamentAI::onUnitRenegade(BWAPI::Unit* unit)
{
}

void SSCAITournamentAI::onSaveGame(std::string gameName)
{
}

bool SSCAITournamentModule::onAction(int actionType, void *parameter)
{
	switch ( actionType )
	{
		case Tournament::EnableFlag:
			switch ( *(int*)parameter )
			{
				case Flag::CompleteMapInformation:		return false;
				case Flag::UserInput:					return false;
			}

		case Tournament::PauseGame:
		case Tournament::RestartGame:
		case Tournament::ResumeGame:
		case Tournament::SetFrameSkip:
		case Tournament::SetGUI:
		case Tournament::SetLocalSpeed:					return false;
		case Tournament::SetMap:						return false; 
		case Tournament::LeaveGame:
		case Tournament::ChangeRace:
		case Tournament::SetLatCom:
		case Tournament::SetTextSize:
		case Tournament::SendText:
		case Tournament::Printf:						
		case Tournament::SetCommandOptimizationLevel:	return false; 
							
		default:										break;
	}

	return true;
}

void SSCAITournamentModule::onFirstAdvertisement()
{

}


std::vector<std::string> SSCAITournamentAI::getLines(const std::string & filename)
{
    // set up the file
    std::ifstream fin(filename.c_str());
    if (!fin.is_open())
    {
		BWAPI::Broodwar->printf("Tournament Module Settings File Not Found, Using Defaults", filename.c_str());
		return std::vector<std::string>();
    }

	std::string line;

    std::vector<std::string> lines;

    // each line of the file will be a new player to add
    while (fin.good())
    {
        // get the line and set up the string stream
        getline(fin, line);
       
        // skip blank lines and comments
        if (line.length() > 1 && line[0] != '#')
        {
            lines.push_back(line);
        }
    }

	fin.close();

    return lines;
}

void SSCAITournamentAI::parseConfigFile(const std::string & filename)
{
    std::vector<std::string> lines(getLines(filename));

	if (lines.size() > 0)
	{
		timerLimits.clear();
		timerLimitsBound.clear();
		timerLimitsExceeded.clear();
	}

    for (size_t l(0); l<lines.size(); ++l)
    {
        std::istringstream iss(lines[l]);
        std::string option;
        iss >> option;

        if (strcmp(option.c_str(), "LocalSpeed") == 0)
        {
			iss >> localSpeed;
        }
        else if (strcmp(option.c_str(), "FrameSkip") == 0)
        {
            iss >> frameSkip;
        }
		else if (strcmp(option.c_str(), "CameraMoveTime") == 0)
		{
			iss >> camera.cameraMoveTime;
		}
		else if (strcmp(option.c_str(), "CameraMoveTimeMin") == 0)
		{
			iss >> camera.cameraMoveTimeMin;
		}
		else if (strcmp(option.c_str(), "WatchScoutWorkerUntil") == 0)
		{
			iss >> camera.watchScoutWorkerUntil;
		}
        else if (strcmp(option.c_str(), "Timeout") == 0)
        {
            int timeLimit = 0;
			int bound = 0;

			iss >> timeLimit;
			iss >> bound;

			timerLimits.push_back(timeLimit);
			timerLimitsBound.push_back(bound);
			timerLimitsExceeded.push_back(0);
        }
        else if (strcmp(option.c_str(), "GameFrameLimit") == 0)
        {
			iss >> gameTimeLimit;
        }
		else if (strcmp(option.c_str(), "ZeroSpeedTime") == 0)
		{
			iss >> zeroSpeedTime;
		}
		else if (strcmp(option.c_str(), "DrawUnitInfo") == 0)
        {
            std::string val;
			iss >> val;
            
			if (strcmp(val.c_str(), "false") == 0)
			{
				drawUnitInfo = false;
			}
        }
		else if (strcmp(option.c_str(), "DrawTournamentInfo") == 0)
        {
            std::string val;
			iss >> val;
            
			if (strcmp(val.c_str(), "false") == 0)
			{
				drawTournamentInfo = false;
			}
        }
		else if (strcmp(option.c_str(), "DrawBotNames") == 0)
        {
            std::string val;
			iss >> val;
            
			if (strcmp(val.c_str(), "false") == 0)
			{
				drawBotNames = false;
			}
        }
		else
		{
			BWAPI::Broodwar->printf("Invalid Option in Tournament Module Settings: %s", option.c_str());
		}
    }
}