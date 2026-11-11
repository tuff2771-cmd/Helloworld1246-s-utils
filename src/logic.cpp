#include "config.hpp"
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/RingObject.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

using namespace geode::prelude;
using namespace cheat;

namespace {

struct ObjCache {
    GJBaseGameLayer* owner = nullptr;
    CCArray* array = nullptr;
    size_t count = 0;
    std::vector<GameObject*> dash;
    std::vector<GameObject*> black;
    std::vector<GameObject*> pads;
    std::vector<GameObject*> gravPads;
    std::vector<GameObject*> gravOrbs;
    std::vector<std::pair<float, GameObject*>> solids;

    void reset() {
        owner = nullptr;
        array = nullptr;
        count = 0;
        dash.clear();
        black.clear();
        pads.clear();
        gravPads.clear();
        gravOrbs.clear();
        solids.clear();
    }
};

ObjCache g_cache;

struct PlayerState {
    bool waveActive = false;
    bool haveY = false;
    float lastY = 0.f;
};

PlayerState g_state[2];
unsigned long long g_frame = 0;

void rebuildCache(GJBaseGameLayer* l) {
    g_cache.reset();
    g_cache.owner = l;
    g_cache.array = l->m_objects;
    if (!l->m_objects) return;
    g_cache.count = l->m_objects->count();

    for (auto obj : CCArrayExt<GameObject*>(l->m_objects)) {
        if (!obj) continue;
        switch (obj->m_objectID) {
            case 1704: g_cache.dash.push_back(obj); break;
            case 1330: g_cache.black.push_back(obj); break;
            case 35: case 140: case 1332: g_cache.pads.push_back(obj); break;
            case 67: g_cache.gravPads.push_back(obj); break;
            case 84: case 1022: g_cache.gravOrbs.push_back(obj); break;
            default:
                if (obj->m_objectType == GameObjectType::Solid) {
                    g_cache.solids.emplace_back(obj->getPositionX(), obj);
                }
                break;
        }
    }
    std::sort(g_cache.solids.begin(), g_cache.solids.end(),
        [](auto const& a, auto const& b) { return a.first < b.first; });
}

int countNear(PlayerObject* p, std::vector<GameObject*> const& v, float rx, float ry) {
    auto pp = p->getPosition();
    int n = 0;
    for (auto* o : v) {
        auto op = o->getPosition();
        if (std::fabs(op.x - pp.x) < rx && std::fabs(op.y - pp.y) < ry) ++n;
    }
    return n;
}

float scanGapCenter(PlayerObject* p, bool& found) {
    float const inf = std::numeric_limits<float>::infinity();
    float px = p->getPositionX();
    float py = p->getPositionY();
    float above = inf;
    float below = -inf;

    auto it = std::lower_bound(
        g_cache.solids.begin(), g_cache.solids.end(), px - 160.f,
        [](auto const& e, float v) { return e.first < v; }
    );
    for (; it != g_cache.solids.end() && it->first < px + 200.f; ++it) {
        CCRect r = it->second->getObjectRect();
        if (r.origin.x > px + 60.f || r.origin.x + r.size.width < px - 20.f) continue;
        float bottom = r.origin.y;
        float top = r.origin.y + r.size.height;
        if (bottom > py) above = std::min(above, bottom);
        else if (top < py) below = std::max(below, top);
    }
    found = above < inf && below > -inf;
    return found ? (above + below) / 2.f : 0.f;
}

void tick(GJBaseGameLayer* l, float dt) {
    size_t objCount = l->m_objects ? l->m_objects->count() : 0;
    if (g_cache.owner != l || g_cache.array != l->m_objects || g_cache.count != objCount) {
        rebuildCache(l);
        g_state[0] = PlayerState{};
        g_state[1] = PlayerState{};
        g_frame = 0;
    }
    ++g_frame;
    bool dual = l->m_gameState.m_isDualMode;

    for (int idx = 0; idx < 2; ++idx) {
        PlayerObject* p = idx == 0 ? l->m_player1 : l->m_player2;
        if (!p || p->m_isDead) continue;
        if (idx == 1 && !dual) continue;

        bool isP1 = idx == 0;
        auto& st = g_state[idx];
        auto pick = [&](char const* a, char const* b) { return Cfg::b(isP1 ? a : b); };
        auto pickF = [&](char const* a, char const* b) { return Cfg::get(isP1 ? a : b); };

        float y = p->getPositionY();
        float vy = 0.f;
        if (st.haveY && dt > 0.f) vy = (y - st.lastY) / (dt * 60.f);
        st.lastY = y;
        st.haveY = true;
        float gdir = p->m_isUpsideDown ? -1.f : 1.f;
        float effVel = vy * gdir;

        int swift = 0;
        int hold = -1;

        if (Cfg::b("green_dash") && pick("gd_p1", "gd_p2")) {
            swift += countNear(p, g_cache.dash, 50.f, 50.f);
        }

        bool easyUfo = Cfg::b("easy_black_ufo") && p->m_isBird;
        if (Cfg::b("black_orb") || easyUfo) {
            bool movingTowardOrbDirection = effVel < -0.1f;
            if (!(easyUfo && movingTowardOrbDirection)) {
                swift += countNear(p, g_cache.black, 50.f, 50.f);
            }
        }

        if (Cfg::b("jump_pads") && countNear(p, g_cache.pads, 36.f, 36.f) > 0) swift += 1;
        if (Cfg::b("grav_pads") && countNear(p, g_cache.gravPads, 36.f, 36.f) > 0) swift += 1;

        if (Cfg::b("auto_swift")) swift += 1;

        if (Cfg::b(isP1 ? "autoclick_p1" : "autoclick_p2")) {
            if (pick("ac1_swift", "ac2_swift")) {
                swift += 1;
            } else {
                int every = std::max(1, static_cast<int>(pickF("ac1_every", "ac2_every")));
                int holdFrames = std::clamp(static_cast<int>(pickF("ac1_hold", "ac2_hold")), 1, every);
                hold = (g_frame % static_cast<unsigned long long>(every)) < static_cast<unsigned long long>(holdFrames) ? 1 : 0;
            }
        }

        if (Cfg::b("straight_fly") && p->m_isShip && pick("sf_p1", "sf_p2")) {
            float thr = pickF("sf_thr_p1", "sf_thr_p2");
            if (effVel > thr) hold = 0;
            else if (effVel < -thr) hold = 1;
        }

        if (Cfg::b("straight_ufo") && p->m_isBird && pick("su_p1", "su_p2")) {
            float thr = pickF("su_thr_p1", "su_thr_p2");
            float below = (Cfg::get("su_y") - y) * gdir;
            if (below > thr && effVel <= 0.f) swift += 1;
        }

        if (Cfg::b("wave_spam") && p->m_isDart && Cfg::b(isP1 ? "wv1_on" : "wv2_on")) {
            float thr = Cfg::get(isP1 ? "wv1_thr" : "wv2_thr");
            bool found = false;
            float center = scanGapCenter(p, found);
            if (found) {
                st.waveActive = true;
                float dy = (y - center) * gdir;
                if (dy < -thr) hold = 1;
                else if (dy > thr) hold = 0;
                else hold = (g_frame & 1ULL) ? 1 : 0;
            } else if (st.waveActive) {
                st.waveActive = false;
                hold = 0; // left the gap: let go once, then give control back
            }

            if (Cfg::b(isP1 ? "wv1_blue" : "wv2_blue")) {
                swift += countNear(p, g_cache.gravOrbs, 50.f, 50.f);
            }
        } else {
            st.waveActive = false;
        }

        bool swiftSent = false;
        if (swift > 0) {
            swift = std::min(swift, 6);
            for (int k = 0; k < swift; ++k) {
                l->handleButton(true, 1, isP1);
                l->handleButton(false, 1, isP1);
            }
            swiftSent = true;
        }
        if (hold != -1) {
            // Compare with the real button state of the game, not a remembered one.
            bool down = swiftSent ? false : p->buttonDown(PlayerButton::Jump);
            if ((hold == 1) != down) l->handleButton(hold == 1, 1, isP1);
        }
    }
}

bool isActivePlayLayer(GJBaseGameLayer* l) {
    auto pl = PlayLayer::get();
    return pl && static_cast<GJBaseGameLayer*>(pl) == l;
}

} // namespace

