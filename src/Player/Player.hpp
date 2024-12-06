#include "../Includes.hpp"
#include "../UI/GhostUI.hpp"

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

    bool spectated = false;
    bool isSpectating = false;
    bool isRacing = false;
    bool completedLevel = false;
    bool ghostCompletedLevel = false;
    bool p1Visible = false;

    PlayerObject* player1 = nullptr;
    PlayerObject* player2 = nullptr;
    SimplePlayer* icon1 = nullptr;
    SimplePlayer* icon2 = nullptr;

    VehicleType currentVehicle1 = VehicleType::Cube;
    VehicleType currentVehicle2 = VehicleType::Cube;

    ReplayInfo info;
    std::vector<Action> actions;
    std::unordered_map<VehicleType, int> icons;

    bool operator==(const Player& other) const {
        return player1 == other.player1 && player2 == other.player2;
    }

    void loadReplay(Replay replay);
    std::pair<cocos2d::CCPoint, cocos2d::CCPoint> getLatestPositions();

    std::vector<Action> getActions();
    PlayerData getBorderPosition(cocos2d::CCPoint, bool);
    std::pair<bool, bool> isInsideCamera(cocos2d::CCPoint, float);
    void loadGhost(Replay, int);
    void loadBestCompletion();
    void clear();
    void setup(PlayLayer*);
    void resetState();
    void handleActions();
    void handlePlaying(GJBaseGameLayer*, int);
    void handleCompletion();
    void updateUpsideDownState();
    void startSpectating(Replay);
    void stopSpectating();
    void stopRacing();
    void playCompleteEffect();
    void playSpawnEffect(PlayerObject*);
    void updateDisabled();
    void updateOpacity(bool);
    void updateColors();
    void updateCamera();
    void hideIcons();

    void handlePositionAction(Action);
    void handleVehicleAction(Action);
    void handleDualAction(Action);
    void handleFlipAction(Action);
    void handleMiniAction(Action);
    void handleSidewaysAction(Action);
    void handleAnimationAction(Action);
    void handleEffectAction(Action);
    void handleInputAction(Action);
    void handleResetAction(Action);

    void setPlayerColors(PlayerObject*, bool, int, int, int, bool);
    void setPlayerSprite(PlayerObject*, VehicleType);
    void setPlayerScale(PlayerObject*, float, float, float);
    
    static void setPlayerIconColors(SimplePlayer*, bool, int, int, int, bool);

};

class PlayerManager {

public:

    GhostUI* ui = nullptr;

    int currentFrame = 0;
    int currentRace = 0;
    float currentSpectate = 0;
    bool spectated = false;
    bool isSpectating = false;
    bool isRacing = false;
    bool spectatorInput = false;
    bool updatePlayer = false;
    bool shouldRestart = false;
    bool canReset = false;
    bool disabled = false;

    ReplayInfo spectateInfo;
    std::vector<Player> players;
    std::unordered_map<float, Player> times;

    static PlayerManager& get();
    void updateUI();
    void setup(PlayLayer*);

    static void addPlayer(Replay, PlayLayer*); 
    static void removePlayer(Player);
    static bool containsTime(float);
    static void hideIcons();

    static void stopSpectating();
    static void clear();
    static void playCompleteEffect();
    static void handleCompletion();
    static void handlePlaying(GJBaseGameLayer*, int);
    static bool canClick();
    static void resetState();
    static bool canUpdatePlayer();
    static void updateDisabled();
    static void updateOpacity(bool);
    static void updateColors();
    static void updateCamera();

    static float getTime() { return get().players[0].info.time; }
    static int& getCurrentFrame() { return get().currentFrame; }
    static int& getCurrentRace() { return get().currentRace; }
    static float& getCurrentSpectate() { return get().currentSpectate; }
    static bool& getSpectated() { return get().spectated; }
    static bool& getIsSpectating() { return get().isSpectating; }
    static bool& getIsRacing() { return get().isRacing; }
    static bool& getSpectatorInput() { return get().spectatorInput; }
    static bool& getUpdatePlayer() { return get().updatePlayer; }
    static bool& getShouldRestart() { return get().shouldRestart; }
    static bool& getCanReset() { return get().canReset; }
    static bool& getDisabled() { return get().disabled; }

};