#ifndef REDISTRIBUTION_HPP
#define REDISTRIBUTION_HPP

#include "graph.hpp"

inline pair<float, vector<pair<graph::Vertex_A *, graph::Edge *>>> graph::find_highest_capacity_path(
        Vertex_A *source,
        Vertex_A *target)
{
        // MAX-priority queue: {bottleneck_capacity, path_vector}
        priority_queue<PathState> pq; 

        // Start with the source node
        vector<pair<Vertex_A *, Edge *>> initial_path;
        initial_path.push_back({source, nullptr});
        pq.push(PathState(std::numeric_limits<float>::max(), initial_path));

        // Map to track the max capacity to reach a node
        unordered_map<Vertex_A *, float> max_capacity_map;
        max_capacity_map[source] = std::numeric_limits<float>::max();

        // Track the best (complete) path found so far
        float best_path_capacity = 0.0f;
        vector<pair<Vertex_A *, Edge *>> best_path;

        while (!pq.empty())
        {
            PathState current = pq.top();
            pq.pop();

            Vertex_A *current_node = current.path.back().first;
            float current_capacity = current.min_available_capacity;

            // If this is a worse path than one we've already found, skip it
            if (max_capacity_map.count(current_node) && current_capacity < max_capacity_map[current_node])
            {
                continue;
            }

            // --- FOUND A PATH TO THE TARGET ---
            if (current_node == target)
            {
                
                if (current_capacity > best_path_capacity)
                {
                    best_path_capacity = current_capacity;
                    best_path = current.path;
                }
                
                continue;
            }

            // --- Explore neighbors in both directions ---
            // 1. Forward neighbors (A -> B)
            if (adj_power_substation.count(current_node))
            {
                for (auto &[neighbor_node, edge] : adj_power_substation[current_node])
                {
                    try_add_capacity_path(current, neighbor_node, edge, pq, max_capacity_map);
                }
            }
            // 2. Reverse neighbors (B -> A)
            if (adj_reverse_power.count(current_node))
            {
                for (auto &[neighbor_node, edge] : adj_reverse_power[current_node])
                {
                    try_add_capacity_path(current, neighbor_node, edge, pq, max_capacity_map);
                }
            }
        }
        return {best_path_capacity, best_path};
    }

inline void graph::try_add_capacity_path(
        const PathState &current_state,
        Vertex_A *next_node,
        Edge *edge,
        priority_queue<PathState> &pq,
        unordered_map<Vertex_A *, float> &max_capacity_map)
{
        // Check for cycles
        for (const auto &pair : current_state.path)
        {
            if (pair.first == next_node)
                return;
        }

        // --- 1. Calculate New Bottleneck Capacity ---
        float edge_available_capacity = edge->max_load - edge->current_load;
        float new_min_capacity = std::min(current_state.min_available_capacity, edge_available_capacity);

        // --- 2. Skip edges with zero or negative capacity
        if (new_min_capacity <= 0.01f)
        {
            return;
        }

        // --- 3. Add to Queue if it's a better path to this *node* ---
        if (!max_capacity_map.count(next_node) || new_min_capacity > max_capacity_map[next_node])
        {
            max_capacity_map[next_node] = new_min_capacity;

            vector<pair<Vertex_A *, Edge *>> new_path = current_state.path;
            new_path.push_back({next_node, edge});

            pq.push(PathState(new_min_capacity, new_path));
        }
    }

inline void graph::propagate_demand_change(Vertex_A *node, float delta, unordered_set<Vertex_A *> &visited_propagation)
{
        // Prevent infinite recursion in cyclic graphs
        if (visited_propagation.count(node))
        {
            return;
        }
        visited_propagation.insert(node);

        // 1. Apply the demand change to this node
        node->current_downstream_demand += delta;

        // 2. Find parents (FAST O(1) lookup)
        if (!adj_reverse_power.count(node))
        {
            visited_propagation.erase(node); // Backtrack
            return;                          // This is a power plant
        }

        // 3. Find total capacity of all parent links
        float total_incoming_capacity = 0.0f;
        float epsilon = 0.01f;
        for (auto &[parent, edge] : adj_reverse_power[node])
        {
            total_incoming_capacity += edge->max_load;
        }

        // 4. Propagate the delta PROPORTIONALLY to all parents.
        for (auto &[parent_node, parent_edge] : adj_reverse_power[node])
        {
            float proportional_delta = 0.0f;

            if (total_incoming_capacity > epsilon)
            {
                float proportion = parent_edge->max_load / total_incoming_capacity;
                proportional_delta = delta * proportion;
            }
            else
            {
                // Fallback: split equally
                proportional_delta = delta / adj_reverse_power[node].size();
            }

            // a. Apply the change to the connecting edge's load
            parent_edge->current_load += proportional_delta;

            // b. Recursively call for the parent node
            propagate_demand_change(parent_node, proportional_delta, visited_propagation);
        }

        visited_propagation.erase(node); // Backtrack
    }

