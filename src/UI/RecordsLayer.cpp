#include "RecordsLayer.hpp"
#include "ManagerLayer.hpp"
#include "../RecordsManager/RecordsManager.hpp"
#include "../Player/Player.hpp"

#include <Geode/ui/GeodeUI.hpp>
#include <random>

void RecordsLayer::open(CCObject*) {
    open();
}

void RecordsLayer::onSettings(CCObject*) {
	geode::openSettingsPopup(Mod::get(), false);
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
    CCMenu* menu = CCMenu::create();
    m_mainLayer->addChild(menu);

    std::string levelName = pl->m_level->m_levelName;
    setTitle("Ghosts for \"" + levelName + "\"");
    m_title->limitLabelWidth(248.f, m_title->getScale(), 0.001f);
    m_title->updateLabel();
    m_title->setPositionY(m_title->getPositionY() + 3);

    cocos2d::CCPoint offset = (CCDirector::sharedDirector()->getWinSize() - m_mainLayer->getContentSize()) / 2;
    m_mainLayer->setPosition(m_mainLayer->getPosition() - offset);
    m_closeBtn->setPosition(m_closeBtn->getPosition() + offset);
    m_bgSprite->setPosition(m_bgSprite->getPosition() + offset);
    m_title->setPosition(m_title->getPosition() + offset);

    noGhostsLabel = CCLabelBMFont::create("No Saved Ghosts", "bigFont.fnt");
    noGhostsLabel->setScale(0.675f);
    noGhostsLabel->setPositionY(10);
    noGhostsLabel->setOpacity(141);
    menu->addChild(noGhostsLabel);
    menu->setZOrder(10);

    addList();

    CCSprite* spr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
    spr->setScale(0.575f);
    CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(RecordsLayer::onSettings));
    btn->setPosition({-134, -112});
    menu->addChild(btn);

    spr = CCSprite::createWithSpriteFrameName("GJ_plainBtn_001.png");
    spr->setScale(0.575f);
    CCSprite* icon = CCSprite::createWithSpriteFrameName("folderIcon_001.png");
    icon->setPosition(spr->getContentSize() / 2);
    icon->setScale(0.7f);
    spr->addChild(icon);

    std::vector<GhostLevel> levels = RecordsManager::getSavedLevels();
    if (levels.empty()) {
        spr->setOpacity(110);
        icon->setOpacity(110);
    }

    btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(RecordsLayer::onManager));
    btn->setPosition({134, -112});
    if (levels.empty()) btn->setEnabled(false);
    menu->addChild(btn);

    return true;
}

void RecordsLayer::addList() {
    std::vector<std::pair<ReplayInfo, std::filesystem::path>> replays = RecordsManager::getLevelCompletions(EditorIDs::getID(PlayLayer::get()->m_level));
    cocos2d::CCSize winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
	CCArray* cellsArray = CCArray::create();
    int personalBest = -1;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 1000);
    bool xd = dist(gen) == 1; 

    for (int i = 0; i < replays.size(); i++) {
        if (i > 99) break;

        if (personalBest == -1) {
            std::string username = RecordsManager::getUsername();
            if (username == replays[i].first.username)
                personalBest = i;
        }

        RecordCell* cell = RecordCell::create(replays[i], i + 1, static_cast<geode::Popup<>*>(this), xd);
        cellsArray->addObject(cell);
        cells.push_back(cell);

        if (PlayerManager::containsTime(replays[i].first.time)) {
            if (PlayerManager::getCurrentSpectate() == replays[i].first.time) {
                cell->spectateToggle->toggle(true);
                selectedSpectate = cell;
            } else {
                cell->raceToggle->toggle(true);
                selectedRaces.push_back(cell);
            }
        }
        
    }

    noGhostsLabel->setVisible(replays.empty());

	ListView* listView = ListView::create(cellsArray, 35, 258, 195);
	CCNode* contentLayer = static_cast<CCNode*>(listView->m_tableView->getChildren()->objectAtIndex(0));

    if (personalBest != -1) {
        int it = 0;
        for (GenericListCell* cell : CCArrayExt<GenericListCell*>(contentLayer->getChildren())) {
            if (it == personalBest) {
                cell->m_backgroundLayer->setColor(ccc3(230, 150, 10));
                break;
            }
            it++;
        }
    }

	GJCommentListLayer* listLayer = GJCommentListLayer::create(listView, "", ccc4(255, 255, 255, 0), 258, 195, true);
	listLayer->setPosition((winSize / 2) - (listLayer->getContentSize() / 2) + ccp(0, 1));
	listLayer->setZOrder(1);
	listLayer->setID("list-layer");
    listLayer->setPositionY(listLayer->getPositionY() + 2.5f);
	m_mainLayer->addChild(listLayer);

	listLayer->setUserObject("dont-correct-borders", cocos2d::CCBool::create(true));

	CCSprite* topBorder = listLayer->getChildByType<CCSprite>(1);
	CCSprite* bottomBorder = listLayer->getChildByType<CCSprite>(0);
	CCSprite* rightBorder = listLayer->getChildByType<CCSprite>(3);
	CCSprite* leftBorder = listLayer->getChildByType<CCSprite>(2);

    topBorder->setScale(0.757f);
    bottomBorder->setScale(0.757f);
    leftBorder->setScaleX(0.757f);
    leftBorder->setPositionX(-5);
    rightBorder->setScaleX(0.757f);
    rightBorder->setPositionX(263);

    if (cellsArray->count() >= 6) {
        Scrollbar* scrollbar = Scrollbar::create(listView->m_tableView);
        scrollbar->setPosition(winSize / 2 + ccp((listLayer->getScaledContentSize().width / 2) + 12.5f, 2.5f));
        scrollbar->setID("scrollbar");
        m_mainLayer->addChild(scrollbar);
    }

    CCScale9Sprite* listBackground = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
	listBackground->setColor({ 194 ,114, 62 });
	listBackground->setPosition(winSize / 2 + ccp(-0.11f, 3));
	listBackground->setContentSize({ 259.1f, 199.4f });
    listBackground->setID("background");
	m_mainLayer->addChild(listBackground);
}

