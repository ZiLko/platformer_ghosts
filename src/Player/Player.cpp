#include "../RecordsManager/RecordsManager.hpp"
#include "Player.hpp"

Player& Player::get() {
    static Player instance;
    return instance;
}

bool Player::canClick() {
    Player& p = get();
    if (!p.isSpectating) return true;
    if (p.spectatorInput) return true;
    return false;
}

std::vector<Action> Player::getActions() {
    return get().actions;
}

void Player::clear() {
    Player& p = get();
    p.actions.clear();
}

void Player::setup(PlayLayer* pl) {
    for (int i = 1; i < 3; i++) {
        PlayerObject* realPlayer = i == 1 ? pl->m_player1 : pl->m_player2;
		PlayerObject* player = PlayerObject::create(1, 1, pl, pl, true);
        player->retain();
        player->setPosition({ 0, 105 });
		player->setID((("ghost-player"_spr) + std::to_string(i)).c_str());
        player->togglePlatformerMode(true);
        pl->m_objectLayer->addChild(player, realPlayer->getZOrder());

        if (i == 1)
            get().player1 = player;
        else
            get().player2 = player;
    }

    get().p1Visible = pl->m_player1->isVisible();

    Replay bestCompletion = RecordsManager::getBestCompletion(EditorIDs::getID(pl->m_level));

    if (!bestCompletion.actions.empty()) {
        Player& p = get();
        p.currentRace = 1;
        p.isRacing = true;
        p.loadReplay(bestCompletion);        
    }
}

void Player::resetState() {
    Player& p = get();

    p.currentAction = 0;
    p.currentFrame = 0;
    p.upsideDown1 = false;
    p.upsideDown2 = false;
    p.isDual = false;
    p.isMini1 = false;
    p.isMini2 = false;
    p.currentVehicle1 = VehicleType::Cube;
    p.currentVehicle2 = VehicleType::Cube;
    p.rotationOffset1 = 0.f;
    p.rotationOffset2 = 0.f;
    p.lastRotation1 = 0.f;
    p.lastRotation2 = 0.f;
    p.completedLevel = false;
    p.spectatorInput = false;

    if (p.actions.empty()) return;

    setPlayerSprite(p.player1, VehicleType::Cube);
    setPlayerSprite(p.player2, VehicleType::Cube);
    updateUpsideDownState();

    p.player1->setScaleX(1.f);
    p.player1->setScaleY(1.f);
    p.player2->setScaleX(1.f);
    p.player2->setScaleY(1.f);

    p.player1->setPosition({0, 105});
    p.player2->setPosition({0, 105});
    p.player1->setVisible(true);
    p.player2->setVisible(false);

    if (p.isSpectating) {
        PlayLayer::get()->m_player1->releaseAllButtons();
        PlayLayer::get()->m_player2->releaseAllButtons();
    }
}

void Player::loadReplay(Replay replay) {
    actions = replay.actions;
    icons = replay.icons;
    PlayerColors colors = replay.colors;
    setPlayerColors(player1, false, colors.color1, colors.color2, colors.glowColor, colors.glowEnabled);
    setPlayerColors(player2, true, colors.color1, colors.color2, colors.glowColor, colors.glowEnabled);
}

void Player::loadGhost(Replay replay, int selected) {
    Player& p = get();

    p.currentAction = 0;
    p.currentRace = selected;
    p.isRacing = true;

    p.loadReplay(replay);
    resetState();
    handleActions();
}

std::pair<cocos2d::CCPoint, cocos2d::CCPoint> Player::getLatestPositions() {
    cocos2d::CCPoint pos1 = ccp(0, 0);
    cocos2d::CCPoint pos2 = ccp(0, 0);

    for (int i = actions.size() - 1; i >= 0; i--) {
        Action action = actions[i];
        if (action.type != ActionType::Position) continue;
            
        PositionData data = std::get<PositionData>(action.data);

        if (pos1.x == 0.f && data.p1.position.x != 0.f)
            pos1.x = data.p1.position.x;
        
        if (pos1.y == 0.f && data.p1.position.y != 0.f)
            pos1.y = data.p1.position.y;

        if (pos2.x == 0.f && data.p1.position.x != 0.f)
            pos2.x = data.p1.position.x;
        
        if (pos2.y == 0.f && data.p2.position.y != 0.f)
            pos2.y = data.p2.position.y;

        if (pos1 != ccp(0, 0) && pos2 != ccp(0, 0)) break;
    }

    return std::make_pair(pos1, pos2);
}

