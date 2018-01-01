#pragma once

#include <queue>
#include "MCTS/selection/TraversedNodeInfo.h"
#include "MCTS/selection/TreeUpdater.h"
#include "MCTS/Config.h"

namespace mcts
{
	namespace selection
	{
		class TreeUpdater
		{
		public:
			TreeUpdater() : bfs_()
#ifndef NDEBUG
				, should_visits_()
#endif
			{}

			TreeUpdater(TreeUpdater const&) = delete;
			TreeUpdater & operator=(TreeUpdater const&) = delete;

			void Update(std::vector<selection::TraversedNodeInfo> const& nodes, selection::TreeNode * last_node, double credit)
			{
				UpdateChosenTimes(nodes, last_node);
				TreeLikeUpdateWinRate(nodes, credit);
				//LinearlyUpdateWinRate(credit);
			}

		private:
			void UpdateChosenTimes(std::vector<selection::TraversedNodeInfo> const& nodes, selection::TreeNode * last_node) {
				for (size_t i = 0; i < nodes.size(); ++i) {
					auto const& item = nodes[i];

					if (item.choice_ >= 0) {
						selection::TreeNodeAddon * next_node_addon = nullptr;
						if ((i + 1) < nodes.size()) {
							next_node_addon = &nodes[i + 1].node_->GetAddon();
						}
						else {
							if (last_node) {
								next_node_addon = &last_node->GetAddon();
							}
						}

						if (next_node_addon) {
							next_node_addon->leading_nodes.AddLeadingNodes(
								item.node_, item.choice_);
						}
					}

					if (item.edge_addon_) {
						auto & edge_addon = *item.edge_addon_;
						edge_addon.AddChosenTimes(1);

						if constexpr (StaticConfigs::kVirtualLoss != 0) {
							static_assert(StaticConfigs::kVirtualLoss > 0);
							edge_addon.AddTotal(-StaticConfigs::kVirtualLoss); // remove virtual loss
							assert(edge_addon.GetTotal() >= 0);
						}
					}
				}
			}

			void LinearlyUpdateWinRate(std::vector<selection::TraversedNodeInfo> const& nodes, double credit) {
				for (auto const& item : nodes) {
					auto * edge_addon = item.edge_addon_;
					if (!edge_addon) continue;

					edge_addon->AddTotal(100);
					edge_addon->AddCredit((int)(credit * 100.0));
				}
			}

			void TreeLikeUpdateWinRate(std::vector<selection::TraversedNodeInfo> const& nodes, double credit) {
				if (nodes.empty()) return;

				assert([&](){
					should_visits_.clear();
					for (auto const& item : nodes) {
						if (item.edge_addon_) {
							should_visits_.insert(item.edge_addon_);
						}
					}
					return true;
				}());

				auto it = nodes.rbegin();
				while (it != nodes.rend()) {
					TreeLikeUpdateWinRate(it->node_, it->choice_, credit);

					// skip to next redirect node
					// all intermediate nodes are already updated
					++it;
					while (it != nodes.rend()) {
						if (it->choice_ < 0) break;
						++it;
					}
					if (it == nodes.rend()) break;

					assert(it->edge_addon_ == nullptr);
					++it;
				}

				assert(should_visits_.empty());
			}

			void TreeLikeUpdateWinRate(selection::TreeNode * start_node, int start_choice, double credit)
			{
				assert(start_node);
				
				assert(bfs_.empty());
				bfs_.push({ start_node, start_choice });

				while (!bfs_.empty()) {
					auto node = bfs_.front().node;
					int choice = bfs_.front().choice;
					auto * edge_addon = node->GetEdgeAddon(choice);
					bfs_.pop();

					assert(edge_addon);
					assert([&]() {
						should_visits_.erase(edge_addon);
						return true;
					}());
					edge_addon->AddTotal(100);
					edge_addon->AddCredit((int)(credit*100.0));

					// use BFS to reduce the lock time
					node->GetAddon().leading_nodes.ForEachLeadingNode(
						[&](selection::TreeNode * leading_node, int leading_choice)
					{
						bfs_.push({ leading_node, leading_choice });
						return true;
					});
				}
			}

		private:
			struct Item {
				selection::TreeNode * node;
				int choice;
			};
			std::queue<Item> bfs_;

#ifndef NDEBUG
			std::unordered_set<selection::EdgeAddon*> should_visits_;
#endif
		};
	}
}
