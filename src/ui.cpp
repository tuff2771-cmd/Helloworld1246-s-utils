#include "config.hpp"
#include <Geode/modify/PauseLayer.hpp>
#include <cstdlib>

using namespace geode::prelude;
using namespace cheat;

namespace {

CCNode* makeArrowIcon() {
    auto node = CCNode::create();
    node->setContentSize({28.f, 28.f});
    auto draw = CCDrawNode::create();
    CCPoint square[4] = {ccp(0.f, 0.f), ccp(28.f, 0.f), ccp(28.f, 28.f), ccp(0.f, 28.f)};
    draw->drawPolygon(square, 4, ccc4f(0.16f, 0.16f, 0.16f, 1.f), 0.f, ccc4f(0.f, 0.f, 0.f, 0.f));
    CCPoint tri[3] = {ccp(10.f, 6.f), ccp(10.f, 22.f), ccp(22.f, 14.f)};
    draw->drawPolygon(tri, 3, ccc4f(0.95f, 0.95f, 0.95f, 1.f), 0.f, ccc4f(0.f, 0.f, 0.f, 0.f));
    node->addChild(draw);
    return node;
}

class OptionsPopup : public geode::Popup {
protected:
    Feature const* m_feature = nullptr;
    std::vector<ButtonSprite*> m_choiceSprites;

    bool init(Feature const* feature) {
        float const rowH = 34.f;
        float const w = 300.f;
        float const rows = static_cast<float>(feature->opts.size());
        if (!Popup::init(w, 66.f + rowH * rows)) return false;

        m_feature = feature;
        m_choiceSprites.assign(feature->opts.size(), nullptr);
        this->setTitle(feature->label.c_str());

        auto list = CCNode::create();
        list->setContentSize({w - 20.f, rowH * rows});
        list->setAnchorPoint({0.5f, 0.5f});
        list->setLayout(
            ColumnLayout::create()
                ->setAxisReverse(true)
                ->setGap(0.f)
                ->setAutoScale(false)
        );
        for (size_t i = 0; i < feature->opts.size(); ++i) {
            list->addChild(this->makeRow(i, w - 20.f, rowH));
        }
        list->updateLayout();
        m_mainLayer->addChildAtPosition(list, Anchor::Center, ccp(0.f, -12.f));
        return true;
    }

    CCNode* makeRow(size_t i, float w, float h) {
        auto const& o = m_feature->opts[i];
        auto row = CCNode::create();
        row->setContentSize({w, h});

        auto label = CCLabelBMFont::create(o.label.c_str(), "bigFont.fnt");
        label->setAnchorPoint({0.f, 0.5f});
        label->limitLabelWidth(150.f, 0.5f, 0.2f);
        label->setPosition({8.f, h / 2.f});
        row->addChild(label);

        if (o.type == OptType::Number) {
            auto input = TextInput::create(100.f, "0", "bigFont.fnt");
            input->setFilter("0123456789.-");
            input->setString(fmt::format("{}", Cfg::get(o.key)));
            input->setCallback([key = o.key](auto const& text) {
                std::string s(text);
                char* end = nullptr;
                float v = std::strtof(s.c_str(), &end);
                if (end != s.c_str()) Cfg::set(key, v);
            });
            input->setPosition({w - 62.f, h / 2.f});
            row->addChild(input);
            return row;
        }

        auto menu = CCMenu::create();
        menu->setAnchorPoint({0.f, 0.f});
        menu->setPosition({0.f, 0.f});
        menu->setContentSize({w, h});

        if (o.type == OptType::Toggle) {
            auto toggler = CCMenuItemToggler::createWithStandardSprites(
                this, menu_selector(OptionsPopup::onToggle), 0.6f
            );
            toggler->setTag(static_cast<int>(i));
            toggler->toggle(Cfg::b(o.key));
            toggler->setPosition({w - 26.f, h / 2.f});
            menu->addChild(toggler);
        } else {
            int cur = std::clamp(Cfg::i(o.key), 0, static_cast<int>(o.choices.size()) - 1);
            auto spr = ButtonSprite::create(o.choices[cur].c_str());
            spr->setScale(0.5f);
            m_choiceSprites[i] = spr;
            auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(OptionsPopup::onChoice));
            btn->setTag(static_cast<int>(i));
            btn->setPosition({w - 60.f, h / 2.f});
            menu->addChild(btn);
        }
        row->addChild(menu);
        return row;
    }

    void onToggle(CCObject* sender) {
        auto t = static_cast<CCMenuItemToggler*>(sender);
        auto const& o = m_feature->opts[t->getTag()];
        Cfg::set(o.key, t->isToggled() ? 0.f : 1.f);
    }

