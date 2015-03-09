#include "CameraModule.h"

using namespace BWAPI;


CameraModule::CameraModule()
{
	cameraMoveTime = 150;
	cameraMoveTimeMin = 50;
	watchScoutWorkerUntil = 7000;
	lastMoved = 0;
	lastMovedPriority = 0;
	lastMovedPosition = BWAPI::Position(0, 0);
	cameraFocusPosition = BWAPI::Position(0, 0);
	cameraFocusUnit = NULL;
	followUnit = false;
}

void CameraModule::onStart(BWAPI::Position startPos, int screenWidth, int screenHeight)
{
	myStartLocation = startPos;
	cameraFocusPosition = startPos;
	scrWidth = screenWidth;
	scrHeight = screenHeight;
}

void CameraModule::onFrame()
{
	moveCameraIsUnderAttack();
	moveCameraIsAttacking();
	if (Broodwar->getFrameCount() <= watchScoutWorkerUntil) {
		moveCameraScoutWorker();
	}
	moveCameraArmy();
	moveCameraDrop();

	updateCameraPosition();
}

bool CameraModule::isNearEnemyBuilding(BWAPI::Unit* unit, std::set<BWAPI::Unit*> &enemyUnits) {
	for (std::set<BWAPI::Unit*>::iterator it = enemyUnits.begin(); it != enemyUnits.end(); it++) {
		if ((*it)->getType().isBuilding() && unit->getDistance(*it) <= TILE_SIZE*20 && (Broodwar->self()->isEnemy((*it)->getPlayer()) || ((*it)->getPlayer()->isNeutral() && (*it)->getType().isAddon())) && (*it)->getType() != UnitTypes::Terran_Vulture_Spider_Mine/* && !(*it)->isLifted()*/) {
			return true;
		}
	}

	return false;
}

bool CameraModule::isNearStartLocation(BWAPI::Position pos) {
	int distance = 1000;
	std::set<BWAPI::TilePosition> startLocations = Broodwar->getStartLocations();

	for (std::set<BWAPI::TilePosition>::iterator it = startLocations.begin(); it != startLocations.end(); it++) {
		Position startLocation = BWAPI::Position(*it);
		
		if (!isNearOwnStartLocation(startLocation) && startLocation.getDistance(pos) <= distance) {
			return true;
		}
	}

	return false;
}

bool CameraModule::isNearOwnStartLocation(BWAPI::Position pos) {
	int distance = 10 * TILE_SIZE;
	return (myStartLocation.getDistance(pos) <= distance);
}

bool CameraModule::isArmyUnit(BWAPI::Unit* unit) {
	return !(unit->getType().isWorker()
		|| unit->getType().isBuilding()
		|| unit->getType() == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine
		|| unit->getType() == BWAPI::UnitTypes::Zerg_Overlord
		|| unit->getType() == BWAPI::UnitTypes::Zerg_Larva);
}

bool CameraModule::shouldMoveCamera(int priority) {
	return !(BWAPI::Broodwar->getFrameCount() - lastMoved < cameraMoveTime && (lastMovedPriority >= priority || BWAPI::Broodwar->getFrameCount() - lastMoved < cameraMoveTimeMin));
}

void CameraModule::moveCameraIsUnderAttack()
{
	int prio = 3;
	if (!shouldMoveCamera(prio))
	{
		return;
	}

	for each (BWAPI::Unit * unit in BWAPI::Broodwar->self()->getUnits())
	{
		if (unit->isUnderAttack())
		{
			moveCamera(unit->getPosition(), prio);
		}
	}
}

void CameraModule::moveCameraIsAttacking()
{
	int prio = 3;
	if (!shouldMoveCamera(prio))
	{
		return;
	}

	for each (BWAPI::Unit * unit in BWAPI::Broodwar->self()->getUnits())
	{
		if (unit->isAttacking())
		{
			moveCamera(unit->getPosition(), prio);
		}
	}
}

void CameraModule::moveCameraScoutWorker() {
	int highPrio = 2;
	int lowPrio = 0;
	if (!shouldMoveCamera(lowPrio))
	{
		return;
	}

	for each (BWAPI::Unit* unit in BWAPI::Broodwar->self()->getUnits()) {
		if (!unit->getType().isWorker()) {
			continue;
		}
		if (isNearStartLocation(unit->getPosition())) {
			moveCamera(unit->getPosition(), highPrio);
		} else if (!isNearOwnStartLocation(unit->getPosition())) {
			moveCamera(unit->getPosition(), lowPrio);
		}
	}
}

void CameraModule::moveCameraDrop() {
	int prio = 2;
	if (!shouldMoveCamera(prio))
	{
		return;
	}
	for each (BWAPI::Unit* unit in BWAPI::Broodwar->self()->getUnits()) {
		if ((unit->getType() == UnitTypes::Zerg_Overlord || unit->getType() == UnitTypes::Terran_Dropship || unit->getType() == UnitTypes::Protoss_Shuttle)
			&& isNearStartLocation(unit->getPosition()) && unit->getLoadedUnits().size() > 0) {
			moveCamera(unit->getPosition(), prio);
		}
	}
}

void CameraModule::moveCameraArmy() {
	int prio = 1;
	if (!shouldMoveCamera(prio))
	{
		return;
	}
	// Double loop, check if army units are close to each other
	int radius = 50;

	BWAPI::Position bestPos;
	int mostUnitsNearby = 0;

	for each (BWAPI::Unit* unit1 in BWAPI::Broodwar->getAllUnits()) {
		if (!isArmyUnit(unit1)) {
			continue;
		}
		BWAPI::Position uPos = unit1->getPosition();

		int nrUnitsNearby = 0;
		for each (BWAPI::Unit* unit2 in BWAPI::Broodwar->getUnitsInRadius(uPos, radius)) {
			if (!isArmyUnit(unit2)) {
				continue;
			}
			nrUnitsNearby++;
		}

		if (nrUnitsNearby > mostUnitsNearby) {
			mostUnitsNearby = nrUnitsNearby;
			bestPos = uPos;
		}
	}

	if (mostUnitsNearby > 1) {
		moveCamera(bestPos, prio);
	}
}

void CameraModule::moveCameraUnitCreated(BWAPI::Unit* unit)
{
	int prio = 1;
	if (!shouldMoveCamera(prio))
	{
		return;
	}
	else if (unit->getPlayer() == Broodwar->self() && !unit->getType().isWorker()) {
		moveCamera(unit->getPosition(), prio);
	}
}

void CameraModule::moveCamera(BWAPI::Position pos, int priority) {
	if (!shouldMoveCamera(priority))
	{
		return;
	}

	cameraFocusPosition = pos;
	lastMovedPosition = cameraFocusPosition;
	lastMoved = BWAPI::Broodwar->getFrameCount();
	lastMovedPriority = priority;
}

void CameraModule::updateCameraPosition() {
	// center position on screen
	BWAPI::Position currentMovedPosition = cameraFocusPosition - BWAPI::Position(scrWidth/2, scrHeight/2 - 40); // -40 to account for HUD

	if (currentMovedPosition.getDistance(lastMovedPosition) > 4.0f * TILE_SIZE && currentMovedPosition.isValid()) {
		BWAPI::Broodwar->setScreenPosition(currentMovedPosition);
	}
}
