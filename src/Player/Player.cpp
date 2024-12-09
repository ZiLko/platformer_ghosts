#include "../RecordsManager/RecordsManager.hpp"
#include "Player.hpp"

PlayerManager& PlayerManager::get() {
    static PlayerManager instance;
    return instance;
}

bool PlayerManager::containsTime(float time) {
    return get().times.contains(time);
}

void PlayerManager::hideIcons() {
    for (Player& player : get().players)
        player.hideIcons();
}

void PlayerManager::resetButtons() {
    PlayLayer* pl = PlayLayer::get();
    if (!pl) return;

    handleButton(pl, PlayerButton::Jump, pl->m_player1, false);
    handleButton(pl, PlayerButton::Left, pl->m_player1, false);
    handleButton(pl, PlayerButton::Right, pl->m_player1, false);

    handleButton(pl, PlayerButton::Jump, pl->m_player2, false);
    handleButton(pl, PlayerButton::Left, pl->m_player2, false);
    handleButton(pl, PlayerButton::Right, pl->m_player2, false);
}

void PlayerManager::handleButton(PlayLayer* pl, PlayerButton btn, PlayerObject* player, bool down) {
    if (!PlayerManager::getIsSpectating()) return;
    if (!pl) return;
    if (!pl->m_uiLayer) return;
    if (player != pl->m_player1 && player != pl->m_player2) return;

    bool player2 = player == pl->m_player2;
    log::debug("{} {} {}", getCurrentFrame(), static_cast<int>(btn), player2);
    int index = btn == PlayerButton::Jump ? 0 : (static_cast<int>(btn) - 1);
    CCNode* uiNode = pl->m_uiLayer->getChildByType<GJUINode>((btn != PlayerButton::Jump ? 0 : 2) + static_cast<int>(player2));

    if (!uiNode) return;
    if (CCSprite* spr = uiNode->getChildByType<CCSprite>(index)) {
        int col = down ? 150 : 255;
        spr->setColor(ccc3(col, col, col));
    }

}

void PlayerManager::addPlayer(Replay replay, PlayLayer* pl) {
    if (!pl) return;

    Player player;
    player.setup(pl);
    player.loadReplay(replay);
    get().players.push_back(player);
    get().times[replay.info.time] = player;
    get().updateUI();
};

void PlayerManager::removePlayer(Player player) {
    for (int i = 0; i < get().players.size(); i++) {
        Player& p = get().players[i];
        if (get().players[i] == player) {
            if (p.isSpectating) {
                p.stopSpectating();
                if (p.player1) p.player1->removeFromParentAndCleanup(true);
                if (p.player2) p.player2->removeFromParentAndCleanup(true);
                if (p.icon1) p.icon1->removeFromParentAndCleanup(true);
                if (p.icon2) p.icon2->removeFromParentAndCleanup(true);
            } else
                p.stopRacing();
            get().players.erase(get().players.begin() + i);
            get().times.erase(player.info.time);
            break;
        }
    }
    get().updateUI();
}

void PlayerManager::resetState() {
    for (Player& player : get().players)
        player.resetState();
}

void PlayerManager::handleCompletion() {
    for (Player& player : get().players)
        player.handleCompletion();
}

void PlayerManager::playCompleteEffect() {
    for (Player& player : get().players)
        player.playCompleteEffect();
}

void PlayerManager::updateOpacity(bool p2) {
    for (Player& player : get().players)
        player.updateOpacity(p2);
}

void PlayerManager::updateDisabled() {
    getDisabled() = Mod::get()->getSettingValue<bool>("player_disabled");
    for (Player& player : get().players)
        player.updateDisabled();
    get().updateUI();
}

void PlayerManager::updateCamera() {
    for (Player& player : get().players)
        player.updateCamera();
}

void PlayerManager::updateColors() {
    for (Player& player : get().players)
        player.updateColors();
}

