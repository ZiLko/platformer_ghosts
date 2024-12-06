#include "GhostUI.hpp"
#include "../Player/Player.hpp"
#include "../RecordsManager/RecordsManager.hpp"

bool GhostUI::init() {
    setAnchorPoint({0, 1});
    setID("ghost_ui"_spr);

    title = CCLabelBMFont::create("Best Time", "goldFont.fnt");
    title->setSkewX(4);
    title->setScale(0.4f);
    title->setPosition({55, -14 + 7});
    title->setID("ui-title");
    addChild(title);

    uiIcon = SimplePlayer::create(1);
    uiIcon->setScale(0.7f);
    uiIcon->setPosition({23, -37 + 7});
    uiIcon->setID("ui-icon");
    addChild(uiIcon, 500);

    uiTime = CCLabelBMFont::create("1.008s", "goldFont.fnt");
    uiTime->setAnchorPoint({0, 0.5f});
    uiTime->setPosition({39, -46 + 7});
    uiTime->setScale(0.525f);
    uiTime->setID("ui-time");
    addChild(uiTime, 500);

    uiName = CCLabelBMFont::create("Zilko", "goldFont.fnt");
    uiName->setAnchorPoint({0, 0.5f});
    uiName->setScale(0.525f);
    uiName->setPosition({39, -28 + 7});
    uiName->setID("ui-name");
    addChild(uiName, 500);

    return true;
}

void GhostUI::update() {
    if (!Mod::get()->getSettingValue<bool>("show_ui") || PlayerManager::get().players.empty() || PlayerManager::getDisabled())
        return setVisible(false);

    setVisible(true);

    ReplayInfo info;
    if (PlayerManager::getIsSpectating()) {
        info = PlayerManager::get().spectateInfo;
        title->setString("Watching");
    } else {
        bool started = false;
        for (Player player : PlayerManager::get().players) {
            if (player.info.time < info.time || !started)
                info = player.info;
            started = true;
        }
        title->setString("Best Time");
    }

    PlayerColors colors = info.colors;
    if (info.icons.contains(VehicleType::Cube))
        uiIcon->updatePlayerFrame(info.icons.at(VehicleType::Cube), IconType::Cube);

    Player::setPlayerIconColors(uiIcon, false, colors.color1, colors.color2, colors.glowColor, colors.glowEnabled);
    uiTime->setString(RecordsManager::getFormattedTime(info.time).c_str());
    uiName->setString(info.username.c_str());

    setScale(Mod::get()->getSettingValue<double>("ui_scale"));
    title->setOpacity(Mod::get()->getSettingValue<int64_t>("ui_opacity"));
    uiIcon->setOpacity(Mod::get()->getSettingValue<int64_t>("ui_opacity"));
    uiName->setOpacity(Mod::get()->getSettingValue<int64_t>("ui_opacity"));
    uiTime->setOpacity(Mod::get()->getSettingValue<int64_t>("ui_opacity"));
}