class $modify(CheatGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        if (isActivePlayLayer(this)) tick(this, dt);
        GJBaseGameLayer::update(dt);
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        if (!isActivePlayLayer(this) || button != 1 || !Cfg::b("link_2p") || !m_gameState.m_isDualMode) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }
        bool driverIsP1 = Cfg::i("link_driver") == 0;
        if (isPlayer1 != driverIsP1) return;

        GJBaseGameLayer::handleButton(down, button, isPlayer1);
        bool followerDown = Cfg::i("link_mode") == 1 ? !down : down;
        GJBaseGameLayer::handleButton(followerDown, button, !isPlayer1);
    }
};

class $modify(CheatPlayLayer, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        g_cache.reset();
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (Cfg::b("noclip")) {
            if (player == m_player1 && Cfg::b("nc_p1")) return;
            if (player == m_player2 && Cfg::b("nc_p2")) return;
        }
        PlayLayer::destroyPlayer(player, object);
    }
};

class $modify(CheatPlayer, PlayerObject) {
    void flipGravity(bool flip, bool noEffects) {
        if (Cfg::b("maintain_grav")) {
            if (auto pl = PlayLayer::get()) {
                if (this == pl->m_player1 && Cfg::b("mg_p1")) return;
                if (this == pl->m_player2 && Cfg::b("mg_p2")) return;
            }
        }
        PlayerObject::flipGravity(flip, noEffects);
    }
};

class $modify(CheatRing, RingObject) {
    void spawnCircle() {
        if (Cfg::b("no_orb_fx")) return;
        RingObject::spawnCircle();
    }
};
