#include "../includes.hpp"

class RecordsManager {

public: 

    static RecordsManager& get();

    static nlohmann::json loadJSON(std::filesystem::path);
    static void handleCompletion(int, float, std::vector<Action>);
    static void saveCompletion(std::filesystem::path, float, std::vector<Action>);

    static std::string getTypeString(ActionType);
    static std::filesystem::path getBestCompletion(int);
    static std::vector<Action> getCompletionActions(std::filesystem::path);
    static std::unordered_map<VehicleType, int> getCompletionIcons(std::filesystem::path);
    static PlayerColors getCompletionColors(std::filesystem::path);
    static float getCompletionTime(std::filesystem::path);
    static Replay getCompletionReplay(std::filesystem::path);

    static void savePositionAction(cocos2d::CCPoint&, cocos2d::CCPoint&, float&, float&, nlohmann::json&, Action);
    static void saveVehicleAction(nlohmann::json&, Action);
    static void saveFlipAction(nlohmann::json&, Action);
    static void saveMiniAction(nlohmann::json&, Action);
    static void saveSidewaysAction(nlohmann::json&, Action);
    static void saveAnimationAction(nlohmann::json&, Action);
    static void saveEffectAction(nlohmann::json&, Action);

    static void loadPositionAction(Action&, nlohmann::json);
    static void loadVehicleAction(Action&, nlohmann::json);
    static void loadFlipAction(Action&, nlohmann::json);
    static void loadMiniAction(Action&, nlohmann::json);
    static void loadSidewaysAction(Action&, nlohmann::json);
    static void loadAnimationAction(Action&, nlohmann::json);
    static void loadEffectAction(Action&, nlohmann::json);

};