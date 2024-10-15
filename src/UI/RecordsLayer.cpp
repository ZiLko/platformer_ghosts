#include "RecordsLayer.hpp"

#include "../RecordsManager/RecordsManager.hpp"
#include "../Player/Player.hpp"

std::string RecordsLayer::getFormattedTime(float time) {
    std::string ret;

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

void RecordsLayer::open(CCObject*) {
    open();
}

void RecordsLayer::open() {
    RecordsLayer* layer = create();
    if (!layer) return;
    layer->m_noElasticity = true;
    layer->show();
}

bool RecordsLayer::setup() {
    PlayLayer* pl = PlayLayer::get();
    if (!pl) return false;

    std::string levelName = pl->m_level->m_levelName;
    setTitle("Ghosts for " + levelName);
    m_title->limitLabelWidth(248.f, m_title->getScale(), 0.001f);
    m_title->updateLabel();
    m_title->setPositionY(m_title->getPositionY() + 3);

    std::vector<Replay> replays = RecordsManager::getLevelCompletions(EditorIDs::getID(pl->m_level));
    
    cocos2d::CCSize winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();

	CCArray* cellsArray = CCArray::create();
    int personalBest = -1;

    for (int i = 0; i < replays.size(); i++) {
        if (i > 99) break;

        if (personalBest == -1) {
            std::string username = GJAccountManager::sharedState()->m_username;
            if (username == replays[i].username)
                personalBest = i;
        }

        RecordCell* cell = RecordCell::create(replays[i], i + 1, static_cast<geode::Popup<>*>(this));
        cellsArray->addObject(cell);
        cells.push_back(cell);

        if (Player::get().currentRace == i + 1) {
            cell->raceToggle->toggle(true);
            selectedRace = cell;
        }

        if (Player::get().currentSpectate == i + 1) {
            cell->spectateToggle->toggle(true);
            selectedSpectate = cell;
        }
    }

	ListView* listView = ListView::create(cellsArray, 35, 245, 160);
	CCNode* contentLayer = static_cast<CCNode*>(listView->m_tableView->getChildren()->objectAtIndex(0));

    if (personalBest != -1) {
        CCArray* children = contentLayer->getChildren();
        CCObject* child;
        int it = 0;

        CCARRAY_FOREACH(children, child) {
            if (GenericListCell* cell = typeinfo_cast<GenericListCell*>(child)) {
                if (it == personalBest) {
                    cell->m_backgroundLayer->setColor(ccc3(230, 150, 10));
                    break;
                }
                it++;
            }
        }
    }

	GJCommentListLayer* listLayer = GJCommentListLayer::create(listView, "", ccc4(255, 255, 255, 0), 245, 160, true);
	listLayer->setPosition((winSize / 2) - (listLayer->getContentSize() / 2) - CCPoint(6, 0) + ccp(0, 1));
	listLayer->setZOrder(1);
	listLayer->setID("list-layer");
    listLayer->setPositionY(listLayer->getPositionY() -10);
	m_mainLayer->addChild(listLayer);

	listLayer->setUserObject("dont-correct-borders", cocos2d::CCBool::create(true));

	CCSprite* topBorder = static_cast<CCSprite*>(listLayer->getChildByID("top-border"));
	CCSprite* bottomBorder = static_cast<CCSprite*>(listLayer->getChildByID("bottom-border"));
	CCSprite* rightBorder = static_cast<CCSprite*>(listLayer->getChildByID("right-border"));
	CCSprite* leftBorder = static_cast<CCSprite*>(listLayer->getChildByID("left-border"));

    topBorder->setScale(0.72);
    bottomBorder->setScale(0.72);
    leftBorder->setScaleX(0.72);
    leftBorder->setPositionX(-5);
    rightBorder->setScaleX(0.72);
    rightBorder->setPositionX(250);

	Scrollbar* scrollbar = Scrollbar::create(listView->m_tableView);
	scrollbar->setPosition(winSize / 2 + ccp((listLayer->getScaledContentSize().width / 2) + 4, -10));
	scrollbar->setID("scrollbar");
	m_mainLayer->addChild(scrollbar);

    CCScale9Sprite* listBackground = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
	listBackground->setColor({ 194 ,114, 62 });
	listBackground->setPosition(winSize / 2 + ccp(-6.11f, -10));
	listBackground->setContentSize({ 246.1f, 164.1f });
	m_mainLayer->addChild(listBackground);

    return true;
}

bool RecordCell::init(Replay replay, int rank) {
	CCMenu* menu = CCMenu::create();
	menu->setPosition({ 0,0 });
	addChild(menu);

    this->replay = replay;
    this->rank = rank;

    float time = replay.time;
    std::string username = replay.username;
    std::unordered_map<VehicleType, int> icons = replay.icons;
	PlayerColors colors = replay.colors;
	
    if (rank <= 3) {
        std::string texture = rank == 1 ? "rankIcon_top10_001.png" : "rankIcon_top50_001.png";

        if (rank == 3)
            texture = "rankIcon_top100_001.png";

        CCSprite* sprite = CCSprite::createWithSpriteFrameName(texture.c_str());
        sprite->setPosition({17, 17.5});
        sprite->setScale(0.65);
        addChild(sprite);
    } else {
        CCLabelBMFont* lbl = CCLabelBMFont::create(std::to_string(rank).c_str(), "goldFont.fnt");
        lbl->setPosition({17, 17.5});
        lbl->setScale(0.6);
        addChild(lbl);
    }

    SimplePlayer* icon = SimplePlayer::create(icons.at(VehicleType::Cube));
    icon->setPosition({44, 17.5});
    icon->setScale(0.6);
    addChild(icon);

    GameManager* gm = GameManager::get();

    icon->setColor(gm->colorForIdx(colors.color1));
    icon->setSecondColor(gm->colorForIdx(colors.color2));
    icon->m_hasGlowOutline = colors.glowEnabled;

    if (icon->m_hasGlowOutline)
        icon->enableCustomGlowColor(gm->colorForIdx(colors.glowColor));
    else
        icon->disableCustomGlowColor();

    icon->updateColors();

	CCSprite* spr = CCSprite::create("GJ_button_01.png");
    spr->setScale(0.5);
	CCSprite* spr2 = CCSprite::create("spectateGhost.png"_spr);
    spr2->setPosition(spr->getContentSize() / 2);
    spr2->setScale(1.1);
    spr->addChild(spr2);

    CCSprite* spr3 = CCSprite::create("GJ_button_01.png");
    spr3->setScale(0.5);
    spr3->setOpacity(140);
    spr3->setColor(ccc3(190, 190, 190));
	CCSprite* spr4 = CCSprite::create("spectateGhost.png"_spr);
    spr4->setOpacity(140);
    spr4->setColor(ccc3(190, 190, 190));
    spr4->setPosition(spr3->getContentSize() / 2);
    spr4->setScale(1.1);
    spr3->addChild(spr4);

    spectateToggle = CCMenuItemToggler::create(spr, spr3, this, menu_selector(RecordCell::onSpectate));
    spectateToggle->setPosition({226, 17.5});
    menu->addChild(spectateToggle);

    spr = CCSprite::create("GJ_button_01.png");
    spr->setScale(0.5);
	spr2 = CCSprite::create("raceGhost.png"_spr);
    spr2->setPosition(spr->getContentSize() / 2);
    spr2->setScale(0.95);
    spr->addChild(spr2);

    spr3 = CCSprite::create("GJ_button_01.png");
    spr3->setScale(0.5);
    spr3->setOpacity(160);
    spr3->setColor(ccc3(190, 190, 190));
	spr4 = CCSprite::create("raceGhost.png"_spr);
    spr4->setColor(ccc3(190, 190, 190));
    spr4->setOpacity(160);
    spr4->setPosition(spr3->getContentSize() / 2);
    spr4->setScale(0.95);
    spr3->addChild(spr4);

    raceToggle = CCMenuItemToggler::create(spr, spr3, this, menu_selector(RecordCell::onRace));
    raceToggle->setPosition({198, 17.5});
    menu->addChild(raceToggle);

    if (username.length() > 14) {
        username = username.substr(0, 14);
        username += "...";   
    }

    CCLabelBMFont* lbl = CCLabelBMFont::create(username.c_str(), "goldFont.fnt");
    lbl->setPosition({91, 20.4});
    lbl->limitLabelWidth(55.f, 0.525f, 0.01f);
    lbl->updateLabel();
    addChild(lbl);

    lbl = CCLabelBMFont::create(RecordsLayer::getFormattedTime(time).c_str(), "goldFont.fnt");
    lbl->setPosition({150, 20.4});
    lbl->setScale(0.525);
    lbl->limitLabelWidth(55.f, 0.525f, 0.01f);
    lbl->updateLabel();
    addChild(lbl);

    lbl = CCLabelBMFont::create("Player", "goldFont.fnt");
    lbl->setPosition({91, 8.5});
    lbl->setScale(0.25);
    lbl->setOpacity(123);
    addChild(lbl);

    lbl = CCLabelBMFont::create("Time", "goldFont.fnt");
    lbl->setPosition({150, 8.5});
    lbl->setScale(0.25);
    lbl->setOpacity(123);
    addChild(lbl);
        
    return true;
}

void RecordCell::onRace(CCObject*) {    
    Loader::get()->queueInMainThread([this] {
        if (!recordsLayer) return;
        RecordsLayer* layer = static_cast<RecordsLayer*>(recordsLayer);

        if (layer->selectedRace)
            layer->selectedRace->raceToggle->toggle(false);

        if (layer->selectedSpectate) {
            layer->selectedSpectate->spectateToggle->toggle(false);
            layer->selectedSpectate = nullptr;
            Player::stopSpectating();
        }

        layer->selectedRace = this;

        Player::loadGhost(replay, rank);
    });
}

void RecordCell::onSpectate(CCObject* obj) {
    PlayLayer* pl = PlayLayer::get();

    if (!pl) return;
    if (!recordsLayer) return;

    CCMenuItemToggler* toggle = static_cast<CCMenuItemToggler*>(obj);
    RecordsLayer* layer = static_cast<RecordsLayer*>(recordsLayer);

    if (layer->selectedSpectate && layer->selectedSpectate != this) {
        layer->selectedSpectate->spectateToggle->toggle(false);
        Player::stopSpectating();
    }

    if (!toggle->isToggled()) {
        layer->selectedSpectate = this;
        if (pl->m_isTestMode) return;

        Player::startSpectating(replay, rank);

        if (!layer->selectedRace) return;
        layer->selectedRace->raceToggle->toggle(false);
        layer->selectedRace = nullptr;
    } else
        Player::stopSpectating();
}