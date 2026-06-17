#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <bits/stdc++.h>
#include <cmath>
#include <queue>
#include <unordered_set>
#include <iomanip>
#include "raylib.h"
#include "raymath.h"

using namespace std;

// global variable
inline int resident_demand, hospital_demand, industry_demand, institute_demand;
static random_device rd;
static mt19937 g(rd());

// A hash function for std::pair, required for the spatial grid
struct PairHash
{
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2> &p) const
    {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        // A simple way to combine hashes
        return h1 ^ (h2 << 1);
    }
};

class graph{
public:
    class Vertex_A
    { // power_plant and substation Vertex
    public:
        string node_type = "";
        float max_limit;
        float current_downstream_demand; // Tracks actual load
        int x, y;                        // <-- Coordinates

        // constructor
        Vertex_A(string type) : node_type(type), max_limit(0), current_downstream_demand(0), x(0), y(0)
        {
        }
    };

    class Vertex_B
    { // consumer Vertex
    public:
        int demand;
        string node_type; 
        int x, y;         // <-- Coordinates

        // constructor
        Vertex_B(string type) : node_type(type), x(0), y(0)
        {
            if (type == "residential")
                demand = resident_demand;
            else if (type == "industrial")
                demand = industry_demand;
            else if (type == "institute")
                demand = institute_demand;
            else
                demand = hospital_demand;
        }
    };

    class Edge
    {
    public:
        float loss;
        float max_load;
        float current_load;

        // constructor
        Edge(Vertex_A *Node_1, Vertex_A *Node_2) : loss(0), max_load(0), current_load(0)
        { // from Node_1 to Node_2
            if (Node_1->node_type == "power_plant" && Node_2->node_type == "substation")
            {
                loss = 2 + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (5 - 2))); // between 2-5 kW
            }
            else if (Node_1->node_type == "substation" && Node_2->node_type == "substation")
            {
                loss = 1 + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (3 - 1))); // between 1-3 kW
            }
        };

        Edge(Vertex_A *Node_1, Vertex_B *Node_2)
        {                                                                                             // from Node_1 to Node_2
            loss = 0.5 + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (1.0f - 0.5f))); // between 0.5 to 1 kw
            max_load = 3.0f * Node_2->demand;                                                         // Max load has a safety factor
            current_load = Node_2->demand;                                                            // Current load is the demand
        }
    };
    unordered_map<Vertex_A *, vector<pair<Vertex_B *, Edge *>>> adj_substion_consumer; // adj list of substation to consumers
    unordered_map<Vertex_A *, vector<pair<Vertex_A *, Edge *>>> adj_power_substation;  // adj list of substation to substaiton and power_plant to substations
    unordered_map<Vertex_A *, vector<pair<Vertex_A *, Edge *>>> adj_reverse_power;
    unordered_set<Edge *> overloaded_edges;
    Edge *edge_being_fixed = nullptr; // Pointer to the specific edge we are fixing
    unordered_map<Vertex_A *, float> nodes_to_throttle;
    unordered_set<Vertex_A *> node_overloads_visual; // Tracks nodes where demand > limit
    unordered_set<Vertex_A *> throttled_nodes_visual;
    vector<pair<Vertex_A *, Edge *>> fix_path; // The alternative path used
    std::string status_message = "System Normal";
    Vertex_B *last_demand_increase_consumer = nullptr;
    Vertex_A *last_event_substation = nullptr;        // Store the substation linked to the last 'I' key event
    Color last_event_substation_color = {0, 0, 0, 0}; // Store the highlight color, initialized to transparent
    

    void clear() {
        unordered_set<Vertex_A*> a_nodes;
        unordered_set<Vertex_B*> b_nodes;
        unordered_set<Edge*> edges;

        for (auto& pair_item : adj_substion_consumer) {
            a_nodes.insert(pair_item.first);
            for (auto& p : pair_item.second) {
                b_nodes.insert(p.first);
                edges.insert(p.second);
            }
        }
        for (auto& pair_item : adj_power_substation) {
            a_nodes.insert(pair_item.first);
            for (auto& p : pair_item.second) {
                a_nodes.insert(p.first);
                edges.insert(p.second);
            }
        }
        
        for (auto* n : a_nodes) delete n;
        for (auto* n : b_nodes) delete n;
        for (auto* e : edges) delete e;

        adj_substion_consumer.clear();
        adj_power_substation.clear();
        adj_reverse_power.clear();
        overloaded_edges.clear();
        nodes_to_throttle.clear();
        node_overloads_visual.clear();
        throttled_nodes_visual.clear();
        fix_path.clear();
        edge_being_fixed = nullptr;
        last_demand_increase_consumer = nullptr;
        last_event_substation = nullptr;
    }

    ~graph() {
        clear();
    }
    
    // Delete copy to prevent shallow copies
    graph(const graph&) = delete;
    graph& operator=(const graph&) = delete;

    // Define move constructor and assignment
    graph() = default;
    graph(graph&&) = default;
    graph& operator=(graph&& other) {
        if (this != &other) {
            clear();
            adj_substion_consumer = std::move(other.adj_substion_consumer);
            adj_power_substation = std::move(other.adj_power_substation);
            adj_reverse_power = std::move(other.adj_reverse_power);
            overloaded_edges = std::move(other.overloaded_edges);
            nodes_to_throttle = std::move(other.nodes_to_throttle);
            node_overloads_visual = std::move(other.node_overloads_visual);
            throttled_nodes_visual = std::move(other.throttled_nodes_visual);
            fix_path = std::move(other.fix_path);
            edge_being_fixed = other.edge_being_fixed;
            last_demand_increase_consumer = other.last_demand_increase_consumer;
            last_event_substation = other.last_event_substation;
            
            // Clean up other
            other.edge_being_fixed = nullptr;
            other.last_demand_increase_consumer = nullptr;
            other.last_event_substation = nullptr;
            other.adj_substion_consumer.clear();
            other.adj_power_substation.clear();
            other.adj_reverse_power.clear();
        }
        return *this;
    }

    // --- Graph Generation Functions ---
    vector<int> distributor(int total_count, int divide_count);


    void mapping(int powerplant_count, int substation_count, int consumer_count);



    void substation_consumer_connector(Vertex_A *substation, int value);


    void map_powerplants_to_substations(vector<Vertex_A *> &powerplants, vector<Vertex_A *> &substations);

