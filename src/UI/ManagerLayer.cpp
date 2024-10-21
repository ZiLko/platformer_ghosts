#include "ManagerLayer.hpp"
#include "RecordsLayer.hpp"
#include "ListLayer.hpp"
#include "../Player/Player.hpp"
#include "../RecordsManager/RecordsManager.hpp"

void ManagerLayer::open(int id) {
	ManagerLayer* layerReal = create(id);
	if (!layerReal) return;
	layerReal->m_noElasticity = true;
	layerReal->show();
}

VOID ManagerLayer::openFolder(CCObject*) {
	std::filesystem::path path = Mod::get()->getSaveDir() / std::to_string(currentLevel.id);
	if (!std::filesystem::exists(path)) return;
	file::openFolder(path);
}

void ManagerLayer::reloadList(int amount) {
	if (CCNode* scrollbar = m_mainLayer->getChildByID("scrollbar"))
		scrollbar->removeFromParentAndCleanup(true);

	if (CCNode* lbl = m_mainLayer->getChildByID("no-ghosts-label"))
		lbl->removeFromParentAndCleanup(true);

	CCNode* listLayer = m_mainLayer->getChildByID("list-layer");
	ListView* listView = getChildOfType<ListView>(listLayer, 0);

	CCLayer* contentLayer = nullptr;
	contentLayer = typeinfo_cast<CCLayer*>(listView->m_tableView->getChildren()->objectAtIndex(0));

	int childrenCount = 0;
	float posY = 0.f;
	if (contentLayer) {
		if (CCArray* children = contentLayer->getChildren())
			childrenCount = children->count();

		posY = contentLayer->getPositionY();
	}
	listLayer->removeFromParentAndCleanup(true);
	m_mainLayer->getChildByID("background")->removeFromParentAndCleanup(true);

	selectedCells.clear();
	allCells.clear();
	selectAllToggle->toggle(false);

	if (RecordsLayer* layer = getChildOfType<RecordsLayer>(CCDirector::sharedDirector()->getRunningScene(), 0))
		layer->refresh();

	addList(childrenCount > 7 && amount != 0, posY + (35.f * amount));
}

void ManagerLayer::deleteSelected(CCObject*) {
	int amount = selectedCells.size();
	if (amount < 1) return;

	geode::createQuickPopup(
		"Warning",
		"Are you sure you want to <cr>delete</c> <cy>" + std::to_string(amount) + "</c> " + "ghost(s)?",
		"Cancel", "Yes",
		[this, amount](auto, bool btn2) {
			if (!btn2) return;
			std::unordered_set<int> deletedRanks;

			for (GhostCell* cell : selectedCells) {
				deletedRanks.insert(cell->rank);
				cell->deleteCell(false);
			}
			if (deletedRanks.contains(Player::get().currentRace))
				Player::stopRacing();

			this->reloadList(amount);
			Notification::create("Ghosts Deleted", NotificationIcon::Success)->show();
		}
	);
}

void ManagerLayer::onSelectAll(CCObject* obj) {
	bool on = !static_cast<CCMenuItemToggler*>(obj)->isToggled();

	for (size_t i = 0; i < allCells.size(); i++) {
		CCMenuItemToggler* toggle = allCells[i]->toggler;
		if (toggle->isToggled() == on) continue;

		toggle->toggle(on);
		allCells[i]->selectCell(false);
	}
}

ManagerLayer* ManagerLayer::create(int id) {
	ManagerLayer* ret = new ManagerLayer();
	if (ret->init(385, 291, id, "GJ_square02.png")) {
		ret->autorelease();
		return ret;
	}

	delete ret;
	return nullptr;
}

void ManagerLayer::switchLevel(CCObject* obj) {
	std::string id = static_cast<CCNode*>(obj)->getID();
    currentLevelIndex += id == "left" ? -1 : 1;
    if (currentLevelIndex == -1) currentLevelIndex = levels.size() - 1;
    else if (currentLevelIndex == levels.size()) currentLevelIndex = 0;

	currentLevel = levels[currentLevelIndex];
	levelNameLabel->setString(currentLevel.name.c_str());
	levelNameLabel->limitLabelWidth(160.f, 0.75f, 0.001f);
	levelNameLabel->updateLabel();
	reloadList(0);
}

