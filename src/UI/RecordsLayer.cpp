#include "RecordsLayer.hpp"

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

    

    cocos2d::CCSize winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();

	CCArray* cells = CCArray::create();
    cells->addObject(RecordCell::create(1));
    cells->addObject(RecordCell::create(2));
    cells->addObject(RecordCell::create(3));
    cells->addObject(RecordCell::create(4));
    cells->addObject(RecordCell::create(5));
    cells->addObject(RecordCell::create(6));
    cells->addObject(RecordCell::create(7));
    cells->addObject(RecordCell::create(8));

	ListView* listView = ListView::create(cells, 35, 245, 160);
	CCNode* contentLayer = static_cast<CCNode*>(listView->m_tableView->getChildren()->objectAtIndex(0));

	CCArray* children = contentLayer->getChildren();
	CCObject* child;

	GJCommentListLayer* listLayer = GJCommentListLayer::create(listView, "", ccc4(255, 255, 255, 0), 245, 160, true);
	listLayer->setPosition((winSize / 2) - (listLayer->getContentSize() / 2) - CCPoint(6, 0) + ccp(0, 1));
	listLayer->setZOrder(1);
	listLayer->setID("list-layer");
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
	scrollbar->setPosition({ (winSize.width / 2) + (listLayer->getScaledContentSize().width / 2) + 4, winSize.height / 2 });
	scrollbar->setID("scrollbar");
	m_mainLayer->addChild(scrollbar);

    return true;
}

bool RecordCell::init(int rank) {
	CCMenu* menu = CCMenu::create();
	menu->setPosition({ 0,0 });
	addChild(menu);
	
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

    SimplePlayer* icon = SimplePlayer::create(1);
    icon->setPosition({44, 17.5});
    icon->setScale(0.6);
    addChild(icon);

	CCSprite* spr = CCSprite::create("GJ_button_01.png");
    spr->setScale(0.5);
	CCSprite* spr2 = CCSprite::create("spectateGhost.png"_spr);
    spr2->setPosition(spr->getContentSize() / 2);
    spr2->setScale(1.1);
    spr->addChild(spr2);

    CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(spr, this, nullptr);
    btn->setPosition({226, 17.5});
    menu->addChild(btn);

    spr = CCSprite::create("GJ_button_01.png");
    spr->setScale(0.5);
	spr2 = CCSprite::create("raceGhost.png"_spr);
    spr2->setPosition(spr->getContentSize() / 2);
    spr2->setScale(0.95);
    spr->addChild(spr2);

    btn = CCMenuItemSpriteExtra::create(spr, this, nullptr);
    btn->setPosition({198, 17.5});
    menu->addChild(btn);

    CCLabelBMFont* lbl = CCLabelBMFont::create("Zilko", "goldFont.fnt");
    lbl->setPosition({91, 20.4});
    lbl->setScale(0.525);
    addChild(lbl);

    lbl = CCLabelBMFont::create("1.008s", "goldFont.fnt");
    lbl->setPosition({150, 20.4});
    lbl->setScale(0.525);
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