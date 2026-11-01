#pragma once

#include "plugin_interface.h"
#include "Engine_classes.hpp"
#include "Chimera_classes.hpp"
#include "Chimera_structs.hpp"
#include "MassCommon_structs.hpp"
#include "MassSignals_classes.hpp"
#include "AuItems_structs.hpp"
#include "plugin_map.h"

#include <cstdint>
#include <iterator>
#include <vector>

// Every member of a game class the production viewer reads, taken from the
// loader's map for the running game build (see plugin_map.h). Resolved once in
// OnPluginLoadHooks; read on the game thread with In(object), which is null
// when the object is null or the member did not resolve. Structs that are only
// stepped through (an array's elements, a stack copy handed to the game) are
// covered by a size check. Engine members (UObject, UWorld, FVector, the
// refcount of a TSharedPtr controller) stay compiled in.

#define PRODUCTIONVIEWER_FIELD(Class, Member) SR_FIELD(decltype(SDK::Class::Member), Class, Member, 0)

namespace ProductionViewerFields
{
	inline auto ItemUniqueName = PRODUCTIONVIEWER_FIELD(UAuItemDataBase, UniqueItemName);
	inline auto ItemName       = PRODUCTIONVIEWER_FIELD(UAuItemDataBase, ItemName);
	inline auto ItemIcon       = PRODUCTIONVIEWER_FIELD(UAuItemDataBase, ItemIcon);

	inline auto CraftingSelectedRecipe = PRODUCTIONVIEWER_FIELD(FCrCraftingFragment, SelectedRecipe);
	inline auto CraftingCurrentRecipe  = PRODUCTIONVIEWER_FIELD(FCrCraftingFragment, CurrentRecipe);
	inline auto CraftingMultiplier     = PRODUCTIONVIEWER_FIELD(FCrCraftingFragment, CraftingMultiplier);

	// An item stack, read off a recipe's output and inputs.
	inline auto ItemStackData  = PRODUCTIONVIEWER_FIELD(FAuSimpleItem, ItemDataBase);
	inline auto ItemStackCount = PRODUCTIONVIEWER_FIELD(FAuSimpleItem, Count);
	inline auto ItemStackSize  = SR_SIZE_CHECK(FAuSimpleItem);

	// A stack copy of the helper is handed to the game's constructor, so the
	// whole struct is checked against the running build.
	inline auto ReplicationHelperSize = SR_SIZE_CHECK(FCrMassEntityReplicationHelper);

	// The world a UMassSignalSubsystem was created in, and the transform a
	// Mass entity's FTransformFragment holds. Both are reflected.
	inline auto SignalSubsystemWorld       = PRODUCTIONVIEWER_FIELD(UMassSignalSubsystem, CachedWorld);
	inline auto TransformFragmentTransform = PRODUCTIONVIEWER_FIELD(FTransformFragment, Transform);
	inline auto TransformTranslation       = PRODUCTIONVIEWER_FIELD(FTransform, Translation);

	// A TSharedPtr<T>: the object pointer and the shared reference controller.
	struct SharedRef
	{
		void* Object;
		void* Controller;
	};
	static_assert(sizeof(SharedRef) == 0x10, "a TSharedPtr is two pointers");

	// UMassEntitySubsystem::EntityManager is a TSharedPtr the Game-SDK folds
	// into Pad_38 (not reflected), and FCrGridEntityHandle's two members sit in
	// a struct the SDK keeps as padding. Their rows below are CL-127004's, the
	// build this plugin was compiled against; every other build comes from the
	// loader's map, and a build with neither turns off the feature using them.
	inline auto EntityManager = PluginField<SharedRef>{ { "UMassEntitySubsystem::EntityManager",
		"UMassEntitySubsystem", "EntityManager", PLUGIN_FIELD_NATIVE, sizeof(SharedRef), 0x38 } };
	inline auto GridEntityHandle = PluginField<uint64_t>{ { "FCrGridEntityHandle::EntityHandle",
		"FCrGridEntityHandle", "EntityHandle", PLUGIN_FIELD_NATIVE, sizeof(uint64_t), 0x0 } };
	inline auto GridEntityData = PluginField<SharedRef>{ { "FCrGridEntityHandle::Data",
		"FCrGridEntityHandle", "Data", PLUGIN_FIELD_NATIVE, sizeof(SharedRef), 0x8 } };
	inline auto GridEntityHandleSize = SR_SIZE_CHECK(FCrGridEntityHandle);

	inline const PluginFieldCandidate kEntityManagerRows[] = {
		{ 127004, 0x38, static_cast<uint32_t>(sizeof(SharedRef)), 0, nullptr },
	};
	inline const PluginFieldCandidate kGridEntityHandleRows[] = {
		{ 127004, 0x0, static_cast<uint32_t>(sizeof(uint64_t)), 0, nullptr },
	};
	inline const PluginFieldCandidate kGridEntityDataRows[] = {
		{ 127004, 0x8, static_cast<uint32_t>(sizeof(SharedRef)), 0, nullptr },
	};

