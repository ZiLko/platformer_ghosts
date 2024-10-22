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

bool Player::canUpdatePlayer() {
    Player& p = get();
    if (!p.isSpectating) return true;
    if (!p.updatePlayer) return false;
    p.updatePlayer = false;
    return true;
}

std::vector<Action> Player::getActions() {
    return get().actions;
}

void Player::clear() {
    Player& p = get();
    p.actions.clear();
    p.player1 = nullptr;
    p.player2 = nullptr;
    p.icon1 = nullptr;
    p.icon2 = nullptr;
}

void Player::setup(PlayLayer* pl) {
    Player& p = get();

    for (int i = 1; i < 3; i++) {
        PlayerObject* realPlayer = i == 1 ? pl->m_player1 : pl->m_player2;
		PlayerObject* player = PlayerObject::create(1, 1, pl, pl, true);
        player->retain();
        player->setPosition({ 0, 105 });
		player->setID((("ghost-player"_spr) + std::to_string(i)).c_str());
        player->togglePlatformerMode(true);
        player->setVisible(false);
        pl->m_objectLayer->addChild(player, realPlayer->getZOrder());

        if (i == 1)
            p.player1 = player;
        else
            p.player2 = player;
    }

    p.icon1 = SimplePlayer::create(1);
    p.icon2 = SimplePlayer::create(1);
    p.icon1->setVisible(false);
    p.icon2->setVisible(false);
    p.icon1->setScale(0.5f);
    p.icon2->setScale(0.5f);
    p.icon1->setID("player-icon1"_spr);
    p.icon2->setID("player-icon2"_spr);
    pl->addChild(p.icon1, 1000);
    pl->addChild(p.icon2, 1000);

    SimplePlayer* cube = SimplePlayer::create(1);
    cube->setScale(0.6f);
    cube->setPosition({6, 4});
    cube->setID("cube"_spr);
    p.icon1->addChild(cube);
    cube = SimplePlayer::create(1);
    cube->setScale(0.6f);
    cube->setPosition({6, 4});
    cube->setID("cube"_spr);
    p.icon2->addChild(cube);
    CCSprite* arrow = CCSprite::create("arrow.png"_spr);
    arrow->setID("arrow"_spr);
    p.icon1->addChild(arrow);
    arrow = CCSprite::create("arrow.png"_spr);
    arrow->setID("arrow"_spr);
    p.icon2->addChild(arrow);

    loadBestCompletion();
}

void Player::updateUI() {
    PlayLayer* pl = PlayLayer::get();
    if (!pl) return;
    Player& p = get();

    if (!Mod::get()->getSettingValue<bool>("show_ui") || p.actions.empty() || (!p.isRacing && !!p.isSpectating)) {
        if (!p.uiIcon) return;
        p.uiIcon->removeFromParentAndCleanup(true);
        p.uiTime->removeFromParentAndCleanup(true);
        p.uiName->removeFromParentAndCleanup(true);
        p.uiIcon = nullptr;
        p.uiTime = nullptr;
        p.uiName = nullptr;
        return;
    } else if (!p.uiIcon) {
        cocos2d::CCSize size = pl->getContentSize();
        p.uiIcon = SimplePlayer::create(1);
        p.uiIcon->setScale(0.7f);
        p.uiIcon->setPosition({23, size.height - 23});
        p.uiIcon->setID("ui-icon"_spr);
        pl->addChild(p.uiIcon, 500);

        p.uiTime = CCLabelBMFont::create("1.008s", "goldFont.fnt");
        p.uiTime->setAnchorPoint({0, 0.5f});
        p.uiTime->setPosition({39, size.height - 32});
        p.uiTime->setScale(0.525f);
        p.uiTime->setID("ui-time"_spr);
        pl->addChild(p.uiTime, 500);

        p.uiName = CCLabelBMFont::create("Zilko", "goldFont.fnt");
        p.uiName->setAnchorPoint({0, 0.5f});
        p.uiName->setScale(0.525f);
        p.uiName->setPosition({39, size.height - 14});
        p.uiName->setID("ui-name"_spr);
        pl->addChild(p.uiName, 500);
    }

    PlayerColors colors = p.info.colors;
    p.uiIcon->updatePlayerFrame(p.info.icons.at(VehicleType::Cube), IconType::Cube);
    setPlayerIconColors(p.uiIcon, false, colors.color1, colors.color2, colors.glowColor, colors.glowEnabled);
    p.uiTime->setString(RecordsManager::getFormattedTime(p.info.time).c_str());
    p.uiName->setString(p.info.username.c_str());
}