private:   
    // --- Helper Functions for Random Layout ---
    double GetDistance(int x1, int y1, int x2, int y2);

    int GetRandom(int min_val, int max_val);

    using GridKey = pair<int, int>;

    bool is_position_ok_A(Vertex_A *node_to_place, int x, int y,const unordered_map<GridKey, vector<Vertex_A *>, PairHash> &grid,int CELL_SIZE);

    bool is_position_ok_B(Vertex_B *node_to_place, int x, int y,const unordered_map<GridKey, vector<Vertex_B *>, PairHash> &grid,int CELL_SIZE);

    
    bool DoesEdgeExist(Vertex_A *sub1, Vertex_A *sub2);

    Edge *find_edge(Vertex_A *from, Vertex_A *to)
    {
        // Check from -> to
        if (adj_power_substation.count(from))
        {
            for (const auto &neighbor_pair : adj_power_substation[from])
            {
                if (neighbor_pair.first == to)
                {
                    return neighbor_pair.second;
                }
            }
        }

        // Check to -> from
        if (adj_power_substation.count(to))
        {
            for (const auto &neighbor_pair : adj_power_substation[to])
            {
                if (neighbor_pair.first == from)
                {
                    return neighbor_pair.second;
                }
            }
        }

        return nullptr; // No edge found in either direction
    }

    // Struct to hold state in our priority queue
    struct PathState
    {
        float min_available_capacity;          // The bottleneck capacity of this path
        vector<pair<Vertex_A *, Edge *>> path; // The path taken

        // Constructor
        PathState(float cap, vector<pair<Vertex_A *, Edge *>> p) : min_available_capacity(cap), path(std::move(p)) {}

        // Comparator for the MAX-priority queue (highest capacity first)
        bool operator<(const PathState &other) const
        {
            return min_available_capacity < other.min_available_capacity;
        }
    };

    pair<float, vector<pair<Vertex_A *, Edge *>>> find_highest_capacity_path(
        Vertex_A *source,
        Vertex_A *target);


    // Helper function for the "widest path" search
    void try_add_capacity_path(
        const PathState &current_state,
        Vertex_A *next_node,
        Edge *edge,
        priority_queue<PathState> &pq,
        unordered_map<Vertex_A *, float> &max_capacity_map);

    void propagate_demand_change(Vertex_A *node, float delta, unordered_set<Vertex_A *> &visited_propagation);


    // Helper function for the Dijkstra's search
    bool can_handle_increase(Vertex_A *node, float increase, unordered_set<Vertex_A *> &visited);

    // Helper function for the  "widest path" search 
    void propagate_limit_increase(Vertex_A *node, float increase, unordered_set<Vertex_A *> &visited);


    

