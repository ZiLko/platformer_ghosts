#include "RecordsManager/RecordsManager.hpp"
#include "UI/RecordsLayer.hpp"
#include "UI/ManagerLayer.hpp"
#include "Recorder/Recorder.hpp"
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
        PlayerManager::updateOpacity(false);
    });
    geode::listenForSettingChanges("p2_opacity", +[](int64_t value) {
        PlayerManager::updateOpacity(true);
    }); 
    geode::listenForSettingChanges("no_colors", +[](bool value) {
        PlayerManager::updateColors();
    });
    geode::listenForSettingChanges("player_disabled", +[](bool value) {
        PlayerManager::updateDisabled();
    });
    geode::listenForSettingChanges("recorder_disabled", +[](bool value) {
        Recorder::get().disabled = value;
    });
    geode::listenForSettingChanges("show_ui", +[](bool value) {
        PlayerManager::get().updateUI();
    });
    geode::listenForSettingChanges("off_screen_indicators", +[](bool value) {
        PlayerManager::updateCamera();
    });
    geode::listenForSettingChanges("ui_opacity", +[](int64_t value) {
        PlayerManager::get().updateUI();
    }); 
    geode::listenForSettingChanges("ui_scale", +[](double value) {
        PlayerManager::get().updateUI();
    });

    PlayerManager::get().disabled = Mod::get()->getSettingValue<bool>("player_disabled");
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

    bool canBeActivatedByPlayer(PlayerObject * p0, EffectGameObject * p1) {
        if (!GJBaseGameLayer::canBeActivatedByPlayer(p0, p1)) return false;

        if (!PlayLayer::get()) return true;
		if (shouldReturn(this)) return true;
        if (p0 != m_player1 && p0 != m_player2) return true;

        int id = p1->m_objectID;
        bool player2 = p0 == m_player2;

        Recorder::handlePortal(id, Recorder::get().totalFrame, player2, p0);

        return true;
    }

    void processCommands(float dt) {
        GJBaseGameLayer::processCommands(dt);
        
        PlayLayer* pl = PlayLayer::get();
		if (!pl) return;

		if (m_gameState.m_currentProgress <= 1) {
            Recorder::resetState(m_levelSettings->m_platformerMode && !m_isTestMode && !m_isPracticeMode);
            PlayerManager::resetState();
            Recorder::get().totalFrame = 0;
            PlayerManager::getSpectated() = false;
        } else if (PlayerManager::getShouldRestart() && !m_levelEndAnimationStarted) {
            if (pl->m_isPracticeMode) pl->togglePracticeMode(false);
            Loader::get()->queueInMainThread([this] {
                if (!m_levelEndAnimationStarted)
                    PlayLayer::get()->resetLevelFromStart();
            });
        }

        int attemptFrame = static_cast<int>(pl->m_gameState.m_levelTime * 240.0);
        if (!pl->m_levelEndAnimationStarted && (Recorder::get().previousFrame == 0 || Recorder::get().previousFrame != attemptFrame || m_player1->m_isDead))
            Recorder::get().totalFrame++;

        Recorder::get().previousFrame = attemptFrame;

        if (PlayerManager::getIsSpectating() && pl->m_levelEndAnimationStarted)
            PlayerManager::handleCompletion();

        if (!Recorder::get().levelComplete && pl->m_levelEndAnimationStarted) {
            PlayerManager::playCompleteEffect();

            Loader::get()->queueInMainThread([this] {
                cocos2d::CCPoint pos1 = m_player1->getPosition();
                cocos2d::CCPoint pos2 = m_player2->getPosition();

                Action action;
                action.type = ActionType::Position;
                action.frame = Recorder::get().totalFrame + 1;

                PlayerData p1Data = { pos1, 0.f };
                PlayerData p2Data = { pos2, 0.f };
                PositionData data = { p1Data, p2Data };

                action.data = data;
                Recorder::get().actions.push_back(action);
            });
        }
        
        Recorder::get().levelComplete = m_levelEndAnimationStarted;

		if (shouldReturn(this)) return;

        Recorder::handleRecording(pl, Recorder::get().totalFrame);
        PlayerManager::handlePlaying(this, Recorder::get().totalFrame);
    }

};

class $modify(PlayerObject) {

    bool shouldReturnPlayer() {
        PlayLayer* pl = PlayLayer::get();
        if (!pl) return true;
        if (shouldReturn(pl)) return true;
        if (this != pl->m_player1 && this != pl->m_player2) return true;

        return false;
    }

    bool pushButton(PlayerButton btn) {
        if (!PlayerManager::canClick()) return false;
        if (!PlayerObject::pushButton(btn)) return false;

        PlayerManager::handleButton(PlayLayer::get(), btn, this, true);

        if (shouldReturnPlayer()) return true;

        int frame = Recorder::get().totalFrame;
        Recorder::handleInput(frame, static_cast<int>(btn), true, this == PlayLayer::get()->m_player2);

        return true;
    }

