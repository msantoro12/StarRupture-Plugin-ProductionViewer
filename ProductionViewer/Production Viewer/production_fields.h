#pragma once

#include "plugin_interface.h"
#include "Engine_classes.hpp"
#include "Chimera_classes.hpp"
#include "Chimera_structs.hpp"
#include "plugin_map.h"

#include <iterator>

// Every member of a game class the production viewer reads, taken from the
// loader's map for the running game build (see plugin_map.h). Resolved once in
// OnPluginLoadHooks; read on the game thread with In(object), which is null
// when the object is null or the member did not resolve.

#define PRODUCTIONVIEWER_FIELD(Class, Member) SR_FIELD(decltype(SDK::Class::Member), Class, Member, 0)

namespace ProductionViewerFields
{
	inline auto ItemUniqueName = PRODUCTIONVIEWER_FIELD(UAuItemDataBase, UniqueItemName);
	inline auto ItemName       = PRODUCTIONVIEWER_FIELD(UAuItemDataBase, ItemName);
	inline auto ItemIcon       = PRODUCTIONVIEWER_FIELD(UAuItemDataBase, ItemIcon);

	inline auto CraftingSelectedRecipe = PRODUCTIONVIEWER_FIELD(FCrCraftingFragment, SelectedRecipe);
	inline auto CraftingCurrentRecipe  = PRODUCTIONVIEWER_FIELD(FCrCraftingFragment, CurrentRecipe);
	inline auto CraftingMultiplier     = PRODUCTIONVIEWER_FIELD(FCrCraftingFragment, CraftingMultiplier);

	// A stack copy of the helper is handed to the game's constructor, so the
	// whole struct is checked against the running build.
	inline auto ReplicationHelperSize = SR_SIZE_CHECK(FCrMassEntityReplicationHelper);

	// UCrGameInstance::ServerSessionName is folded into the SDK's trailing padding,
	// so it has no name there: a native row. The built offset is CL-127004's and
	// CL-128786 moved it by 16; the rows below are this plugin's own until the
	// loader's map carries both.
	inline auto ServerSessionName = PluginField<SDK::FString>{ { "UCrGameInstance::ServerSessionName",
		"UCrGameInstance", "ServerSessionName", PLUGIN_FIELD_NATIVE, sizeof(SDK::FString), 0x290 } };

	inline const PluginFieldCandidate kServerSessionNameRows[] = {
		{ 127004, 0x290, sizeof(SDK::FString), 0, nullptr },
		{ 128786, 0x2A0, sizeof(SDK::FString), 0, nullptr },
	};

	// Asks the loader for every row above. False when any did not resolve; the
	// loader has then recorded the failure and refuses the plugin.
	inline bool Resolve(IPluginSelf* self, IPluginHookScanner* scanner)
	{
		bool all = PluginMap::ResolveFields(scanner, self,
			ItemUniqueName, ItemName, ItemIcon,
			CraftingSelectedRecipe, CraftingCurrentRecipe, CraftingMultiplier,
			ReplicationHelperSize);
		all = PluginMap::ResolveField(scanner, self, ServerSessionName, kServerSessionNameRows,
			static_cast<uint32_t>(std::size(kServerSessionNameRows))) && all;
		return all;
	}

	// The member's value, or a fallback when the member did not resolve.
	template <class T, class Object>
	T ValueOr(const PluginField<T>& field, const Object* object, T fallback = T{})
	{
		const T* at = field.In(static_cast<const void*>(object));
		return at ? *at : fallback;
	}
}