void ManagerLayer::onLevelsList(CCObject*) {
	ListLayer* layer = ListLayer::create();
	layer->m_noElasticity = true;
	layer->show();
}

bool ManagerLayer::setup(int id) {
	#ifdef GEODE_IS_ANDROID
	invertSort = true;
	#endif

	levels = RecordsManager::getSavedLevels();
	if (levels.empty()) return false;

	int index = 0;
	if (id != 0) {
		for (int i = 0; i < levels.size(); i++) {
			if (levels[i].id != id) continue;
			index = i;
			break;
		}
	}

	currentLevel = levels[index];
	currentLevelIndex = index;

	setTitle("Manage Ghosts");
	m_title->setPositionY(m_title->getPositionY() + 6);
	m_title->setScale(0.55f);
	m_title->setOpacity(179);
	m_closeBtn->setScale(0.7f);

	CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(CCLabelBMFont::create(
		"              ", "goldFont.fnt"),
		this, menu_selector(ManagerLayer::onLevelsList)
	);
	btn->setPositionY(104);
	m_buttonMenu->addChild(btn);

	levelNameLabel = CCLabelBMFont::create(currentLevel.name.c_str(), "goldFont.fnt");
	levelNameLabel->limitLabelWidth(160.f, 0.75f, 0.001f);
	levelNameLabel->updateLabel();
	levelNameLabel->setPosition(getChildOfType<CCLabelBMFont>(btn, 0)->getPosition());
	btn->addChild(levelNameLabel);

	CCSprite* spr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
	spr->setScale(0.675f);
	if (levels.size() == 1) spr->setOpacity(110);
	btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ManagerLayer::switchLevel));
	btn->setPosition({-99, 104});
	btn->setID("left");
	if (levels.size() == 1) btn->setEnabled(false);
	m_buttonMenu->addChild(btn);


	spr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
	spr->setFlipX(true);
	spr->setScale(0.675f);
	if (levels.size() == 1) spr->setOpacity(110);
	btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ManagerLayer::switchLevel));
	btn->setPosition({99, 104});
	if (levels.size() == 1) btn->setEnabled(false);
	m_buttonMenu->addChild(btn);

	CCSprite* emptyBtn = CCSprite::createWithSpriteFrameName("GJ_plainBtn_001.png");
	emptyBtn->setScale(0.560f);
	spr = CCSprite::createWithSpriteFrameName("folderIcon_001.png");
	spr->setPosition(emptyBtn->getContentSize() / 2);
	spr->setScale(0.7f);
	emptyBtn->addChild(spr);
	btn = CCMenuItemSpriteExtra::create(
		emptyBtn,
		this,
		menu_selector(ManagerLayer::openFolder)
	);
	btn->setPosition({120, -121});
    m_buttonMenu->addChild(btn);

	spr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
	spr->setScale(0.61f);
	btn = CCMenuItemSpriteExtra::create(
		spr,
		this,
		menu_selector(ManagerLayer::deleteSelected)
	);
	btn->setPosition({163, -121});
    m_buttonMenu->addChild(btn);

	CCSprite* spr1 = CCSprite::create("GJ_button_01.png");
	CCSprite* spr2 = CCSprite::createWithSpriteFrameName("GJ_sortIcon_001.png");
	spr2->setPosition({20, 20});
	spr1->addChild(spr2);

	CCSprite* spr3 = CCSprite::create("GJ_button_02.png");
	CCSprite* spr4 = CCSprite::createWithSpriteFrameName("GJ_sortIcon_001.png");
	spr4->setPosition({20, 20});
	spr3->addChild(spr4);

	sortToggle = CCMenuItemToggler::create(spr1, spr3, this, menu_selector(ManagerLayer::updateSort));
	sortToggle->setPosition({76, -121});
	sortToggle->setScale(0.55f);
	sortToggle->toggle(false);
	m_buttonMenu->addChild(sortToggle);

	CCSprite* spriteOn = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
	CCSprite* spriteOff = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");

	selectAllToggle = CCMenuItemToggler::create(spriteOff, spriteOn, this, menu_selector(ManagerLayer::onSelectAll));
	selectAllToggle->setScale(0.585f);
	selectAllToggle->setPosition({ -165, -121 });
    m_buttonMenu->addChild(selectAllToggle);

	CCLabelBMFont* lbl = CCLabelBMFont::create("Select all", "bigFont.fnt");
	lbl->setScale(0.4f);
	lbl->setPosition({ -110, -121 });
    m_buttonMenu->addChild(lbl);

	ghostCountLabel = CCLabelBMFont::create("13 Ghosts", "chatFont.fnt");
	ghostCountLabel->setOpacity(108);
	ghostCountLabel->setScale(0.55f);
	ghostCountLabel->setAnchorPoint({1.f, 0.5f});
	ghostCountLabel->setPosition({180, 130});
	m_buttonMenu->addChild(ghostCountLabel);

	addList();
	return true;
}