void Player::handleActions() {
    Player& p = get();

    if (!p.player1 || !p.player2) return;

    while (p.currentAction < p.actions.size() && p.actions[p.currentAction].frame <= p.currentFrame) {
        Action action = p.actions[p.currentAction];

        switch (action.type) {
            case ActionType::Position: p.handlePositionAction(action); break;
            case ActionType::Sideways: p.handleSidewaysAction(action); break;
            case ActionType::Vehicle: p.handleVehicleAction(action); break;
            case ActionType::Dual: p.handleDualAction(action); break;
            case ActionType::Flip: p.handleFlipAction(action); break;
            case ActionType::Mini: p.handleMiniAction(action); break;
            case ActionType::Animation: p.handleAnimationAction(action); break;
            case ActionType::Effect: p.handleEffectAction(action); break;
            case ActionType::Input: p.handleInputAction(action); break;
        }
			
        if (action.type != ActionType::Position)
            updateUpsideDownState();
        else if (p.isSpectating) {
            PlayLayer* pl = PlayLayer::get();
            pl->m_player1->setPosition(p.player1->getPosition());
            if (pl->m_gameState.m_isDualMode)
                pl->m_player2->setPosition(p.player2->getPosition());
        }

        p.currentAction++;
	}

    if (p.currentAction >= p.actions.size() && p.isSpectating) {
        std::pair<cocos2d::CCPoint, cocos2d::CCPoint> pos = p.getLatestPositions();
        PlayLayer::get()->m_player1->setPosition(pos.first);
        PlayLayer::get()->m_player2->setPosition(pos.second);
        stopSpectating();
    }
}

void Player::handlePlaying(GJBaseGameLayer* bgl, int frame) {
    Player& p = get();
    p.currentFrame = frame;

    if (!p.isRacing && !p.isSpectating) return;
    if (p.actions.empty()) return;

    if (!p.player1 || !p.player2) {
		p.player1 = static_cast<PlayerObject*>(bgl->m_objectLayer->getChildByID("ghost-player1"_spr));
		p.player2 = static_cast<PlayerObject*>(bgl->m_objectLayer->getChildByID("ghost-player2"_spr));
    }

	handleActions();

    if (p.isSpectating) {
        PlayLayer* pl = PlayLayer::get();
        pl->m_player1->setVisible(false);
        pl->m_player2->setVisible(false);
    }
}

void Player::startSpectating(Replay replay, int spectate) {
    Player& p = get();
    p.currentAction = 0;
    p.currentSpectate = spectate;
    p.currentRace = 0;
    p.isRacing = false;
    p.isSpectating = true;
    p.shouldRestart = true;
    p.loadReplay(replay);
}

void Player::stopSpectating() {
    Player& p = get();
    p.actions.clear();
    p.currentAction = 0;
    p.currentSpectate = 0;
    p.currentRace = 0;
    p.spectatorInput = false;
    p.isRacing = false;
    p.isSpectating = false;
    p.shouldRestart = true;
    PlayLayer::get()->m_player1->setVisible(true);
    PlayLayer::get()->m_player1->releaseAllButtons();
    PlayLayer::get()->m_player2->releaseAllButtons();
}

void Player::updateUpsideDownState() {
    Player& p = get();

    if (p.upsideDown1 && !nonInvertedVehicles.contains(p.currentVehicle1))
        p.player1->setScaleY(-1 * abs(p.player1->getScaleY()));
    else
        p.player1->setScaleY(abs(p.player1->getScaleY()));

    if (p.upsideDown2 && !nonInvertedVehicles.contains(p.currentVehicle2))
        p.player2->setScaleY(-1 * abs(p.player2->getScaleY()));
    else
        p.player2->setScaleY(abs(p.player2->getScaleY()));
}

