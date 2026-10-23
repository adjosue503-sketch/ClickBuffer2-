#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

enum class Kind { Hold, Repress, None };

static Kind kindOf(PlayerObject* p) {
    if (p->m_isShip || p->m_isBird || p->m_isDart || p->m_isSwing) return Kind::None;
    if (p->m_isBall || p->m_isSpider) return Kind::Repress;
    return Kind::Hold;
}

class $modify(ClickBufferLayer, GJBaseGameLayer) {
    struct Fields {
        bool pending = false;
        bool repress = false;
        bool releasedEarly = false;
        bool releaseNext = false;
        bool injecting = false;
        float timeLeft = 0.f;
    };

    void sendButton(bool down) {
        m_fields->injecting = true;
        GJBaseGameLayer::handleButton(down, 1, true);
        m_fields->injecting = false;
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        if (!Mod::get()->getSettingValue<bool>("enabled") ||
            m_fields->injecting || button != 1 || !isPlayer1 || !m_player1) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        if (down) {
            auto kind = kindOf(m_player1);
            if (!m_player1->m_isOnGround && kind != Kind::None) {
                m_fields->pending = true;
                m_fields->repress = (kind == Kind::Repress);
                m_fields->releasedEarly = false;
                m_fields->timeLeft =
                    Mod::get()->getSettingValue<int64_t>("buffer-ms") / 1000.f;
            }
        } else if (m_fields->pending) {
            m_fields->releasedEarly = true;
            if (!m_fields->repress) return;
        }

        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void update(float dt) {
        GJBaseGameLayer::update(dt);

        if (m_fields->releaseNext) {
            m_fields->releaseNext = false;
            this->sendButton(false);
        }

        if (!m_fields->pending || !m_player1) return;

        m_fields->timeLeft -= dt;
        if (m_player1->m_isOnGround) {
            m_fields->pending = false;
            if (m_fields->repress) {
                if (!m_fields->releasedEarly) this->sendButton(false);
                this->sendButton(true);
            }
            if (m_fields->releasedEarly) m_fields->releaseNext = true;
        } else if (m_fields->timeLeft <= 0.f) {
            m_fields->pending = false;
            if (m_fields->releasedEarly && !m_fields->repress) this->sendButton(false);
        }
    }
};
