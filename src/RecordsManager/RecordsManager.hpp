#include "../includes.hpp"

class RecordsManager {

public: 

    std::vector<Action> actions;

    static RecordsManager& get() {
        static RecordsManager instance;
        return instance;
    }

    static void pushAction(Action action);

    static void handleCompletion(int levelId, float completionTime, std::vector<Action> actions);

    static void saveCompletion(std::filesystem::path path, float time, std::vector<Action> actions);

    static std::filesystem::path getBestCompletion(int levelId);

    static std::vector<Action> getCompletionActions(std::filesystem::path path);

    static float getCompletionTime(std::filesystem::path path);

    static std::string getTypeString(ActionType type);

};