void Player::setPlayerColors(PlayerObject* player, bool player2, int color1ID, int color2ID, int glowColorID, bool glowEnabled) {
    GameManager* gm = GameManager::get();

    cocos2d::ccColor3B color1 = gm->colorForIdx(color1ID);
    cocos2d::ccColor3B color2 = gm->colorForIdx(color2ID);

    if (player2) {
        cocos2d::ccColor3B tempColor = color1;
        color1 = color2;
        color2 = tempColor;
    }
    
    player->setColor(color1);
    player->setSecondColor(color2);
    player->m_hasGlow = glowEnabled;

    if (glowEnabled)
        player->enableCustomGlowColor(gm->colorForIdx(glowColorID));
    else
        player->disableCustomGlowColor();

    player->updateGlowColor();
    player->updatePlayerGlow();
}

void Player::setPlayerSprite(PlayerObject* player, VehicleType vehicle) {
    if (getActions().empty()) return;

    int id = get().icons.at(vehicle);

    switch (vehicle) {
        case VehicleType::Cube: 
            player->toggleFlyMode(false, false),
            player->toggleRollMode(false, false),
            player->toggleBirdMode(false, false),
            player->toggleDartMode(false, false),
            player->toggleRobotMode(false, false),
            player->toggleSpiderMode(false, false),
            player->toggleSwingMode(false, false),
            player->updatePlayerFrame(id);
            break;
        case VehicleType::Ship: player->toggleFlyMode(true, false), player->updatePlayerJetpackFrame(id); break;
        case VehicleType::Ball: player->toggleRollMode(true, false), player->updatePlayerRollFrame(id); break;
        case VehicleType::Ufo: player->toggleBirdMode(true, false), player->updatePlayerBirdFrame(id); break;
        case VehicleType::Wave: player->toggleDartMode(true, false), player->updatePlayerDartFrame(id); break;
        case VehicleType::Robot: player->toggleRobotMode(true, false), player->updatePlayerRobotFrame(id); break;
        case VehicleType::Spider: player->toggleSpiderMode(true, false), player->updatePlayerSpiderFrame(id); break;
        case VehicleType::Swing: player->toggleSwingMode(true, false), player->updatePlayerSwingFrame(id); break;
    }

    int opacity = static_cast<int>(player == get().player1 ? Mod::get()->getSettingValue<int64_t>("p1_opacity") / 100.f * 255
    : Mod::get()->getSettingValue<int64_t>("p1_opacity") / 100.f * 255);

    player->setOpacity(opacity);
    player->m_spiderSprite->GJRobotSprite::setOpacity(opacity);
    player->m_robotSprite->GJRobotSprite::setOpacity(opacity);
}

void Player::setPlayerScale(PlayerObject* player, float scale, float x, float y) {
    int negX = x < 0 ? -1 : 1;
    int negY = y < 0 ? -1 : 1;

    player->setScaleX(scale * negX);
    player->setScaleY(scale * negY);
}

void Player::playCompleteEffect() {
    Player& p = get();
    if (p.player1->isVisible()) p.player1->playCompleteEffect(false, false);
    if (p.player2->isVisible()) p.player2->playCompleteEffect(false, false);
}

void Player::handleCompletion() {
    Player& p = get();

    if (p.completedLevel) return;
    p.completedLevel = true;

    playCompleteEffect();
    
    if (p.isSpectating) {
        PlayLayer* pl = PlayLayer::get();
        pl->m_player1->setVisible(true);
        pl->m_player2->setVisible(pl->m_gameState.m_isDualMode);
    }
}

void Player::handlePositionAction(Action action) {
    PositionData pos = std::get<PositionData>(action.data);

    if (pos.p1.position.x != 0.f)
    	player1->setPositionX(pos.p1.position.x);
    if (pos.p1.position.y != 0.f)
    	player1->setPositionY(pos.p1.position.y);
    if (pos.p1.rotation != 0.f || pos.p1.rotationZero) {
	    player1->setRotation(pos.p1.rotation + rotationOffset1);
        lastRotation1 = pos.p1.rotation;
    } else
		player1->setRotation(lastRotation1 + rotationOffset1);

    if (isDual) {
        if (pos.p2.position.x != 0.f)
            player2->setPositionX(pos.p2.position.x);
        if (pos.p2.position.y != 0.f)
            player2->setPositionY(pos.p2.position.y);
        if (pos.p2.rotation != 0.f || pos.p2.rotationZero){ 
		    player2->setRotation(pos.p2.rotation + rotationOffset2);
            lastRotation2 = pos.p2.rotation;
        } else
			player2->setRotation(lastRotation2 + rotationOffset2);
    }
}