void PlayerManager::handlePlaying(GJBaseGameLayer* bgl, int frame) {
    PlayerManager::getCurrentFrame() = frame;
    for (Player& player : get().players)
        player.handlePlaying(bgl, frame);
}

bool PlayerManager::canUpdatePlayer() {
    if (!getIsSpectating()) return true;
    if (!getUpdatePlayer()) return false;
    getUpdatePlayer() = false;
    return true;
}

bool PlayerManager::canClick() {
    if (!getIsSpectating()) return true;
    return PlayerManager::getSpectatorInput();
}

void PlayerManager::setup(PlayLayer* pl) {
    currentFrame = 0;
    currentRace = 0;
    currentSpectate = 0;
    PlayerManager::getSpectated() = false;
    isSpectating = false;
    isRacing = false;
    spectatorInput = false;
    updatePlayer = false;
    shouldRestart = false;
    canReset = false;
    disabled = false;
    ui = nullptr;
    players.clear();
    times.clear();

    std::vector<std::pair<ReplayInfo, std::filesystem::path>> completions;
    completions = RecordsManager::getLevelCompletions(EditorIDs::getID(pl->m_level));
    int i = 0;

    for (std::pair<ReplayInfo, std::filesystem::path> completion : completions) {
        if (!Mod::get()->getSettingValue<bool>("load_all_ghosts")) {
            i++;
            if (i > Mod::get()->getSettingValue<int64_t>("load_ghosts")) break;
        }
        Replay replay = {
            completion.first,
            RecordsManager::getCompletionActions(completion.second)
        };
        addPlayer(replay, pl);
    }

    updateDisabled();
}

void PlayerManager::clear() {
    for (Player& player : get().players)
        player.clear();
}

void PlayerManager::stopSpectating() {
    Replay replay;
    for (Player& player : get().players) {
        if (!player.isSpectating) continue;
        replay = {player.info, player.actions};
        player.stopSpectating();
        break;
    }
    get().spectated = true;
    get().isSpectating = false;
    get().spectatorInput = false;
    if (PlayerManager::containsTime(replay.info.time))
        PlayerManager::removePlayer(PlayerManager::get().times.at(replay.info.time));
    PlayerManager::addPlayer(replay, PlayLayer::get());
}

void PlayerManager::updateUI() {
    if (!ui) {
        PlayLayer* pl = PlayLayer::get();
        if (!pl) return;

        ui = GhostUI::create();
        ui->setPosition({0, pl->getContentSize().height});
        pl->addChild(ui);
    }

    if (!ui) return;
    if (!PlayLayer::get()) {
        ui = nullptr;
        return;
    }

    ui->update();
}

std::vector<Action> Player::getActions() {
    return actions;
}

void Player::clear() {
    actions.clear();
    player1 = nullptr;
    player2 = nullptr;
    icon1 = nullptr;
    icon2 = nullptr;
}

void Player::setup(PlayLayer* pl) {
    for (int i = 1; i < 3; i++) {
        PlayerObject* realPlayer = i == 1 ? pl->m_player1 : pl->m_player2;
		PlayerObject* player = PlayerObject::create(1, 1, pl, pl, true);
        player->setPosition({ 0, 105 });
		player->setID((("ghost-player"_spr) + std::to_string(i)).c_str());
        player->togglePlatformerMode(true);
        player->setVisible(false);
        pl->m_objectLayer->addChild(player, realPlayer->getZOrder());

        if (i == 1)
            player1 = player;
        else
            player2 = player;
    }

    icon1 = SimplePlayer::create(1);
    icon2 = SimplePlayer::create(1);
    icon1->setVisible(false);
    icon2->setVisible(false);
    icon1->setScale(0.5f);
    icon2->setScale(0.5f);
    icon1->setID("player-icon1"_spr);
    icon2->setID("player-icon2"_spr);
    pl->addChild(icon1, 1000);
    pl->addChild(icon2, 1000);

    SimplePlayer* cube = SimplePlayer::create(1);
    cube->setScale(0.6f);
    cube->setPosition({6, 4});
    cube->setID("cube"_spr);
    icon1->addChild(cube);
    cube = SimplePlayer::create(1);
    cube->setScale(0.6f);
    cube->setPosition({6, 4});
    cube->setID("cube"_spr);
    icon2->addChild(cube);
    CCSprite* arrow = CCSprite::create("arrow.png"_spr);
    arrow->setID("arrow"_spr);
    icon1->addChild(arrow);
    arrow = CCSprite::create("arrow.png"_spr);
    arrow->setID("arrow"_spr);
    icon2->addChild(arrow);
}