void Player::loadBestCompletion() {
    Player& p = get();
    Replay bestCompletion = RecordsManager::getBestCompletion(EditorIDs::getID(PlayLayer::get()->m_level));
    if (!bestCompletion.actions.empty()) {
        p.currentRace = 1;
        p.isRacing = true;
        p.loadReplay(bestCompletion);        
    }
}

void Player::updateDisabled() {
    Player& p = get();
    p.disabled = Mod::get()->getSettingValue<bool>("player_disabled");
    if (!PlayLayer::get()) return;
    if (p.disabled) {
        if (p.player1) p.player1->setVisible(false);
        if (p.player2) p.player2->setVisible(false);
    }
    else if (!p.actions.empty())
        loadGhost({ p.info, p.actions }, p.currentRace);
}

void Player::updateOpacity(bool player2) {
    Player& p = get();
    PlayerObject* player = player2 ? p.player2 : p.player1;
    if (!player) return;

    int opacity = static_cast<int>(Mod::get()->getSettingValue<int64_t>(player2 ? "p2_opacity" : "p1_opacity") / 100.f * 255);
    player->setOpacity(opacity);
    player->m_spiderSprite->GJSpiderSprite::setOpacity(opacity);
    player->m_robotSprite->GJRobotSprite::setOpacity(opacity);
}

void Player::updateColors() {
    PlayerColors colors;
    Player& p = get();

    if (!Mod::get()->getSettingValue<bool>("no_colors"))
        colors = p.info.colors;

    setPlayerColors(p.player1, false, colors.color1, colors.color2, colors.glowColor, colors.glowEnabled);
    setPlayerColors(p.player2, true, colors.color1, colors.color2, colors.glowColor, colors.glowEnabled);
    setPlayerIconColors(p.icon1, false, colors.color1, colors.color2, colors.glowColor, colors.glowEnabled);
    setPlayerIconColors(p.icon2, true, colors.color1, colors.color2, colors.glowColor, colors.glowEnabled);

    if (CCNode* cube = p.icon1->getChildByID("cube"_spr))
        setPlayerIconColors(static_cast<SimplePlayer*>(cube), false, colors.color1, colors.color2, colors.glowColor, colors.glowEnabled);
    if (CCNode* cube = p.icon2->getChildByID("cube"_spr))
        setPlayerIconColors(static_cast<SimplePlayer*>(cube), true, colors.color1, colors.color2, colors.glowColor, colors.glowEnabled);
}

void Player::updateCamera() {
    if (!PlayLayer::get()) return;
    Player& p = get();
    if (!p.icon1 || !p.icon2) return;

    bool enabled = Mod::get()->getSettingValue<bool>("off_screen_indicators");
    if (p.actions.empty() || p.ghostCompletedLevel || !enabled) {
        p.icon1->setVisible(false);
        p.icon2->setVisible(false);
        return;
    }

    for (std::pair<PlayerObject*, SimplePlayer*> pair : {std::make_pair(p.player1, p.icon1), std::make_pair(p.player2, p.icon2)}) {
        if (pair.first == p.player2 && !p.isDual) {
            pair.second->setVisible(false);
            continue;
        }
        cocos2d::CCPoint pos = pair.first->getPosition();
        std::pair<bool, bool> visible = isInsideCamera(pos, 1.f);
        pair.second->setVisible(!visible.first);
        if (!visible.first) {
            PlayerData data = getBorderPosition(pos, visible.second);
            pair.second->setPosition(data.position);
            pair.second->getChildByID("arrow"_spr)->setRotation(data.rotation);
        }
    }
}