void RecordsLayer::refresh() {
    if (CCNode* scrollbar = m_mainLayer->getChildByID("scrollbar"))
		scrollbar->removeFromParentAndCleanup(true);
	if (CCNode* lbl = m_mainLayer->getChildByID("no-ghosts-label"))
		lbl->removeFromParentAndCleanup(true);
	CCNode* listLayer = m_mainLayer->getChildByID("list-layer");
	ListView* listView = listLayer->getChildByType<ListView>(0);
	CCLayer* contentLayer = nullptr;
	contentLayer = typeinfo_cast<CCLayer*>(listView->m_tableView->getChildren()->objectAtIndex(0));
    listLayer->removeFromParentAndCleanup(true);
	m_mainLayer->getChildByID("background")->removeFromParentAndCleanup(true);

    addList();
}

void RecordsLayer::onManager(CCObject*) {
	ManagerLayer::open(EditorIDs::getID(PlayLayer::get()->m_level));
}

bool RecordCell::init(std::pair<ReplayInfo, std::filesystem::path> replay, int rank, bool xd) {
	CCMenu* menu = CCMenu::create();
	menu->setPosition({ 0,0 });
	addChild(menu);

    this->info = replay.first;
    this->path = replay.second;
    this->rank = rank;

    float time = info.time;
    std::string username = info.username;
    std::unordered_map<VehicleType, int> icons = info.icons;
	PlayerColors colors = info.colors;
	
    if (rank <= 3) {
        std::string texture = rank == 1 ? "rankIcon_top10_001.png" : "rankIcon_top50_001.png";

        if (rank == 3)
            texture = "rankIcon_top100_001.png";

        CCSprite* sprite = CCSprite::createWithSpriteFrameName(texture.c_str());
        sprite->setPosition({17, 17.5});
        sprite->setScale(0.65f);
        addChild(sprite);
    } else {
        CCLabelBMFont* lbl = CCLabelBMFont::create(std::to_string(rank).c_str(), "goldFont.fnt");
        lbl->setPosition({17, 17.5});
        lbl->setScale(0.6f);
        addChild(lbl);
    }

    SimplePlayer* icon = SimplePlayer::create(icons.at(VehicleType::Cube));
    icon->setPosition({44, 17.5});
    icon->setScale(0.6f);
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

    std::string xdTexture = xd ? "spectateGhost2.png"_spr : "spectateGhost.png"_spr;

	CCSprite* spr = CCSprite::create("GJ_button_01.png");
    spr->setScale(0.5f);
	CCSprite* spr2 = CCSprite::create(xdTexture.c_str());
    spr2->setPosition(spr->getContentSize() / 2);
    spr2->setScale(1.1f);
    spr->addChild(spr2);

    CCSprite* spr3 = CCSprite::create("GJ_button_01.png");
    spr3->setScale(0.5f);
    spr3->setOpacity(140);
    spr3->setColor(ccc3(190, 190, 190));
	CCSprite* spr4 = CCSprite::create(xdTexture.c_str());
    spr4->setOpacity(140);
    spr4->setColor(ccc3(190, 190, 190));
    spr4->setPosition(spr3->getContentSize() / 2);
    spr4->setScale(1.1f);
    spr3->addChild(spr4);
 
    spectateToggle = CCMenuItemToggler::create(spr, spr3, this, menu_selector(RecordCell::onSpectate));
    spectateToggle->setPosition({240, 17.5});
    menu->addChild(spectateToggle); 

    spr = CCSprite::create("GJ_button_01.png"); 
    spr->setScale(0.5f);
	spr2 = CCSprite::create("raceGhost.png"_spr);
    spr2->setPosition(spr->getContentSize() / 2);
    spr2->setScale(1.1f);
    spr->addChild(spr2); 

    spr3 = CCSprite::create("GJ_button_01.png");
    spr3->setScale(0.5f);
    spr3->setOpacity(160);
    spr3->setColor(ccc3(190, 190, 190));
	spr4 = CCSprite::create("raceGhost.png"_spr);
    spr4->setColor(ccc3(190, 190, 190));
    spr4->setOpacity(160);
    spr4->setPosition(spr3->getContentSize() / 2);
    spr4->setScale(1.1f);
    spr3->addChild(spr4);

    raceToggle = CCMenuItemToggler::create(spr, spr3, this, menu_selector(RecordCell::onRace));
    raceToggle->setPosition({212, 17.5});
    menu->addChild(raceToggle);

    if (username.length() > 14) {
        username = username.substr(0, 14);
        username += "...";   
    }

    CCLabelBMFont* lbl = CCLabelBMFont::create(username.c_str(), "goldFont.fnt");
    lbl->setPosition({95, 20.4});
    lbl->limitLabelWidth(65.f, 0.525f, 0.01f);
    lbl->updateLabel();
    addChild(lbl);

    

    lbl = CCLabelBMFont::create(RecordsManager::getFormattedTime(time).c_str(), "goldFont.fnt");
    lbl->setPosition({161, 20.4});
    lbl->setScale(0.525f);
    lbl->limitLabelWidth(65.f, 0.525f, 0.01f);
    lbl->updateLabel();
    addChild(lbl);

    lbl = CCLabelBMFont::create("Player", "goldFont.fnt");
    lbl->setPosition({95, 8.5});
    lbl->setScale(0.25f);
    lbl->setOpacity(123);
    addChild(lbl);

    lbl = CCLabelBMFont::create("Time", "goldFont.fnt");
    lbl->setPosition({161, 8.5});
    lbl->setScale(0.25f);
    lbl->setOpacity(123);
    addChild(lbl);
        
    return true;
}

