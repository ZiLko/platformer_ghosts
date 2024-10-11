#include "../includes.hpp"

class Player {

public:

    int currentAction = 0;
    bool isDual = false;
    bool upsideDown1 = false;
    bool upsideDown2 = false;
    bool isMini1 = false;
    bool isMini2 = false;
    float rotationOffset1 = 0.f;
    float rotationOffset2 = 0.f;
    float lastRotation1 = 0.f;
    float lastRotation2 = 0.f;

    PlayerObject* player1 = nullptr;
    PlayerObject* player2 = nullptr;

    VehicleType currentVehicle1 = VehicleType::Cube;
    VehicleType currentVehicle2 = VehicleType::Cube;

    std::vector<Action> actions;
    std::unordered_map<VehicleType, int> icons;

    static Player& get();

    static std::vector<Action> getActions();
    static void clearActions();
    static void setup(PlayLayer*);
    static void resetState();
    static void handlePlaying(GJBaseGameLayer*, int);
    static void updateUpsideDownState();

    void handlePositionAction(Action);
    void handleVehicleAction(Action);
    void handleDualAction(Action);
    void handleFlipAction(Action);
    void handleMiniAction(Action);
    void handleSidewaysAction(Action);
    void handleAnimationAction(Action);
    void handleEffectAction(Action);

    static void setPlayerColors(PlayerObject*, bool, int, int, int, bool);
    static void setPlayerSprite(PlayerObject*, VehicleType);
    static void setPlayerScale(PlayerObject*, float, float, float);

};