inline bool graph::can_handle_increase(Vertex_A *node, float increase, unordered_set<Vertex_A *> &visited)
{
        // Prevent infinite recursion in cycles
        if (visited.count(node))
        {
            return true;
        }
        visited.insert(node);

        bool all_parents_ok = true;
        bool has_at_least_one_parent = false;

        // Traverse all power-substation adjacency pairs to find parent edges
        for (auto &[potential_parent, neighbors] : adj_power_substation)
        {
            for (auto &[child, edge] : neighbors)
            {
                if (child == node)
                {
                    has_at_least_one_parent = true;

                    // 1. Check if this edge can handle the additional load
                    if (edge->current_load + increase > edge->max_load)
                    {
                        all_parents_ok = false;
                        break; 
                    }

                    // 2. Recursively check further upstream edges from the parent
                    if (!can_handle_increase(potential_parent, increase, visited))
                    {
                        all_parents_ok = false;
                        break; 
                    }
                }
            }
            if (!all_parents_ok)
                break;
        }
        visited.erase(node);
        return all_parents_ok;
    }

inline void graph::propagate_limit_increase(Vertex_A *node, float increase, unordered_set<Vertex_A *> &visited)
{
        // This function propagates a capacity increase up the graph to parents
        if (visited.count(node))
        {
            return; // Cycle detected
        }
        visited.insert(node);

        // Find all parents 
        if (!adj_reverse_power.count(node))
        {
            visited.erase(node);
            return; // Is a power plant
        }

        for (auto &[parent_node, parent_edge] : adj_reverse_power[node])
        {
            float parent_increase = increase;

            // Power plants get an extra 20% buffer, as per original mapping logic
            if (parent_node->node_type == "power_plant")
            {
                parent_increase *= 1.2f;
            }

            // Increase the parent node's limit
            parent_node->max_limit += parent_increase;

            //update the max_load of the edge connecting them
            parent_edge->max_load += increase;

            // Recurse
            propagate_limit_increase(parent_node, parent_increase, visited);
        }

        visited.erase(node); // Backtrack
    }

inline void graph::check_edge_overloads()
{
    // Clear visualization state from the *previous* frame
    overloaded_edges.clear();
    edge_being_fixed = nullptr; // These will be set by the first fix
    fix_path.clear();

    bool has_overloads = false;
    for (auto &[source, neighbors] : adj_power_substation)
    {
        for (auto &[target, edge] : neighbors)
        {
            if (edge->max_load > 0.01f) // Avoid division by zero
            {
                float ratio = edge->current_load / edge->max_load;
                if (ratio >= 0.75f)
                {
                    overloaded_edges.insert(edge); // Add to set for *initial* red highlighting
                    has_overloads = true;
                }
            }
        }
    }
    if (has_overloads && status_message.find("WARNING:") == string::npos) {
        status_message += " | WARNING: Edge Overload! Press 'O' to fix.";
    }
}

inline void graph::fix_all_overloads()
{
    vector<OverloadInfo> problems_to_fix;

    // Repopulate based on current overloads
    for (auto &[source, neighbors] : adj_power_substation)
    {
        for (auto &[target, edge] : neighbors)
        {
            if (overloaded_edges.count(edge))
            {
                float ratio = edge->current_load / edge->max_load;
                problems_to_fix.push_back({ratio, source, target, edge});
            }
        }
    }

    if (problems_to_fix.empty())
    {
        status_message = "No overloads to fix.";
        return;
    }

    sort(problems_to_fix.begin(), problems_to_fix.end(), std::greater<OverloadInfo>());

    bool fix_was_applied_this_tick = false;

    for (auto &problem : problems_to_fix)
    {
        if (overloaded_edges.count(problem.edge))
        {
            bool visualize_this_fix = !fix_was_applied_this_tick;
            
            if (fix_overloading_problem(problem.source, problem.target, problem.edge, visualize_this_fix))
            {
                fix_was_applied_this_tick = true;
            }
            else
            {
                if (!nodes_to_throttle.empty())
                {
                    break;
                }
            }
        }
    }
}

inline void graph::overloading_edge()
{
    check_edge_overloads();
    fix_all_overloads();
}

