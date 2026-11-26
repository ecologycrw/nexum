#include "Mu/Session/Ping.hpp"

#include <cmath>

namespace mu::session {

std::uint32_t ClientMilliseconds(nexum::time::Seconds now) noexcept {
    const double milliseconds = std::floor(now.count() * 1000.0);
    if (!std::isfinite(milliseconds) || milliseconds < 0.0) {
        return 0;
    }
    return static_cast<std::uint32_t>(std::fmod(milliseconds, 4294967296.0));
}

nexum::core::Result<PingMessage> WritePing(std::uint32_t milliseconds, std::uint16_t attackSpeed,
                                           std::uint16_t magicSpeed) {
    PingMessage message{};
    auto writer = protocol::dialect::Ping::Writer::Create(message);
    if (!writer) {
        return tl::unexpected(writer.error());
    }
    writer->SetTickCount(milliseconds);
    writer->SetAttackSpeed(attackSpeed);
    writer->SetMagicSpeed(magicSpeed);
    return message;
}

void PingClock::Start(nexum::time::Seconds now) noexcept {
    m_timer.emplace(now, kPingPeriod);
}

void PingClock::Stop() noexcept {
    m_timer.reset();
}

void PingClock::SetSpeeds(std::uint16_t attackSpeed, std::uint16_t magicSpeed) noexcept {
    m_attackSpeed = attackSpeed;
    m_magicSpeed = magicSpeed;
}

std::optional<nexum::core::Result<PingMessage>> PingClock::Due(nexum::time::Seconds now) {
    if (!m_timer || !m_timer->Fire(now)) {
        return std::nullopt;
    }
    return WritePing(ClientMilliseconds(now), m_attackSpeed, m_magicSpeed);
}

} // namespace mu::session
