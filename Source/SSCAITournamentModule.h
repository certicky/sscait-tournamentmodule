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
#include "CameraModule.h"

#define MINIMUM_COMMAND_OPTIMIZATION 1

class SSCAITournamentModule : public BWAPI::TournamentModule
{
  virtual bool onAction(int actionType, void *parameter = NULL);
  virtual void onFirstAdvertisement();
};

class SSCAITournamentAI : public BWAPI::AIModule
{
private:
  CameraModule camera;
  BWAPI::Position myStartLocation;

public:
  virtual void onStart();
  virtual void onEnd(bool isWinner);
  virtual void onFrame();
  virtual void onSendText(std::string text);
  virtual void onReceiveText(BWAPI::Player* player, std::string text);
  virtual void onPlayerLeft(BWAPI::Player* player);
  virtual void onNukeDetect(BWAPI::Position target);
  virtual void onUnitDiscover(BWAPI::Unit* unit);
  virtual void onUnitEvade(BWAPI::Unit* unit);
  virtual void onUnitShow(BWAPI::Unit* unit);
  virtual void onUnitHide(BWAPI::Unit* unit);
  virtual void onUnitCreate(BWAPI::Unit* unit);
  virtual void onUnitDestroy(BWAPI::Unit* unit);
  virtual void onUnitMorph(BWAPI::Unit* unit);
  virtual void onUnitRenegade(BWAPI::Unit* unit);
  virtual void onSaveGame(std::string gameName);
  virtual void onUnitComplete(BWAPI::Unit *unit);
  virtual void onPlayerDropped(BWAPI::Player* player);
  virtual void drawUnitInformation(int x, int y);
  virtual void drawTournamentModuleSettings();
  virtual void parseConfigFile(const std::string & filename);
  virtual std::vector<std::string> getLines(const std::string & filename);
};