void ManagerLayer::updateSort(CCObject*) {
	if (!sortToggle) return;

	invertSort = !sortToggle->isToggled();

	#ifdef GEODE_IS_ANDROID
	invertSort = !invertSort;
	#endif

	reloadList(0);
}

void ManagerLayer::addList(bool refresh, float prevScroll) {
	cocos2d::CCSize winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
	std::vector<std::pair<ReplayInfo, std::filesystem::path>> ghosts = RecordsManager::getLevelCompletions(currentLevel.id);
	CCArray* cells = CCArray::create();

	for (int i = invertSort ? ghosts.size() - 1 : 0; invertSort ? i >= 0 : i < ghosts.size(); invertSort ? --i : ++i) {
		GhostCell* cell = GhostCell::create(static_cast<geode::Popup<int>*>(this), ghosts[i], i + 1);
		cells->addObject(cell);
	}

	ghostCountLabel->setString(fmt::format("{} Ghosts", std::to_string(cells->count())).c_str());

	if (cells->count() == 0) {
		CCLabelBMFont* lbl = CCLabelBMFont::create("No Ghosts", "bigFont.fnt");
		lbl->setPosition(winSize / 2);
		lbl->setScale(0.5f);
		lbl->setOpacity(100);
		lbl->setID("no-ghosts-label");
		m_mainLayer->addChild(lbl);
	}

	ListView* listView = ListView::create(cells, 45, 323, 180);
	CCNode* contentLayer = static_cast<CCNode*>(listView->m_tableView->getChildren()->objectAtIndex(0));
	if (refresh) contentLayer->setPositionY(prevScroll);
    
	cocos2d::ccColor3B color = ccc3(51, 68, 153);
	cocos2d::ccColor3B color1 = ccc3(std::max(0, color.r - 70), std::max(0, color.g - 70), std::max(0, color.b - 70));
	cocos2d::ccColor3B color2 = ccc3(std::max(0, color.r - 55), std::max(0, color.g - 55), std::max(0, color.b - 55));

    CCArray* children = contentLayer->getChildren();
	CCObject* child;
	int it = 0;

	CCARRAY_FOREACH(children, child) {
		if (GenericListCell* cell = typeinfo_cast<GenericListCell*>(child)) {
			allCells.push_back(static_cast<GhostCell*>(cell->getChildren()->objectAtIndex(2)));
			cocos2d::ccColor3B col = (it % 2 == 0) ? color1 : color2;
			cell->m_backgroundLayer->setColor(col); 
			it++;
		}
	}

	GJCommentListLayer* listLayer = GJCommentListLayer::create(listView, "", ccc4(255, 255, 255, 0), 323, 180, true);
	listLayer->setPosition((winSize / 2) - (listLayer->getContentSize() / 2) - CCPoint(6, 0) + ccp(0, 1));
	listLayer->setZOrder(1);
	listLayer->setID("list-layer");
	listView->setPositionY(-12);
	m_mainLayer->addChild(listLayer);

	listLayer->setUserObject("dont-correct-borders", cocos2d::CCBool::create(true));

	CCSprite* topBorder = getChildOfType<CCSprite>(listLayer, 1);
	CCSprite* bottomBorder = getChildOfType<CCSprite>(listLayer, 0);
	CCSprite* rightBorder = getChildOfType<CCSprite>(listLayer, 3);
	CCSprite* leftBorder = getChildOfType<CCSprite>(listLayer, 2);

	topBorder->setScaleX(0.945f);
	topBorder->setScaleY(1.f);
	topBorder->setPosition(ccp(161.25, 162.f));

	bottomBorder->setScaleX(0.945f);
	bottomBorder->setScaleY(1.f);
	bottomBorder->setPosition({ 161.25, -7.f });

	rightBorder->setScaleX(0.8f);
	rightBorder->setScaleY(5.9f);
	rightBorder->setPosition({ 328, -12 });

	leftBorder->setScaleX(0.8f);
	leftBorder->setScaleY(5.6f);
	leftBorder->setPosition({ -5.45, -1 });

	CCScale9Sprite* listBackground = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
	listBackground->setScale(0.7f);
	listBackground->setColor({ 0,0,0 });
	listBackground->setOpacity(75);
	listBackground->setPosition(winSize / 2 + ccp(-0.11f - 6, -10.5f));
	listBackground->setContentSize({ 461.1f, 255.1f });
	listBackground->setID("background");
	m_mainLayer->addChild(listBackground);

	Scrollbar* scrollbar = Scrollbar::create(listView->m_tableView);
	scrollbar->setPosition({ (winSize.width / 2) + (listLayer->getScaledContentSize().width / 2) + 4, winSize.height / 2 - 12 });
	scrollbar->setID("scrollbar");
	m_mainLayer->addChild(scrollbar);
}

