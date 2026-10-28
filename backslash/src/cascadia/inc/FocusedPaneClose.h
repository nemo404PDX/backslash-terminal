// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include <functional>
#include <utility>
#include <vector>
#include <algorithm>
namespace Microsoft::Terminal::Backslash
{
    // UI-thread gate shared by visible-X and deliberate whole-tab requests.
    class CloseRequestGate
    {
    public:
        unsigned long long Begin()
        {
            // Concurrent terminal exits can collapse an ancestor without delivering
            // this leaf's ClosedByParent. Reconcile only once its node has left the tree.
            if (_pending && _attached && !_attached()) { End(_pending); }
            if (_pending) { return 0; }
            _pending = ++_generation; return _pending;
        }
        void TrackCollapse(unsigned long long lease, std::function<bool()> attached)
        {
            if (lease == _pending) { _attached = std::move(attached); }
        }
        void End(unsigned long long lease) noexcept
        {
            if (lease == _pending) { _pending = 0; _attached = {}; }
        }
    private:
        std::function<bool()> _attached;
        unsigned long long _generation = 0;
        unsigned long long _pending = 0;
    };
    template<typename Node, typename Content>
    Node FindContentLeaf(const Node& root, const Content& content)
    {
        if (!root || !content) { return {}; }
        return root->WalkTree([&](const auto& pane) -> Node { return pane->GetContent() == content ? pane : Node{}; });
    }
    template<typename Node>
    Node FocusedCloseLeaf(const Node& root)
    {
        if (!root) { return {}; }
        const auto active = root->GetActivePane();
        const auto leaf = active ? active->GetLastFocusedLeaf() : Node{};
        return leaf && FindContentLeaf(root, leaf->GetContent()) == leaf ? leaf : Node{};
    }
    template<typename Node, typename Content>
    bool CloseTargetMatches(const Node& currentRoot, const Node& originalRoot, const Node& leaf, const Content& content)
    {
        return currentRoot == originalRoot && leaf && leaf->GetContent() == content && FindContentLeaf(currentRoot, content) == leaf;
    }
    template<typename Node>
    bool ContainsPaneNode(const Node& root, const Node& node)
    {
        return root && node && root->WalkTree([&](const auto& candidate) { return candidate == node; });
    }
    template<typename Node, typename Content>
    bool LeafContentsMatch(const Node& root, const std::vector<Content>& expected)
    {
        if (!root || static_cast<std::size_t>(root->GetLeafPaneCount()) != expected.size()) { return false; }
        auto remaining = expected;
        bool matches = true;
        root->WalkTree([&](const auto& pane) {
            if (const auto content = pane->GetContent())
            {
                const auto found = std::find(remaining.begin(), remaining.end(), content);
                if (found == remaining.end()) { matches = false; }
                else { remaining.erase(found); }
            }
        });
        return matches && remaining.empty();
    }

}
