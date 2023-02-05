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
#include "CommentaryModule.h"

#define DEFAULT_COMMAND_OPTIMIZATION 1
#define MINIMUM_COMMAND_OPTIMIZATION 0

class SSCAITournamentModule : public BWAPI::TournamentModule
{
  bool onAction(BWAPI::Tournament::ActionID actionType, void *parameter = NULL) override;
  void onFirstAdvertisement() override;
};

class SSCAITournamentAI : public BWAPI::AIModule
{
private:
  CameraModule camera;
  CommentaryModule commentary;
  BWAPI::Position myStartLocation;

public:
  virtual void onStart() override;
  virtual void onEnd(bool isWinner) override;
  virtual void onFrame() override;
  virtual void onSendText(std::string text) override;
  virtual void onReceiveText(BWAPI::Player player, std::string text) override;
  virtual void onPlayerLeft(BWAPI::Player player) override;
  virtual void onNukeDetect(BWAPI::Position target) override;
  virtual void onUnitDiscover(BWAPI::Unit unit) override;
  virtual void onUnitEvade(BWAPI::Unit unit) override;
  virtual void onUnitShow(BWAPI::Unit unit) override;
  virtual void onUnitHide(BWAPI::Unit unit) override;
  virtual void onUnitCreate(BWAPI::Unit unit) override;
  virtual void onUnitDestroy(BWAPI::Unit unit) override;
  virtual void onUnitMorph(BWAPI::Unit unit) override;
  virtual void onUnitRenegade(BWAPI::Unit unit) override;
  virtual void onSaveGame(std::string gameName) override;
  virtual void onUnitComplete(BWAPI::Unit unit) override;
  virtual void updateFrameTimers();
  virtual void drawUnitInformation(int x, int y);
  virtual void drawTournamentModuleSettings();
  virtual void parseTMConfigFile(const std::string & filename);
  virtual void parseCommentaryConfigFile(const std::string & filename);
  virtual std::vector<std::string> getLines(const std::string & filename);
};
