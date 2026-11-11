#pragma once
#include <Geode/Geode.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace cheat {

enum class OptType { Toggle, Number, Choice };

struct Opt {
    std::string key;
    std::string label;
    OptType type = OptType::Toggle;
    float def = 0.f;
    std::vector<std::string> choices;
};

struct Feature {
    std::string key;
    std::string label;
    std::vector<Opt> opts;
};

inline Opt tog(std::string key, std::string label, float def = 1.f) {
    Opt o;
    o.key = std::move(key);
    o.label = std::move(label);
    o.type = OptType::Toggle;
    o.def = def;
    return o;
}

inline Opt num(std::string key, std::string label, float def) {
    Opt o;
    o.key = std::move(key);
    o.label = std::move(label);
    o.type = OptType::Number;
    o.def = def;
    return o;
}

inline Opt choice(std::string key, std::string label, std::vector<std::string> choices, float def = 0.f) {
    Opt o;
    o.key = std::move(key);
    o.label = std::move(label);
    o.type = OptType::Choice;
    o.def = def;
    o.choices = std::move(choices);
    return o;
}

inline std::vector<Feature> const& features() {
    static std::vector<Feature> const list = {
        {"green_dash", "Click Green Dash Orbs",
            {tog("gd_p1", "Player 1"), tog("gd_p2", "Player 2")}},
        {"no_orb_fx", "No Orb Effects", {}},
        {"black_orb", "Click Black Orbs", {}},
        {"easy_black_ufo", "Easy Click Black Orb UFO", {}},
        {"jump_pads", "Click Jump Pads", {}},
        {"grav_pads", "Click Gravity Pads", {}},
        {"maintain_grav", "Maintain Gravity",
            {tog("mg_p1", "Player 1"), tog("mg_p2", "Player 2")}},
        {"straight_fly", "Auto Straight Fly",
            {tog("sf_p1", "Player 1"), tog("sf_p2", "Player 2"),
             num("sf_thr_p1", "Threshold P1", 0.5f), num("sf_thr_p2", "Threshold P2", 0.5f)}},
        {"straight_ufo", "Auto Straight UFO",
            {tog("su_p1", "Player 1"), tog("su_p2", "Player 2"),
             num("su_thr_p1", "Threshold P1", 4.f), num("su_thr_p2", "Threshold P2", 4.f),
             num("su_y", "Y Position Target", 150.f)}},
        {"autoclick_p1", "AutoClicker Player 1",
            {num("ac1_every", "Click every (frames)", 10.f), num("ac1_hold", "Hold for (frames)", 2.f),
             tog("ac1_swift", "Swift", 0.f)}},
        {"autoclick_p2", "AutoClicker Player 2",
            {num("ac2_every", "Click every (frames)", 10.f), num("ac2_hold", "Hold for (frames)", 2.f),
             tog("ac2_swift", "Swift", 0.f)}},
        {"noclip", "Noclip",
            {tog("nc_p1", "Player 1"), tog("nc_p2", "Player 2")}},
        {"auto_swift", "Auto Swift", {}},
        {"link_2p", "2P Control Link",
            {choice("link_mode", "Mode", {"Copy", "Invert"}),
             choice("link_driver", "Driving side", {"Player 1", "Player 2"})}},
                {"wave_spam", "Moving Wave Gap Spam",
            {tog("wv1_on", "Player 1"), num("wv1_thr", "P1 Threshold", 4.f),
             tog("wv1_blue", "P1 Blue Orb", 0.f),
             tog("wv2_on", "Player 2"), num("wv2_thr", "P2 Threshold", 4.f),
             tog("wv2_blue", "P2 Blue Orb", 0.f)}},
    return list;
}

namespace Cfg {

inline std::unordered_map<std::string, float>& map() {
    static std::unordered_map<std::string, float> m;
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        auto* mod = geode::Mod::get();
        for (auto const& f : features()) {
            m[f.key] = mod->getSavedValue<float>(f.key, 0.f);
            for (auto const& o : f.opts) {
                m[o.key] = mod->getSavedValue<float>(o.key, o.def);
            }
        }
    }
    return m;
}

inline float get(std::string const& key) {
    auto& m = map();
    auto it = m.find(key);
    return it == m.end() ? 0.f : it->second;
}

inline bool b(std::string const& key) { return get(key) > 0.5f; }
inline int i(std::string const& key) { return static_cast<int>(get(key)); }

inline void set(std::string const& key, float value) {
    map()[key] = value;
    geode::Mod::get()->setSavedValue<float>(key, value);
}

} // namespace Cfg
} // namespace cheat
