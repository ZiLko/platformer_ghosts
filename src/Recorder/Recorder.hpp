#include "../includes.hpp"

class Recorder {

public:

    bool goingLeft1 = false;
    bool goingLeft2 = false;
    bool upsideDown1 = false;
    bool upsideDown2 = true;
    bool sideways1 = false;
    bool sideways2 = false;
    bool moving1 = false;
    bool moving2 = false;
    bool falling1 = false;
    bool falling2 = false;
    bool onGround1 = false;
    bool onGround2 = false;
    bool jumped1 = false;
    bool jumped2 = false;

    int updatePlayer2Animation = 0;
    int fps = 60;
    bool disabled = false;

    cocos2d::CCPoint prevPos1 = ccp(0, 0);
    cocos2d::CCPoint prevPos2 = ccp(0, 0);

    float prevRot1 = 0.f;
    float prevRot2 = 0.f;

    std::vector<Action> actions;

    static Recorder& get();

    static std::vector<Action> getActions();
    static void resetState(bool);
    static VehicleType getCurrentVehicle(PlayerObject*);

    static void handleRecording(PlayLayer*, int);
    static void handlePortal(int, int, bool, PlayerObject*);
    static void handleEffect(int, EffectType, bool);
    static void handleInput(int, int, bool, bool);

    static void recordVehicleAction(VehicleType, int, bool);
    static void recordFlipAction(int, bool, bool, bool);
    static void recordDualAction(bool, int);
    static void recordMiniAction(int, bool, bool);
    static void recordSidewaysAction(int, float, bool);
    static void recordAnimationAction(int, bool, bool, AnimationType);

};