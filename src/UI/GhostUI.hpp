#pragma once

#include "../Includes.hpp"

class GhostUI : public CCNode {

public:

    SimplePlayer* uiIcon = nullptr;
    CCLabelBMFont* title = nullptr;
    CCLabelBMFont* uiTime = nullptr;
    CCLabelBMFont* uiName = nullptr;

    static GhostUI* create() {
        GhostUI* ret = new GhostUI();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool init();

    void update();

};