public:
    vector<pair<string, pair<int, int>>> layout_graph();

    void add_realistic_connections(int k_nearest = 2); // k_nearsest -->number of substations a substation be connected

    Vertex_B *increase_random_consumer_demand()
    {
        int total_consumers = 0;
        for (auto const &[substation, consumer_list] : adj_substion_consumer)
        {
            total_consumers += consumer_list.size();
        }

        if (total_consumers == 0)
        {
            cout << "Event: No consumers found to increase demand." << endl;
            return nullptr;
        }

        int random_index = rand() % total_consumers;

        for (auto const &[substation, consumer_list] : adj_substion_consumer)
        {
            if (random_index < consumer_list.size())
            {
                auto &[consumer_node, consumer_edge] = consumer_list[random_index];

                // 1. Calculate proposed increase
                float increase = std::max(5.0f, consumer_node->demand * 0.2f);


                if (consumer_edge->current_load + increase > consumer_edge->max_load)
                {
                    cout << "--- Random Event FAILED ---" << endl;
                    cout << "  Consumer: " << consumer_node->node_type << " (" << consumer_node << ")" << endl;
                    cout << "  Reason: Local consumer line is at max capacity." << endl;
                    return nullptr; // Stop here
                }

                // All other checks are removed. Apply the change.
                consumer_node->demand += increase;
                consumer_edge->current_load += increase;

                // 3. Propagate this increase up the network
                unordered_set<Vertex_A *> visited_propagation;
                propagate_demand_change(substation, increase, visited_propagation);

                cout << "--- Random Event ---" << endl;
                cout << "Increased demand for " << consumer_node->node_type
                     << " (" << consumer_node << ")" << endl;
                cout << "  By: " << increase << " kW. New Total: " << consumer_node->demand << " kW." << endl;
                cout << "  Parent Substation: " << substation << endl;

                this->last_demand_increase_consumer = consumer_node;
                return consumer_node;
            }

            random_index -= consumer_list.size();
        }
        return nullptr;
    }

   // Helper struct to store and sort overload problems
    struct OverloadInfo
    {
        float ratio; // e.g., 0.95 (95%)
        Vertex_A *source;
        Vertex_A *target;
        Edge *edge;

        // Sort by overload ratio, descending (most severe first)
        bool operator>(const OverloadInfo &other) const
        {
            return ratio > other.ratio;
        }
    };
    void check_edge_overloads();
    void fix_all_overloads();
    void overloading_edge();


    bool fix_overloading_problem(Vertex_A *source, Vertex_A *target,
                                 Edge *overloaded_edge,
                                 bool visualize_this_fix);


    void apply_demand_reduction_updates();


    void check_node_overloads();

    void upgrade_selected_node_limit(Vertex_A *selected_node);

} ;



inline vector<int> graph::distributor(int total_count, int divide_count)
{
        if (divide_count <= 0)
            return {};
        vector<int> weights(divide_count);
        int weightconsumers = 0;
        int base = total_count / divide_count;
        int minWeight = max(1, base - (3 * base) / 10); // 30% less than average
        int maxWeight = max(1, base + (3 * base) / 10); // 30% more than average
        for (int i = 0; i < divide_count; i++)
        {
            weights[i] = rand() % (maxWeight - minWeight + 1) + minWeight;
            weightconsumers += weights[i];
        }

        if (weightconsumers == 0)
        { // Avoid divide by zero
            for (int i = 0; i < divide_count; i++)
                weights[i] = 1;
            weightconsumers = divide_count;
        }

        vector<int> distribution(divide_count);
        int distributedconsumers = 0;
        for (int i = 0; i < divide_count - 1; i++)
        {
            distribution[i] = (long long)weights[i] * total_count / weightconsumers;
            distributedconsumers += distribution[i];
        }
        distribution[divide_count - 1] = total_count - distributedconsumers;
        return distribution;
    }

