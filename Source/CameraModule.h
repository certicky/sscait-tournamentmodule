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

class CameraModule
{
private:
  BWAPI::Position myStartLocation;

public:
  int cameraMoveTime;
  int cameraMoveTimeMin;
  int watchScoutWorkerUntil;
  int lastMoved;
  int lastMovedPriority;
  BWAPI::Position lastMovedPosition;

  CameraModule();
  virtual void onStart(BWAPI::Position startPos);
  virtual void onFrame();
  virtual bool isNearEnemyBuilding(BWAPI::Unit* unit, std::set<BWAPI::Unit*> &enemyUnits);
  virtual bool isNearStartLocation(BWAPI::Position pos);
  virtual bool isNearOwnStartLocation(BWAPI::Position pos);
  virtual bool isArmyUnit(BWAPI::Unit* unit);
  virtual bool shouldMoveCamera(int priority);
  virtual void moveCameraIsAttacking();
  virtual void moveCameraIsUnderAttack();
  virtual void moveCamera(BWAPI::Position pos, int priority);
  virtual void moveCameraScoutWorker();
  virtual void moveCameraDrop();
  virtual void moveCameraArmy();
  virtual void moveCameraUnitCreated(BWAPI::Unit* unit);
};