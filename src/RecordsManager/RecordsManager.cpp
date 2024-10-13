#include "RecordsManager.hpp"

RecordsManager& RecordsManager::get() {
    static RecordsManager instance;
    return instance;
}

nlohmann::json RecordsManager::loadJSON(std::filesystem::path path) {
    std::ifstream jsonFile(path);
    nlohmann::json json;

    if (!jsonFile.is_open())
        return json;

    jsonFile >> json;

    return json;
}

void RecordsManager::handleCompletion(int levelId, float completionTime, std::vector<Action> actions) {		
    std::filesystem::path levelFolder = Mod::get()->getSaveDir() / std::to_string(levelId);

	if (!std::filesystem::exists(levelFolder)) {
		std::filesystem::create_directory(levelFolder);
        return saveCompletion(levelFolder, completionTime, actions);
    }

    // std::filesystem::path bestCompletion = getBestCompletion(levelId);
    // float bestTime = getCompletionTime(bestCompletion);

    // if (completionTime < bestTime || bestTime == 0.f) {

    //     if (!bestCompletion.empty())
    //         std::filesystem::remove(bestCompletion);

        saveCompletion(levelFolder, completionTime, actions);
    // }
}

float RecordsManager::getCompletionTime(std::filesystem::path path) {
    if (path.empty())
        return 0.f;

    nlohmann::json json = loadJSON(path);
    if (json.empty()) return 0.f;

    if (!json.contains("time")) return 0.f;

    return json["time"];
}

std::filesystem::path RecordsManager::getBestCompletion(int levelId) {
    std::filesystem::path levelFolder = Mod::get()->getSaveDir() / std::to_string(levelId);

    if (!std::filesystem::exists(levelFolder))
        return "";

	std::vector<std::filesystem::path> records = file::readDirectory(levelFolder).value();

    std::filesystem::path ret = "";
    float bestTime = 0.f;

    for (std::filesystem::path path : records) {
        float time = getCompletionTime(path);
        if (time > bestTime) {
            bestTime = time;
            ret = path;
        }
    } 

    return ret;
}

Replay RecordsManager::getCompletionReplay(std::filesystem::path path) {
    Replay replay;

    nlohmann::json json = loadJSON(path);
    if (json.empty()) return replay;

    replay.actions = getCompletionActions(path);
    replay.colors = getCompletionColors(path);
    replay.icons = getCompletionIcons(path);

    replay.levelName = json["level_name"].get<std::string>();
    replay.username = json["username"].get<std::string>();
    replay.levelId = json["level_id"].get<int>();
    replay.time = json["time"].get<float>();

    return replay;
}

std::vector<Action> RecordsManager::getCompletionActions(std::filesystem::path path) {
    std::vector<Action> actions;

    nlohmann::json json = loadJSON(path);
    if (json.empty()) return actions;

    for (const nlohmann::json actionJson : json["actions"]) {
        Action action;

        action.type = static_cast<ActionType>(actionJson["t"].get<int>());
        action.frame = actionJson["f"];

        switch (action.type) {
            case ActionType::Animation: loadAnimationAction(action, actionJson); break;
            case ActionType::Position: loadPositionAction(action, actionJson); break;
            case ActionType::Sideways: loadSidewaysAction(action, actionJson); break;
            case ActionType::Dual: action.data = actionJson["d"].get<bool>(); break;
            case ActionType::Vehicle: loadVehicleAction(action, actionJson); break;
            case ActionType::Effect: loadEffectAction(action, actionJson); break;
            case ActionType::Flip: loadFlipAction(action, actionJson); break;
            case ActionType::Mini: loadMiniAction(action, actionJson); break;
        }

        actions.push_back(action);
    }

    return actions;
}