inline bool graph::fix_overloading_problem(Vertex_A *source, Vertex_A *target,
                                 Edge *overloaded_edge,
                                 bool visualize_this_fix)
{
        // --- 1. Validate ---
        if (!overloaded_edge) return false;

        // --- 2. Calculate required load reduction ---
        float overload_threshold = 0.75f;
        float target_load_factor = 0.70f;
        float current_load_factor = (overloaded_edge->max_load > 0) ? (overloaded_edge->current_load / overloaded_edge->max_load) : 1.0f;
        float load_to_reduce = overloaded_edge->current_load - (target_load_factor * overloaded_edge->max_load);
        float epsilon = 0.01f;

        // --- 3. Check if reduction is still needed ---
        if (current_load_factor < overload_threshold || load_to_reduce < epsilon)
        {
            if (current_load_factor < overload_threshold) {
                overloaded_edges.erase(overloaded_edge);
            }
            return false; // No fix applied
        }

        // --- 4. Find the path with the HIGHEST available capacity ---
        auto [max_available_capacity, best_path] = find_highest_capacity_path(source, target);

        // --- 5. Decide if this path is good enough ---
        bool path_is_usable = false;
        float actual_rerouted_load = 0.0f;

        // Check if a path was found AND it's not just the source
        if (!best_path.empty() && best_path.size() > 1)
        {
            // We can reroute *at most* what the path can handle
            actual_rerouted_load = std::min(load_to_reduce, max_available_capacity);

            // ---  Minimum Partial Fix (20% rule) ---
            if (actual_rerouted_load < (load_to_reduce * 0.20f))
            {
                cout << "Load Balancing: Found path, but capacity " << max_available_capacity << " is < 20% of required " << load_to_reduce << ". Throttling." << endl;
            }
            else
            {
                // --- Stability Check (75% rule) ---
                bool path_is_stable = true;
                for (size_t i = 1; i < best_path.size(); ++i)
                {
                    Edge* path_edge = best_path[i].second;
                    if (path_edge->max_load > epsilon)
                    {
                        float new_load = path_edge->current_load + actual_rerouted_load;
                        if (new_load / path_edge->max_load >= overload_threshold)
                        {
                            path_is_stable = false;
                            cout << "Load Balancing: Found path, but it would become unstable. Throttling." << endl;
                            break;
                        }
                    }
                }
                
                if (path_is_stable)
                {
                    path_is_usable = true;
                }
            }
        }
        
        // --- 6.  Apply Fix or Throttle ---
        if (path_is_usable)
        {
            // ---  Apply Best Possible Fix (Partial or Full) ---
            cout << "--- Load Balancing Action ---" << endl;
            cout << "Overloaded Edge: (" << source << " -> " << target << ")" << endl;
            cout << "   Original Load: " << overloaded_edge->current_load << " / " << overloaded_edge->max_load << " (" << fixed << setprecision(1) << current_load_factor * 100 << "%)" << endl;
            cout << "   Strategy: Found highest-capacity STABLE path." << endl;
            cout << "   Path Capacity: " << fixed << setprecision(2) << max_available_capacity << endl;
            cout << "   Required Reduction: " << fixed << setprecision(2) << load_to_reduce << endl;
            cout << "   ACTUAL Rerouting Load: " << fixed << setprecision(2) << actual_rerouted_load << endl;

            // --- 7. Apply Changes ---
            overloaded_edge->current_load -= actual_rerouted_load;
            cout << "   New Load on Original Edge: " << overloaded_edge->current_load << " / " << overloaded_edge->max_load << endl;
            cout << "   Via Path: " << source;

            for (int i = 1; i < best_path.size(); ++i)
            {
                Vertex_A *path_node_child = best_path[i].first;
                Edge *path_edge = best_path[i].second;

                path_edge->current_load += actual_rerouted_load;
                cout << " -> (" << path_node_child << " | New Edge Load: " << path_edge->current_load << ")";
            }
            cout << endl;

            if ((overloaded_edge->current_load / overloaded_edge->max_load) >= overload_threshold)
            {
                status_message = "Fixed overloaded edge (Partial Reroute Applied).";
                cout << "   Status: STILL OVERLOADED (Partial Fix Applied)" << endl;
            }
            else
            {
                overloaded_edges.erase(overloaded_edge);
                status_message = "Successfully rerouted flow to fix overload.";
                cout << "   Status: OK (Fixed)" << endl;
            }

            if (visualize_this_fix)
            {
                edge_being_fixed = overloaded_edge;
                fix_path = best_path;
            }

            cout << "---------------------------" << endl;
            return true; // A fix was applied
        }
        else
        {
            // ---  No Usable Path Found -> Throttle ---
            if (best_path.empty()) {
                cout << "Load Balancing: Overload on edge (" << source << " -> " << target << "). No alternative paths exist." << endl;
            } // (Other failure cases already logged above)
            
            cout << "   -> SCHEDULING PRECISION DEMAND REDUCTION for node: " << target << " by " << fixed << setprecision(2) << load_to_reduce << " kW" << endl;
            status_message = "No alternative path. Scheduled precision demand reduction.";
            overloaded_edges.erase(overloaded_edge);
            nodes_to_throttle[target] += load_to_reduce; 
            return false; // No fix applied this tick
        }
    }

