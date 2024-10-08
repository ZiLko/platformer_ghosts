#include "RecordsManager/RecordsManager.hpp"
#include "Recorder/Recorder.hpp"
#include "Player/Player.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>

class $modify(GameLayer, GJBaseGameLayer) {

	struct Fields {
		unsigned int totalFrame = 0;
	};

    bool canBeActivatedByPlayer(PlayerObject * p0, EffectGameObject * p1) {
        if (!GJBaseGameLayer::canBeActivatedByPlayer(p0, p1)) return false;

        if (!PlayLayer::get()) return true;
		if (!m_levelSettings->m_platformerMode || m_isTestMode || m_isPracticeMode || m_levelEndAnimationStarted) return true;
        if (p0 != m_player1 && p0 != m_player2) return true;

        int id = p1->m_objectID;
        bool player2 = p0 == m_player2;

        Recorder::handlePortal(id, m_fields->totalFrame, player2, p0);

        return true;
    }

    void processCommands(float dt) {
        GJBaseGameLayer::processCommands(dt);

        if (m_levelEndAnimationStarted) return;

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
		if (!m_levelSettings->m_platformerMode || m_isTestMode || m_isPracticeMode || m_levelEndAnimationStarted) return;

        Recorder::handleRecording(this, m_fields->totalFrame);
        Player::handlePlaying(this, m_fields->totalFrame);
		
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