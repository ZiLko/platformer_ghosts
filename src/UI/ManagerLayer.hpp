#pragma once

#include "../Includes.hpp"

class GhostCell : public CCNode {

	geode::Popup<int>* loadLayer = nullptr;
	ReplayInfo info;
	std::filesystem::path path;
	std::string time;

public:

	int rank = 0;
	CCMenu* menu = nullptr;
	CCMenuItemToggler* toggler = nullptr;

	static GhostCell* create(geode::Popup<int>*, std::pair<ReplayInfo, std::filesystem::path>, int);

	bool init(std::pair<ReplayInfo, std::filesystem::path>);
	void onDelete(CCObject*);
	void deleteCell(bool reload);
	void onSelect(CCObject*);
	void selectCell(bool single);
	void setColors(SimplePlayer*);
};

class ManagerLayer : public geode::Popup<int>, public TextInputDelegate {
public:

	CCMenuItemToggler* selectAllToggle = nullptr;
	CCMenuItemToggler* sortToggle = nullptr;
	CCLabelBMFont* ghostCountLabel = nullptr;
	CCLabelBMFont* levelNameLabel = nullptr;

	std::vector<GhostLevel> levels;
	std::vector<GhostCell*> selectedCells;
	std::vector<GhostCell*> allCells;
	std::string search = "";

	bool invertSort = false;
	int currentLevelIndex = 0;
	GhostLevel currentLevel;
	
	static ManagerLayer* create(int id = 0);
	bool setup(int) override;
	static void open(int id = 0);
	void openFolder(CCObject*);
	void addList(bool refresh = false, float prevScroll = 0.f);
	void reloadList(int amount = 1);
	void deleteSelected(CCObject*);
	void onSelectAll(CCObject*);
	void onLevelsList(CCObject*);
	void updateSort(CCObject*);
	void switchLevel(CCObject*);
};