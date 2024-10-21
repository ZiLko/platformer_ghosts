#include "../Includes.hpp"

class LevelSelectCell : public CCNode {

    int levelId = 0;

public:

	static LevelSelectCell* create(GhostLevel level) {
        LevelSelectCell* ret = new LevelSelectCell();
        if (ret->init(level)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

	bool init(GhostLevel);
    void onSelect(CCObject*);
    
};


class ListLayer : public geode::Popup<> {

public:

    bool setup() override;
	static ListLayer* create();

};