void RecordsManager::saveCompletion(std::filesystem::path folder, float time, std::vector<Action> actions) {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    std::filesystem::path path = folder / (std::to_string(timestamp) + ".json");

    std::string username = GJAccountManager::sharedState()->m_username;
    std::string levelName = "";
    int levelId = 0;

    if (PlayLayer* pl = PlayLayer::get()) {
        levelName = pl->m_level->m_levelName;
        levelId = EditorIDs::getID(pl->m_level);
    }

    nlohmann::json json;
    json["username"] = username;
    json["time"] = time;
    json["level_name"] = levelName;
    json["level_id"] = levelId;

    GameManager* gm = GameManager::get();

    json["colors"]["color1"] = gm->getPlayerColor();
    json["colors"]["color2"] = gm->getPlayerColor2();
    json["colors"]["glow_color"] = gm->getPlayerGlowColor();
    json["colors"]["glow_enabled"] = gm->getPlayerGlow();

    json["icons"]["cube"] = gm->getPlayerFrame();
    json["icons"]["ship"] = gm->getPlayerJetpack();
    json["icons"]["ball"] = gm->getPlayerBall();
    json["icons"]["ufo"] = gm->getPlayerBird();
    json["icons"]["wave"] = gm->getPlayerDart();
    json["icons"]["robot"] = gm->getPlayerRobot();
    json["icons"]["spider"] = gm->getPlayerSpider();
    json["icons"]["swing"] = gm->getPlayerSwing();

    cocos2d::CCPoint prev1 = ccp(0, 0);
    cocos2d::CCPoint prev2 = ccp(0, 0);
    float prevRot1 = 0.f;
    float prevRot2 = 0.f;

    for (const Action action : actions) {
        nlohmann::json actionJson;

        actionJson["t"] = static_cast<int>(action.type);
        actionJson["f"] = action.frame;

        switch (action.type) {
            case ActionType::Position: savePositionAction(prev1, prev2, prevRot1, prevRot2, actionJson, action); break;
            case ActionType::Animation: saveAnimationAction(actionJson, action); break;
            case ActionType::Sideways: saveSidewaysAction(actionJson, action); break;
            case ActionType::Vehicle: saveVehicleAction(actionJson, action); break;
            case ActionType::Mini: saveMiniAction(actionJson, action); break;
            case ActionType::Flip: saveFlipAction(actionJson, action); break;
            case ActionType::Effect: saveEffectAction(actionJson, action); break;
            case ActionType::Dual: actionJson["d"] = std::get<bool>(action.data); break;
        }

        json["actions"].push_back(actionJson);
    }
    
    std::ofstream jsonFile(path);
    if (jsonFile.is_open()) {
        jsonFile << json.dump(4);
        jsonFile.close();
    }
}

std::unordered_map<VehicleType, int> RecordsManager::getCompletionIcons(std::filesystem::path path) {
    std::unordered_map<VehicleType, int> ret;

    nlohmann::json json = loadJSON(path);
    if (json.empty()) return ret;

    ret[VehicleType::Cube] = json["icons"]["cube"].get<int>();
    ret[VehicleType::Ship] = json["icons"]["ship"].get<int>();
    ret[VehicleType::Ball] = json["icons"]["ball"].get<int>();
    ret[VehicleType::Ufo] = json["icons"]["ufo"].get<int>();
    ret[VehicleType::Wave] = json["icons"]["wave"].get<int>();
    ret[VehicleType::Robot] = json["icons"]["robot"].get<int>();
    ret[VehicleType::Spider] = json["icons"]["spider"].get<int>();
    ret[VehicleType::Swing] = json["icons"]["swing"].get<int>();

    return ret;
}

PlayerColors RecordsManager::getCompletionColors(std::filesystem::path path) {
    PlayerColors ret;

    nlohmann::json json = loadJSON(path);
    if (json.empty()) return ret;

    ret.color1 = json["colors"]["color1"].get<int>();
    ret.color2 = json["colors"]["color2"].get<int>();
    ret.glowColor = json["colors"]["glow_color"].get<int>();
    ret.glowEnabled = json["colors"]["glow_enabled"].get<bool>();

    return ret;
}

void RecordsManager::savePositionAction(cocos2d::CCPoint& prev1, cocos2d::CCPoint& prev2, float& prevRot1, float& prevRot2, nlohmann::json& json, Action action) {
    PositionData data = std::get<PositionData>(action.data);
    cocos2d::CCPoint pos1 = data.p1Data.position;
    cocos2d::CCPoint pos2 = data.p2Data.position;
    float rot1 = data.p1Data.rotation;
    float rot2 = data.p2Data.rotation;
            
    if (pos1.x != 0 && pos1.x != prev1.x)
        json["d"]["x1"] = pos1.x;
    if (pos1.y != 0 && pos1.y != prev1.y)
        json["d"]["y1"] = pos1.y;
    if (rot1 != 0.f && rot1 != prevRot1)
        json["d"]["r1"] = rot1;

    if (pos2.x != 0 && pos2.x != prev2.x)
        json["d"]["x2"] = pos2.x;
    if (pos2.y != 0 && pos2.y != prev2.y)
        json["d"]["y2"] = pos2.y;
    if (rot2 != 0.f && rot2 != prevRot2)
        json["d"]["r2"] = rot2;

    prev1 = pos1;
    prev2 = pos2;
    prevRot1 = rot1;
    prevRot2 = rot2;
}