void Player::hideIcons() {
    if (icon1) icon1->setVisible(false);
    if (icon2) icon2->setVisible(false);
}

void Player::loadBestCompletion() {
    Replay bestCompletion = RecordsManager::getBestCompletion(EditorIDs::getID(PlayLayer::get()->m_level));
    if (!bestCompletion.actions.empty()) {
        PlayerManager::getCurrentRace() = 1;
        isRacing = true;
        loadReplay(bestCompletion);      
    }
}

void Player::updateDisabled() {
    if (!PlayLayer::get()) return;

    if (PlayerManager::getDisabled()) {
        if (player1) player1->setVisible(false);
        if (player2) player2->setVisible(false);
        if (icon1) icon1->setVisible(false);
        if (icon2) icon2->setVisible(false);
    }
    else if (!actions.empty())
        loadReplay({ info, actions });
}

void Player::updateOpacity(bool isPlayer2) {
    PlayerObject* player = isPlayer2 ? player2 : player1;
    if (!player) return;

    int opacity = 255;
    if (!Mod::get()->getSettingValue<bool>("spectator_opacity") || !isSpectating)
        opacity = static_cast<int>(Mod::get()->getSettingValue<int64_t>(isPlayer2 ? "p2_opacity" : "p1_opacity") / 100.f * 255);

    player->setOpacity(opacity);
    player->m_spiderSprite->GJSpiderSprite::setOpacity(opacity);
    player->m_robotSprite->GJRobotSprite::setOpacity(opacity);
}

void Player::updateColors() {
    if (!player1 || !player2) return;
    PlayerColors colors;

    if (!Mod::get()->getSettingValue<bool>("no_colors"))
        colors = info.colors;

    setPlayerColors(player1, false, colors.color1, colors.color2, colors.glowColor, colors.glowEnabled);
    setPlayerColors(player2, true, colors.color1, colors.color2, colors.glowColor, colors.glowEnabled);
    setPlayerIconColors(icon1, false, colors.color1, colors.color2, colors.glowColor, colors.glowEnabled);
    setPlayerIconColors(icon2, true, colors.color1, colors.color2, colors.glowColor, colors.glowEnabled);

    if (CCNode* cube = icon1->getChildByID("cube"_spr))
        setPlayerIconColors(static_cast<SimplePlayer*>(cube), false, colors.color1, colors.color2, colors.glowColor, colors.glowEnabled);
    if (CCNode* cube = icon2->getChildByID("cube"_spr))
        setPlayerIconColors(static_cast<SimplePlayer*>(cube), true, colors.color1, colors.color2, colors.glowColor, colors.glowEnabled);
}

