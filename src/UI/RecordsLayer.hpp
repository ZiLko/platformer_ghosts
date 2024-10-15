#include "../includes.hpp"

class RecordCell : public CCNode {

public:

    Replay replay;
    int rank;

    CCMenuItemToggler* raceToggle = nullptr;
    CCMenuItemToggler* spectateToggle = nullptr;

    geode::Popup<>* recordsLayer = nullptr;

	static RecordCell* create(Replay replay, int rank, geode::Popup<>* layer) {
        RecordCell* ret = new RecordCell();
        if (ret->init(replay, rank)) {
            ret->recordsLayer = layer;
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

	bool init(Replay, int);

    void disable(CCMenuItemToggler*);

    void onRace(CCObject*);

    void onSpectate(CCObject*);

};

class RecordsLayer: public geode::Popup<> {

public:

    std::vector<RecordCell*> cells;
    RecordCell* selectedRace = nullptr;
    RecordCell* selectedSpectate = nullptr;

    static RecordsLayer* create() {
        RecordsLayer* ret = new RecordsLayer();
        if (ret->init(302, 212, "GJ_square02.png")) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    bool setup() override;

    void open(CCObject*);

    static void open();

    static std::string getFormattedTime(float);

};