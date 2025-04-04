#include "Recorder.hpp"
#include "../Player/Player.hpp"

Recorder& Recorder::get() {
    static Recorder instance;
    return instance;
}

std::vector<Action> Recorder::getActions() {
    return get().actions;
}

void Recorder::setTime(float time) {
    get().time = time;
}

void Recorder::resetState(bool shouldRecord) {
    Recorder& r = get();
    
    r.totalFrame = 0;
    r.previousFrame = 0;
    r.levelComplete = false;

    r.goingLeft1 = false;
    r.goingLeft2 = false;
    r.upsideDown1 = false;
    r.upsideDown2 = false;
    r.prevPos1 = ccp(0, 0);
    r.prevPos2 = ccp(0, 0);
    r.prevRot1 = 0.f;
    r.prevRot2 = 0.f;
    r.moving1 = false;
    r.moving2 = false;
    r.falling1 = false;
    r.falling2 = false;
    r.jumped1 = false;
    r.jumped2 = false;
    r.currentCompletionTime = 0.f;
    r.time = 0.f;
    r.compareTime = 0.f;
    r.actions.clear();

    if (!shouldRecord) return;

    PlayLayer* pl = PlayLayer::get();
    if (!pl) return;

    r.upsideDown1 = pl->m_player1->m_isUpsideDown;
    r.upsideDown2 = pl->m_player2->m_isUpsideDown;
    r.goingLeft1 = pl->m_player1->m_isGoingLeft;
    r.goingLeft2 = pl->m_player2->m_isGoingLeft;

    if (pl->m_player1->m_holdingButtons[1]) handleInput(1, 1, true, false);
    if (pl->m_player1->m_holdingButtons[2]) handleInput(1, 2, true, false);
    if (pl->m_player1->m_holdingButtons[3]) handleInput(1, 3, true, false);
    if (pl->m_player2->m_holdingButtons[1]) handleInput(1, 1, true, true);
    if (pl->m_player2->m_holdingButtons[2]) handleInput(1, 2, true, true);
    if (pl->m_player2->m_holdingButtons[3]) handleInput(1, 3, true, true);

    recordDualAction(pl->m_gameState.m_isDualMode, 1);
    for (int i = 0; i < 2; i++) {
        bool player2 = i == 1;
        PlayerObject* player = player2 ? pl-> m_player2 : pl->m_player1;
        VehicleType vehicle = getCurrentVehicle(player);
        recordVehicleAction(vehicle, 1, player2);
        recordMiniAction(1, static_cast<float>(player->m_vehicleSize) == 0.6f, player2);
    }
}

