#include "../Includes.hpp"

class RecordCell : public CCNode {

public:

    ReplayInfo info;
    std::filesystem::path path;
    int rank;

    CCMenuItemToggler* raceToggle = nullptr;
    CCMenuItemToggler* spectateToggle = nullptr;

    geode::Popup<>* recordsLayer = nullptr;

	static RecordCell* create(std::pair<ReplayInfo, std::filesystem::path> replay, int rank, geode::Popup<>* layer, bool xd = false) {
        RecordCell* ret = new RecordCell();
        if (ret->init(replay, rank, xd)) {
            ret->recordsLayer = layer;
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

	bool init(std::pair<ReplayInfo, std::filesystem::path>, int, bool);

    void onRace(CCObject*);

    void onSpectate(CCObject*);

};

class RecordsLayer: public geode::Popup<> {

public:

    std::vector<RecordCell*> cells;
    std::vector<RecordCell*> selectedRaces;
    RecordCell* selectedSpectate = nullptr;
    CCLabelBMFont* noGhostsLabel = nullptr;

    static RecordsLayer* create() {
        RecordsLayer* ret = new RecordsLayer();
        if (ret->initAnchored(315, 270, "GJ_square02.png")) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    bool setup() override;
    void addList();
    void refresh();

    static void open();
    void open(CCObject*);
    void onSettings(CCObject*);
    void onManager(CCObject*);

};