inline void graph::apply_demand_reduction_updates()
{
        if (nodes_to_throttle.empty())
        {
            return;
        }

        cout << "--- Applying Precision Demand Reduction (Throttling) ---" << endl;

        // NEW: Iterate the map to get the substation AND the amount to reduce
        for (auto const &[substation, required_reduction] : nodes_to_throttle)
        {
            if (!adj_substion_consumer.count(substation) || required_reduction <= 0.01f)
            {
                continue; // This substation has no consumers or no reduction needed
            }

            // ---  Distribute the reduction proportionally ---

            // 1. Find the total demand at this substation
            float total_demand_at_substation = 0.0f;
            for (auto &[consumer, edge] : adj_substion_consumer[substation])
            {
                total_demand_at_substation += consumer->demand;
            }

            float total_demand_reduction_applied = 0.0f; // Track the *actual* change
            float epsilon = 0.01f;

            if (total_demand_at_substation < epsilon)
            {
                // No demand to reduce, can't fix it.
                cout << "   -> Node " << substation << " has 0 consumer demand. Cannot apply reduction." << endl;
                continue;
            }

            // 2. Iterate consumers and apply their share of the reduction
            for (auto &[consumer, edge] : adj_substion_consumer[substation])
            {
                // Calculate this consumer's share of the reduction
                float proportion = consumer->demand / total_demand_at_substation;
                float delta = -required_reduction * proportion; // delta is a negative number

                // Don't let demand go below zero
                if (consumer->demand + delta < 0)
                {
                    delta = -consumer->demand;
                }

                if (delta < -epsilon) // Only apply if there's a real change
                {
                    consumer->demand += delta;
                    edge->current_load += delta; // Update local edge
                    total_demand_reduction_applied += delta;
                }
            }

            // 3. If we reduced demand, propagate this *single* change up the network
            if (total_demand_reduction_applied < -epsilon)
            {
                throttled_nodes_visual.insert(substation);
                cout << "   Throttled node " << substation << ". Total demand reduced by: " << fixed << setprecision(2) << total_demand_reduction_applied << " kW" << endl;

                unordered_set<Vertex_A *> visited_propagation;
                // Propagate the *actual* amount reduced
                propagate_demand_change(substation, total_demand_reduction_applied, visited_propagation);
            }
        }

        nodes_to_throttle.clear(); // Clear the map after processing
        cout << "------------------------------------------------" << endl;
    }

inline void graph::check_node_overloads()
{
        node_overloads_visual.clear(); // Clear previous state

        // We must check all nodes in the graph.
        unordered_set<Vertex_A *> all_nodes;
        for (auto const &[parent, children] : adj_power_substation)
        {
            all_nodes.insert(parent);
            for (auto const &pair : children)
                all_nodes.insert(pair.first);
        }
        for (auto const &[parent, children] : adj_substion_consumer)
        {
            all_nodes.insert(parent);
        }

        for (auto *node : all_nodes)
        {
            if (node->node_type == "power_plant")
                continue; // Skip power plants

            if (node->current_downstream_demand > node->max_limit)
            {
                node_overloads_visual.insert(node);
            }
        }
    }

inline void graph::upgrade_selected_node_limit(Vertex_A *selected_node)
{
        if (selected_node == nullptr || selected_node->node_type == "power_plant")
        {
            cout << "Upgrade: No valid substation selected." << endl;
            return;
        }

        // Only upgrade if it's actually overloaded
        if (node_overloads_visual.count(selected_node))
        {
            // Calculate a 25% increase
            float increase_amount = selected_node->max_limit * 0.25f;

            // make sure it's at least enough to cover the deficit
            float deficit = selected_node->current_downstream_demand - selected_node->max_limit;
            if (increase_amount < deficit)
            {
                increase_amount = deficit * 1.1f; // Add 10% safety buffer
            }

            cout << "--- Node Upgrade ---" << endl;
            cout << "Upgrading node " << selected_node << " limit." << endl;
            cout << "  Old Limit: " << selected_node->max_limit << endl;
            selected_node->max_limit += increase_amount;
            cout << "  New Limit: " << selected_node->max_limit << endl;

            
            unordered_set<Vertex_A *> visited;
            propagate_limit_increase(selected_node, increase_amount, visited);

            // Re-run the check
            check_node_overloads();
        }
        else
        {
            cout << "Upgrade: Selected node is not overloaded." << endl;
        }
    }

#endif
