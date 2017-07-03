#pragma once

#include <cstdlib>

#include "MCTS/builder/TreeBuilder.h"
#include "MCTS/board/BoardView-impl.h"
#include "MCTS/board/ActionParameterGetter-impl.h"
#include "MCTS/board/RandomGenerator-impl.h"
#include "MCTS/detail/BoardNodeMap-impl.h"

#include "MCTS/board/BoardActionAnalyzer.h"
#include "MCTS/board/BoardActionAnalyzer-impl.h"

namespace mcts
{
	namespace builder
	{
		// Never returns kResultInvalid. Will automatically retry if an invalid action is applied
		// Note: can only be called when current player is the viewer of 'board'
		inline TreeBuilder::SelectResult TreeBuilder::PerformSelect(
			TreeNode * node, board::Board & board, TreeUpdater * updater)
		{
			assert(node);

			episode_state_.Start(kStageSelection, board);
			assert(episode_state_.IsValid());
			assert(episode_state_.GetStage() == kStageSelection);

			// TODO [PROFILING]:
			// We save the board every time here. This is just for the case an invalid action is applied
			// Ideally, invalid actions should be rarely happened.
			// So if copy a board is comparatively expensive, we need to decide if this is benefitial.
			episode_state_.GetBoard().SaveState();

			assert(node->GetAddon().consistency_checker.CheckBoard(board.CreateView()));
			selection_stage_.StartNewMainAction(node);

			TreeBuilder::SelectResult perform_result(ApplyAction(
				node->GetAddon().action_analyzer, selection_stage_));

			assert(episode_state_.IsValid());
			statistic_.ApplyActionSucceeded();

			assert(turn_start_node_);
			assert(turn_start_node_->GetActionType().GetType() == ActionType::kMainAction);
			perform_result.node = selection_stage_.FinishMainAction(
				turn_start_node_->GetAddon().board_node_map,
				episode_state_.GetBoard(),
				&perform_result.new_node_created);

			assert(updater);
			updater->PushBackNodes(selection_stage_.GetTraversedPath());

			assert([](builder::TreeBuilder::TreeNode* node) {
				if (!node->GetActionType().IsValid()) return true;
				return node->GetActionType().GetType() == ActionType::kMainAction;
			}(perform_result.node));

			return perform_result;
		}
		
		// Never returns kResultInvalid. Will automatically retry if an invalid action is applied
		// Note: can only be called when current player is the viewer of 'board'
		inline Result TreeBuilder::PerformSimulate(board::Board & board)
		{
			episode_state_.Start(kStageSimulation, board);
			assert(episode_state_.IsValid());
			assert(episode_state_.GetStage() == kStageSimulation);

			// TODO [PROFILING]:
			// We save the board every time here. This is just for the case an invalid action is applied
			// Ideally, invalid actions should be rarely happened.
			// So if copy a board is comparatively expensive, we need to decide if this is benefitial.
			episode_state_.GetBoard().SaveState();

			assert(episode_state_.GetStage() == kStageSimulation);
			simulation_stage_.StartNewAction();

			board::BoardActionAnalyzer action_analyzer;

			Result result = ApplyAction(action_analyzer, simulation_stage_);

			assert(episode_state_.IsValid());
			statistic_.ApplyActionSucceeded();

			return result;
		}

		template <typename StageHandler>
		inline Result TreeBuilder::ApplyAction(
			board::BoardActionAnalyzer & action_analyzer,
			StageHandler&& stage_handler)
		{
			// sometimes an action might in fact an INVALID action
			// here use a loop to retry on those cases
			while (true) {
				// TODO: record the valid actions when the node is expanded at the first time
				// then, we can use it for the next iterations
				int choices = episode_state_.GetBoard().GetActionsCount(action_analyzer);
				int choice = this->ChooseAction(ActionType(ActionType::kMainAction), [choices]() {
					return board::ActionChoices(choices);
				});

				if (episode_state_.IsValid()) {
					Result result = episode_state_.GetBoard().ApplyAction(
						choice, action_analyzer, random_generator_, action_parameter_getter_);
					if (result != Result::kResultInvalid) return result;
				}

				stage_handler.ReportInvalidAction();
				stage_handler.RestartAction();

				episode_state_.GetBoard().RestoreState();
				episode_state_.SetValid();

				statistic_.ApplyActionFailed();
			}
			assert(false); // not reach here
			return Result::kResultSecondPlayerWin;
		}

		inline int TreeBuilder::ChooseAction(ActionType action_type, board::ActionChoicesGetter const& choices_getter)
		{
			int choice = -1;
			if (episode_state_.GetStage() == kStageSelection) {
				choice = selection_stage_.ChooseAction(episode_state_.GetBoard(), action_type, choices_getter);
			}
			else {
				assert(episode_state_.GetStage() == kStageSimulation);
				choice = simulation_stage_.ChooseAction(episode_state_.GetBoard(), action_type, choices_getter);
			}

			if (choice < 0) {
				episode_state_.SetInvalid();
			}
			return choice;
		}
	}
}