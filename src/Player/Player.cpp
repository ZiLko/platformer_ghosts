#include "../RecordsManager/RecordsManager.hpp"
#include "Player.hpp"

void Player::clearActions() {
    get().actions.clear();
}

void Player::setup(PlayLayer* pl) {
    for (int i = 1; i < 3; i++) {
		PlayerObject* player = PlayerObject::create(1, 1, pl, pl, true);
        player->retain();
        player->setPosition({ 0, 105 });
		player->setID((("ghost-player"_spr) + std::to_string(i)).c_str());
        player->togglePlatformerMode(true);
        player->setOpacity(110);
        pl->m_objectLayer->addChild(player);

        setPlayerColors(player, i == 2);

        if (i == 1)
            get().player1 = player;
        else
            get().player2 = player;
    }

    std::filesystem::path bestCompletion = RecordsManager::getBestCompletion(EditorIDs::getID(pl->m_level));

    if (!bestCompletion.empty())
        get().actions = RecordsManager::getCompletionActions(bestCompletion);
}

void Player::resetState() {
    Player& p = get();
    p.currentAction = 0;
    p.upsideDown1 = false;
    p.upsideDown2 = false;
    p.isDual = false;
    p.player1 = nullptr;
    p.player2 = nullptr;
    p.currentVehicle1 = VehicleType::Cube;
    p.currentVehicle2 = VehicleType::Cube;
}

void Player::handlePlaying(GJBaseGameLayer* bgl, int frame) {
    Player& p = get();

    if (p.actions.empty()) return;

    if (!p.player1 || !p.player2) {
		p.player1 = static_cast<PlayerObject*>(bgl->m_objectLayer->getChildByID("ghost-player1"_spr));
		p.player2 = static_cast<PlayerObject*>(bgl->m_objectLayer->getChildByID("ghost-player2"_spr));
    }

    if (!p.player1 || !p.player2) return;

	while (p.currentAction < p.actions.size() && p.actions[p.currentAction].frame <= frame) {
        Action action = p.actions[p.currentAction];

        switch (action.type) {
            case ActionType::Position: {
                PositionData pos = std::get<PositionData>(action.data);

                if (pos.p1Data.position.x != 0.f)
    			    p.player1->setPositionX(pos.p1Data.position.x);
                if (pos.p1Data.position.y != 0.f)
    			    p.player1->setPositionY(pos.p1Data.position.y);
                if (pos.p1Data.rotation != 0.f)
			        p.player1->setRotation(pos.p1Data.rotation);

                if (p.isDual) {
                    if (pos.p2Data.position.x != 0.f)
                        p.player2->setPositionX(pos.p2Data.position.x);
                    if (pos.p2Data.position.y != 0.f)
                        p.player2->setPositionY(pos.p2Data.position.y);
                    if (pos.p2Data.rotation != 0.f)
			            p.player2->setRotation(pos.p2Data.rotation);
                }
                break;
            } case ActionType::Death:
                break;
            case ActionType::Respawn:
                break;
            case ActionType::Vehicle: {
                VehicleData data = std::get<VehicleData>(action.data);
                PlayerObject* player = data.player2 ? p.player2 : p.player1;
                float scale = player->getScaleX();

                setPlayerSprite(player, data.vehicle, 2);
                player->setScaleX(scale);

                if (data.player2)
                    p.currentVehicle2 = data.vehicle;
                else 
                    p.currentVehicle1 = data.vehicle;

                break;
            } case ActionType::RobotAnimation:
                break;
            case ActionType::SpiderAnimation:
                break;
            case ActionType::Dual: {
                bool data = std::get<bool>(action.data);

                if (data == p.isDual) break;

                p.isDual = data;
                p.player2->setVisible(p.isDual);

                if (p.isDual) {
                    setPlayerSprite(p.player2, p.currentVehicle1, 2);
                    p.currentVehicle2 = p.currentVehicle1;
                }
                break;
            } case ActionType::Flip: {
                    FlipData data = std::get<FlipData>(action.data);

                if (data.y) {
                    if (data.player2)
                        p.upsideDown2 = data.flip;
                    else
                        p.upsideDown1 = data.flip;

                    updateUpsideDownState();
                    break;
                }

                PlayerObject* player = data.player2 ? p.player2 : p.player1;

                float scale = 1.f;
                int neg = data.flip ? -1 : 1;

                player->setScaleX(scale * neg);

                break;
                }
        }
			
        if (action.type != ActionType::Position)
            updateUpsideDownState();

        p.currentAction++;
	}
}

void Player::updateUpsideDownState() {
    Player& p = get();

    if (p.upsideDown1 && p.currentVehicle1 != VehicleType::Cube && p.currentVehicle1 != VehicleType::Swing)
        p.player1->setScaleY(-1);
    else
        p.player1->setScaleY(1);

    if (p.upsideDown2 && p.currentVehicle2 != VehicleType::Cube && p.currentVehicle2 != VehicleType::Swing)
        p.player2->setScaleY(-1);
    else
        p.player2->setScaleY(1);
}

void Player::setPlayerColors(PlayerObject* player, bool player2) {
    GameManager* gm = GameManager::get();

    cocos2d::ccColor3B color1 = gm->colorForIdx(gm->getPlayerColor());
    cocos2d::ccColor3B color2 = gm->colorForIdx(gm->getPlayerColor2());

    if (player2) {
        cocos2d::ccColor3B tempColor = color1;
        color1 = color2;
        color2 = tempColor;
    }

    cocos2d::ccColor3B glow = gm->colorForIdx(gm->getPlayerGlowColor());

    player->m_hasGlow = gm->getPlayerGlow();

    player->setColor(color1);
    player->setSecondColor(color2);

    if (player->m_hasGlow)
        player->enableCustomGlowColor(glow);
    else
        player->disableCustomGlowColor();

    player->updateGlowColor();
    player->updatePlayerGlow();
}

void Player::setPlayerSprite(PlayerObject* player, VehicleType vehicle, int id) {
    switch (vehicle) {
        case VehicleType::Cube: 
            player->toggleFlyMode(false, false);
            player->toggleRollMode(false, false);
            player->toggleBirdMode(false, false);
            player->toggleDartMode(false, false);
            player->toggleRobotMode(false, false);
            player->toggleSpiderMode(false, false);
            player->toggleSwingMode(false, false);
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
}