void RecordCell::onRace(CCObject*) {
    bool toggle = !raceToggle->isToggled(); 

    if (!toggle && PlayerManager::containsTime(info.time)) {
        PlayerManager::removePlayer(PlayerManager::get().times.at(info.time));
        return;
    } else if (!toggle) return;

    if (PlayerManager::getCurrentSpectate() == info.time) {
        onSpectate(nullptr);
        spectateToggle->toggle(false);
        raceToggle->toggle(false);
    }
    Loader::get()->queueInMainThread([this] {
        Replay replay = {
            info,
            RecordsManager::getCompletionActions(path)
        };
        PlayerManager::addPlayer(replay, PlayLayer::get());
    });
}

void RecordCell::onSpectate(CCObject* obj) {
    bool toggle = !spectateToggle->isToggled();

    if (toggle) {
        if (raceToggle->isToggled()) raceToggle->toggle(false);
        if (PlayerManager::containsTime(info.time)) {
            Replay replay = {
                info,
                RecordsManager::getCompletionActions(path)
            };

            Player player = PlayerManager::get().times.at(info.time);
            for (int i = 0; i < PlayerManager::get().players.size(); i++) {
                if (PlayerManager::get().players[i] == player) {
                    PlayerManager::get().players[i].startSpectating(replay);
                    break;
                }
            }
        }
    } else {
        PlayerManager::stopSpectating();
        raceToggle->toggle(true);
    }
}