void Player::updateCamera() {
    if (!PlayLayer::get()) return;
    if (!icon1 || !icon2) return;

    bool enabled = Mod::get()->getSettingValue<bool>("off_screen_indicators");
    if (actions.empty() || ghostCompletedLevel || !enabled || isSpectating) {
        icon1->setVisible(false);
        icon2->setVisible(false);
        return;
    }

    for (std::pair<PlayerObject*, SimplePlayer*> pair : {std::make_pair(player1, icon1), std::make_pair(player2, icon2)}) {
        if (pair.first == player2 && !isDual) {
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
    currentAction = 0;
    PlayerManager::getCurrentFrame() = 0;
    upsideDown1 = false;
    upsideDown2 = false;
    isDual = false;
    isMini1 = false;
    isMini2 = false;
    currentVehicle1 = VehicleType::Cube;
    currentVehicle2 = VehicleType::Cube;
    rotationOffset1 = 0.f;
    rotationOffset2 = 0.f;
    lastRotation1 = 0.f;
    lastRotation2 = 0.f;
    completedLevel = false;
    ghostCompletedLevel = false;
    PlayerManager::getSpectatorInput() = false;
    PlayerManager::getUpdatePlayer() = false;
    PlayerManager::getCanReset() = false;

    if (actions.empty()) return;
    if (!player1 || !player2) return;
    if (PlayerManager::getDisabled()) return;

    setPlayerSprite(player1, VehicleType::Cube);
    setPlayerSprite(player2, VehicleType::Cube);
    updateUpsideDownState();

    player1->CCNode::setScaleX(1.f);
    player1->CCNode::setScaleY(1.f);
    player2->CCNode::setScaleX(1.f);
    player2->CCNode::setScaleY(1.f);

    player1->setPosition({0, 105});
    player2->setPosition({0, 105});
    player1->setVisible(true);
    player2->setVisible(false);

    if (isSpectating) {
        PlayLayer::get()->m_player1->releaseAllButtons();
        PlayLayer::get()->m_player2->releaseAllButtons();
    }
}

void Player::loadReplay(Replay replay) {
    actions = replay.actions;
    icons = replay.info.icons;
    PlayerColors colors = replay.info.colors;
    info = replay.info;
    isRacing = true;
    updateColors();
    PlayerManager::get().updateUI();
    currentAction = 0;
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

    if (!player1 || !player2) return;

    if (info.isNew && isSpectating) {
        PlayLayer* pl = PlayLayer::get();
        player1->setPosition(pl->m_player1->getPosition());
        player1->setRotation(pl->m_player1->getRotation());
        if (pl->m_gameState.m_isDualMode) {
            player2->setPosition(pl->m_player2->getPosition());
            player2->setRotation(pl->m_player2->getRotation());
        }
    }

    while (currentAction < actions.size() && actions[currentAction].frame <= PlayerManager::getCurrentFrame()) {
        Action action = actions[currentAction];

        switch (action.type) {
            case ActionType::Position: handlePositionAction(action); break;
            case ActionType::Sideways: handleSidewaysAction(action); break;
            case ActionType::Vehicle: handleVehicleAction(action); break;
            case ActionType::Dual: handleDualAction(action); break;
            case ActionType::Flip: handleFlipAction(action); break;
            case ActionType::Mini: handleMiniAction(action); break;
            case ActionType::Animation: handleAnimationAction(action); break;
            case ActionType::Effect: handleEffectAction(action); break;
            case ActionType::Input: handleInputAction(action); break;
            case ActionType::Reset: handleResetAction(action); break;
        }
			
        if (action.type != ActionType::Position)
            updateUpsideDownState();
        else if (isSpectating && !info.isNew) {
            PlayLayer* pl = PlayLayer::get();
            pl->m_player1->setPosition(player1->getPosition());
            if (pl->m_gameState.m_isDualMode)
                pl->m_player2->setPosition(player2->getPosition());

            PlayerManager::getUpdatePlayer() = true;
            PlayerManager::getSpectated() = true;
        }

        currentAction++;
	}
}

void Player::handlePlaying(GJBaseGameLayer* bgl, int frame) {
    if (!isRacing && !isSpectating) return updateCamera();
    if (actions.empty() || PlayerManager::getDisabled()) return updateCamera();
    if (isSpectating && PlayerManager::getShouldRestart()) return;

    if (isSpectating && frame >= static_cast<int>(info.time * 240.f) - 1)
        PlayerManager::stopSpectating();

    if (!player1 || !player2) {
		player1 = static_cast<PlayerObject*>(bgl->m_objectLayer->getChildByID("ghost-player1"_spr));
		player2 = static_cast<PlayerObject*>(bgl->m_objectLayer->getChildByID("ghost-player2"_spr));
    }

	handleActions();

    if (isSpectating) {
        PlayLayer* pl = PlayLayer::get();
        pl->m_player1->setVisible(false);
        pl->m_player2->setVisible(false);
    }

    updateCamera();
}

void Player::startSpectating(Replay replay) {
    PlayerManager::getIsSpectating() = true;
    PlayerManager::getCurrentSpectate() = replay.info.time;
    PlayerManager::get().spectateInfo = replay.info;
    PlayerManager::getShouldRestart() = true;
    PlayerManager::getCanReset() = false;
    loadReplay(replay);
    if (player1) player1->setVisible(true);
    updateOpacity(true);
    updateOpacity(false);
    currentAction = 0;
    isSpectating = true;
    isRacing = false;
}

void Player::stopRacing() {
    resetState();

    if (player1) player1->removeFromParentAndCleanup(true);
    if (player2) player2->removeFromParentAndCleanup(true);
    if (icon1) icon1->removeFromParentAndCleanup(true);
    if (icon2) icon2->removeFromParentAndCleanup(true);

    player1 = nullptr;
    player2 = nullptr;
    icon1 = nullptr;
    icon2 = nullptr;

    actions.clear();
    isRacing = false;
    PlayerManager::getCurrentRace() = 0;
    PlayerManager::get().updateUI();
}

void Player::stopSpectating() {
    if (!isSpectating) return;

    actions.clear();
    currentAction = 0;
    isRacing = false;
    isSpectating = false;
    
    PlayerManager::getIsSpectating() = false;
    PlayerManager::getCurrentSpectate() = 0.f;
    PlayerManager::getSpectatorInput() = false;
    PlayerManager::getCanReset() = false;
    PlayerManager::getShouldRestart() = true;
    PlayerManager::resetButtons();

    PlayLayer::get()->m_player1->setVisible(true);
    PlayLayer::get()->m_player1->releaseAllButtons();
    PlayLayer::get()->m_player2->releaseAllButtons();
    PlayerManager::get().updateUI();

    if (!player1 || !player2) return;

    player1->setVisible(false);
    player2->setVisible(false);
    updateOpacity(false);
    updateOpacity(true);
}

void Player::updateUpsideDownState() {
    if (!player1 || !player2) return;

    if (upsideDown1 && !nonInvertedVehicles.contains(currentVehicle1))
        player1->CCNode::setScaleY(-1 * abs(player1->getScaleY()));
    else
        player1->CCNode::setScaleY(abs(player1->getScaleY()));

    if (upsideDown2 && !nonInvertedVehicles.contains(currentVehicle2))
        player2->CCNode::setScaleY(-1 * abs(player2->getScaleY()));
    else
        player2->CCNode::setScaleY(abs(player2->getScaleY()));
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
    if (!player) return;

    int id = icons.at(vehicle);

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
    SimplePlayer* icon = player == player2 ? icon2 : icon1;
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
        static_cast<SimplePlayer*>(cube)->updatePlayerFrame(icons.at(VehicleType::Cube), IconType::Cube);
    }

    icon->updatePlayerFrame(id, iconType);
    if (iconType == IconType::Robot)
        icon->m_robotSprite->GJRobotSprite::setOpacity(220);
    else if (iconType == IconType::Spider)
        icon->m_spiderSprite->GJSpiderSprite::setOpacity(220);
    else
        icon->setOpacity(220);

    updateOpacity(player == player2);
}

void Player::setPlayerScale(PlayerObject* player, float scale, float x, float y) {
    int negX = x < 0 ? -1 : 1;
    int negY = y < 0 ? -1 : 1;

    player->CCNode::setScaleX(scale * negX);
    player->CCNode::setScaleY(scale * negY);
}

void Player::playCompleteEffect() {
    if (!player1 || !player2) return;

    if (player1->isVisible()) player1->playCompleteEffect(false, false);
    if (player2->isVisible()) player2->playCompleteEffect(false, false);
    player1->setVisible(false);
    player2->setVisible(false);
}

void Player::playSpawnEffect(PlayerObject* player) {
    updateOpacity(false);
    updateOpacity(true);
    player->setVisible(true);
    player->stopAllActions();
    cocos2d::CCBlink* flashAction = cocos2d::CCBlink::create(0.4f, 4);
    player->runAction(flashAction);
}

void Player::handleCompletion() {

    if (completedLevel) return;
    completedLevel = true;

    playCompleteEffect();
    
    if (isSpectating || PlayerManager::getSpectated()) {
        PlayLayer* pl = PlayLayer::get();
        pl->m_player1->setVisible(true);
        pl->m_player2->setVisible(pl->m_gameState.m_isDualMode);
        PlayerManager::stopSpectating();
    }
}

void Player::handlePositionAction(Action action) {
    if (isSpectating && info.isNew) return;
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
    player->CCNode::setScaleX(scale);

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

    player->CCNode::setScaleX(abs(player->getScaleX()) * neg);

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
        player2->CCNode::setScaleX(player2->getScaleX() * -1);
    } else {
        rotationOffset1 = data.rotation;
        player1->CCNode::setScaleX(player1->getScaleX() * -1);
    }
}

