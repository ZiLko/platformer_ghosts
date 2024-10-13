#include "../includes.hpp"

class RecordCell : public CCNode {

public:

	static RecordCell* create(int rank) {
        RecordCell* ret = new RecordCell();
        if (ret->init(rank)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

	bool init(int rank);

};

class RecordsLayer: public geode::Popup<> {

public:

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

};