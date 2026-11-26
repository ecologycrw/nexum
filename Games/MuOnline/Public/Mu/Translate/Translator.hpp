#pragma once

// The translation of the packets of the world into neutral events (IV.14.4 of the reference spec): the data
// of the character, the players, NPCs and monsters that come into the view and leave it, the walks and the
// corrections, the stats of the hero, the inventory and the changes of the terrain. The variant of each
// packet is the one of the 106.3 dialect (Mu/Protocol/Dialect.hpp), and the fields come from the protocol
// spec of the knowledge center (docs/specs/protocolo/protocolo-106-3-m2.md, sections 6.11, 6.12, 7 and 8) and
// from the findings 2026-11-21-variantes-por-versao-m2.md (section 4) and 2026-11-21-aparencia-estendida.md
// (section 2).
//
// The reactions follow IV.14.6: a packet that cannot be read is logged and dropped whole; an unknown code is
// ignored and logged once; the known packets that this slice does not use (section 6.12 and the C1 B2 1A) are
// ignored without a word.

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "Mu/World/EventQueue.hpp"
#include "Mu/World/WorldEvent.hpp"
#include "Nexum/Debug/Logger.hpp"

namespace mu::translate {

class Translator {
public:
    // `hero` is the id with which the player always sees itself (section 6.5). The log must outlive the
    // translator.
    Translator(world::ObjectId hero, nexum::debug::Logger& log);

    // Translates one message of the world into events at the end of `events`, in the order of the packet.
    // Gives how many events it put there.
    std::size_t Translate(std::span<const std::byte> message, world::EventQueue& events);

private:
    using Message = std::span<const std::byte>;

    // False when the message is not one of the packets this slice translates.
    [[nodiscard]] bool TranslateWorldPacket(Message message, world::EventQueue& events);
    void TranslateHero(Message message, world::EventQueue& events);
    void TranslateStats(Message message, world::EventQueue& events);
    void TranslatePlayer(Message message, world::EventQueue& events);
    void TranslateNpcs(Message message, world::EventQueue& events);
    void TranslateLeaving(Message message, world::EventQueue& events);
    void TranslateWalk(Message message, world::EventQueue& events);
    void TranslateCorrection(Message message, world::EventQueue& events);
    void TranslateInventory(Message message, world::EventQueue& events);
    void TranslateTerrain(Message message, world::EventQueue& events);

    void Put(world::EventQueue& events, const world::WorldEvent& event);
    // Logs and drops a packet that cannot be read.
    void Drop(std::string_view packet, std::string_view reason) noexcept;
    // Logs the first message of an unknown code and ignores the next ones.
    void ReportUnknown(Message message) noexcept;

    world::ObjectId m_hero;
    nexum::debug::Logger* m_log;
    // One bit for each code (and subcode, for the codes that have them) already logged as unknown.
    std::bitset<0x10000> m_reported;
    bool m_queueGrew = false;
};

} // namespace mu::translate