	// UCrGameInstance::ServerSessionName is folded into the SDK's trailing padding,
	// so it has no name there: a native row. The row below is CL-127004's, the
	// build this plugin was compiled against; every other build comes from the
	// loader's map.
	inline auto ServerSessionName = PluginField<SDK::FString>{ { "UCrGameInstance::ServerSessionName",
		"UCrGameInstance", "ServerSessionName", PLUGIN_FIELD_NATIVE, sizeof(SDK::FString), 0x290 } };

	inline const PluginFieldCandidate kServerSessionNameRows[] = {
		{ 127004, 0x290, sizeof(SDK::FString), 0, nullptr },
	};

	// The map keys of a feature's fields, as the loader sees them.
	inline void AddKey(std::vector<const char*>& keys, const char* key) { keys.push_back(key); }
	inline void AddKey(std::vector<const char*>& keys, const PluginFieldSpec& field) { keys.push_back(field.key); }

	template <class... Key>
	std::vector<const char*> KeysOf(const Key&... keys)
	{
		std::vector<const char*> all;
		(AddKey(all, keys), ...);
		return all;
	}

	// The parts of the plugin that can be off while the rest runs. A key the
	// loader cannot resolve on a game build turns off the part that needs it;
	// the others carry on. A key two parts need is listed in both. The viewer
	// window and the network stay core.
	inline PluginMap::Feature Tracking{ "tracking", "Counting what factories craft",
		KeysOf("UWorld::GetSubsystem<UMassEntitySubsystem>", "GetMassFragment<FCrCraftingFragment>",
			"UMassSignalSubsystem::SignalEntity",
			CraftingSelectedRecipe, CraftingCurrentRecipe, CraftingMultiplier,
			ItemUniqueName, ItemName, ItemStackData, ItemStackCount, ItemStackSize,
			EntityManager, SignalSubsystemWorld), {} };
	inline PluginMap::Feature BaseCores{ "basecores", "Splitting production by base",
		KeysOf("UCrBaseCoreSubsystem::GetBaseCoresRangedBuilding",
			"FCrMassEntityReplicationHelper::FCrMassEntityReplicationHelper",
			"UCrBuildingCustomNameSubsystem::GetBuildingCustomNameByReplicationHelper",
			ReplicationHelperSize, GridEntityHandle, GridEntityData, GridEntityHandleSize,
			EntityManager, TransformFragmentTransform, TransformTranslation), {} };
	inline PluginMap::Feature ItemIcons{ "icons", "Item icons", KeysOf(ItemIcon, ItemUniqueName), {} };
	inline PluginMap::Feature SessionSave{ "sessionsave", "Keeping totals for each saved game",
		KeysOf(ServerSessionName), {} };

	// Call before Resolve and ProductionTracker::ResolvePatterns, so a miss
	// lands on a feature and not on the plugin.
	inline void DeclareFeatures(IPluginSelf* self, IPluginHookScanner* scanner)
	{
		PluginMap::DeclareFeatures(scanner, self, Tracking, BaseCores, ItemIcons, SessionSave);
	}

	// Asks the loader for every row above. False when any did not resolve; the
	// loader has then recorded the failure and refuses the plugin.
	inline bool Resolve(IPluginSelf* self, IPluginHookScanner* scanner)
	{
		bool all = PluginMap::ResolveFields(scanner, self,
			ItemUniqueName, ItemName, ItemIcon,
			CraftingSelectedRecipe, CraftingCurrentRecipe, CraftingMultiplier,
			ItemStackData, ItemStackCount, ItemStackSize, ReplicationHelperSize,
			SignalSubsystemWorld, TransformFragmentTransform, TransformTranslation, GridEntityHandleSize);
		all = PluginMap::ResolveField(scanner, self, ServerSessionName, kServerSessionNameRows,
			static_cast<uint32_t>(std::size(kServerSessionNameRows))) && all;
		all = PluginMap::ResolveField(scanner, self, EntityManager, kEntityManagerRows,
			static_cast<uint32_t>(std::size(kEntityManagerRows))) && all;
		all = PluginMap::ResolveField(scanner, self, GridEntityHandle, kGridEntityHandleRows,
			static_cast<uint32_t>(std::size(kGridEntityHandleRows))) && all;
		all = PluginMap::ResolveField(scanner, self, GridEntityData, kGridEntityDataRows,
			static_cast<uint32_t>(std::size(kGridEntityDataRows))) && all;
		return all;
	}

	// The member's value, or a fallback when the member did not resolve.
	template <class T, class Object>
	T ValueOr(const PluginField<T>& field, const Object* object, T fallback = T{})
	{
		const T* at = field.In(static_cast<const void*>(object));
		return at ? *at : fallback;
	}

	// The FMassEntityManager* a UMassEntitySubsystem holds, or null when the
	// subsystem is null or the member did not resolve.
	inline void* EntityManagerOf(const void* subsystem)
	{
		const SharedRef* at = EntityManager.In(subsystem);
		return at ? at->Object : nullptr;
	}
}
