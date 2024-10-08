#include "RecordsManager.hpp"

void RecordsManager::handleCompletion(int levelId, float completionTime, std::vector<Action> actions) {		
    std::filesystem::path levelFolder = Mod::get()->getSaveDir() / std::to_string(levelId);

	if (!std::filesystem::exists(levelFolder)) {
		std::filesystem::create_directory(levelFolder);
        return saveCompletion(levelFolder, completionTime, actions);
    }

    std::filesystem::path bestCompletion = getBestCompletion(levelId);
    float bestTime = getCompletionTime(bestCompletion);

    if (completionTime < bestTime || bestTime == 0.f) {

        if (!bestCompletion.empty())
            std::filesystem::remove(bestCompletion);

        saveCompletion(levelFolder, completionTime, actions);
    }
}

float RecordsManager::getCompletionTime(std::filesystem::path path) {
    if (path.empty())
        return 0.f;

    std::ifstream jsonFile(path);
    if (!jsonFile.is_open())
        return 0.f;

    nlohmann::json json;
    jsonFile >> json;

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

std::vector<Action> RecordsManager::getCompletionActions(std::filesystem::path path) {
    std::vector<Action> actions;
    std::ifstream jsonFile(path);

    if (!jsonFile.is_open())
        return actions;

    nlohmann::json json;
    jsonFile >> json;

    for (const nlohmann::json actionJson : json["actions"]) {
        Action action;

        action.type = static_cast<ActionType>(actionJson["t"].get<int>());
        action.frame = actionJson["f"];

        switch (action.type) {
            case ActionType::Position: {

                float x1 = actionJson["d"].contains("x1") ? actionJson["d"]["x1"].get<float>() : 0.f;
                float y1 = actionJson["d"].contains("y1") ? actionJson["d"]["y1"].get<float>() : 0.f;
                float r1 = actionJson["d"].contains("r1") ? actionJson["d"]["r1"].get<float>() : 0.f;

                float x2 = actionJson["d"].contains("x2") ? actionJson["d"]["x2"].get<float>() : 0.f;
                float y2 = actionJson["d"].contains("y2") ? actionJson["d"]["y2"].get<float>() : 0.f;    
                float r2 = actionJson["d"].contains("r2") ? actionJson["d"]["r2"].get<float>() : 0.f;

                PlayerData p1Data = { ccp(x1, y1), r1 };
                PlayerData p2Data = { ccp(x2, y2), r2 };
                PositionData data = { p1Data, p2Data };
                
                action.data = data;

                break;
            } case ActionType::Death:
                break;
            case ActionType::Respawn:
                break;
            case ActionType::Vehicle: {
                VehicleData data;

                data.vehicle = static_cast<VehicleType>(actionJson["d"]["id"].get<int>());
                data.player2 = static_cast<VehicleType>(actionJson["d"]["p2"].get<bool>());

                action.data = data;
                break;
            } case ActionType::RobotAnimation:
                break;
            case ActionType::SpiderAnimation:
                break;
            case ActionType::Dual:
                action.data = actionJson["d"].get<bool>();
                break;
            case ActionType::Flip: {
                FlipData data;

                data.player2 = actionJson["d"]["p2"].get<bool>();
                data.flip = actionJson["d"]["flip"].get<bool>();
                data.y = actionJson["d"]["y"].get<bool>();

                action.data = data;
                break;
            }
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

    nlohmann::json json;
    json["username"] = username;
    json["time"] = time;
    // json["icons"] = "New York";
    // level id and name

    cocos2d::CCPoint prev1 = ccp(0, 0);
    cocos2d::CCPoint prev2 = ccp(0, 0);
    float prevRot1 = 0.f;
    float prevRot2 = 0.f;

    for (const Action action : actions) {
        nlohmann::json actionJson;

        actionJson["t"] = static_cast<int>(action.type);
        actionJson["f"] = action.frame;

        switch (action.type) {
            case ActionType::Position: {
                PositionData data = std::get<PositionData>(action.data);
                cocos2d::CCPoint pos1 = data.p1Data.position;
                cocos2d::CCPoint pos2 = data.p2Data.position;
                float rot1 = data.p1Data.rotation;
                float rot2 = data.p2Data.rotation;
            
                if (pos1.x != 0 && pos1.x != prev1.x)
                    actionJson["d"]["x1"] = pos1.x;
                if (pos1.y != 0 && pos1.y != prev1.y)
                    actionJson["d"]["y1"] = pos1.y;
                if (rot1 != 0.f && rot1 != prevRot1)
                    actionJson["d"]["r1"] = rot1;

                if (pos2.x != 0 && pos2.x != prev2.x)
                    actionJson["d"]["x2"] = pos2.x;
                if (pos2.y != 0 && pos2.y != prev2.y)
                    actionJson["d"]["y2"] = pos2.y;
                if (rot2 != 0.f && rot2 != prevRot2)
                    actionJson["d"]["r2"] = rot2;

                prev1 = pos1;
                prev2 = pos2;
                prevRot1 = rot1;
                prevRot2 = rot2;
                
                break;
            } case ActionType::Death:
                break; 
            case ActionType::Respawn:
                break;
            case ActionType::Vehicle:
                VehicleData data = std::get<VehicleData>(action.data);
                actionJson["d"]["id"] = static_cast<int>(data.vehicle);
                actionJson["d"]["p2"] = data.player2;
                break;
            case ActionType::RobotAnimation:
                break;
            case ActionType::SpiderAnimation:
                break;
            case ActionType::Dual:
                actionJson["d"] = std::get<bool>(action.data);
                break;
            case ActionType::Flip: {
                FlipData data = std::get<FlipData>(action.data);

                actionJson["d"]["p2"] = data.player2;
                actionJson["d"]["flip"] = data.flip;
                actionJson["d"]["y"] = data.y;
                break;
            }
        }

        json["actions"].push_back(actionJson);
    }
    
    std::ofstream jsonFile(path);
    if (jsonFile.is_open()) {
        jsonFile << json.dump(4);
        jsonFile.close();
    }
}   