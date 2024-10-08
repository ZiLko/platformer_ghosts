#include "../includes.hpp"

class Recorder {

public:

    bool goingLeft1 = false;
    bool goingLeft2 = false;
    bool upsideDown1 = false;
    bool upsideDown2 = true;

    cocos2d::CCPoint prevPos1 = ccp(0, 0);
    cocos2d::CCPoint prevPos2 = ccp(0, 0);

    float prevRot1 = 0.f;
    float prevRot2 = 0.f;

    std::vector<Action> actions;

    static Recorder& get() {
        static Recorder instance;
        return instance;
    }

    static std::vector<Action> getActions();

    static void resetState(bool);

    static void handleRecording(GJBaseGameLayer*, int);

    static void handlePortal(int, int, bool, PlayerObject*);

    static void recordVehicleAction(VehicleType, int, bool);

    static void recordFlipAction(int, bool, bool, bool);

    static void recordDualAction(bool, int);

    static VehicleType getCurrentVehicle(PlayerObject*);

};