GhostCell* GhostCell::create(geode::Popup<int>* layer, std::pair<ReplayInfo, std::filesystem::path> replay, int rank) {
	GhostCell* ret = new GhostCell();
	if (!ret->init(replay)) {
		delete ret;
		return nullptr;
	}

    ret->loadLayer = layer;
    ret->rank = rank;
	ret->autorelease();
	return ret;
}

bool GhostCell::init(std::pair<ReplayInfo, std::filesystem::path> replay) {
	this->info = replay.first;

	path = replay.second.parent_path();
	time = RecordsManager::getFormattedTime(info.time);

    menu = CCMenu::create();
    menu->setPosition({0, 0});
    addChild(menu);

	SimplePlayer* icon = SimplePlayer::create(info.icons.at(VehicleType::Cube));
    icon->setPosition({18, 28.5});
    icon->setScale(0.62f);
	setColors(icon);
    addChild(icon);

	CCLabelBMFont* nameLabel = CCLabelBMFont::create("Zilko", "bigFont.fnt");
	nameLabel->setPosition({40, 28.5});
    nameLabel->setScale(0.38f);
	nameLabel->setAnchorPoint({0, 0.5});
	addChild(nameLabel);

	CCLabelBMFont* lbl = CCLabelBMFont::create("|", "chatFont.fnt");
	lbl->setPosition({50 + nameLabel->getContentSize().width * nameLabel->getScale(), 28.5});
	lbl->setOpacity(74);
    lbl->setScaleX(0.8f);
    lbl->setScaleY(0.475f);
	addChild(lbl);

	CCLabelBMFont* timeLabel = CCLabelBMFont::create(time.c_str(), "bigFont.fnt");
	timeLabel->setPosition({lbl->getPositionX() + 10, 28.5});
    timeLabel->setScale(0.38f);
	timeLabel->setAnchorPoint({0, 0.5});
	addChild(timeLabel);

	CCSprite* spr2 = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
	spr2->setScale(0.485f);
	CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(
		spr2,
		this,
		menu_selector(GhostCell::onDelete)
	);
	btn->setPosition({303, 22.5f});
	menu->addChild(btn);

	CCSprite* spriteOn = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
	CCSprite* spriteOff = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");

	toggler = CCMenuItemToggler::create(spriteOff, spriteOn, this, menu_selector(GhostCell::onSelect));
	toggler->setScale(0.535f);
	toggler->setPosition({ 275, 22.5f });
	menu->addChild(toggler);

	lbl = CCLabelBMFont::create("Icons:", "chatFont.fnt");
	lbl->setPosition({20, 9.1f});
	lbl->setScale(0.5f);
	lbl->setOpacity(130);
	addChild(lbl);

	icon = SimplePlayer::create(1);
    icon->setPosition({40, 9.1f});
    icon->setScale(0.35f);
	icon->setOpacity(138);
    icon->updatePlayerFrame(info.icons.at(VehicleType::Ship), IconType::Jetpack);
    setColors(icon);
    addChild(icon);

	icon = SimplePlayer::create(1);
    icon->setPosition({60, 9.1f});
    icon->setScale(0.35f);
	icon->setOpacity(138);
    icon->updatePlayerFrame(info.icons.at(VehicleType::Ball), IconType::Ball);
	setColors(icon);
    addChild(icon);

	icon = SimplePlayer::create(1);
    icon->setPosition({80, 9.1f});
    icon->setScale(0.35f);
	icon->setOpacity(138);
    icon->updatePlayerFrame(info.icons.at(VehicleType::Ufo), IconType::Ufo);
	setColors(icon);
    addChild(icon);

	icon = SimplePlayer::create(1);
    icon->setPosition({100, 9.1f});
    icon->setScale(0.35f);
    icon->updatePlayerFrame(info.icons.at(VehicleType::Robot), IconType::Robot);
	setColors(icon);
    icon->m_robotSprite->GJRobotSprite::setOpacity(138);
    addChild(icon);
	
	icon = SimplePlayer::create(1);
    icon->setPosition({120, 9.1f});
    icon->setScale(0.35f);
    icon->updatePlayerFrame(info.icons.at(VehicleType::Spider), IconType::Spider);
	setColors(icon);
    icon->m_spiderSprite->GJSpiderSprite::setOpacity(138);
    addChild(icon);

	lbl = CCLabelBMFont::create(("|      Date: " + info.date).c_str(), "chatFont.fnt");
	lbl->setPosition({138.5, 9.1f});
	lbl->setScale(0.5f);
	lbl->setOpacity(130);
	lbl->setAnchorPoint({0, 0.5f});
	addChild(lbl);

	return true;
}