void Player::handleAnimationAction(Action action) {
    AnimationData data = std::get<AnimationData>(action.data);
    PlayerObject* player = data.player2 ? player2 : player1;

    if (data.vehicle == VehicleType::Cube) {
        player->animatePlatformerJump(1.0f);
        return;
    }

    if (data.vehicle == VehicleType::Robot)
        player->m_robotSprite->tweenToAnimation(animationStrings.at(data.animation).c_str(), 0.1f);
    else
        player->m_spiderSprite->tweenToAnimation(animationStrings.at(data.animation).c_str(), 0.1f);
}

void Player::handleEffectAction(Action action) {
    EffectData data = std::get<EffectData>(action.data);
    PlayerObject* player = data.player2 ? player2 : player1;
    switch(data.effect) {
        case EffectType::Death: {
            player->playDeathEffect();
            if (isSpectating) {
                PlayLayer* pl = PlayLayer::get();
                pl->PlayLayer::destroyPlayer(pl->m_player1, pl->m_player1);
            }
            break;
        }
        case EffectType::Respawn: Player::playSpawnEffect(player); break;
        case EffectType::Complete: {
            #ifndef GEODE_IS_MACOS
            if (player->isVisible()) player->playCompleteEffect(false, false); 
            #endif

            ghostCompletedLevel = true;
            if (icon1) icon1->setVisible(false);
            if (icon2) icon2->setVisible(false);

            break;
        }
    }
}   

void Player::handleInputAction(Action action) {
    if (!isSpectating) return;

    PlayLayer* pl = PlayLayer::get();
    if (!pl) return;

    InputData data = std::get<InputData>(action.data);
    PlayerObject* player = data.player2 ? pl->m_player2 : pl->m_player1;
    PlayerButton btn = static_cast<PlayerButton>(data.button);

    PlayerManager::getSpectatorInput() = true;
    data.down ? player->pushButton(btn) : player->releaseButton(btn);
    PlayerManager::getUpdatePlayer() = true;
    Loader::get()->queueInMainThread([] {
        PlayerManager::getSpectatorInput() = false;
    });
}

void Player::handleResetAction(Action action) {
    if (!isSpectating) return;
    if (PlayLayer* pl = PlayLayer::get()) {
        PlayerManager::getCanReset() = true;
        pl->resetLevel();
        PlayerManager::getCanReset() = false;
    }
}