inline void graph::mapping(int powerplant_count, int substation_count, int consumer_count)
{
        vector<Vertex_A *> powerplants;
        for (int i = 0; i < powerplant_count; i++)
            powerplants.push_back(new Vertex_A("power_plant"));
        vector<Vertex_A *> substations;
        vector<int> Substations = distributor(consumer_count, substation_count);
        for (int val : Substations)
        {
            Vertex_A *s = new Vertex_A("substation");
            substation_consumer_connector(s, val); // substation Vertex and number of consumers needs to be mapped with it
            substations.push_back(s);
        }
        map_powerplants_to_substations(powerplants, substations);
    }

inline void graph::substation_consumer_connector(Vertex_A *substation, int value)
{
        int hospital_count = 0.2f * value;
        int industrial_count = 0.1f * value;
        int institute_count = 0.2f * value;
        int resident_count = value - (hospital_count + industrial_count + institute_count);
        int total_demand = 0;
        vector<pair<Vertex_B *, Edge *>> connect;
        auto add_consumer = [&](const string &type, int count)
        {
            for (int i = 0; i < count; i++)
            {
                Vertex_B *node = new Vertex_B(type);
                total_demand += node->demand;
                Edge *edge = new Edge(substation, node); // Constructor sets loads
                connect.push_back({node, edge});
            }
        };
        add_consumer("hospital", hospital_count);
        add_consumer("industrial", industrial_count);
        add_consumer("institute", institute_count);
        add_consumer("residential", resident_count);

        substation->max_limit = total_demand * 1.5f;          // Node's max capacity (150% of demand)
        substation->current_downstream_demand = total_demand; // Node's current load

        adj_substion_consumer[substation] = connect;
    }

inline void graph::map_powerplants_to_substations(vector<Vertex_A *> &powerplants, vector<Vertex_A *> &substations)
{
        int num_powerplants = (int)powerplants.size();
        int num_substations = (int)substations.size();

        // --- Percentage of substations connected directly to power plants ---
        int substations_for_powerplants = (int)(0.4 * num_substations); // 40%

        if (num_substations > 0 && num_powerplants > 0 && substations_for_powerplants == 0)
        {
            substations_for_powerplants = 1;
        }

        vector<int> substation_indices(num_substations);
        iota(substation_indices.begin(), substation_indices.end(), 0); // iota --> fill the vector with sequential values starting from 1
        shuffle(substation_indices.begin(), substation_indices.end(), g);
        int idx = 0;

        // --- 1. Connect Power Plants to primary Substations ---
        for (int i = 0; i < substations_for_powerplants && idx < num_substations; i++, idx++)
        {
            Vertex_A *plant = powerplants[i % num_powerplants];
            Vertex_A *sub = substations[substation_indices[idx]];
            Edge *edge = new Edge(plant, sub); // A-to-A constructor

            // Set edge loads based on the child substation
            edge->current_load = sub->current_downstream_demand;
            edge->max_load = 2 * sub->max_limit;

            adj_power_substation[plant].push_back({sub, edge});
            adj_reverse_power[sub].push_back({plant, edge});
        }

        // --- 2. Connect remaining Substations to ANY previously placed Substation (now 60%) ---
        while (idx < num_substations)
        {
            // This check prevents a divide-by-zero if idx is 0
            if (idx == 0)
                break;

            int src_idx = rand() % idx; // Picks ANY previously placed parent

            Vertex_A *src = substations[substation_indices[src_idx]]; // Parent substation (can be primary or secondary)
            Vertex_A *dest = substations[substation_indices[idx]];    // Child substation
            Edge *edge = new Edge(src, dest);

            // Propagate loads up the tree
            src->max_limit += dest->max_limit;                                 // Parent's limit must accommodate the child's limit
            src->current_downstream_demand += dest->current_downstream_demand; // Parent's load increases

            // Set edge loads based on the child substation
            edge->current_load = dest->current_downstream_demand;
            edge->max_load = dest->max_limit;

            adj_power_substation[src].push_back({dest, edge});
            adj_reverse_power[dest].push_back({src, edge});
            idx++;
        }

        // --- 3. Set Power Plant total limits based on their children ---
        for (auto &plant_entry : adj_power_substation)
        {
            Vertex_A *plant = plant_entry.first;
            if (plant->node_type != "power_plant")
                continue; // Only process plants

            float sum_substation_limits = 0;
            float sum_substation_demands = 0;

            for (auto &pair : plant_entry.second)
            {
                Vertex_A *sub = pair.first; // This is a direct child substation
                sum_substation_limits += sub->max_limit;
                sum_substation_demands += sub->current_downstream_demand;
            }
            plant->max_limit = sum_substation_limits * 1.2f; // 120% of all substation's limit

            // The plant's "current load" is the sum of all demands it serves
            plant->current_downstream_demand = sum_substation_demands;
        }
    }

