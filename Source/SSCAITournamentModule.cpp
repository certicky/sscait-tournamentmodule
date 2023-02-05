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

	void update(std::vector<int> & times, int nrFrames)
	{
		// if game ended because of no-kills limit, set framecount to frame limit (parameter)
		frameCount		= nrFrames;

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
bool gameOverNoKillsLimit = false;

int targetLocalSpeed = 10; // the local speed used during gameplay (excluding initial and final max speeds)
int localSpeed = targetLocalSpeed; // actual current game speed, gets overwritten when changing speed
int frameSkip = 0;
int gameTimeLimit = 85714;
int zeroSpeedTime = 85714;
int oldFrameCount = -1;
int numPrevEventsThisFrame = 0;
int noKillsSecondsLimit = 300;
int gameStartMaxSpeedTime = 90*16; // nr of frames with max speed in start of the game (16 frames/second)
int noCombatSpeedUpTime = 10*60*16; // nr of frames to start speed up non-combat situations (10 in-game minutes)
int noCombatSpeedUpDelay = 30*16; // nr of frames before speeding up game (30 in-game seconds)
int noActionCrashLimit = 60 * 16; // nr of frames of inactivity before regarding as crash
bool possibleInactivityCrash = true; // will be set to false if any unit moves within the time specified by noActionCrashLimit
bool alreadyPrintedInactivityMsg = false; // used to only print message once

int screenWidth = 640;
int screenHeight = 480;

bool drawBotNames = true;
bool drawUnitInfo = false;
bool drawTournamentInfo = true;
bool eventTimesVaried = false;
bool enableCommentary = false;
std::string commentaryGeneratorURL = "";

char buffer[MAX_PATH];
std::string folder;

std::vector<int> frameTimes(100000,0);

Timer killLimitTimer;
int timeOfLastKill = 0;
int nrFramesOfLastCombat = 0;

void SSCAITournamentAI::updateFrameTimers()
{
	const int eventTime = BWAPI::Broodwar->getLastEventTime();
	const int frameCount = BWAPI::Broodwar->getFrameCount();

	// For a client bot, if the TM calls BWAPI v4.4.0's getLastEventTime() it
	// returns the total time for all events for the current frame (not just
	// for the last event), and it returns the same value regardless of which
	// TM callback method (onUnitDiscover(), onFrame() etc) is calling
	// getLastEventTime(). We don't want to count the same amount multiple
	// times. So, we try to detect whether we should interpret the value as
	// the total time for all events for that frame or just the time for the
	// last event, by examining whether getLastEventTime() has ever returned
	// different values during the same frame. For the frames before it is
	// detected, we interpret it as meaning the total time for all events for
	// that frame. Future versions of BWAPI might solve the problem for us,
	// but for v4.4.0 at least, we use this workaround. BWAPI versions before
	// v4.4.0 don't time client bots at all, so the workaround isn't needed
	// in those versions.
	if (frameCount != oldFrameCount)
	{
		frameTimes[frameCount] = eventTime;
		numPrevEventsThisFrame = 1;
		oldFrameCount = frameCount;
	}
	else
	{
		if (eventTimesVaried)
		{
			frameTimes[frameCount] += eventTime;
		}
		else if (eventTime != frameTimes[frameCount])
		{
			eventTimesVaried = true;
			frameTimes[frameCount] = (frameTimes[frameCount] * numPrevEventsThisFrame) + eventTime;
		}

		++numPrevEventsThisFrame;
	}
}

void SSCAITournamentAI::onStart()
{
	GetModuleFileName(NULL, buffer, MAX_PATH);
	BWAPI::Broodwar->printf("Path is %s", buffer);
	std::string path(buffer);
	folder = path.substr(0, path.find_last_of("/\\")); // removes \\StarCraft.exe

	// Set the command optimization level (reduces high APM, size of bloated replays, etc)
	Broodwar->setCommandOptimizationLevel(DEFAULT_COMMAND_OPTIMIZATION);

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
	parseTMConfigFile("bwapi-data\\tm_settings.ini");

	Broodwar->setLocalSpeed(0); // starts game with max speed, switches to normal speed after some time
	localSpeed = 0;
	Broodwar->setFrameSkip(frameSkip);

	updateFrameTimers();

	myStartLocation = Position(Broodwar->self()->getStartLocation().x*TILE_SIZE, Broodwar->self()->getStartLocation().y*TILE_SIZE);
	Broodwar->printf("Start position: %d %d", myStartLocation.x, myStartLocation.y);

	camera.onStart(myStartLocation, screenWidth, screenHeight);
	killLimitTimer.start();

	const std::string& name = "commentary_settings.ini";
	if (FILE *file = fopen(name.c_str(), "r")) {
		fclose(file);
		parseCommentaryConfigFile(name);
	}
	if (enableCommentary) {
		commentary = CommentaryModule();
		commentary.onStart(commentaryGeneratorURL);
	}
}


void SSCAITournamentAI::onEnd(bool isWinner)
{
	TournamentModuleState state = TournamentModuleState();
	state.ended();
	int frameCount = gameOverNoKillsLimit ? gameTimeLimit : Broodwar->getFrameCount();
	state.update(timerLimitsExceeded, frameCount);
	state.write("gameState.txt", folder);
}

void SSCAITournamentAI::onFrame()
{
	if ((int)frameTimes.size() < BWAPI::Broodwar->getFrameCount() + 10)
	{
		frameTimes.push_back(0);
	}

	camera.onFrame();

	if (Broodwar->getFrameCount() % 360 == 0 && (!possibleInactivityCrash || Broodwar->getFrameCount() < noActionCrashLimit))
	{
		TournamentModuleState state = TournamentModuleState();
		state.update(timerLimitsExceeded, Broodwar->getFrameCount());
		state.write("gameState.txt", folder);
	}
	else if (Broodwar->getFrameCount() >= noActionCrashLimit && possibleInactivityCrash && !alreadyPrintedInactivityMsg)
	{
		Broodwar->sendText("No activity detected for the first in-game minute, exiting");
		alreadyPrintedInactivityMsg = true;
	}

	if (killLimitTimer.getElapsedTimeInSec() - timeOfLastKill > noKillsSecondsLimit)
	{
		Broodwar->sendText("No unit killed for %d seconds, exiting", noKillsSecondsLimit);
		gameOverNoKillsLimit = true;
		Broodwar->leaveGame();
	}

	if ((gameTimeLimit > 0) && (Broodwar->getFrameCount() > gameTimeLimit))
	{
		Broodwar->sendText("Game time limit of %d frames reached, exiting", gameTimeLimit);
		Broodwar->leaveGame();
	}

	if (possibleInactivityCrash)
	{
		// Checking for activity
		for each (BWAPI::Unit unit in Broodwar->self()->getUnits()) {
			if (unit->getLastCommandFrame() > 0) {
				possibleInactivityCrash = false;
			}
		}
	}
	
	int frame = BWAPI::Broodwar->getFrameCount();
	// check the sum of the times for the previous frame

	if (frame < 10)
	{
		return;
	}

	// add the framer times for this frame
	updateFrameTimers();

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

	drawTournamentModuleSettings();

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
			for each (BWAPI::Unit unit1 in Broodwar->enemy()->getUnits())
			{
				BWAPI::Position uPos = unit1->getPosition();
				for each (BWAPI::Unit unit2 in BWAPI::Broodwar->getUnitsInRadius(uPos, radius))
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

	if (Broodwar->getFrameCount() % 50 == 0) {
		if (enableCommentary) commentary.speak();
	}
}

void SSCAITournamentAI::drawTournamentModuleSettings()
{
	bool largeScreen = screenWidth > 800 && screenHeight > 600;
	int hudOffset = 190;
	int nrTextRows = 5;
	BWAPI::Text::Size::Enum textSize = BWAPI::Text::Size::Default;
	BWAPI::Text::Size::Enum textSizeLarger = BWAPI::Text::Size::Large;
	int rowDistance = 10;
	int width = 120;
	int rightSideInset = 220;
	if (largeScreen)
	{
		textSize = BWAPI::Text::Size::Large;
		textSizeLarger = BWAPI::Text::Size::Huge;
		rowDistance = 15;
		width = 180;
		rightSideInset = 300;
	}
	int x = 10;
	int y = screenHeight - hudOffset - nrTextRows*rowDistance;
	int drawX = x;
	int drawY = y;

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
		BWAPI::Broodwar->setTextSize(textSize);

		BWAPI::Broodwar->drawTextScreen(drawX, drawY, "\x04 Elapsed in-game time: ");
		BWAPI::Broodwar->drawTextScreen(drawX + width, drawY, " %d:%s%d (%d)", Broodwar->elapsedTime() / 60, (Broodwar->elapsedTime() % 60 < 10) ? "0" : "", Broodwar->elapsedTime() % 60, Broodwar->getFrameCount());
		drawY += rowDistance;

		BWAPI::Broodwar->drawTextScreen(drawX, drawY, "\x04 Game time limit: ");
		BWAPI::Broodwar->drawTextScreen(drawX + width, drawY, " %d:%s%d (%d)", gameTimeLimit / 16 / 60, ((gameTimeLimit / 16) % 60 < 10) ? "0" : "", (gameTimeLimit / 16) % 60, gameTimeLimit);
		drawY += rowDistance;

		BWAPI::Broodwar->drawTextScreen(drawX, drawY, "\x04 BWAPI local speed: ");
		BWAPI::Broodwar->drawTextScreen(drawX + width, drawY, " %d", localSpeed);
		drawY += rowDistance;

		BWAPI::Broodwar->drawTextScreen(drawX, drawY, "\x04 BWAPI frame skip: ");
		BWAPI::Broodwar->drawTextScreen(drawX + width, drawY, " %d", frameSkip);
		drawY += rowDistance;

		BWAPI::Broodwar->drawTextScreen(drawX, drawY, "\x04 No-kills time limit: ");
		int seconds = (noKillsSecondsLimit - (int)(killLimitTimer.getElapsedTimeInSec() - timeOfLastKill));
		BWAPI::Broodwar->drawTextScreen(drawX + width, drawY, " %i:%s%i", seconds/60, (seconds % 60 < 10) ? "0" : "", seconds % 60);
		drawY += rowDistance;

		drawX = screenWidth - rightSideInset;
		drawY = y;

		BWAPI::Broodwar->setTextSize(textSizeLarger);
		Broodwar->drawTextScreen(drawX, drawY, "\x03%s", BWAPI::Broodwar->mapFileName().c_str());
		BWAPI::Broodwar->setTextSize(textSize);

		drawX -= 5;
		drawY += rowDistance+10;
		for (size_t t(0); t<timerLimits.size(); ++t)
		{
			BWAPI::Broodwar->drawTextScreen(drawX, drawY, "\x04 # Frames > %d ms ", timerLimits[t]);
			BWAPI::Broodwar->drawTextScreen(drawX + width, drawY, " %d   (Max %d)", timerLimitsExceeded[t], timerLimitsBound[t]);
			drawY += rowDistance;
		}
	}

	if (drawBotNames)
	{
		int boxX = screenWidth/2 - 120;
		int rowY = screenHeight - 180;
		int boxSize = 17;
		
		Broodwar->drawBoxScreen(boxX, rowY, boxX + boxSize, rowY + boxSize, Colors::Green, true);
		Broodwar->drawBoxScreen(boxX + 1, rowY + 1, boxX + boxSize-1, rowY + boxSize-1, Broodwar->self()->getColor(), true);

		BWAPI::Broodwar->setTextSize(textSizeLarger);
		Broodwar->drawTextScreen(boxX + 25, rowY - 1, "\x07%s (%c)", BWAPI::Broodwar->self()->getName().c_str(), Broodwar->self()->getRace().getName().c_str()[0]);

		rowY = rowY + 30;

		Broodwar->drawBoxScreen(boxX, rowY, boxX + boxSize, rowY + boxSize, Colors::Green, true);
		Broodwar->drawBoxScreen(boxX + 1, rowY + 1, boxX + boxSize-1, rowY + boxSize-1, Broodwar->enemy()->getColor(), true);

		BWAPI::Broodwar->setTextSize(textSizeLarger);
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
	updateFrameTimers();
	if (enableCommentary) commentary.onSendText(text);
}

void SSCAITournamentAI::onReceiveText(BWAPI::Player player, std::string text)
{
	updateFrameTimers();
	if (enableCommentary) commentary.onReceiveText(player, text);
}

void SSCAITournamentAI::onPlayerLeft(BWAPI::Player player)
{
	updateFrameTimers();
}

void SSCAITournamentAI::onNukeDetect(BWAPI::Position target)
{
	updateFrameTimers();

	camera.moveCameraNukeDetect(target);
}

void SSCAITournamentAI::onUnitDiscover(BWAPI::Unit unit)
{
	updateFrameTimers();
}

void SSCAITournamentAI::onUnitEvade(BWAPI::Unit unit)
{
	updateFrameTimers();
}

void SSCAITournamentAI::onUnitShow(BWAPI::Unit unit)
{
	updateFrameTimers();
	if (enableCommentary) commentary.onUnitShow(unit);
}

void SSCAITournamentAI::onUnitHide(BWAPI::Unit unit)
{
	updateFrameTimers();
}

void SSCAITournamentAI::onUnitCreate(BWAPI::Unit unit)
{
	updateFrameTimers();
}

void SSCAITournamentAI::onUnitDestroy(BWAPI::Unit unit)
{
	updateFrameTimers();
	if (!(unit->getType().isMineralField() || unit->getType().isSpell() || unit->isHallucination()))
	{
		timeOfLastKill = (int)killLimitTimer.getElapsedTimeInSec();
		nrFramesOfLastCombat = Broodwar->getFrameCount();
	}
	
	if (localSpeed != targetLocalSpeed && Broodwar->getFrameCount() < zeroSpeedTime)
	{
		Broodwar->setLocalSpeed(targetLocalSpeed);
		localSpeed = targetLocalSpeed;
	}
	if (enableCommentary) commentary.onUnitDestroy(unit);
}

void SSCAITournamentAI::onUnitMorph(BWAPI::Unit unit)
{
	updateFrameTimers();
}

void SSCAITournamentAI::onUnitComplete(BWAPI::Unit unit)
{
	updateFrameTimers();
	
	camera.moveCameraUnitCreated(unit);
}

void SSCAITournamentAI::onUnitRenegade(BWAPI::Unit unit)
{
	updateFrameTimers();
}

void SSCAITournamentAI::onSaveGame(std::string gameName)
{
	updateFrameTimers();
}

bool SSCAITournamentModule::onAction(BWAPI::Tournament::ActionID actionType, void *parameter)
{
	switch ( actionType )
	{
		case Tournament::EnableFlag:
			switch ( *(int*)parameter )
			{
				case Flag::CompleteMapInformation:		return false;
				case Flag::UserInput:					return false;
				default:								break;
			}
			// If more flags are added, by default disallow unrecognized flags
			return false;

		case Tournament::PauseGame:						return false;
		case Tournament::ResumeGame:					return false;
		case Tournament::SetFrameSkip:					return false;
		case Tournament::SetGUI:						return false;
		case Tournament::SetLocalSpeed:					return false;
		case Tournament::SetMap:						return false; 
		case Tournament::LeaveGame:						return true;
		case Tournament::SetLatCom:						return true;
		case Tournament::SetTextSize:					return true;
		case Tournament::SendText:						return true;
		case Tournament::Printf:						return true;
		case Tournament::SetCommandOptimizationLevel:
			return *(int*)parameter >= MINIMUM_COMMAND_OPTIMIZATION;
							
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

void SSCAITournamentAI::parseTMConfigFile(const std::string & filename)
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
		else if (strcmp(option.c_str(), "ScreenWidth") == 0)
		{
			iss >> screenWidth;
		}
		else if (strcmp(option.c_str(), "ScreenHeight") == 0)
		{
			iss >> screenHeight;
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

void SSCAITournamentAI::parseCommentaryConfigFile(const std::string & filename)
{
	std::vector<std::string> lines(getLines(filename));

	for (size_t l(0); l < lines.size(); ++l)
	{
		std::istringstream iss(lines[l]);
		std::string option;
		iss >> option;

		if (strcmp(option.c_str(), "EnableCommentary") == 0)
		{
			std::string val;
			iss >> val;

			if (strcmp(val.c_str(), "1") == 0 || strcmp(val.c_str(), "true") == 0)
			{
				enableCommentary = true;
			}
		}
		else if (strcmp(option.c_str(), "CommentaryGeneratorURL") == 0)
		{
			iss >> commentaryGeneratorURL;
		}
	}
}