PlayerData Player::getBorderPosition(cocos2d::CCPoint pos, bool isX) {
    PlayLayer* pl = PlayLayer::get();
    cocos2d::CCSize offset = ccp(22, 22);
    cocos2d::CCSize size = (pl->getContentSize() - (offset * 2)) / pl->m_gameState.m_cameraZoom;
    cocos2d::CCPoint cam = pl->m_gameState.m_cameraPosition;
    cocos2d::CCPoint center = cam + size / 2;
    cocos2d::CCPoint center2 = size / 2;

    float a = abs(abs(center.x) - abs(pos.x));
    float b = abs(abs(center.y) - abs(pos.y));
    float m = b / a;
    float x = pos.x > cam.x ? size.width : 0;
    float y = m * (center.x - cam.x);
    y = center2.y + y * (pos.y > center.y ? 1 : -1);
    
    if (!isX || (isX && (y > size.height || y < 0))) {
        m = a / b;
        x = m * (center.y - cam.y);
        x = center2.x + x * (pos.x > center.x ? 1 : -1);
        y = pos.y > cam.y ? size.height : 0;
    }

    float rotation = -(std::atan2(pos.y - center.y, pos.x - center.x) * (180.0 / M_PI) - 90.f);
    return {ccp(x , y) * pl->m_gameState.m_cameraZoom + offset, rotation, false};
}

std::pair<bool, bool> Player::isInsideCamera(cocos2d::CCPoint pos, float scale) {
    PlayLayer* pl = PlayLayer::get();
    cocos2d::CCSize win = pl->getContentSize() / pl->m_gameState.m_cameraZoom;
    cocos2d::CCPoint cam = pl->m_gameState.m_cameraPosition;
    cocos2d::CCPoint posLeft = pos + ccp(32.5f, 32.5f) * scale;
    cocos2d::CCPoint posRight = pos - ccp(32.5f, 32.5f) * scale;
    bool isInside = posLeft > cam && posRight < cam + win;
    bool isX = posLeft.x < cam.x || posRight.x > cam.x + win.width;
    return std::make_pair(isInside, isX);
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
    p.ghostCompletedLevel = false;
    p.spectatorInput = false;
    p.updatePlayer = false;
    p.spectated = false;

    if (p.actions.empty()) return;
    if (!p.player1 || !p.player2) return;

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
    icons = replay.info.icons;
    PlayerColors colors = replay.info.colors;
    info = replay.info;
    updateColors();
    updateUI();
}

void Player::loadGhost(Replay replay, int selected) {
    Player& p = get();

    p.currentAction = 0;
    p.currentRace = selected;
    p.isRacing = true;

    if (p.player1) p.player1->setVisible(true);

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

            p.updatePlayer = true;
            p.spectated = true;
        }

        p.currentAction++;
	}
}

