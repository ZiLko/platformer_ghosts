#include "ListLayer.hpp"
#include "ManagerLayer.hpp"
#include "../RecordsManager/RecordsManager.hpp"

ListLayer* ListLayer::create() {
	ListLayer* ret = new ListLayer();
	if (ret->initAnchored(330, 280, "GJ_square02.png")) {
		ret->autorelease();
		return ret;
	}

	delete ret;
	return nullptr;
}

bool ListLayer::setup() {
    setTitle("Levels");
    m_title->setScale(0.8f);
    
    cocos2d::CCSize winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
	CCArray* cellsArray = CCArray::create();

    for (GhostLevel level : RecordsManager::getSavedLevels())
        cellsArray->addObject(LevelSelectCell::create(level));

	ListView* listView = ListView::create(cellsArray, 40, 280, 210);
	CCNode* contentLayer = static_cast<CCNode*>(listView->m_tableView->getChildren()->objectAtIndex(0));

	GJCommentListLayer* listLayer = GJCommentListLayer::create(listView, "", ccc4(255, 255, 255, 0), 280, 210, true);
	listLayer->setPosition((winSize / 2) - (listLayer->getContentSize() / 2) + ccp(0, 1));
	listLayer->setZOrder(1);
	listLayer->setID("list-layer");
    listLayer->setPositionY(listLayer->getPositionY() - 12.5f);
	m_mainLayer->addChild(listLayer);

	listLayer->setUserObject("dont-correct-borders", cocos2d::CCBool::create(true));

	CCSprite* topBorder = listLayer->getChildByType<CCSprite>(1);
	CCSprite* bottomBorder = listLayer->getChildByType<CCSprite>(0);
    topBorder->setScaleX(0.824f);
    bottomBorder->setScaleX(0.824f);

    if (cellsArray->count() >= 6) {
        Scrollbar* scrollbar = Scrollbar::create(listView->m_tableView);
        scrollbar->setPosition(winSize / 2 + ccp((listLayer->getScaledContentSize().width / 2) + 11.5f, -12.5f));
        scrollbar->setID("scrollbar");
        m_mainLayer->addChild(scrollbar);
    }

    CCScale9Sprite* listBackground = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
	listBackground->setColor({ 194 ,114, 62 });
	listBackground->setPosition(winSize / 2 + ccp(-0.11f, 3 - 12.5f));
	listBackground->setContentSize({ 281.1f, 214.4f });
    listBackground->setID("background");
	m_mainLayer->addChild(listBackground);

    return true;
}

bool LevelSelectCell::init(GhostLevel level) {
    this->levelId = level.id;

    CCLabelBMFont* lbl = CCLabelBMFont::create(level.name.c_str(), "goldFont.fnt");
    lbl->setAnchorPoint({0, 0.45f});
    lbl->setPosition({10, 26});
    lbl->limitLabelWidth(180.5f, 0.725f, 0.01f);
    addChild(lbl);

    lbl = CCLabelBMFont::create(("ID: " + std::to_string(levelId)).c_str(), "chatFont.fnt");
    lbl->setAnchorPoint({0, 0.5f});
    lbl->setPosition({11, 8});
    lbl->setScale(0.45f);
    lbl->setColor(ccc3(0, 0, 0));
    lbl->setOpacity(93);
    addChild(lbl);

    CCMenu* menu = CCMenu::create();
    menu->setPosition({0, 0});
    menu->setAnchorPoint({0, 0});
    addChild(menu);

    ButtonSprite* spr = ButtonSprite::create("Select");
    spr->setScale(0.625f);
    CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(LevelSelectCell::onSelect));
    btn->setPosition({236, 20});
    menu->addChild(btn);

    return true;
}

void LevelSelectCell::onSelect(CCObject*) {
    CCScene* scene = CCDirector::sharedDirector()->getRunningScene();
    int id = levelId;

    if (ListLayer* layer = scene->getChildByType<ListLayer>(0))
        static_cast<FLAlertLayer*>(layer)->keyBackClicked();
    if (ManagerLayer* layer = scene->getChildByType<ManagerLayer>(0))
        static_cast<FLAlertLayer*>(layer)->keyBackClicked();

    ManagerLayer::open(id);
    
}