#include "RecordsManager/RecordsManager.hpp"
#include "Recorder/Recorder.hpp"
#include "UI/RecordsLayer.hpp"
#include "UI/ManagerLayer.hpp"
#include "Player/Player.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>

$execute {
    geode::listenForSettingChanges("smoothness", +[](int64_t value) {
        Recorder::get().fps = value;
    });
    geode::listenForSettingChanges("p1_opacity", +[](int64_t value) {
        Player::updateOpacity(false);
    });
    geode::listenForSettingChanges("p2_opacity", +[](int64_t value) {
        Player::updateOpacity(true);
    }); 
    geode::listenForSettingChanges("no_colors", +[](bool value) {
        Player::updateColors();
    });
    geode::listenForSettingChanges("player_disabled", +[](bool value) {
        Player::updateDisabled();
    });
    geode::listenForSettingChanges("recorder_disabled", +[](bool value) {
        Recorder::get().disabled = value;
    });
    geode::listenForSettingChanges("show_ui", +[](bool value) {
        Player::updateUI();
    });
    geode::listenForSettingChanges("off_screen_indicators", +[](bool value) {
        Player::updateCamera();
    }); 

    Player::get().disabled = Mod::get()->getSettingValue<bool>("player_disabled");
    Recorder::get().disabled = Mod::get()->getSettingValue<bool>("recorder_disabled");
    Recorder::get().fps = Mod::get()->getSettingValue<int64_t>("smoothness");
};

bool shouldReturn(PlayLayer* pl) {
    return !pl->m_levelSettings->m_platformerMode || pl->m_isTestMode || pl->m_isPracticeMode || pl->m_levelEndAnimationStarted;
}

bool shouldReturn(GJBaseGameLayer* pl) {
    return !pl->m_levelSettings->m_platformerMode || pl->m_isTestMode || pl->m_isPracticeMode || pl->m_levelEndAnimationStarted;
}

class $modify(GameLayer, GJBaseGameLayer) {

	struct Fields {
		int totalFrame = 0;
        int previousFrame = 0;
        bool levelComplete = false;
	};

    bool canBeActivatedByPlayer(PlayerObject * p0, EffectGameObject * p1) {
        if (!GJBaseGameLayer::canBeActivatedByPlayer(p0, p1)) return false;

        if (!PlayLayer::get()) return true;
		if (shouldReturn(this)) return true;
        if (p0 != m_player1 && p0 != m_player2) return true;

        int id = p1->m_objectID;
        bool player2 = p0 == m_player2;

        Recorder::handlePortal(id, m_fields->totalFrame, player2, p0);

        return true;
    }

    void processCommands(float dt) {
        GJBaseGameLayer::processCommands(dt);
        
        PlayLayer* pl = PlayLayer::get();
		if (!pl) return;

		if (m_gameState.m_currentProgress <= 1) {
            Recorder::resetState(m_levelSettings->m_platformerMode && !m_isTestMode && !m_isPracticeMode);
            Player::resetState();
            m_fields->totalFrame = 0;
        }

        int attemptFrame = static_cast<int>(pl->m_gameState.m_levelTime * 240.0);
        if (!pl->m_levelEndAnimationStarted && (m_fields->previousFrame == 0 || m_fields->previousFrame != attemptFrame))
            m_fields->totalFrame++;

        m_fields->previousFrame = attemptFrame;

        if (Player::get().isSpectating && pl->m_levelEndAnimationStarted)
            Player::handleCompletion();

        if (!m_fields->levelComplete && pl->m_levelEndAnimationStarted) {
            Player::playCompleteEffect();

            Loader::get()->queueInMainThread([this] {
                cocos2d::CCPoint pos1 = m_player1->getPosition();
                cocos2d::CCPoint pos2 = m_player2->getPosition();

                Action action;
                action.type = ActionType::Position;
                action.frame = m_fields->totalFrame + 1;

                PlayerData p1Data = { pos1, 0.f };
                PlayerData p2Data = { pos2, 0.f };
                PositionData data = { p1Data, p2Data };

                action.data = data;
                Recorder::get().actions.push_back(action);
            });
        }
        
        m_fields->levelComplete = m_levelEndAnimationStarted;

		if (shouldReturn(this)) return;

        Recorder::handleRecording(pl, m_fields->totalFrame);
        Player::handlePlaying(this, m_fields->totalFrame);
    }

};