inline double graph::GetDistance(int x1, int y1, int x2, int y2)
{
        return sqrt(pow(static_cast<double>(x1 - x2), 2) + pow(static_cast<double>(y1 - y2), 2));
    }

inline int graph::GetRandom(int min_val, int max_val)
{
        if (min_val > max_val)
            std::swap(min_val, max_val);
        return (rand() % (max_val - min_val + 1)) + min_val;
    }

inline bool graph::is_position_ok_A(Vertex_A *node_to_place, int x, int y,const unordered_map<GridKey, vector<Vertex_A *>, PairHash> &grid,int CELL_SIZE)
{
        float min_dist = (node_to_place->node_type == "power_plant") ? 400.0f : 50.0f;
        int grid_x = x / CELL_SIZE;
        int grid_y = y / CELL_SIZE;

        for (int dx = -1; dx <= 1; ++dx)
        {
            for (int dy = -1; dy <= 1; ++dy)
            {
                GridKey key = {grid_x + dx, grid_y + dy};
                if (grid.count(key))
                {
                    for (auto *other_node : grid.at(key))
                    {
                        if (other_node->node_type == node_to_place->node_type)
                        {
                            if (GetDistance(x, y, other_node->x, other_node->y) < min_dist)
                            {
                                return false;
                            }
                        }
                    }
                }
            }
        }
        return true;
    }

