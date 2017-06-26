#pragma once

#include "FlowControl/FlowContext.h"

namespace mcts
{
	class ActionType
	{
	public:
		enum Types {
			kInvalid,
			kMainAction, // play card, attack, hero-power, or end-turn
			kRandom, // choose by random
			kChooseHandCard,
			kChooseAttacker,
			kChooseDefender,
			kChooseMinionPutLocation,
			kChooseTarget, // battlecry target, spell target, etc.
			kChooseOne, // choose one card id. E.g., druid choose one, tracking, adapt
		};

	public:
		ActionType(Types type) : type_(type) {}

		Types GetType() const { return type_; }

		bool operator==(ActionType const& rhs) const {
			return type_ == rhs.type_;
		}
		bool operator!=(ActionType const& rhs) const { return !(*this == rhs); }

		bool IsChosenRandomly() const { return type_ == kRandom; }
		bool IsChosenManually() const { return !IsChosenRandomly(); }

	private:
		Types type_;
	};

	using Result = FlowControl::Result;

	enum Stage {
		kStageSelection,
		kStageSimulation
	};
}