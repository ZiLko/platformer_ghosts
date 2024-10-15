#include "RecordsManager/RecordsManager.hpp"
#include "Recorder/Recorder.hpp"
#include "UI/RecordsLayer.hpp"
#include "Player/Player.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>

$execute {
    geode::listenForSettingChanges("smoothness", +[](int64_t value) {
        Recorder::get().fps = value;
    });
    geode::listenForSettingChanges("player_disabled", +[](bool value) {
        Player::get().disabled = value;
    });
    geode::listenForSettingChanges("recorder_disabled", +[](bool value) {
        Recorder::get().disabled = value;
    });

    Player::get().disabled = Mod::get()->getSettingValue<bool>("player_disabled");
    Recorder::get().disabled = Mod::get()->getSettingValue<bool>("recorder_disabled");
    Recorder::get().fps = Mod::get()->getSettingValue<int64_t>("smoothness");

};

class $modify(CCKeyboardDispatcher) {

    bool dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool down, bool repeat) {
        if (key == cocos2d::enumKeyCodes::KEY_J && down) {
            RecordsLayer::open();
        }

        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat);
    }

};

bool shouldReturn(PlayLayer* pl) {
    return !pl->m_levelSettings->m_platformerMode || pl->m_isTestMode || pl->m_isPracticeMode || pl->m_levelEndAnimationStarted;
}

bool shouldReturn(GJBaseGameLayer* pl) {
    return !pl->m_levelSettings->m_platformerMode || pl->m_isTestMode || pl->m_isPracticeMode || pl->m_levelEndAnimationStarted;
}

class $modify(GameLayer, GJBaseGameLayer) {

	struct Fields {
		unsigned int totalFrame = 0;
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

		m_fields->totalFrame++;

        if (Player::get().isSpectating && m_levelEndAnimationStarted)
            Player::handleCompletion();

        if (!m_fields->levelComplete && m_levelEndAnimationStarted) {
            m_fields->levelComplete = true;
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
        

		if (shouldReturn(this)) return;

        Recorder::handleRecording(pl, m_fields->totalFrame);
        Player::handlePlaying(this, m_fields->totalFrame);
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

    void playCompleteEffect(bool b1, bool b2) {
        PlayerObject::playCompleteEffect(b1, b2);

        PlayLayer* pl = PlayLayer::get();
        if (!pl) return;

        if (!pl->m_levelSettings->m_platformerMode || pl->m_isTestMode || pl->m_isPracticeMode) return;
        if (this != pl->m_player1 && this != pl->m_player2) return;

        int frame = static_cast<GameLayer*>(m_gameLayer)->m_fields->totalFrame;
        Recorder::handleEffect(frame, EffectType::Complete, this == pl->m_player2);
    }

    void incrementJumps() {
        PlayerObject::incrementJumps();

        if (shouldReturnPlayer()) return;

        if (this == PlayLayer::get()->m_player2)
            Recorder::get().jumped2 = true;
        else
            Recorder::get().jumped1 = true;
    }

};

class $modify(PlayLayer) {
	
	void setupHasCompleted() {
		PlayLayer::setupHasCompleted();
        Player::clear();
        Player::setup(this);
	}

	void levelComplete() {
		PlayLayer::levelComplete();

		if (!m_levelSettings->m_platformerMode || m_isTestMode || m_isPracticeMode || Player::get().isSpectating) return;

        auto& fields = static_cast<GameLayer*>(m_player1->m_gameLayer)->m_fields;
		float completionTime = fields->totalFrame / 240.f; 
		int levelId = EditorIDs::getID(m_level);

		RecordsManager::handleCompletion(levelId, completionTime, Recorder::getActions());
	}

    void resetLevel() {
        PlayLayer::resetLevel();
        static_cast<GameLayer*>(m_player1->m_gameLayer)->m_fields->levelComplete = false;
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