#include "RecordsManager.hpp"
#include "../Player/Player.hpp"

nlohmann::json RecordsManager::loadJSON(std::filesystem::path path) {
    std::ifstream jsonFile(path);
    nlohmann::json json;

    if (!jsonFile.is_open())
        return json;

    jsonFile >> json;
    return json;
}

std::string RecordsManager::getFormattedTime(float time) {
    std::string ret = "";
    int hours = static_cast<int>(time / 3600);
    time = std::fmod(time, 3600);
    int minutes = static_cast<int>(time / 60);
    time = std::fmod(time, 60);
    int seconds = static_cast<int>(time);
    int milliseconds = static_cast<int>((time - seconds) * 100);

    if (hours > 0)
        ret = std::format("{:02}:{:02}:{:02}.{:02}", hours, minutes, seconds, milliseconds);
    else if (minutes > 0)
        ret = std::format("{:02}:{:02}.{:02}", minutes, seconds, milliseconds);
    else
        ret = std::format("{}.{:03}s", seconds, static_cast<int>((time - seconds) * 1000));

    return ret;
}

std::vector<std::pair<ReplayInfo, std::filesystem::path>> RecordsManager::getLevelCompletions(int levelId) {
    std::vector<std::pair<ReplayInfo, std::filesystem::path>> ret;

    std::filesystem::path levelFolder = Mod::get()->getSaveDir() / std::to_string(levelId);
	if (!std::filesystem::exists(levelFolder)) return ret;

    for (const auto& dir : std::filesystem::directory_iterator(levelFolder)) {
        if (!std::filesystem::is_directory(dir.path())) continue;

        std::filesystem::path info = dir.path() / "info.json";
        if (!std::filesystem::exists(info)) continue;
        std::filesystem::path actions = dir.path() / "actions.json";
        if (!std::filesystem::exists(actions)) continue;

        ret.push_back(std::make_pair(getCompletionInfo(info), actions));
    }
    
    std::sort(ret.begin(), ret.end(), [](std::pair<ReplayInfo, std::filesystem::path> a, std::pair<ReplayInfo, std::filesystem::path> b) {
        return a.first.time < b.first.time;
    });

    return ret;
}

std::vector<GhostLevel> RecordsManager::getSavedLevels() {
    std::vector<GhostLevel> ret;

    for (const auto& dir : std::filesystem::directory_iterator(Mod::get()->getSaveDir())) {
        std::filesystem::path path = dir.path();
        GhostLevel level;
        if (!path.extension().string().empty()) continue;

        for (const auto& dir2 : std::filesystem::directory_iterator(path)) {
            std::filesystem::path path2 = dir2.path();
            if (!std::filesystem::exists(path2 / "info.json")) continue;
            ReplayInfo info = getCompletionInfo(path2 / "info.json");
            level.name = info.levelName;
            level.id = info.levelId;
            break;
        }

        if (level.id != 0) ret.push_back(level);
    }

    return ret;
}

void RecordsManager::handleCompletion(int levelId, float time, std::vector<Action> actions) {		
    std::filesystem::path levelFolder = Mod::get()->getSaveDir() / std::to_string(levelId);

	if (!std::filesystem::exists(levelFolder))
		std::filesystem::create_directory(levelFolder);

    saveCompletion(levelFolder, time, actions);
    Player& p = Player::get();
    if (p.currentRace == 1 || p.currentRace == 0)
        Player::loadBestCompletion();
    else if (time < p.info.time) p.currentRace++;

    std::vector<std::pair<ReplayInfo, std::filesystem::path>> ghosts = getLevelCompletions(levelId);
    if (ghosts.empty()) return;

    if (ghosts.size() > Mod::get()->getSettingValue<int64_t>("max_ghosts")) {
		std::filesystem::remove_all(ghosts.back().second.parent_path());
        ghosts.pop_back();
        if (p.currentRace == ghosts.size()) {
            p.isRacing = true;
            p.loadReplay({ ghosts.back().first, getCompletionActions(ghosts.back().second) }); 
        }
    }
}

float RecordsManager::getCompletionTime(std::filesystem::path path) {
    if (path.empty())
        return 0.f;

    nlohmann::json json = loadJSON(path);
    if (json.empty()) return 0.f;

    if (!json.contains("time")) return 0.f;

    return json["time"];
}

Replay RecordsManager::getBestCompletion(int levelId) {
    std::vector<std::pair<ReplayInfo, std::filesystem::path>> replays = getLevelCompletions(levelId);
    Replay replay;
    if (replays.empty()) return replay;
    std::string username = GJAccountManager::sharedState()->m_username;
    for (std::pair<ReplayInfo, std::filesystem::path> r : replays) {
        if (r.first.username == username) {
            replay.info = r.first;
            replay.actions = getCompletionActions(r.second);
            break;
        }
    }
    return replay;
}

ReplayInfo RecordsManager::getCompletionInfo(std::filesystem::path path) {
    ReplayInfo info;

    nlohmann::json json = loadJSON(path);
    if (json.empty()) return info;

    info.colors = getCompletionColors(path);
    info.icons = getCompletionIcons(path);
    info.levelName = json["level_name"].get<std::string>();
    info.username = json["username"].get<std::string>();
    info.date = json["date"].get<std::string>();
    info.levelId = json["level_id"].get<int>();
    info.time = json["time"].get<float>();

    return info;
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
            case ActionType::Vehicle: loadVehicleAction(action, actionJson); break;
            case ActionType::Effect: loadEffectAction(action, actionJson); break;
            case ActionType::Flip: loadFlipAction(action, actionJson); break;
            case ActionType::Mini: loadMiniAction(action, actionJson); break;
            case ActionType::Input: loadInputAction(action, actionJson); break;
            case ActionType::Dual: action.data = actionJson["d"].get<bool>(); break;
        }

        actions.push_back(action);
    }

    return actions;
}

