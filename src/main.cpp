#include <Geode/Geode.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/ColorAction.hpp>
#include <Geode/binding/EffectGameObject.hpp>
#include <Geode/binding/GJEffectManager.hpp>
#include <Geode/binding/GJSpriteColor.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/LevelSettingsObject.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/ui/Notification.hpp>
#include <cfloat>
#include <set>
#include <string>

using namespace geode::prelude;



class $modify(MyEditorUI, EditorUI) {
  bool init(LevelEditorLayer *editorLayer) {
    if (!EditorUI::init(editorLayer))
      return false;

    if (m_editSpecialBtn) {
      if (auto parent = m_editSpecialBtn->getParent()) {
        auto btnSpr = CCSprite::create("GJ_button_02.png"_spr);
        if (btnSpr) {
          auto label = CCLabelBMFont::create("C", "bigFont.fnt");
          if (label) {
            label->setScale(0.6f);
            label->setColor({0xEF, 0x93, 0xA2});
            label->setPosition(btnSpr->getContentSize() / 2 + CCPoint(0, 2.f));
            btnSpr->addChild(label);
          }

          auto btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(MyEditorUI::onCreatorColorClick));
          btn->setID("creator-color-btn"_spr);
          float scale = 0.8f;
          btn->setScale(scale);

          auto specialPos = m_editSpecialBtn->getPosition();
          float offset = (btnSpr->getContentSize().width * scale) + 5.0f;
          btn->setPosition({specialPos.x - offset, specialPos.y});
          btn->setZOrder(m_editSpecialBtn->getZOrder());

          parent->addChild(btn);
        }
      }
    }

    return true;
  }

  void onPlaytest(cocos2d::CCObject *sender) {
    EditorUI::onPlaytest(sender);
    setMenuVisible(false);
  }

  void onStopPlaytest(cocos2d::CCObject *sender) {
    EditorUI::onStopPlaytest(sender);
    setMenuVisible(true);
  }

  void playtestStopped() {
    EditorUI::playtestStopped();
    setMenuVisible(true);
  }

  void setMenuVisible(bool visible) {
    if (m_editSpecialBtn) {
      if (auto parent = m_editSpecialBtn->getParent()) {
        if (auto btn = parent->getChildByID("creator-color-btn"_spr)) {
          btn->setVisible(visible);
        }
      }
    }
  }

  void onCreatorColorClick(CCObject*) {
    if (!m_selectedObjects || m_selectedObjects->count() == 0) {
      FLAlertLayer::create("Colors", "Please select some objects first!", "OK")->show();
      return;
    }

    std::set<int> uniqueChannels;
    float minX = FLT_MAX;
    float minY = FLT_MAX;

    for (int i = 0; i < m_selectedObjects->count(); ++i) {
      auto obj = static_cast<GameObject*>(m_selectedObjects->objectAtIndex(i));
      if (!obj) continue;

      auto pos = obj->getPosition();
      if (pos.x < minX)
        minX = pos.x;
      if (pos.y < minY)
        minY = pos.y;

      int baseID = obj->m_baseColor ? obj->m_baseColor->m_colorID : 0;
      int detailID = obj->m_detailColor ? obj->m_detailColor->m_colorID : 0;

      if (baseID > 0 && !(baseID >= 1000 && baseID <= 1014)) {
        uniqueChannels.insert(baseID);
      }
      if (detailID > 0 && !(detailID >= 1000 && detailID <= 1014)) {
        uniqueChannels.insert(detailID);
      }
    }

    if (uniqueChannels.empty()) {
      FLAlertLayer::create(
          "Colors", "No custom color channels found in the selected objects.",
          "OK")
          ->show();
      return;
    }

    auto spawnedTriggers = CCArray::create();
    int index = 0;

    for (int channelID : uniqueChannels) {
      CCPoint position = {minX - 30.f, minY + index * 30.f};
      auto obj = m_editorLayer->createObject(899, position, true);
      if (!obj)
        continue;

      auto trigger = static_cast<EffectGameObject *>(obj);

      ccColor3B color = {255, 255, 255};
      float opacity = 1.0f;
      bool blending = false;

      if (auto settings = m_editorLayer->m_levelSettings) {
        if (auto effectManager = settings->m_effectManager) {
          if (auto action = effectManager->getColorAction(channelID)) {
            color = action->m_fromColor;
            opacity = action->m_fromOpacity;
            blending = action->m_blending;
          }
        }
      }

      trigger->m_triggerTargetColor = color;
      trigger->m_opacity = opacity;
      trigger->m_usesBlending = blending;
      trigger->m_duration = 0.0f;
      trigger->m_targetColor = channelID;
      trigger->setTargetID(channelID);
      trigger->m_targetGroupID = channelID;

      trigger->updateSpecialColor();

      spawnedTriggers->addObject(obj);
      index++;
    }

    if (spawnedTriggers->count() > 0) {
      auto undoObj =
          UndoObject::createWithArray(spawnedTriggers, UndoCommand::New);
      m_editorLayer->addToUndoList(undoObj, true);

      m_editorLayer->updateObjectColors(spawnedTriggers);
      for (int i = 0; i < spawnedTriggers->count(); ++i) {
        auto obj = static_cast<GameObject *>(spawnedTriggers->objectAtIndex(i));
        m_editorLayer->updateObjectLabel(obj);
      }

      this->selectObjects(spawnedTriggers, true);
      this->updateObjectInfoLabel();
      this->updateScaleControl();

      std::string msg =
          "Exported " + std::to_string(spawnedTriggers->count()) + " colors!";
      Notification::create(msg.c_str(), NotificationIcon::Success)->show();
    }
  }
};
