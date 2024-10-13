#include "RecordsManager/RecordsManager.hpp"
#include "Recorder/Recorder.hpp"
#include "UI/RecordsLayer.hpp"
#include "Player/Player.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>

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

        if (m_levelEndAnimationStarted || !PlayLayer::get()) return;

		if (m_gameState.m_currentProgress <= 1) {
            Recorder::resetState(m_levelSettings->m_platformerMode && !m_isTestMode && !m_isPracticeMode);
            Player::resetState();
            m_fields->totalFrame = 0;
        }

		m_fields->totalFrame++;
    }

	void update(float dt) {
		GJBaseGameLayer::update(dt);

		PlayLayer* pl = PlayLayer::get();
		if (!pl) return;
		if (shouldReturn(this)) return;

        Recorder::handleRecording(PlayLayer::get(), m_fields->totalFrame);
        Player::handlePlaying(this, m_fields->totalFrame);
	}

};

class $modify(PlayerObject) {

    void playDeathEffect() {
        PlayerObject::playDeathEffect();

        PlayLayer* pl = PlayLayer::get();
        if (!pl) return;

        if (shouldReturn(pl)) return;
        if (this != pl->m_player1 && this != pl->m_player2) return;

        int frame = static_cast<GameLayer*>(pl->m_player1->m_gameLayer)->m_fields->totalFrame;
        Recorder::handleEffect(frame, EffectType::Death, this == pl->m_player2);
    }

    void playSpawnEffect() {
        PlayerObject::playSpawnEffect();

        PlayLayer* pl = PlayLayer::get();
        if (!pl) return;

        if (shouldReturn(pl)) return;
        if (this != pl->m_player1 && this != pl->m_player2) return;

        int frame = static_cast<GameLayer*>(pl->m_player1->m_gameLayer)->m_fields->totalFrame;
        Recorder::handleEffect(frame, EffectType::Respawn, this == pl->m_player2);
    }

    void playCompleteEffect(bool b1, bool b2) {
        PlayerObject::playCompleteEffect(b1, b2);

        PlayLayer* pl = PlayLayer::get();
        if (!pl) return;

        if (!pl->m_levelSettings->m_platformerMode || pl->m_isTestMode || pl->m_isPracticeMode) return;
        if (this != pl->m_player1 && this != pl->m_player2) return;

        int frame = static_cast<GameLayer*>(pl->m_player1->m_gameLayer)->m_fields->totalFrame;
        Recorder::handleEffect(frame, EffectType::Complete, this == pl->m_player2);
    }

    void incrementJumps() {
        PlayerObject::incrementJumps();

        PlayLayer* pl = PlayLayer::get();
        if (!pl) return;

        if (shouldReturn(pl)) return;
        if (this != pl->m_player1 && this != pl->m_player2) return;

        if (this == pl->m_player2)
            Recorder::get().jumped2 = true;
        else
            Recorder::get().jumped1 = true;
    }

};

class $modify(PlayLayer) {
	
	void setupHasCompleted() {
		PlayLayer::setupHasCompleted();
        Player::clearActions();
        Player::setup(this);
	}

	void levelComplete() {
		PlayLayer::levelComplete();

		if (!m_levelSettings->m_platformerMode || m_isTestMode || m_isPracticeMode) return;

        auto& fields = static_cast<GameLayer*>(m_player1->m_gameLayer)->m_fields;
		float completionTime = fields->totalFrame / 240.f; 
		int levelId = EditorIDs::getID(m_level);

		RecordsManager::handleCompletion(levelId, completionTime, Recorder::getActions());
	}
	
};

class $modify(PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        PlayLayer* pl = PlayLayer::get();
        if (!pl) return;

        if (!pl->m_levelSettings->m_platformerMode || pl->m_isPracticeMode || pl->m_isTestMode) return;

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
};