    bool releaseButton(PlayerButton btn) {
        if (!PlayerManager::canClick()) return false;
        if (!PlayerObject::releaseButton(btn)) return false;

        PlayerManager::handleButton(PlayLayer::get(), btn, this, false);
        
        if (shouldReturnPlayer()) return true;
        
        int frame = Recorder::get().totalFrame;
        Recorder::handleInput(frame, static_cast<int>(btn), false, this == PlayLayer::get()->m_player2);

        return true;
    }

    void playerDestroyed(bool v1) {
        PlayerObject::playerDestroyed(v1);
        PlayerManager::resetButtons();
    }

    void playDeathEffect() {
        PlayerObject::playDeathEffect();

        if (shouldReturnPlayer()) return;

        int frame = Recorder::get().totalFrame;
        Recorder::handleEffect(frame, EffectType::Death, this == PlayLayer::get()->m_player2);
    }

    void playSpawnEffect() {
        PlayerObject::playSpawnEffect();

        if (shouldReturnPlayer()) return;

        int frame = Recorder::get().totalFrame;
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
        if (!PlayerManager::getIsSpectating())
            PlayerObject::activateStreak();
    }

};

class $modify(PlayLayer) {

    void onQuit() {
        PlayerManager::get().ui = nullptr;
        PlayerManager::resetState();
        PlayerManager::stopSpectating();
        PlayerManager::get().players.clear();
        PlayLayer::onQuit();
    }

	void setupHasCompleted() {
		PlayLayer::setupHasCompleted();
        PlayerManager::clear();

        Recorder::resetState(false);
        PlayerManager::resetState();

        if (PlayerManager::getIsSpectating()) PlayerManager::stopSpectating();

        if (!m_levelSettings->m_platformerMode) return;

        Loader::get()->queueInMainThread([this] {
            PlayerManager::get().setup(this);
        });
	}

    void resetLevel() {
        if (PlayerManager::getCanReset() || !PlayerManager::getIsSpectating() || PlayerManager::getShouldRestart() || m_levelEndAnimationStarted)
            PlayLayer::resetLevel();

        if (Recorder::get().disabled) return;

        PlayerManager::getShouldRestart() = false;
        
        Recorder::recordResetAction(Recorder::get().totalFrame);
    }

	void levelComplete() {
        bool wasTestMode = m_isTestMode;
        if (PlayerManager::getSpectated()) m_isTestMode = true;
		PlayLayer::levelComplete();
        m_isTestMode = wasTestMode;

        PlayerManager::hideIcons();

		if (!m_levelSettings->m_platformerMode || m_isTestMode || m_isPracticeMode || PlayerManager::getSpectated()) return;

        Recorder& r = Recorder::get();
        r.compareTime = PlayerManager::get().players.empty() ? RecordsManager::getBestCompletion(EditorIDs::getID(PlayLayer::get()->m_level)).info.time : PlayerManager::getTime();
        r.time = 0.f;
        if (!PlayerManager::getIsRacing())
            r.compareTime = RecordsManager::getBestCompletion(EditorIDs::getID(PlayLayer::get()->m_level)).info.time;

        int frame = Recorder::get().totalFrame;
        float time = frame / 240.f;
        r.time = time;
        if (r.compareTime == 0.f) r.compareTime = time * 2;

        if (r.disabled) return;
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
            btn->setPosition({-214, 88});
            menu->addChild(btn);
            return;
        }

        CCNode* menu = this->getChildByID("left-button-menu");
        menu->addChild(btn);
        menu->updateLayout();
    }

    void onEdit(CCObject* obj) {
        PauseLayer::onEdit(obj);
        PlayerManager::get().ui = nullptr;
    }

};

class $modify(EndLevelLayer) {

    void addTime() {
        float time = Recorder::get().time;
        float compareTime = Recorder::get().compareTime;
        if (compareTime == 0.f || time == 0.f || PlayerManager::getSpectated()) return;

        CCLabelBMFont* timeLabel = nullptr;
        if (Loader::get()->isModLoaded("geode.node-ids")) {
            if (CCNode* lbl = m_mainLayer->getChildByID("time-label"))
                timeLabel = static_cast<CCLabelBMFont*>(lbl);
        } else {
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

        float timeDifference = compareTime - time;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(abs(timeDifference) >= 0.01 ? 2 : 3) << abs(timeDifference);
        std::string timeString = oss.str() + "s";
        timeString = (timeDifference < 0 ? "+" : "-") + timeString;

        cocos2d::ccColor3B color = timeDifference < 0 ? ccc3(255, 29, 29) : ccc3(106, 255, 29);
        if (time == compareTime) {
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
    
    void customSetup() {
        EndLevelLayer::customSetup();

        if (!PlayLayer::get()->m_levelSettings->m_platformerMode) return;
        if (Mod::get()->getSettingValue<bool>("no_time_difference")) return;

        Loader::get()->queueInMainThread([this] {
            this->addTime();
        });
    }
    
};