void Player::handleVehicleAction(Action action) {
    VehicleData data = std::get<VehicleData>(action.data);
    PlayerObject* player = data.player2 ? player2 : player1;
    float scale = player->getScaleX();

    setPlayerSprite(player, data.vehicle);
    player->setScaleX(scale);

    if (data.player2)
        currentVehicle2 = data.vehicle;
    else 
        currentVehicle1 = data.vehicle;

}

void Player::handleDualAction(Action action) {
    bool data = std::get<bool>(action.data);

    if (data == isDual && data) return;

    isDual = data;
    player2->setVisible(isDual);

    if (isDual) {
        setPlayerSprite(player2, currentVehicle1);
        setPlayerScale(player2, isMini1 ? 0.6f : 1.f, player2->getScaleX(), player2->getScaleY());
        currentVehicle2 = currentVehicle1;
        isMini2 = isMini1;
    }
}

void Player::handleFlipAction(Action action) {
    FlipData data = std::get<FlipData>(action.data);

    if (data.y) {
        if (data.player2)
            upsideDown2 = data.flip;
        else
            upsideDown1 = data.flip;

        updateUpsideDownState();
        return;
    }

    PlayerObject* player = data.player2 ? player2 : player1;
    float rot = data.player2 ? rotationOffset2 : rotationOffset1;
    int neg = data.flip ? -1 : 1;

    if (rot != 0.f)
        neg *= -1;

    player->setScaleX(abs(player->getScaleX()) * neg);

}

void Player::handleMiniAction(Action action) {
    MiniData data = std::get<MiniData>(action.data);
    PlayerObject* player = data.player2 ? player2 : player1;
    bool mini = data.mini;

    if (data.player2)
        isMini2 = mini;
    else
        isMini1 = mini;

    setPlayerScale(player, data.mini ? 0.6f : 1.f, player->getScaleX(), player->getScaleY());
}

void Player::handleSidewaysAction(Action action) {
    SidewaysData data = std::get<SidewaysData>(action.data);
    if (data.player2) {
        rotationOffset2 = data.rotation;
        player2->setScaleX(player2->getScaleX() * -1);
    } else {
        rotationOffset1 = data.rotation;
        player1->setScaleX(player1->getScaleX() * -1);
    }
}

void Player::handleAnimationAction(Action action) {
    AnimationData data = std::get<AnimationData>(action.data);
    PlayerObject* player = data.player2 ? player2 : player1;

    if (data.robot)
        player->m_robotSprite->tweenToAnimation(animationStrings.at(data.animation).c_str(), 0.1f);
    else
        player->m_spiderSprite->tweenToAnimation(animationStrings.at(data.animation).c_str(), 0.1f);
}

void Player::handleEffectAction(Action action) {
    EffectData data = std::get<EffectData>(action.data);
    PlayerObject* player = data.player2 ? player2 : player1;
    switch(data.effect) {
        case EffectType::Death: player->playDeathEffect(); break;
        case EffectType::Respawn: player->playSpawnEffect(); player->setOpacity(110); break;
        case EffectType::Complete: 
            if (!player->isVisible()) break;
            player->playCompleteEffect(false, false); 
            break;
    }
}

void Player::handleInputAction(Action action) {
    if (!get().isSpectating) return;

    PlayLayer* pl = PlayLayer::get();
    if (!pl) return;

    InputData data = std::get<InputData>(action.data);
    PlayerObject* player = data.player2 ? pl->m_player2 : pl->m_player1;
    PlayerButton btn = static_cast<PlayerButton>(data.button);

    data.down ? player->pushButton(btn) : player->releaseButton(btn);
}