void GhostCell::setColors(SimplePlayer* icon) {
	GameManager* gm = GameManager::get();
	icon->setColor(gm->colorForIdx(info.colors.color1));
    icon->setSecondColor(gm->colorForIdx(info.colors.color2));
    icon->m_hasGlowOutline = info.colors.glowEnabled;
    if (icon->m_hasGlowOutline)
        icon->enableCustomGlowColor(gm->colorForIdx(info.colors.glowColor));
    else
        icon->disableCustomGlowColor();
    icon->updateColors();
}

void GhostCell::onDelete(CCObject*) {
	geode::createQuickPopup(
		"Warning",
		"Are you sure you want to <cr>delete</c> this ghost? <cl>(" + time + ")</c>",
		"Cancel", "Yes",
		[this](auto, bool btn2) {
			if (btn2) {
				this->deleteCell(true);
			}
		}
	);
}

void GhostCell::deleteCell(bool reload) {
	try {
		if (std::filesystem::remove_all(path)) {
			if (reload) {
				if (rank == Player::get().currentRace)
					Player::stopRacing();
				static_cast<ManagerLayer*>(loadLayer)->reloadList();
				Notification::create("Ghost Deleted", NotificationIcon::Success)->show();
			}
			this->removeFromParentAndCleanup(true);
		}
		else
			return FLAlertLayer::create("Error", "There was an error deleting this ghost.", "Ok")->show();

	}
	catch (const std::filesystem::filesystem_error& e) {
		return FLAlertLayer::create("Error", "There was an error deleting this ghost.", "Ok")->show();
	}
}

void GhostCell::onSelect(CCObject*) {
	selectCell(true);
}

void GhostCell::selectCell(bool single) {
	ManagerLayer* layer = static_cast<ManagerLayer*>(loadLayer);
	std::vector<GhostCell*>& selectedCells = layer->selectedCells;

	auto it = std::remove(selectedCells.begin(), selectedCells.end(), this);

	if (it != selectedCells.end()) {
		selectedCells.erase(it, selectedCells.end());
		if (single) layer->selectAllToggle->toggle(false);
	}
	else
		selectedCells.push_back(this);

	if (selectedCells.size() == layer->allCells.size() && single)
		layer->selectAllToggle->toggle(true);
}