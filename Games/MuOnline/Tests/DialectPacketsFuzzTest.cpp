// Fuzz target of the reading of the packets the client receives in the 106.3 dialect
// (cmake/NexumFuzzing.cmake): any bytes, taken as one message, are read as each of those packets, and every
// field of the ones that accept the bytes is read, the lists, the items and the appearances included. The
// same message is then translated into events of the world, which a view of the world applies. Nothing reads
// past the end and nothing crashes.
#include <cstddef>
#include <cstdint>
#include <span>

#include "Mu/Protocol/Dialect/ConnectServer.hpp"
#include "Mu/Protocol/Dialect/FirstEntry.hpp"
#include "Mu/Protocol/Dialect/Login.hpp"
#include "Mu/Protocol/Dialect/World.hpp"
#include "Mu/Protocol/ExtendedAppearance.hpp"
#include "Mu/Protocol/ExtendedItem.hpp"
#include "Mu/Session/SessionEpoch.hpp"
#include "Mu/Translate/Translator.hpp"
#include "Mu/World/EventQueue.hpp"
#include "Mu/World/WorldView.hpp"
#include "Nexum/Debug/Logger.hpp"

namespace {

namespace dialect = mu::protocol::dialect;

// A log output that keeps nothing: the lines are still built, so building them is fuzzed too.
class SilentLogSink final : public nexum::debug::LogSink {
public:
    void Write(const nexum::debug::LogRecord& /*record*/) noexcept override {}
};

// Keeps the values that were read, so the reading cannot be left out of the build.
class Sink {
public:
    template <typename Value>
    void Take(Value value) noexcept {
        m_sum = m_sum + static_cast<std::uint64_t>(value);
    }
    void Take(std::span<const std::byte> bytes) noexcept { Take(bytes.size()); }
    void Take(mu::protocol::PacketText text) noexcept { Take(text.size()); }
    void Take(const mu::characters::Appearance& appearance) noexcept {
        Take(appearance.characterClass);
        Take(appearance.gameMaster);
    }

private:
    volatile std::uint64_t m_sum = 0;
};

template <typename Packet, typename ReadFields>
void TryRead(std::span<const std::byte> message, ReadFields&& readFields) {
    const auto view = Packet::View::Read(message);
    if (view) {
        readFields(*view);
    }
}

void ReadConnectServer(std::span<const std::byte> message, Sink& sink) {
    TryRead<dialect::Hello>(message, [](const auto&) {});
    TryRead<dialect::ServerListResponse>(message, [&](const auto& list) {
        sink.Take(list.ServerCount());
        for (std::size_t index = 0; index < list.ServersCount(); ++index) {
            if (const auto server = list.Servers(index)) {
                sink.Take(server->ServerId());
                sink.Take(server->LoadPercentage());
            }
        }
    });
    TryRead<dialect::ConnectionInfo>(message, [&](const auto& info) {
        sink.Take(info.IpAddress());
        sink.Take(info.Port());
    });
}

void ReadLogin(std::span<const std::byte> message, Sink& sink) {
    TryRead<dialect::GameServerEntered>(message, [&](const auto& entered) {
        sink.Take(entered.Success());
        sink.Take(entered.PlayerId());
        sink.Take(entered.VersionString());
    });
    TryRead<dialect::LoginResponse>(message, [&](const auto& response) { sink.Take(response.Success()); });
    TryRead<dialect::CharacterList>(message, [&](const auto& list) {
        sink.Take(list.UnlockFlags());
        sink.Take(list.CharacterCount());
        sink.Take(list.IsVaultExtended());
        for (std::size_t index = 0; index < list.CharactersCount(); ++index) {
            if (const auto character = list.Characters(index)) {
                sink.Take(character->SlotIndex());
                sink.Take(character->Name());
                sink.Take(character->Level());
                sink.Take(character->Status());
                sink.Take(mu::protocol::ReadExtendedAppearance(character->Appearance()));
                sink.Take(character->GuildPosition());
            }
        }
    });
    TryRead<dialect::CharacterClassCreationUnlock>(
        message, [&](const auto& unlock) { sink.Take(unlock.UnlockFlags()); });
}

void ReadCharacter(std::span<const std::byte> message, Sink& sink) {
    TryRead<dialect::CharacterInformation>(message, [&](const auto& character) {
        sink.Take(character.X());
        sink.Take(character.Y());
        sink.Take(character.MapId());
        sink.Take(character.CurrentExperience());
        sink.Take(character.CurrentHealth());
        sink.Take(character.MaximumHealth());
        sink.Take(character.Money());
        sink.Take(character.HeroState());
        sink.Take(character.Status());
        sink.Take(character.AttackSpeed());
        sink.Take(character.MagicSpeed());
        sink.Take(character.Resets());
        sink.Take(character.MapDiscriminator());
    });
    // Each entry of the inventory is the slot, then the item (section 6.12 of the protocol spec).
    TryRead<dialect::CharacterInventory>(message, [&](const auto& inventory) {
        sink.Take(inventory.ItemCount());
        const auto items = inventory.ItemsBytes();
        for (std::size_t start = 0; start < items.size();) {
            sink.Take(items[start]);
            const auto item = mu::protocol::ReadExtendedItem(items.subspan(start + 1));
            sink.Take(item.item.number);
            start += 1 + item.size;
        }
    });
    TryRead<dialect::CurrentStats>(message, [&](const auto& stats) {
        sink.Take(stats.Health());
        sink.Take(stats.Shield());
        sink.Take(stats.Mana());
        sink.Take(stats.Ability());
        sink.Take(stats.AttackSpeed());
        sink.Take(stats.MagicSpeed());
    });
    TryRead<dialect::MasterStatsUpdate>(message, [](const auto&) {});
    TryRead<dialect::SkillListUpdate>(message, [](const auto&) {});
}

void ReadScope(std::span<const std::byte> message, Sink& sink) {
    TryRead<dialect::AddCharacterToScope>(message, [&](const auto& player) {
        sink.Take(player.Id());
        sink.Take(player.CurrentPositionX());
        sink.Take(player.TargetPositionY());
        sink.Take(player.Rotation());
        sink.Take(player.HeroState());
        sink.Take(player.AttackSpeed());
        sink.Take(player.Name());
        sink.Take(mu::protocol::ReadExtendedAppearance(player.AppearanceAndEffects()));
    });
    TryRead<dialect::AddNpcsToScope>(message, [&](const auto& npcs) {
        sink.Take(npcs.NpcCount());
        for (std::size_t index = 0; index < npcs.NPCsCount(); ++index) {
            if (const auto npc = npcs.NPCs(index)) {
                sink.Take(npc->Id());
                sink.Take(npc->TypeNumber());
                sink.Take(npc->CurrentPositionX());
                sink.Take(npc->TargetPositionY());
                sink.Take(npc->Rotation());
                sink.Take(npc->EffectCount());
            }
        }
    });
    TryRead<dialect::MapObjectOutOfScope>(message, [&](const auto& out) {
        sink.Take(out.ObjectCount());
        for (std::size_t index = 0; index < out.ObjectsCount(); ++index) {
            if (const auto object = out.Objects(index)) {
                sink.Take(object->Id());
            }
        }
    });
}

void ReadMovement(std::span<const std::byte> message, Sink& sink) {
    TryRead<dialect::ObjectWalked>(message, [&](const auto& walked) {
        sink.Take(walked.ObjectId());
        sink.Take(walked.SourceX());
        sink.Take(walked.TargetY());
        sink.Take(walked.TargetRotation());
        sink.Take(walked.StepCount());
        sink.Take(walked.StepData());
    });
    TryRead<dialect::ObjectMoved>(message, [&](const auto& moved) {
        sink.Take(moved.ObjectId());
        sink.Take(moved.PositionX());
        sink.Take(moved.PositionY());
    });
    TryRead<dialect::ChangeTerrainAttributes>(message, [&](const auto& terrain) {
        sink.Take(terrain.Type());
        sink.Take(terrain.Attribute());
        sink.Take(terrain.RemoveAttribute());
        sink.Take(terrain.AreaCount());
        for (std::size_t index = 0; index < terrain.AreasCount(); ++index) {
            if (const auto area = terrain.Areas(index)) {
                sink.Take(area->StartX());
                sink.Take(area->EndY());
            }
        }
    });
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const auto message = std::as_bytes(std::span<const std::uint8_t>(data, size));
    Sink sink;
    ReadConnectServer(message, sink);
    ReadLogin(message, sink);
    ReadCharacter(message, sink);
    ReadScope(message, sink);
    ReadMovement(message, sink);

    SilentLogSink silent;
    nexum::debug::Logger log{silent, nexum::debug::LogLevel::Trace};
    mu::translate::Translator translator{mu::world::ObjectId{0x0200}, log};
    mu::world::EventQueue events{16};
    static_cast<void>(translator.Translate(mu::session::SessionEpoch{1}, message, events));
    mu::world::WorldView view{log};
    events.Drain(
        [&](mu::session::SessionEpoch /*epoch*/, const mu::world::WorldEvent& event) { view.Apply(event); });
    sink.Take(view.Objects().size());
    return 0;
}