void Recorder::handleRecording(PlayLayer* pl, int frame) {
    if (PlayerManager::getIsSpectating()) return;
    
    Recorder& r = get();
    if (r.disabled) return;

    PlayerObject* p1 = pl->m_player1;  
    PlayerObject* p2 = pl->m_player2;
    cocos2d::CCPoint pos1 = p1->getPosition();
    cocos2d::CCPoint pos2 = p2->getPosition();
    float rot1 = p1->getRotation();
    float rot2 = p2->getRotation();
    bool diffPos = pos1 != r.prevPos1 || static_cast<int>(rot1) != r.prevRot1 || pos2 != r.prevPos2 || static_cast<int>(rot2) != r.prevRot2;

    int m = static_cast<int>(240.f / r.fps);
    if (frame % m == 0 && diffPos)
        recordPositionAction(PlayerManager::getCurrentFrame(), false);

    if (r.goingLeft1 != p1->m_isGoingLeft)
        recordFlipAction(frame, false, p1->m_isGoingLeft, false);
    if (r.goingLeft2 != p2->m_isGoingLeft)
        recordFlipAction(frame, true, p2->m_isGoingLeft, false);
    if (r.upsideDown1 != p1->m_isUpsideDown)
        recordFlipAction(frame, false, p1->m_isUpsideDown, true);
    if (r.upsideDown2 != p2->m_isUpsideDown)
        recordFlipAction(frame, true, p2->m_isUpsideDown, true);
    if (r.sideways1 != p1->m_isSideways)
        recordSidewaysAction(frame, 90.f, false);
    if (r.sideways2 != p2->m_isSideways)
        recordSidewaysAction(frame, 90.f, true);

    bool moving1 = p1->m_platformerXVelocity > 0.1 || p1->m_platformerXVelocity < -0.1;
    bool moving2 = p2->m_platformerXVelocity > 0.1 || p2->m_platformerXVelocity < -0.1;
    float vel1 = p1->m_yVelocity * (p1->m_isUpsideDown ? -1 : 1);
    float vel2 = p2->m_yVelocity * (p2->m_isUpsideDown ? -1 : 1);
    bool falling1 = vel1 < 0.0;
    bool falling2 = vel2 < 0.0;
    
    if (p1->m_isRobot || p1->m_isSpider) {
        if (p1->m_isOnGround && moving1 && (r.moving1 != moving1 || r.onGround1 != p1->m_isOnGround))
            recordAnimationAction(frame, getCurrentVehicle(p1), false, AnimationType::Run);
        if (p1->m_isOnGround && !moving1 && (r.moving1 != moving1 || r.onGround1 != p1->m_isOnGround))
            recordAnimationAction(frame, getCurrentVehicle(p1), false, AnimationType::Idle);
        if (!p1->m_isOnGround && falling1 && r.falling1 != falling1)
            recordAnimationAction(frame, getCurrentVehicle(p1), false, AnimationType::Fall);
    }

    if (!p1->m_isOnGround && vel1 > 0.0 && r.jumped1) {
        recordAnimationAction(frame, getCurrentVehicle(p1), false, AnimationType::Jump);
        r.jumped1 = false;
    }

    if (p2->m_isRobot || p2->m_isSpider) {
        if (p2->m_isOnGround && moving2 && (r.moving2 != moving2 || r.onGround2 != p2->m_isOnGround))
            recordAnimationAction(frame, getCurrentVehicle(p2), true, AnimationType::Run);
        if (p2->m_isOnGround && !moving2 && (r.moving2 != moving2 || r.onGround2 != p2->m_isOnGround))
            recordAnimationAction(frame, getCurrentVehicle(p2), true, AnimationType::Idle);
        if (!p2->m_isOnGround && falling2 && r.falling2 != falling2)
            recordAnimationAction(frame, getCurrentVehicle(p2), true, AnimationType::Fall);
    }

    if (!p2->m_isOnGround && vel2 > 0.0 && r.jumped2) {
        recordAnimationAction(frame, getCurrentVehicle(p2), true, AnimationType::Jump);
        r.jumped2 = false;
    }

    if (r.updatePlayer2Animation != 0 && r.updatePlayer2Animation == frame) {
        r.updatePlayer2Animation = 0;

        if (!pl->m_gameState.m_isDualMode) return;

        if (p2->m_isRobot || p2->m_isSpider) {
            if (p2->m_isOnGround && moving2)
                recordAnimationAction(frame, getCurrentVehicle(p2), true, AnimationType::Run);
            if (p2->m_isOnGround && !moving2)
                recordAnimationAction(frame, getCurrentVehicle(p2), true, AnimationType::Idle);
            if (!p2->m_isOnGround && falling2)
                recordAnimationAction(frame, getCurrentVehicle(p2), true, AnimationType::Fall);
            if (!p2->m_isOnGround && vel2 > 0.0)
                recordAnimationAction(frame, getCurrentVehicle(p2), true, AnimationType::Jump);
        }
    }

    r.moving1 = moving1;
    r.moving2 = moving2;
    r.falling1 = falling1;
    r.falling2 = falling2;
    r.onGround1 = p1->m_isOnGround;
    r.onGround2 = p2->m_isOnGround;
    r.goingLeft1 = p1->m_isGoingLeft;
    r.goingLeft2 = p2->m_isGoingLeft;
    r.upsideDown1 = p1->m_isUpsideDown;
    r.upsideDown2 = p2->m_isUpsideDown;
    r.sideways1 = p1->m_isSideways;
    r.sideways2 = p2->m_isSideways;
    r.prevPos1 = pos1;
    r.prevPos2 = pos2;
    r.prevRot1 = static_cast<int>(rot1);
    r.prevRot2 = static_cast<int>(rot2);
}

void Recorder::handlePortal(int id, int frame, bool player2, PlayerObject* player) {
    if (get().disabled) return;
    if (gamemodePortals.contains(id)) {
        recordVehicleAction(static_cast<VehicleType>(id), frame, player2);
        return;
    } 

    if (id == 286 || id == 287) {
        recordDualAction(id == 286, frame);

        if (id == 286) { // Enter dual portal
            get().updatePlayer2Animation = frame + 3;
            return;
        }

        recordVehicleAction(getCurrentVehicle(player), frame, false);
        recordMiniAction(frame, static_cast<float>(player->m_vehicleSize) == 0.6f, false);

        return;
    }

    if (id == 101 || id == 99) {
        recordMiniAction(frame, id == 101, player2);
        return;
    }
}

void Recorder::handleInput(int frame, int button, bool down, bool player2) {
    if (get().disabled) return;
    Action action;
    InputData data = {
        button,
        down,
        player2
    };
    action.frame = frame;
    action.data = data;
    action.type = ActionType::Input;
    get().actions.push_back(action);
}