class $modify(PlayerObject) {

    static void onModify(auto& self) {
        if (!self.setHookPriority("PlayerObject::playSpawnEffect", 100))
            log::warn("PlayerObject::playSpawnEffect hook priority fail xD.");
        if (!self.setHookPriority("PlayerObject::playDeathEffect", 100))
            log::warn("PlayerObject::playDeathEffect hook priority fail xD.");
    }

    void update(float dt) {
        if (Player::canUpdatePlayer())
            PlayerObject::update(dt);
    }

    bool shouldReturnPlayer() {
        PlayLayer* pl = PlayLayer::get();
        if (!pl) return true;

        if (shouldReturn(pl)) return true;
        if (this != pl->m_player1 && this != pl->m_player2) return true;

        return false;
    }

    bool pushButton(PlayerButton btn) {
        if (!Player::canClick()) return false;
        if (!PlayerObject::pushButton(btn)) return false;

        if (shouldReturnPlayer()) return true;

        int frame = static_cast<GameLayer*>(m_gameLayer)->m_fields->totalFrame;
        Recorder::handleInput(frame, static_cast<int>(btn), true, this == PlayLayer::get()->m_player2);

        return true;
    }

    bool releaseButton(PlayerButton btn) {
        if (!Player::canClick()) return false;
        if (!PlayerObject::releaseButton(btn)) return false;

        if (shouldReturnPlayer()) return true;
        
        int frame = static_cast<GameLayer*>(m_gameLayer)->m_fields->totalFrame;
        Recorder::handleInput(frame, static_cast<int>(btn), false, this == PlayLayer::get()->m_player2);

        return true;
    }

    void playDeathEffect() {
        PlayerObject::playDeathEffect();

        if (shouldReturnPlayer()) return;

        int frame = static_cast<GameLayer*>(m_gameLayer)->m_fields->totalFrame;
        Recorder::handleEffect(frame, EffectType::Death, this == PlayLayer::get()->m_player2);
    }

    void playSpawnEffect() {
        PlayerObject::playSpawnEffect();

        if (shouldReturnPlayer()) return;

        int frame = static_cast<GameLayer*>(m_gameLayer)->m_fields->totalFrame;
        Recorder::handleEffect(frame, EffectType::Respawn, this == PlayLayer::get()->m_player2);
    }

    void incrementJumps() {
        PlayerObject::incrementJumps();

        if (shouldReturnPlayer()) return;

        if (this == PlayLayer::get()->m_player2)
            Recorder::get().jumped2 = true;
        else
            Recorder::get().jumped1 = true;
    }

    void activateStreak() {
        if (!Player::get().isSpectating)
            PlayerObject::activateStreak();
    }

};

class $modify(PlayLayer) {
	
	void setupHasCompleted() {
		PlayLayer::setupHasCompleted();
        Player::clear();

        if (!m_levelSettings->m_platformerMode) return;

        Loader::get()->queueInMainThread([this] {
            Player::setup(this);
        });
	}

	void levelComplete() {
		PlayLayer::levelComplete();
        Recorder& r = Recorder::get();
        Player& p = Player::get();

        if (p.icon1) p.icon1->setVisible(false);
        if (p.icon2) p.icon2->setVisible(false);

        r.currentCompletionTime = 0.f;
        r.compareTime = p.info.time;
        if (!p.isRacing)
            r.compareTime = RecordsManager::getBestCompletion(EditorIDs::getID(PlayLayer::get()->m_level)).info.time;

		if (!m_levelSettings->m_platformerMode || m_isTestMode || m_isPracticeMode || p.spectated) return;

        int frame = static_cast<GameLayer*>(m_player1->m_gameLayer)->m_fields->totalFrame;
        float time = frame / 240.f;
        r.currentCompletionTime = time;
        if (r.compareTime == 0.f) r.compareTime = time * 2;

        Recorder::handleEffect(frame, EffectType::Complete, false);
        Recorder::handleEffect(frame, EffectType::Complete, true);
		RecordsManager::handleCompletion(EditorIDs::getID(m_level), time, Recorder::getActions());
	}
	
};