void Player::handlePlaying(GJBaseGameLayer* bgl, int frame) {
    Player& p = get();
    p.currentFrame = frame;

    if (!p.isRacing && !p.isSpectating) return updateCamera();
    if (p.actions.empty() || p.disabled) return updateCamera();

    if (p.isSpectating && frame == static_cast<int>(p.info.time * 240.f)) {
        std::pair<cocos2d::CCPoint, cocos2d::CCPoint> pos = p.getLatestPositions();
        PlayLayer::get()->m_player1->setPosition(pos.first);
        PlayLayer::get()->m_player2->setPosition(pos.second);
        stopSpectating();
    }

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

    updateCamera();
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

void Player::stopRacing() {
    Player& p = get();
    Player::resetState();
    if (p.player1) p.player1->setVisible(false);
    p.actions.clear();
    p.isRacing = false;
    p.currentRace = 0;
    updateUI();
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

    if (!p.player1 || !p.player2) return;

    p.player1->setVisible(false);
    p.player2->setVisible(false);
    PlayLayer::get()->m_player1->setVisible(true);
    PlayLayer::get()->m_player1->releaseAllButtons();
    PlayLayer::get()->m_player2->releaseAllButtons();
    updateUI();
}

void Player::updateUpsideDownState() {
    Player& p = get();
    if (!p.player1 || !p.player2) return;

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

void Player::setPlayerIconColors(SimplePlayer* icon, bool player2, int color1ID, int color2ID, int glowColorID, bool glowEnabled) {
    GameManager* gm = GameManager::get();
    cocos2d::ccColor3B color1 = gm->colorForIdx(color1ID);
    cocos2d::ccColor3B color2 = gm->colorForIdx(color2ID);
    if (player2) {
        cocos2d::ccColor3B tempColor = color1;
        color1 = color2;
        color2 = tempColor;
    }
    icon->setColor(color1);
    icon->setSecondColor(color2);
    icon->m_hasGlowOutline = glowEnabled;
    if (icon->m_hasGlowOutline)
        icon->enableCustomGlowColor(gm->colorForIdx(glowColorID));
    else
        icon->disableCustomGlowColor();
    icon->updateColors();
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

    IconType iconType = IconType::Cube;
    SimplePlayer* icon = player == get().player2 ? get().icon2 : get().icon1;
    if (!icon) return;
    switch (vehicle) {
        case VehicleType::Ship: iconType = IconType::Jetpack; break;
        case VehicleType::Ball: iconType = IconType::Ball; break;
        case VehicleType::Ufo: iconType = IconType::Ufo; break;
        case VehicleType::Wave: iconType = IconType::Wave; break;
        case VehicleType::Robot: iconType = IconType::Robot; break;
        case VehicleType::Spider: iconType = IconType::Spider; break;
        case VehicleType::Swing: iconType = IconType::Swing; break;
    }

    if (CCNode* cube = icon->getChildByID("cube"_spr)) {
        static_cast<SimplePlayer*>(cube)->setVisible(iconType == IconType::Jetpack);
        static_cast<SimplePlayer*>(cube)->setOpacity(220);
        static_cast<SimplePlayer*>(cube)->updatePlayerFrame(get().icons.at(VehicleType::Cube), IconType::Cube);
    }

    icon->updatePlayerFrame(id, iconType);
    if (iconType == IconType::Robot)
        icon->m_robotSprite->GJRobotSprite::setOpacity(220);
    else if (iconType == IconType::Spider)
        icon->m_spiderSprite->GJSpiderSprite::setOpacity(220);
    else
        icon->setOpacity(220);

    updateOpacity(player == get().player2);
}

void Player::setPlayerScale(PlayerObject* player, float scale, float x, float y) {
    int negX = x < 0 ? -1 : 1;
    int negY = y < 0 ? -1 : 1;

    player->setScaleX(scale * negX);
    player->setScaleY(scale * negY);
}

void Player::playCompleteEffect() {
    Player& p = get();
    
    if (!p.player1 || !p.player2) return;

    if (p.player1->isVisible()) p.player1->playCompleteEffect(false, false);
    if (p.player2->isVisible()) p.player2->playCompleteEffect(false, false);
    p.player1->setVisible(false);
    p.player2->setVisible(false);
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
        case EffectType::Respawn: Player::playSpawnEffect(player); break;
        case EffectType::Complete: {
            if (player->isVisible()) player->playCompleteEffect(false, false); 

            Player& p = get();
            p.ghostCompletedLevel = true;
            if (p.icon1) p.icon1->setVisible(false);
            if (p.icon2) p.icon2->setVisible(false);

            break;
        }
    }
}

void Player::playSpawnEffect(PlayerObject* player) {
    updateOpacity(false);
    updateOpacity(true);
    player->setVisible(true);
    player->stopAllActions();
    cocos2d::CCBlink* flashAction = cocos2d::CCBlink::create(0.4f, 4);
    player->runAction(flashAction);
}   

void Player::handleInputAction(Action action) {
    if (!get().isSpectating) return;

    PlayLayer* pl = PlayLayer::get();
    if (!pl) return;

    InputData data = std::get<InputData>(action.data);
    PlayerObject* player = data.player2 ? pl->m_player2 : pl->m_player1;
    PlayerButton btn = static_cast<PlayerButton>(data.button);

    data.down ? player->pushButton(btn) : player->releaseButton(btn);
    get().updatePlayer = true;
}