void Recorder::handleEffect(int frame, EffectType effect, bool player2) {
    if (get().disabled) return;
    Action action;
    EffectData data = {
        effect,
        player2
    };
    action.type = ActionType::Effect;
    action.data = data;
    action.frame = frame;
    Recorder::get().actions.push_back(action);

    if (effect != EffectType::Complete) return;

    Loader::get()->queueInMainThread([] {
        Recorder::recordPositionAction(PlayerManager::getCurrentFrame(), true);
    });
}

void Recorder::recordPositionAction(int frame, bool rot = false) {
    PlayLayer* pl = PlayLayer::get();
    if (!pl) return;

    Action action;
    action.type = ActionType::Position;
    action.frame = PlayerManager::getCurrentFrame();

    cocos2d::CCPoint pos1 = pl->m_player1->getPosition();
    cocos2d::CCPoint pos2 = pl->m_player2->getPosition();
    float rot1 = pl->m_player1->getRotation();
    float rot2 = pl->m_player2->getRotation();

    PlayerData p1Data = { pos1, rot1, rot };
    PlayerData p2Data = { pos2, rot2, rot };
    PositionData data = { p1Data, p2Data };

    action.data = data;
    get().actions.push_back(action);
}

void Recorder::recordVehicleAction(VehicleType vehicle, int frame, bool player2) {
    Action action;
    VehicleData data = {
        vehicle,
        player2
    };
    action.frame = frame;
    action.type = ActionType::Vehicle;
    action.data = data;
    get().actions.push_back(action);

    Loader::get()->queueInMainThread([] {
        Recorder::recordPositionAction(PlayerManager::getCurrentFrame(), true);
    });
}

void Recorder::recordFlipAction(int frame, bool player2, bool flip, bool y) {
    Action action;
    action.type = ActionType::Flip;
    action.frame = frame;
    FlipData data = {
        player2,
        flip,
        y
    };
    action.data = data;
    get().actions.push_back(action);
    }

void Recorder::recordDualAction(bool dual, int frame) {
    Action action;
    action.type = ActionType::Dual;
    action.data = dual;
    action.frame = frame;
    get().actions.push_back(action);
}

VehicleType Recorder::getCurrentVehicle(PlayerObject* player) {
    if (player->m_isShip) return VehicleType::Ship;
    if (player->m_isBird) return VehicleType::Ufo;
    if (player->m_isDart) return VehicleType::Wave;
    if (player->m_isBall) return VehicleType::Ball;
    if (player->m_isSpider) return VehicleType::Spider;
    if (player->m_isRobot) return VehicleType::Robot;
    if (player->m_isSwing) return VehicleType::Swing;
    return VehicleType::Cube;
}

void Recorder::recordMiniAction(int frame, bool mini, bool player2) {
    Action action;
    MiniData data;
    data.mini = mini;
    data.player2 = player2;
    action.type = ActionType::Mini;
    action.data = data;
    action.frame = frame;
    get().actions.push_back(action);
}

void Recorder::recordSidewaysAction(int frame, float rotation, bool player2) {
    Action action;
    SidewaysData data;
    data.rotation = rotation;
    data.player2 = player2;
    action.type = ActionType::Sideways;
    action.data = data;
    action.frame = frame;
    get().actions.push_back(action);

    if (PlayLayer* pl = PlayLayer::get())
        recordFlipAction(frame + 1, player2, (player2 ? pl->m_player2 : pl->m_player1)->m_isGoingLeft, false);
}

void Recorder::recordAnimationAction(int frame, VehicleType vehicle, bool player2, AnimationType animation) {
    if (vehicle != VehicleType::Cube && vehicle != VehicleType::Robot && vehicle != VehicleType::Spider) return;
    if (vehicle == VehicleType::Cube) {
        PlayerObject* player = player2 ? PlayLayer::get()->m_player2 : PlayLayer::get()->m_player1;
        int r = std::round(abs(player->getRotation()));
        if (player->m_rotationSpeed != 0.0 || player->m_platformerXVelocity != 0.0) return;
        if (r != 0 && r != 90 && r != 360 && r != 270 && r != 180) return;
    }
    Action action;
    AnimationData data = {
        animation,
        vehicle,
        player2
    };
    action.type = ActionType::Animation;
    action.data = data;
    action.frame = frame;
    get().actions.push_back(action);
}

void Recorder::recordResetAction(int frame) {
    Action action;
    action.type = ActionType::Reset;
    action.data = true;
    action.frame = frame;
    get().actions.push_back(action);

    Loader::get()->queueInMainThread([] {
        Loader::get()->queueInMainThread([] {
            Recorder::recordPositionAction(PlayerManager::getCurrentFrame(), true);
        });
    });
}