    void onChoice(CCObject* sender) {
        auto item = static_cast<CCMenuItemSpriteExtra*>(sender);
        size_t idx = static_cast<size_t>(item->getTag());
        auto const& o = m_feature->opts[idx];
        int next = (Cfg::i(o.key) + 1) % static_cast<int>(o.choices.size());
        Cfg::set(o.key, static_cast<float>(next));
        if (auto spr = m_choiceSprites[idx]) spr->setString(o.choices[next].c_str());
    }

public:
    static OptionsPopup* create(Feature const* feature) {
        auto ret = new OptionsPopup();
        if (ret->init(feature)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

class CheatPopup : public geode::Popup {
protected:
    bool init() {
        if (!Popup::init(360.f, 290.f)) return false;
        this->setTitle("Cheats");

        auto section = CCLabelBMFont::create("Gameplay", "goldFont.fnt");
        section->setScale(0.6f);
        m_mainLayer->addChildAtPosition(section, Anchor::Top, ccp(0.f, -44.f));

        float const sw = 330.f;
        float const sh = 200.f;

        auto bg = CCLayerColor::create(ccc4(0, 0, 0, 90), sw + 10.f, sh + 6.f);
        bg->ignoreAnchorPointForPosition(false);
        bg->setAnchorPoint({0.5f, 0.5f});
        m_mainLayer->addChildAtPosition(bg, Anchor::Center, ccp(0.f, -22.f));

        auto scroll = ScrollLayer::create({sw, sh});
        scroll->ignoreAnchorPointForPosition(false);
        scroll->setAnchorPoint({0.5f, 0.5f});
        m_mainLayer->addChildAtPosition(scroll, Anchor::Center, ccp(0.f, -22.f));

        auto content = scroll->m_contentLayer;
        content->setLayout(
            ColumnLayout::create()
                ->setAxisReverse(true)
                ->setAxisAlignment(AxisAlignment::End)
                ->setAutoGrowAxis(sh)
                ->setGap(2.f)
                ->setAutoScale(false)
        );
        auto const& list = features();
        for (size_t i = 0; i < list.size(); ++i) {
            content->addChild(this->makeRow(i, sw));
        }
        content->updateLayout();
        scroll->moveToTop();
        return true;
    }

    CCNode* makeRow(size_t i, float w) {
        auto const& f = features()[i];
        float const h = 32.f;
        auto row = CCNode::create();
        row->setContentSize({w, h});

        auto menu = CCMenu::create();
        menu->setAnchorPoint({0.f, 0.f});
        menu->setPosition({0.f, 0.f});
        menu->setContentSize({w, h});

        auto toggler = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(CheatPopup::onFeatureToggle), 0.65f
        );
        toggler->setTag(static_cast<int>(i));
        toggler->toggle(Cfg::b(f.key));
        toggler->setPosition({24.f, h / 2.f});
        menu->addChild(toggler);

        if (!f.opts.empty()) {
            auto btn = CCMenuItemSpriteExtra::create(
                makeArrowIcon(), this, menu_selector(CheatPopup::onFeatureOptions)
            );
            btn->setTag(static_cast<int>(i));
            btn->setPosition({w - 26.f, h / 2.f});
            menu->addChild(btn);
        }
        row->addChild(menu);

        auto label = CCLabelBMFont::create(f.label.c_str(), "bigFont.fnt");
        label->setAnchorPoint({0.f, 0.5f});
        label->limitLabelWidth(w - 110.f, 0.5f, 0.2f);
        label->setPosition({48.f, h / 2.f});
        row->addChild(label);
        return row;
    }

    void onFeatureToggle(CCObject* sender) {
        auto t = static_cast<CCMenuItemToggler*>(sender);
        auto const& f = features()[t->getTag()];
        Cfg::set(f.key, t->isToggled() ? 0.f : 1.f);
    }

    void onFeatureOptions(CCObject* sender) {
        auto item = static_cast<CCMenuItemSpriteExtra*>(sender);
        if (auto popup = OptionsPopup::create(&features()[item->getTag()])) {
            popup->show();
        }
    }

public:
    static CheatPopup* create() {
        auto ret = new CheatPopup();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

} // namespace

class $modify(CheatPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto win = CCDirector::sharedDirector()->getWinSize();
        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});

        auto spr = ButtonSprite::create("Cheats");
        spr->setScale(0.7f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(CheatPauseLayer::onCheats));
        btn->setPosition({52.f, win.height - 24.f});
        menu->addChild(btn);
        this->addChild(menu, 10);
    }

    void onCheats(CCObject*) {
        if (auto popup = CheatPopup::create()) popup->show();
    }
};