inline bool graph::is_position_ok_B(Vertex_B *node_to_place, int x, int y,const unordered_map<GridKey, vector<Vertex_B *>, PairHash> &grid,int CELL_SIZE)
{
        const float min_dist = 5.0f;
        int grid_x = x / CELL_SIZE;
        int grid_y = y / CELL_SIZE;

        for (int dx = -1; dx <= 1; ++dx)
        {
            for (int dy = -1; dy <= 1; ++dy)
            {
                GridKey key = {grid_x + dx, grid_y + dy};
                if (grid.count(key))
                {
                    for (auto *other_node : grid.at(key))
                    {
                        if (GetDistance(x, y, other_node->x, other_node->y) < min_dist)
                        {
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }

inline bool graph::DoesEdgeExist(Vertex_A *sub1, Vertex_A *sub2)
{
        // Check A -> B
        if (adj_power_substation.count(sub1))
        {
            for (auto &pair : adj_power_substation[sub1])
            {
                if (pair.first == sub2)
                    return true;
            }
        }
        // Check B -> A
        if (adj_power_substation.count(sub2))
        {
            for (auto &pair : adj_power_substation[sub2])
            {
                if (pair.first == sub1)
                    return true;
            }
        }
        return false;
    }

inline vector<pair<string, pair<int, int>>> graph::layout_graph()
{
        vector<pair<string, pair<int, int>>> coordinates_list;
        unordered_set<Vertex_A *> visited_a;
        unordered_set<Vertex_B *> visited_b;
        queue<Vertex_A *> q; // Queue for BFS traversal

        vector<Vertex_A *> power_plants;

        using GridKey = std::pair<int, int>;
        unordered_map<GridKey, vector<Vertex_A *>, PairHash> placed_a_grid;
        unordered_map<GridKey, vector<Vertex_B *>, PairHash> placed_b_grid;

        const int MAX_TRIES_PER_NODE = 1000;
        const int PLANT_BOUNDS_MIN = 0;
        const int PLANT_BOUNDS_MAX = 2000;
        const int SUB_RANGE = 200;
        const int CONSUMER_RANGE = 50;
        const int CELL_SIZE = 50;

        // --- 1. Collect Power Plants ---
        for (auto const &map_entry : adj_power_substation)
        {
            if (map_entry.first->node_type == "power_plant")
            {
                power_plants.push_back(map_entry.first);
            }
        }

        if (power_plants.empty())
        {
            cout << "No power plants found to start layout." << endl;
            return coordinates_list;
        }

        // --- 2. Place Power Plants Randomly ---
        for (auto *plant : power_plants)
        {
            if (visited_a.count(plant))
                continue;

            int tries = 0;
            bool placed = false;
            while (tries < MAX_TRIES_PER_NODE)
            {
                int x = GetRandom(PLANT_BOUNDS_MIN, PLANT_BOUNDS_MAX);
                int y = GetRandom(PLANT_BOUNDS_MIN, PLANT_BOUNDS_MAX);

                if (is_position_ok_A(plant, x, y, placed_a_grid, CELL_SIZE))
                {
                    plant->x = x;
                    plant->y = y;
                    coordinates_list.push_back({plant->node_type, {x, y}});
                    visited_a.insert(plant);
                    q.push(plant);

                    int grid_x = x / CELL_SIZE;
                    int grid_y = y / CELL_SIZE;
                    placed_a_grid[{grid_x, grid_y}].push_back(plant);

                    placed = true;
                    break;
                }
                tries++;
            }

            if (!placed)
            {
                cout << "Warning: Failed to find ideal spot for power plant " << plant << ". Placing anyway.\n";
                plant->x = GetRandom(PLANT_BOUNDS_MIN, PLANT_BOUNDS_MAX);
                plant->y = GetRandom(PLANT_BOUNDS_MIN, PLANT_BOUNDS_MAX);
                coordinates_list.push_back({plant->node_type, {plant->x, plant->y}});
                visited_a.insert(plant);
                q.push(plant);

                int grid_x = plant->x / CELL_SIZE;
                int grid_y = plant->y / CELL_SIZE;
                placed_a_grid[{grid_x, grid_y}].push_back(plant);
            }
        }

        // --- 3. Position Substations and Consumers (BFS) ---
        while (!q.empty())
        {
            Vertex_A *parent = q.front();
            q.pop();

            // --- 3a. Place child A-nodes (substations) ---
            if (adj_power_substation.count(parent))
            {
                for (auto &edge_pair : adj_power_substation[parent])
                {
                    Vertex_A *child_sub = edge_pair.first;
                    if (child_sub->node_type == "substation" && visited_a.find(child_sub) == visited_a.end())
                    {
                        int tries = 0;
                        bool placed = false;
                        while (tries < MAX_TRIES_PER_NODE)
                        {
                            int x = GetRandom(parent->x - SUB_RANGE, parent->x + SUB_RANGE);
                            int y = GetRandom(parent->y - SUB_RANGE, parent->y + SUB_RANGE);

                            if (is_position_ok_A(child_sub, x, y, placed_a_grid, CELL_SIZE))
                            {
                                child_sub->x = x;
                                child_sub->y = y;
                                coordinates_list.push_back({child_sub->node_type, {x, y}});
                                visited_a.insert(child_sub);
                                q.push(child_sub);

                                int grid_x = x / CELL_SIZE;
                                int grid_y = y / CELL_SIZE;
                                placed_a_grid[{grid_x, grid_y}].push_back(child_sub);

                                placed = true;
                                break;
                            }
                            tries++;
                        }
                        if (!placed)
                        {
                            child_sub->x = parent->x;
                            child_sub->y = parent->y;
                            coordinates_list.push_back({child_sub->node_type, {child_sub->x, child_sub->y}});
                            visited_a.insert(child_sub);
                            q.push(child_sub);

                            int grid_x = child_sub->x / CELL_SIZE;
                            int grid_y = child_sub->y / CELL_SIZE;
                            placed_a_grid[{grid_x, grid_y}].push_back(child_sub);
                        }
                    }
                }
            }

            // --- 3b. Place child B-nodes (consumers) ---
            if (adj_substion_consumer.count(parent))
            {
                for (auto &edge_pair : adj_substion_consumer[parent])
                {
                    Vertex_B *consumer = edge_pair.first;
                    if (visited_b.find(consumer) == visited_b.end())
                    {
                        int tries = 0;
                        bool placed = false;
                        while (tries < MAX_TRIES_PER_NODE)
                        {
                            int x = GetRandom(parent->x - CONSUMER_RANGE, parent->x + CONSUMER_RANGE);
                            int y = GetRandom(parent->y - CONSUMER_RANGE, parent->y + CONSUMER_RANGE);

                            if (is_position_ok_B(consumer, x, y, placed_b_grid, CELL_SIZE))
                            {
                                consumer->x = x;
                                consumer->y = y;
                                coordinates_list.push_back({consumer->node_type, {x, y}});
                                visited_b.insert(consumer);

                                int grid_x = x / CELL_SIZE;
                                int grid_y = y / CELL_SIZE;
                                placed_b_grid[{grid_x, grid_y}].push_back(consumer);

                                placed = true;
                                break;
                            }
                            tries++;
                        }
                        if (!placed)
                        {
                            consumer->x = parent->x;
                            consumer->y = parent->y;
                            coordinates_list.push_back({consumer->node_type, {consumer->x, consumer->y}});
                            visited_b.insert(consumer);

                            int grid_x = consumer->x / CELL_SIZE;
                            int grid_y = consumer->y / CELL_SIZE;
                            placed_b_grid[{grid_x, grid_y}].push_back(consumer);
                        }
                    }
                }
            }
        }
        return coordinates_list;
    }

inline void graph::add_realistic_connections(int k_nearest) // k_nearsest -->number of substations a substation be connected
{
        // 1. Collect all substations from all adjacency lists
        unordered_set<Vertex_A *> sub_set;
        for (auto &entry : adj_substion_consumer)
        {
            sub_set.insert(entry.first); // Add all subs that have consumers
        }
        for (auto &entry : adj_power_substation)
        {
            if (entry.first->node_type == "substation")
                sub_set.insert(entry.first);
            for (auto &pair : entry.second)
            {
                if (pair.first->node_type == "substation")
                    sub_set.insert(pair.first);
            }
        }

        vector<Vertex_A *> all_substations(sub_set.begin(), sub_set.end());
        if (all_substations.size() < k_nearest + 1)
            return; // Not enough to connect

        // 2. For each substation, find its k-nearest neighbors and connect
        for (auto *sub1 : all_substations)
        {
            priority_queue<pair<double, Vertex_A *>> pq;

            for (auto *sub2 : all_substations)
            {
                if (sub1 == sub2)
                    continue; // Don't connect to self

                double dist = GetDistance(sub1->x, sub1->y, sub2->x, sub2->y);

                if (pq.size() < k_nearest)
                {
                    pq.push({dist, sub2});
                }
                else if (dist < pq.top().first)
                {
                    // If this node is closer than the "farthest" of the k-nearest
                    pq.pop();
                    pq.push({dist, sub2});
                }
            }

            // 3. Add edges to the k-nearest neighbors
            while (!pq.empty())
            {
                Vertex_A *sub2 = pq.top().second;
                pq.pop();

                // If an edge doesn't already exist (in either direction)
                if (!DoesEdgeExist(sub1, sub2))
                {
                    // Create a "unnecessary" edge for visual/backup purposes
                    Edge *edge = new Edge(sub1, sub2);
                    edge->current_load = 0;                                 // It's a backup line
                    edge->max_load = min(sub1->max_limit, sub2->max_limit); // Max load is for info

                    adj_power_substation[sub1].push_back({sub2, edge});
                    adj_reverse_power[sub2].push_back({sub1, edge});
                }
            }
        }
    }

#include "redistribution.hpp"

#endif // GRAPH_HPP
