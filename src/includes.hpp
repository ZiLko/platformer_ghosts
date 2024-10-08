#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <cvolton.level-id-api/include/EditorIDs.hpp>
#include <nlohmann/json.hpp>

const std::unordered_set<int> gamemodePortals = {12, 13, 47, 111, 660, 745, 1331, 1933};

enum ActionType {
	Position = 1,
	Death = 2,
	Respawn = 3,
	Vehicle = 4,
	RobotAnimation = 5,
	SpiderAnimation = 6,
	Dual = 7,
	Mini = 8,
	Flip = 9
};

enum VehicleType {
	Cube = 12,
	Ship = 13,
    Ball = 47,
	Ufo = 111,
    Wave = 660,
    Robot = 745,
    Spider = 1331,
    Swing = 1933
};

struct FlipData {
	bool player2 = false;
	bool flip = false;
	bool y = false;
};

struct VehicleData {
	VehicleType vehicle;
	bool player2;
};

struct PlayerData {
	cocos2d::CCPoint position = { 0.f, 0.f };
	float rotation = 0.f;
};

struct PositionData {
	PlayerData p1Data;
	PlayerData p2Data;
};

struct Action {
	ActionType type = ActionType::Position;
	unsigned int frame = 1;
	std::variant<PositionData, FlipData, VehicleData, std::string, bool> data;
};

// switch (p0->m_objectID) {
        //     case 13: {
        //         Action action;
        //     break;
        //     } case 12; {
        //         Action action;
        //     // cube
        //     break;
        //     } case 47: {
        //         Action action;
        //     //ball
        //     break;
        //     } case 2926: {
        //         Action action;
        //     //green portal
        //     break;
        //     } case 286: {
        //         Action action;
        //     //enter dual
        //     break;
        //     } case 287: {
        //         Action action;
        //     // exit dual
        //     break;
        //     } case 99: {
        //         Action action;
        //     //big
        //     break;
        //     } case 101: {
        //         Action action;
        //     //mini
        //     break;
        //     } case 745: {
        //         Action action;
        //     //robot
        //     break;
        //     } case 660: {
        //         Action action;
        //     //wave
        //     break;
        //     } case 111: {
        //         Action action;
        //     //ufo
        //     break;
        //     } case 1331: {
        //         Action action;
        //     //spider
        //     break;
        //     } case 1933: {
        //         Action action;
        //     //swing
        //     break;
        //     } case 10: {
        //         Action action;
        //     //normal gravity
        //     break;
        //     } case 11: {
        //         Action action;
        //     //inverted gravity
        //     } break;
        // }