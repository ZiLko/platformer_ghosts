#pragma once

using namespace geode::prelude;

#include <cvolton.level-id-api/include/EditorIDs.hpp>
#include <nlohmann/json.hpp>

enum ActionType {
	Position = 1,
	Effect = 2,
	Vehicle = 3,
	Animation = 4,
	Dual = 5,
	Mini = 6,
	Flip = 7,
	Sideways = 8,
	Input = 9,
	Reset = 10
};

enum AnimationType {
	Run = 1,
	Idle = 2,
	Jump = 3,
	Fall = 4
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

enum EffectType {
	Death = 1,
	Respawn = 2,
	Complete = 3,
};

struct GhostLevel {
    std::string name = "";
    int id = 0;
};

struct InputData {
	int button;
	bool down;
	bool player2;
};

struct EffectData {
	EffectType effect;
	bool player2;
};

struct AnimationData {
	AnimationType animation;
	VehicleType vehicle;
	bool player2;
};

struct SidewaysData {
	bool player2;
	float rotation;
};

struct FlipData {
	bool player2 = false;
	bool flip = false;
	bool y = false;
};

struct MiniData {
    bool mini = false;
    bool player2 = false;
};

struct VehicleData {
	VehicleType vehicle;
	bool player2;
};

struct PlayerData {
	cocos2d::CCPoint position = { 0.f, 0.f };
	float rotation = 0.f;
	bool rotationZero = false;
};

struct PositionData {
	PlayerData p1;
	PlayerData p2;
};

struct Action {
	ActionType type = ActionType::Position;
	unsigned int frame = 1;
	std::variant<PositionData, MiniData, FlipData, VehicleData, SidewaysData, AnimationData, EffectData, InputData, bool> data;
};

struct PlayerColors {
	int color1 = 12;
	int color2 = 12;
	int glowColor = 12;
	bool glowEnabled = false;
};

struct ReplayInfo {
	float time = 0.f;
	int levelId = 0;
	std::string username = "";
	std::string levelName = "";
	std::string date = "";
	std::unordered_map<VehicleType, int> icons = {
		{VehicleType::Cube, 1},
		{VehicleType::Ship, 1},
		{VehicleType::Ball, 1},
		{VehicleType::Ufo, 1},
		{VehicleType::Wave, 1},
		{VehicleType::Robot, 1},
		{VehicleType::Spider, 1},
		{VehicleType::Swing, 1},
		{VehicleType::Cube, 1}
	};
	PlayerColors colors;
	bool isNew = false;
};

struct Replay {
	ReplayInfo info;
	std::vector<Action> actions;
};

const std::unordered_set<int> gamemodePortals = {12, 13, 47, 111, 660, 745, 1331, 1933};

const std::unordered_set<int> nonInvertedVehicles = {
	VehicleType::Cube,	
	VehicleType::Swing,
	VehicleType::Ball	
};

const std::unordered_map<AnimationType, std::string> animationStrings = {
	{ AnimationType::Run, "run" },
	{ AnimationType::Idle, "idle01" },
	{ AnimationType::Jump, "jump_loop" },
	{ AnimationType::Fall, "fall_loop" }
};