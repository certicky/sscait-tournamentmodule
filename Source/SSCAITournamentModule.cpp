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
		// TODO if no-kills limit, set framecount to frame limit
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
	
	bool write(std::string filename, std::string directory)
	{
		char tmpDir[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, tmpDir);
		// change current directory the correct one
		SetCurrentDirectory(directory.c_str());
		
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

		// change current directory back to the previous, to not make life too hard for bot developers
		SetCurrentDirectory(tmpDir);
	}	
};

std::vector<int> timerLimits;
std::vector<int> timerLimitsBound;
std::vector<int> timerLimitsExceeded;

int targetLocalSpeed = 10; // the local speed used during gameplay (excluding initial and final max speeds)
int localSpeed = targetLocalSpeed; // actual current game speed, gets overwritten when changing speed
int frameSkip = 0;
int gameTimeLimit = 85714;
int zeroSpeedTime = 85714;
int noKillsSecondsLimit = 300;
int gameStartMaxSpeedTime = 90*16; // nr of frames with max speed in start of the game (16 frames/second)
int noCombatSpeedUpTime = 10*60*16; // nr of frames to start speed up non-combat situations (10 in-game minutes)
int noCombatSpeedUpDelay = 30*16; // nr of frames before speeding up game (30 in-game seconds)

bool drawBotNames = true;
bool drawUnitInfo = false;
bool drawTournamentInfo = true;

char buffer[MAX_PATH];
std::string folder;

std::vector<int> frameTimes(100000,0);

Timer killLimitTimer;
int timeOfLastKill = 0;
int nrFramesOfLastCombat = 0;

void SSCAITournamentAI::onStart()
{
	GetModuleFileName(NULL, buffer, MAX_PATH);
	BWAPI::Broodwar->printf("Path is %s", buffer);
	std::string path(buffer);
	folder = path.substr(0, path.find_last_of("/\\")); // removes \\StarCraft.exe

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
	
	Broodwar->setLocalSpeed(0); // starts game with max speed, switches to normal speed after some time
	localSpeed = 0;
	Broodwar->setFrameSkip(frameSkip);

	myStartLocation = Position(Broodwar->self()->getStartLocation().x()*TILE_SIZE, Broodwar->self()->getStartLocation().y()*TILE_SIZE);
	Broodwar->printf("Start position: %d %d", myStartLocation.x(), myStartLocation.y());

	camera.onStart(myStartLocation);
	killLimitTimer.start();
}


void SSCAITournamentAI::onEnd(bool isWinner)
{
	TournamentModuleState state = TournamentModuleState();
	state.ended();
	state.update(timerLimitsExceeded);
	state.write("gameState.txt", folder);
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
		state.write("gameState.txt", folder);
	}

	if (killLimitTimer.getElapsedTimeInSec() - timeOfLastKill > noKillsSecondsLimit)
	{
		Broodwar->sendText("No unit killed for %d seconds, exiting", noKillsSecondsLimit);
		Broodwar->leaveGame();
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

	if (localSpeed != targetLocalSpeed && Broodwar->getFrameCount() < zeroSpeedTime && Broodwar->getFrameCount() >= gameStartMaxSpeedTime)
	{
		Broodwar->setLocalSpeed(targetLocalSpeed);
		localSpeed = targetLocalSpeed;
	}

	if (Broodwar->getFrameCount() >= zeroSpeedTime && Broodwar->getFrameCount() <= zeroSpeedTime+32) {
		Broodwar->setLocalSpeed(0);
		localSpeed = 0;
	}

	if (Broodwar->getFrameCount() >= noCombatSpeedUpTime 
		&& Broodwar->getFrameCount() < zeroSpeedTime) //if in correct interval
	{
		if (Broodwar->getFrameCount() % 16 == 0)
		{
			//Check if any of our units are close to enemies, if so, make sure not zero speed
			int radius = 150;
			bool unitCloseEnough = false;
			for each (BWAPI::Unit* unit1 in Broodwar->enemy()->getUnits())
			{
				BWAPI::Position uPos = unit1->getPosition();
				for each (BWAPI::Unit* unit2 in BWAPI::Broodwar->getUnitsInRadius(uPos, radius))
				{
					if (unit2->getPlayer() == Broodwar->self())
					{
						unitCloseEnough = true;
						break;
					}
				}
				if (unitCloseEnough)
				{
					nrFramesOfLastCombat = Broodwar->getFrameCount();
					break;
				}
			}
		}

		if (localSpeed != 0 && Broodwar->getFrameCount() > nrFramesOfLastCombat+noCombatSpeedUpDelay)
		{
			Broodwar->setLocalSpeed(0);
			localSpeed = 0;
		}
		else if (localSpeed != targetLocalSpeed && Broodwar->getFrameCount() <= nrFramesOfLastCombat+noCombatSpeedUpDelay)
		{
			Broodwar->setLocalSpeed(targetLocalSpeed);
			localSpeed = targetLocalSpeed;
		}
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

		BWAPI::Broodwar->drawTextScreen(drawX, drawY, "\x04 No-kills time limit: ");
		int seconds = (noKillsSecondsLimit - (int)(killLimitTimer.getElapsedTimeInSec() - timeOfLastKill));
		BWAPI::Broodwar->drawTextScreen(drawX + width, drawY, " %i:%s%i", seconds/60, (seconds % 60 < 10) ? "0" : "", seconds % 60);
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
	if (!(unit->getType().isMineralField() || unit->getType().isSpell() || unit->isHallucination()))
	{
		timeOfLastKill = killLimitTimer.getElapsedTimeInSec();
		nrFramesOfLastCombat = Broodwar->getFrameCount();
	}
	
	if (localSpeed != targetLocalSpeed && Broodwar->getFrameCount() < zeroSpeedTime)
	{
		Broodwar->setLocalSpeed(targetLocalSpeed);
		localSpeed = targetLocalSpeed;
	}
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
			iss >> targetLocalSpeed;
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
		else if (strcmp(option.c_str(), "NoKillsRealSecondsLimit") == 0)
		{
			iss >> noKillsSecondsLimit;
		}
		else if (strcmp(option.c_str(), "NoCombatSpeedUpTime") == 0)
		{
			iss >> noCombatSpeedUpTime;
		}
		else if (strcmp(option.c_str(), "NoCombatSpeedUpDelay") == 0)
		{
			iss >> noCombatSpeedUpDelay;
		}
		else if (strcmp(option.c_str(), "InitMaxSpeedTime") == 0)
		{
			iss >> gameStartMaxSpeedTime;
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