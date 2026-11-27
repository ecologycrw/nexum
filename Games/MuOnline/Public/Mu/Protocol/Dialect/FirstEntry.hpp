#pragma once

// The 106.3 dialect (Mu/Protocol/Dialect.hpp), on the first entry in the world: what the server sends after
// the data of the character, without waiting for the client (sections 5.2 and 6.12 of the protocol spec, with
// the order of a real session in the finding 2026-11-21-capturas-reais-sessao-106-3.md).

#include "Mu/Protocol/Packets/ServerToClient/AddLetter.hpp"
#include "Mu/Protocol/Packets/ServerToClient/AddSummonedMonstersToScope.hpp"
#include "Mu/Protocol/Packets/ServerToClient/AddTransformedCharacterToScopeExtended.hpp"
#include "Mu/Protocol/Packets/ServerToClient/ApplyKeyConfiguration.hpp"
#include "Mu/Protocol/Packets/ServerToClient/AssignCharacterToGuild.hpp"
#include "Mu/Protocol/Packets/ServerToClient/CashShopBannerVersion.hpp"
#include "Mu/Protocol/Packets/ServerToClient/CashShopScriptVersion.hpp"
#include "Mu/Protocol/Packets/ServerToClient/CastleSiegeTaxRateNotification.hpp"
#include "Mu/Protocol/Packets/ServerToClient/CharacterInventory.hpp"
#include "Mu/Protocol/Packets/ServerToClient/FriendRequest.hpp"
#include "Mu/Protocol/Packets/ServerToClient/ItemsDropped.hpp"
#include "Mu/Protocol/Packets/ServerToClient/LegacyQuestStateList.hpp"
#include "Mu/Protocol/Packets/ServerToClient/MagicEffectStatus.hpp"
#include "Mu/Protocol/Packets/ServerToClient/MasterStatsUpdateExtended.hpp"
#include "Mu/Protocol/Packets/ServerToClient/MessengerInitialization.hpp"
#include "Mu/Protocol/Packets/ServerToClient/MuHelperConfigurationData.hpp"
#include "Mu/Protocol/Packets/ServerToClient/QuestStateList.hpp"
#include "Mu/Protocol/Packets/ServerToClient/ServerMessage.hpp"
#include "Mu/Protocol/Packets/ServerToClient/SkillListUpdate.hpp"
#include "Mu/Protocol/Packets/ServerToClient/WarpListExtended.hpp"
#include "Mu/Protocol/Packets/ServerToClient/WeatherStatusUpdate.hpp"

namespace mu::protocol::dialect {

// What the M2 reads: the inventory, with the extended item (section 7.1).
using CharacterInventory = server::CharacterInventory;
using MasterStatsUpdate = server::MasterStatsUpdateExtended;
using SkillListUpdate = server::SkillListUpdate;

// What the M2 does not use. The client knows these packets so that it ignores them without a warning; only
// their code and subcode matter.
using KeyConfiguration = server::ApplyKeyConfiguration;
using LegacyQuestStates = server::LegacyQuestStateList;
using QuestStates = server::QuestStateList;
using CashShopScriptVersion = server::CashShopScriptVersion;
using CashShopBannerVersion = server::CashShopBannerVersion;
using WarpList = server::WarpListExtended;
using ServerMessage = server::ServerMessage;
using AddTransformedCharacterToScope = server::AddTransformedCharacterToScopeExtended;
using AddSummonedMonstersToScope = server::AddSummonedMonstersToScope;
using ItemsDropped = server::ItemsDropped;
using Weather = server::WeatherStatusUpdate;
using HelperConfiguration = server::MuHelperConfigurationData;
using GameMasterEffect = server::MagicEffectStatus;
using GuildAssignment = server::AssignCharacterToGuild;
using MessengerInitialization = server::MessengerInitialization;
using FriendRequest = server::FriendRequest;
using Letter = server::AddLetter;
using CastleSiegeTaxRate = server::CastleSiegeTaxRateNotification;

} // namespace mu::protocol::dialect