void RecordsManager::saveVehicleAction(nlohmann::json& json, Action action) {
    VehicleData data = std::get<VehicleData>(action.data);
    json["d"]["id"] = static_cast<int>(data.vehicle);
    json["d"]["p2"] = data.player2;
}

void RecordsManager::saveFlipAction(nlohmann::json& json, Action action) {
    FlipData data = std::get<FlipData>(action.data);

    json["d"]["p2"] = data.player2;
    json["d"]["flip"] = data.flip;
    json["d"]["y"] = data.y;
}

void RecordsManager::saveMiniAction(nlohmann::json& json, Action action) {
    MiniData data = std::get<MiniData>(action.data);
    json["d"]["mini"] = data.mini;
    json["d"]["p2"] = data.player2;
}

void RecordsManager::saveSidewaysAction(nlohmann::json& json, Action action) {
    SidewaysData data = std::get<SidewaysData>(action.data);
    json["d"]["p2"] = data.player2;
    json["d"]["rot"] = data.rotation;
}

void RecordsManager::saveAnimationAction(nlohmann::json& json, Action action) {
    AnimationData data = std::get<AnimationData>(action.data);
    json["d"]["a"] = static_cast<int>(data.animation);
    json["d"]["r"] = data.robot;
    json["d"]["p2"] = data.player2;
}

void RecordsManager::saveEffectAction(nlohmann::json& json, Action action) {
    EffectData data = std::get<EffectData>(action.data);
    json["d"]["effect"] = static_cast<int>(data.effect);
    json["d"]["p2"] = data.player2;
}

void RecordsManager::loadPositionAction(Action& action, nlohmann::json json) {
    float x1 = json["d"].contains("x1") ? json["d"]["x1"].get<float>() : 0.f;
    float y1 = json["d"].contains("y1") ? json["d"]["y1"].get<float>() : 0.f;
    float r1 = json["d"].contains("r1") ? json["d"]["r1"].get<float>() : 0.f;

    float x2 = json["d"].contains("x2") ? json["d"]["x2"].get<float>() : 0.f;
    float y2 = json["d"].contains("y2") ? json["d"]["y2"].get<float>() : 0.f;    
    float r2 = json["d"].contains("r2") ? json["d"]["r2"].get<float>() : 0.f;

    PlayerData p1Data = { ccp(x1, y1), r1 };
    PlayerData p2Data = { ccp(x2, y2), r2 };
    PositionData data = { p1Data, p2Data };
                
    action.data = data;
}

void RecordsManager::loadVehicleAction(Action& action, nlohmann::json json) {
    VehicleData data;

    data.vehicle = static_cast<VehicleType>(json["d"]["id"].get<int>());
    data.player2 = static_cast<VehicleType>(json["d"]["p2"].get<bool>());

    action.data = data;
}

void RecordsManager::loadFlipAction(Action& action, nlohmann::json json) {
    FlipData data;

    data.player2 = json["d"]["p2"].get<bool>();
    data.flip = json["d"]["flip"].get<bool>();
    data.y = json["d"]["y"].get<bool>();

    action.data = data;
}

void RecordsManager::loadMiniAction(Action& action, nlohmann::json json) {
    MiniData data;
    data.mini = json["d"]["mini"].get<bool>();
    data.player2 = json["d"]["p2"].get<bool>();
    action.data = data;
}

void RecordsManager::loadSidewaysAction(Action& action, nlohmann::json json) {
    SidewaysData data;
    data.rotation = json["d"]["rot"].get<float>();
    data.player2 = json["d"]["p2"].get<bool>();
    action.data = data;
}

void RecordsManager::loadAnimationAction(Action& action, nlohmann::json json) {
    AnimationData data;
    data.animation = static_cast<AnimationType>(json["d"]["a"].get<int>());
    data.player2 = json["d"]["p2"].get<bool>();
    data.robot = json["d"]["r"].get<bool>();
    action.data = data;
}

void RecordsManager::loadEffectAction(Action& action, nlohmann::json json) {
    EffectData data;
    data.effect = static_cast<EffectType>(json["d"]["effect"].get<int>());
    data.player2 = json["d"]["p2"].get<bool>();
    action.data = data;
}