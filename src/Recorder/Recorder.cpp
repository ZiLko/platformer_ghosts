#include "Recorder.hpp"

std::vector<Action> Recorder::getActions() {
    return get().actions;
}

void Recorder::resetState(bool shouldRecord) {
    Recorder& r = get();
    r.goingLeft1 = false;
    r.goingLeft2 = false;
    r.upsideDown1 = false;
    r.upsideDown2 = true;
    r.prevPos1 = ccp(0, 0);
    r.prevPos2 = ccp(0, 0);
    r.prevRot1 = 0.f;
    r.prevRot2 = 0.f;
    r.actions.clear();

    if (!shouldRecord) return;

    PlayLayer* pl = PlayLayer::get();

    recordDualAction(pl->m_gameState.m_isDualMode, 1);

    for (int i = 0; i < 2; i++) {
        bool player2 = i == 1;
        PlayerObject* player = player2 ? pl-> m_player2 : pl->m_player1;
        VehicleType vehicle = getCurrentVehicle(player);
        recordVehicleAction(vehicle, 1, player2);
    }
}

void Recorder::handleRecording(GJBaseGameLayer* bgl, int frame) {
    Recorder& r = get();

    PlayerObject* p1 = bgl->m_player1;
    PlayerObject* p2 = bgl->m_player2;

    cocos2d::CCPoint pos1 = p1->getPosition();
    float rot1 = p1->getRotation();
        
    cocos2d::CCPoint pos2 = p2->getPosition();
    float rot2 = p2->getRotation();

    if (pos1 != r.prevPos1 || static_cast<int>(rot1) != r.prevRot1 || pos2 != r.prevPos2 || static_cast<int>(rot2) != r.prevRot2) {
        Action action;
        action.type = ActionType::Position;
        action.frame = frame;

        PlayerData p1Data = {
            pos1,
            rot1
        };
        PlayerData p2Data = {
            pos2,
            rot2
        };
        PositionData data = {
            p1Data,
            p2Data
        };

        action.data = data;
        r.actions.push_back(action);
    }

    if (r.goingLeft1 != p1->m_isGoingLeft)
        recordFlipAction(frame, false, p1->m_isGoingLeft, false);

    if (r.goingLeft2 != p2->m_isGoingLeft)
        recordFlipAction(frame, true, p2->m_isGoingLeft, false);

    if (r.upsideDown1 != p1->m_isUpsideDown)
        recordFlipAction(frame, false, p1->m_isUpsideDown, true);

    if (r.upsideDown2 != p2->m_isUpsideDown)
        recordFlipAction(frame, true, p2->m_isUpsideDown, true);

    r.goingLeft1 = p1->m_isGoingLeft;
    r.goingLeft2 = p2->m_isGoingLeft;
    r.upsideDown1 = p1->m_isUpsideDown;
    r.upsideDown2 = p2->m_isUpsideDown;
    r.prevPos1 = pos1;
    r.prevPos2 = pos2;
    r.prevRot1 = static_cast<int>(rot1);
    r.prevRot2 = static_cast<int>(rot2);
}

void Recorder::handlePortal(int id, int frame, bool player2, PlayerObject* player) {
    if (gamemodePortals.contains(id)) {
        recordVehicleAction(static_cast<VehicleType>(id), frame, player2);
        return;
    } 

    if (id == 286 || id == 287) {
        recordDualAction(id == 286, frame);

        if (id == 286)
            return;

        recordVehicleAction(getCurrentVehicle(player), frame, false);

        return;
    }
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