void RecordsManager::saveCompletion(std::filesystem::path folder, float time, std::vector<Action> actions) {
    // auto now = std::chrono::system_clock::now();
    // auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::filesystem::path realFolder = folder / "lole";
	
    std::filesystem::create_directory(realFolder);
    std::filesystem::path infoPath = realFolder / "info.json";
    std::filesystem::path actionsPath = realFolder / "actions.json";

    std::string username = GJAccountManager::sharedState()->m_username;
    std::string levelName = "";
    int levelId = 0;

    // std::time_t rn = std::chrono::system_clock::to_time_t(now);
    // std::tm* localTime = std::localtime(&rn);
    // std::ostringstream oss;
    // oss << std::put_time(localTime, "%Y-%m-%d");
    // std::string date = oss.str();
    std::string date = "xd";

    if (PlayLayer* pl = PlayLayer::get()) {
        levelName = pl->m_level->m_levelName;
        levelId = EditorIDs::getID(pl->m_level);
    }

    nlohmann::json infoJson;
    nlohmann::json actionsJson;

    infoJson["date"] = date;
    infoJson["username"] = username;
    infoJson["time"] = time;
    infoJson["level_name"] = levelName;
    infoJson["level_id"] = levelId;

    GameManager* gm = GameManager::get();

    infoJson["colors"]["color1"] = gm->getPlayerColor();
    infoJson["colors"]["color2"] = gm->getPlayerColor2();
    infoJson["colors"]["glow_color"] = gm->getPlayerGlowColor();
    infoJson["colors"]["glow_enabled"] = gm->getPlayerGlow();

    infoJson["icons"]["cube"] = gm->getPlayerFrame();
    infoJson["icons"]["ship"] = gm->getPlayerJetpack();
    infoJson["icons"]["ball"] = gm->getPlayerBall();
    infoJson["icons"]["ufo"] = gm->getPlayerBird();
    infoJson["icons"]["wave"] = gm->getPlayerDart();
    infoJson["icons"]["robot"] = gm->getPlayerRobot();
    infoJson["icons"]["spider"] = gm->getPlayerSpider();
    infoJson["icons"]["swing"] = gm->getPlayerSwing();

    std::ofstream infoFile(infoPath);
    if (infoFile.is_open()) {
        infoFile << infoJson.dump();
        infoFile.close();
    }

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
            case ActionType::Input: saveInputAction(actionJson, action); break;
            case ActionType::Dual: actionJson["d"] = std::get<bool>(action.data); break;
        }

        actionsJson["actions"].push_back(actionJson);
    }

    std::ofstream actionsFile(actionsPath);
    if (actionsFile.is_open()) {
        actionsFile << actionsJson.dump();
        actionsFile.close();
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
    cocos2d::CCPoint pos1 = data.p1.position;
    cocos2d::CCPoint pos2 = data.p2.position;
    float rot1 = data.p1.rotation;
    float rot2 = data.p2.rotation;
            
    if (pos1.x != 0 && pos1.x != prev1.x)
        json["d"]["x1"] = pos1.x;
    if (pos1.y != 0 && pos1.y != prev1.y)
        json["d"]["y1"] = pos1.y;
    if ((rot1 != 0.f && rot1 != prevRot1) || data.p1.rotationZero)
        json["d"]["r1"] = rot1;

    if (pos2.x != 0 && pos2.x != prev2.x)
        json["d"]["x2"] = pos2.x;
    if (pos2.y != 0 && pos2.y != prev2.y)
        json["d"]["y2"] = pos2.y;
    if ((rot2 != 0.f && rot2 != prevRot2) || data.p2.rotationZero)
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
    json["d"]["m"] = data.mini;
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
    json["d"]["e"] = static_cast<int>(data.effect);
    json["d"]["p2"] = data.player2;
}

void RecordsManager::saveInputAction(nlohmann::json& json, Action action) {
    InputData data = std::get<InputData>(action.data);
    json["d"]["b"] = data.button;
    json["d"]["d"] = data.down;
    json["d"]["p2"] = data.player2;
}

void RecordsManager::loadPositionAction(Action& action, nlohmann::json json) {
    float x1 = json["d"].contains("x1") ? json["d"]["x1"].get<float>() : 0.f;
    float y1 = json["d"].contains("y1") ? json["d"]["y1"].get<float>() : 0.f;
    float r1 = json["d"].contains("r1") ? json["d"]["r1"].get<float>() : 0.f;

    float x2 = json["d"].contains("x2") ? json["d"]["x2"].get<float>() : 0.f;
    float y2 = json["d"].contains("y2") ? json["d"]["y2"].get<float>() : 0.f;    
    float r2 = json["d"].contains("r2") ? json["d"]["r2"].get<float>() : 0.f;

    PlayerData p1 = { ccp(x1, y1), r1 };
    PlayerData p2 = { ccp(x2, y2), r2 };
    PositionData data = { p1, p2 };

    if (json["d"].contains("r1") && r1 == 0.f) data.p1.rotationZero = true;
    if (json["d"].contains("r2") && r2 == 0.f) data.p2.rotationZero = true;
                
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
    data.mini = json["d"]["m"].get<bool>();
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
    data.effect = static_cast<EffectType>(json["d"]["e"].get<int>());
    data.player2 = json["d"]["p2"].get<bool>();
    action.data = data;
}

void RecordsManager::loadInputAction(Action& action, nlohmann::json json) {
    InputData data;
    data.button = json["d"]["b"].get<int>();
    data.down = json["d"]["d"].get<bool>();
    data.player2 = json["d"]["p2"].get<bool>();
    action.data = data;
}