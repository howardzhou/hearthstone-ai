#pragma once

#include "FlowControl/enchantment/Enchantments.h"

namespace FlowControl
{
	namespace enchantment
	{
		enum EnchantmentTiers
		{
			kEnchantmentTier1,
			kEnchantmentTier2,
			kEnchantmentTier3,
			kEnchantmentAura
		};

		class TieredEnchantments
		{
		public:
			using ContainerType = Enchantments::ContainerType;
			using IdentifierType = std::pair<EnchantmentTiers, ContainerType::Identifier>;

			template <typename T>
			typename IdentifierType PushBack(T && item)
			{
				using EnchantmentType = std::decay_t<T>;
				return std::make_pair(EnchantmentType::tier,
					GetEnchantments<EnchantmentType::tier>().PushBack(item.apply_functor));
			}

			void Remove(IdentifierType id)
			{
				switch (id.first) {
				case kEnchantmentTier1:
					return tier1_.Remove(id.second);
				case kEnchantmentTier2:
					return tier2_.Remove(id.second);
				case kEnchantmentTier3:
					return tier3_.Remove(id.second);
				case kEnchantmentAura:
					return aura_.Remove(id.second);
				}
				assert(false);
			}

			void Clear()
			{
				tier1_.Clear();
				tier2_.Clear();
				tier3_.Clear();
				aura_.Clear();
			}

			bool Exists(IdentifierType id) const
			{
				switch (id.first) {
				case kEnchantmentTier1:
					return tier1_.Exists(id.second);
				case kEnchantmentTier2:
					return tier2_.Exists(id.second);
				case kEnchantmentTier3:
					return tier3_.Exists(id.second);
				case kEnchantmentAura:
					return aura_.Exists(id.second);
				}
				assert(false);
				return false;
			}

			void ApplyAll(state::Cards::EnchantableStates & card)
			{
				tier1_.ApplyAll(card);
				tier2_.ApplyAll(card);
				tier3_.ApplyAll(card);
				aura_.ApplyAll(card);
			}

		private:
			template <int Tier> Enchantments & GetEnchantments();
			template <int Tier> const Enchantments & GetEnchantments() const;

		private:
			Enchantments tier1_;
			Enchantments tier2_;
			Enchantments tier3_;
			Enchantments aura_;
		};

		template <> inline Enchantments & TieredEnchantments::GetEnchantments<kEnchantmentTier1>() { return tier1_; }
		template <> inline const Enchantments & TieredEnchantments::GetEnchantments<kEnchantmentTier1>() const { return tier1_; }
		template <> inline Enchantments & TieredEnchantments::GetEnchantments<kEnchantmentTier2>() { return tier2_; }
		template <> inline const Enchantments & TieredEnchantments::GetEnchantments<kEnchantmentTier2>() const { return tier2_; }
		template <> inline Enchantments & TieredEnchantments::GetEnchantments<kEnchantmentTier3>() { return tier3_; }
		template <> inline const Enchantments & TieredEnchantments::GetEnchantments<kEnchantmentTier3>() const { return tier3_; }
		template <> inline Enchantments & TieredEnchantments::GetEnchantments<kEnchantmentAura>() { return aura_; }
		template <> inline const Enchantments & TieredEnchantments::GetEnchantments<kEnchantmentAura>() const { return aura_; }
	}
}