class $modify(PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        PlayLayer* pl = PlayLayer::get();
        if (!pl) return;

        if (!pl->m_levelSettings->m_platformerMode) return;

        CCSprite* sprite = CCSprite::createWithSpriteFrameName("GJ_plainBtn_001.png");
        sprite->setScale(1.05);        

        GameManager* gm = GameManager::get();

        SimplePlayer* icon = SimplePlayer::create(gm->getPlayerFrame());
        icon->setOpacity(160);
        icon->setScale(0.75);
        icon->setRotation(15);

        sprite->addChild(icon);
        icon->setPosition(sprite->getContentSize() / 2);

        CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(sprite,
            this,
            menu_selector(RecordsLayer::open)
        );

        if (!Loader::get()->isModLoaded("geode.node-ids")) {
            CCMenu* menu = CCMenu::create();
            menu->setID("button"_spr);
            addChild(menu);
            btn->setPosition({214, 88});
            menu->addChild(btn);
            return;
        }

        CCNode* menu = this->getChildByID("left-button-menu");
        menu->addChild(btn);
        menu->updateLayout();
    }

    void onResume(CCObject* obj) {
        PlayLayer* pl = PlayLayer::get();
        if (!pl) return PauseLayer::onResume(obj);
        if (!Player::get().shouldRestart) return PauseLayer::onResume(obj);
        if (pl->m_isTestMode) return PauseLayer::onResume(obj);

        if (pl->m_isPracticeMode)
            PauseLayer::onNormalMode(nullptr);
        else
            PauseLayer::onResume(obj);

        Player::get().shouldRestart = false;

        Loader::get()->queueInMainThread([pl] {
            pl->resetLevelFromStart();
        });
    }

    void onPracticeMode(CCObject* obj) {        
        PlayLayer* pl = PlayLayer::get();

        if (!pl) return PauseLayer::onPracticeMode(obj);
        if (!Player::get().shouldRestart) return PauseLayer::onPracticeMode(obj);
        if (pl->m_isTestMode) return PauseLayer::onPracticeMode(obj);

        PauseLayer::onResume(nullptr);

        Player::get().shouldRestart = false;
        Loader::get()->queueInMainThread([pl] {
            pl->resetLevelFromStart();
        });
    }
};

class $modify(EndLevelLayer) {
    
    void customSetup() {
        EndLevelLayer::customSetup();
        if (Mod::get()->getSettingValue<bool>("no_time_difference")) return;

        float time = Recorder::get().currentCompletionTime;
        if (time == 0.f || Player::get().spectated) return;

        float timeDifference = Recorder::get().compareTime - time;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(abs(timeDifference) >= 0.01 ? 2 : 3) << abs(timeDifference);
        std::string timeString = oss.str() + "s";
        timeString = (timeDifference < 0 ? "+" : "-") + timeString;

        CCLabelBMFont* timeLabel = nullptr;
        if (Loader::get()->isModLoaded("geode.node-ids"))
            timeLabel = static_cast<CCLabelBMFont*>(m_mainLayer->getChildByID("time-label"));
        else {
            for (CCNode* child : CCArrayExt<CCNode*>(m_mainLayer->getChildren())) {
                CCLabelBMFont* lbl = typeinfo_cast<CCLabelBMFont*>(child);
                if (!lbl) continue;
                std::string_view str = lbl->getString();
                if (!str.starts_with("Time")) continue;
                timeLabel = lbl;
                break;
            }
        }
        if (!timeLabel) return;

        cocos2d::ccColor3B color = timeDifference < 0 ? ccc3(255, 29, 29) : ccc3(106, 255, 29);
        if (time == Recorder::get().compareTime) {
            color = ccc3(255, 127, 29);
            timeString = "Tied";
        }

        CCLabelBMFont* lbl = CCLabelBMFont::create(("(" + timeString + ")").c_str(), "bigFont.fnt");
        lbl->setAnchorPoint({0, 0.5f});
        lbl->setID("time-difference"_spr);
        lbl->setScale(0.4f);
        lbl->setPositionY(timeLabel->getPositionY() - 2);
        lbl->setPositionX((timeLabel->getContentSize().width / 2.f) * timeLabel->getScale() + timeLabel->getPositionX());
        lbl->setColor(color);
        m_mainLayer->addChild(lbl);

    }
    
};