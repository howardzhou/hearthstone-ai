#pragma once

#include "State/State.h"
#include "FlowControl/Result.h"
#include "FlowControl/ActionParameterWrapper.h"
#include "FlowControl/Helpers/Resolver.h"
#include "FlowControl/Dispatchers/Minions.h"
#include "FlowControl/Context/OnAttack.h"
#include "FlowControl/Context/BattleCry.h"
#include "FlowControl/Context/BeforeMinionSummoned.h"
#include "FlowControl/IRandomGenerator.h"

namespace FlowControl
{
	namespace Helpers
	{
		class Attack
		{
		public:
			Attack(state::State & state, FlowContext & flow_context, state::CardRef attacker, state::CardRef defender)
				: state_(state), flow_context_(flow_context), attacker_(attacker), defender_(defender)
			{

			}

			Result Go()
			{
				Result rc = kResultNotDetermined;

				if (!attacker_.IsValid()) return rc;
				if (!defender_.IsValid()) return rc;

				DoAttack();

				return Resolver(state_, flow_context_).Resolve();
			}

		private:
			void DoAttack()
			{
				while (true) {
					state::CardRef origin_attacker = attacker_;
					state::CardRef origin_defender = defender_;
					state_.event_mgr.TriggerEvent<state::Events::EventTypes::BeforeAttack>(state_, attacker_, defender_);

					if (!attacker_.IsValid()) return;
					if (state_.mgr.Get(attacker_).GetHP() <= 0) return;
					if (state_.mgr.Get(attacker_).GetZone() != state::kCardZonePlay) return;

					if (!defender_.IsValid()) return;
					if (state_.mgr.Get(defender_).GetHP() <= 0) return;
					if (state_.mgr.Get(defender_).GetZone() != state::kCardZonePlay) return;

					if (origin_attacker == attacker_ && origin_defender == defender_) break;
				}

				state_.event_mgr.TriggerEvent<state::Events::EventTypes::OnAttack>(
					Context::OnAttack(state_, flow_context_, attacker_, defender_));
				// TODO: attacker lose stealth

				Manipulate(state_, flow_context_).Character(defender_).Damage().Take(state_.mgr.Get(attacker_).GetAttack());
				Manipulate(state_, flow_context_).Character(attacker_).Damage().Take(state_.mgr.Get(defender_).GetAttack());

				state_.event_mgr.TriggerEvent<state::Events::EventTypes::AfterAttack>(state_, attacker_, defender_);

				{
					const state::Cards::Card & attacker_card = state_.mgr.Get(attacker_);
					if (attacker_card.GetCardType() == state::kCardTypeHero) {
						state::CardRef weapon_ref = attacker_card.GetRawData().weapon_ref;
						if (weapon_ref.IsValid()) {
							Manipulate(state_, flow_context_).Card(weapon_ref).Damage().Take(1);
						}
					}
				}
			}

		private:
			state::State & state_;
			FlowContext & flow_context_;
			state::CardRef attacker_;
			state::CardRef defender_;
		};
	}
}