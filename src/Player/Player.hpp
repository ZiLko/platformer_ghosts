#include "../includes.hpp"

class Player {

public:

    int currentAction = 0;
    bool upsideDown1 = false;
    bool upsideDown2 = false;
    bool isDual = false;

    PlayerObject* player1 = nullptr;
    PlayerObject* player2 = nullptr;

    VehicleType currentVehicle1 = VehicleType::Cube;
    VehicleType currentVehicle2 = VehicleType::Cube;

    std::vector<Action> actions;

    static Player& get() {
        static Player instance;
        return instance;
    }

    static void clearActions();

    static void setup(PlayLayer*);

    static void resetState();

    static void handlePlaying(GJBaseGameLayer*, int);

    static void updateUpsideDownState();

    static void setPlayerColors(PlayerObject*, bool);

    static void setPlayerSprite(PlayerObject*, VehicleType, int);

};