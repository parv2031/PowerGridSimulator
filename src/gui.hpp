#ifndef GUI_HPP
#define GUI_HPP
#include "graph.hpp"
#include "graph.hpp"

// --- RAYLIB HELPER FUNCTIONS --- 
void DrawNodeA(graph::Vertex_A *node)
{
    if (node->node_type == "power_plant")
    {
        DrawCircle(node->x, node->y, 15.0f, PINK);
        DrawCircleLines(node->x, node->y, 15.0f, MAROON);
    }
    else if (node->node_type == "substation")
    {
        DrawCircle(node->x, node->y, 10.0f, BLUE);
        DrawCircleLines(node->x, node->y, 10.0f, DARKBLUE);
    }
}
void DrawNodeB(graph::Vertex_B *node)
{
    float radius = 5.0f;
    Color color = BROWN; // Default for residential

    if (node->node_type == "hospital")
    {
        color = DARKBLUE;
        radius = 7.0f;
    }
    else if (node->node_type == "industrial")
    {
        color = ORANGE;
        radius = 7.0f;
    }
    else if (node->node_type == "institute")
    {
        color = PURPLE;
        radius = 7.0f;
    }

    DrawCircle(node->x, node->y, radius, color);
}
void DrawArrow(Vector2 start, Vector2 end, float endRadius, float thickness, Color color)
{
    // 1. Calculate direction and the point where the arrow should stop
    Vector2 dir = Vector2Normalize(Vector2Subtract(end, start));
    Vector2 arrowPoint = Vector2Subtract(end, Vector2Scale(dir, endRadius));

    // 2. Draw the main line
    DrawLineEx(start, arrowPoint, thickness, color);

    // 3. Calculate the two "wings" of the arrowhead
    float arrowSize = 8.0f; // Size of the arrowhead wings
    Vector2 leftWingDir = Vector2Rotate(dir, -150 * DEG2RAD);
    Vector2 rightWingDir = Vector2Rotate(dir, 150 * DEG2RAD);

    Vector2 leftWingEnd = Vector2Add(arrowPoint, Vector2Scale(leftWingDir, arrowSize / (thickness > 2.0f ? 1.5f : 1.0f)));
    Vector2 rightWingEnd = Vector2Add(arrowPoint, Vector2Scale(rightWingDir, arrowSize / (thickness > 2.0f ? 1.5f : 1.0f)));

    // 4. Draw the wings
    DrawLineEx(arrowPoint, leftWingEnd, thickness, color);
    DrawLineEx(arrowPoint, rightWingEnd, thickness, color);
}
// --- Define Game States ---
enum GameState
{
    STATE_INPUT,
    STATE_VISUALIZATION
};

//RAYLIB MAIN FUNCTION
void run_simulation(){

    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    srand((unsigned int)time(0)); // Seed random number generator

    // --- 1. Initialize Raylib Window ---
    InitWindow(1280, 720, "Power Grid Visualizer");
    int monitor = GetCurrentMonitor();
    int screenWidth = GetMonitorWidth(monitor);
    int screenHeight = GetMonitorHeight(monitor);
    SetWindowSize(screenWidth, screenHeight);
    ToggleFullscreen();

    // --- 2. Define States ---
    GameState currentState = STATE_INPUT;

    // --- 3. Variables for STATE_INPUT (Using basic Raylib) ---
    const int inputFieldCount = 7;
    const char *labels[inputFieldCount] = {
        "Power Plants:", "Substations:", "Consumers:",
        "Resident Demand:", "Hospital Demand:", "Industrial Demand:", "Institute Demand:"};
    Rectangle textBoxes[inputFieldCount];
    std::string inputStrings[inputFieldCount]; 
    int activeTextBox = -1;                    // Index of the currently selected text box, -1 for none

    // Set default values for the strings
    inputStrings[0] = "3";
    inputStrings[1] = "18";
    inputStrings[2] = "100"; // scaled by 1/9
    inputStrings[3] = "10";
    inputStrings[4] = "100";
    inputStrings[5] = "500";
    inputStrings[6] = "80";

    Rectangle startButton;
    bool showCursor = false;
    int framesCounter = 0;

    // Layout for input screen
    int startY = 100;
    int inputHeight = 40;
    int inputWidth = 200;
    int labelWidth = 250;
    int padding = 15;
    int fontSize = 20;

    for (int i = 0; i < inputFieldCount; i++)
    {
        textBoxes[i] = {(float)screenWidth / 2 - inputWidth / 2, (float)startY + i * (inputHeight + padding), (float)inputWidth, (float)inputHeight};
    }
    startButton = {(float)screenWidth / 2 - inputWidth / 2, (float)startY + inputFieldCount * (inputHeight + padding) + 20, (float)inputWidth, (float)inputHeight + 10};

    // --- 4. Variables for STATE_VISUALIZATION ---
    graph G; // Create graph object
    Camera2D camera = {0};
    const float ZOOM_LOD_SUBSTATION = 0.5f;
    const float ZOOM_LOD_CONSUMER = 1.0f;
    graph::Vertex_A *selected_a_node = nullptr;
    graph::Vertex_B *selected_b_node = nullptr;
    graph::Edge *selected_edge = nullptr;
    unordered_set<void *> visited_nodes;
    vector<graph::Vertex_A *> a_nodes_to_draw;
    vector<graph::Vertex_B *> b_nodes_to_draw;

    // ---  Variables for timed highlighting ---
    enum FixVizState
    {
        VIZ_IDLE,
        VIZ_SHOWING_OVERLOAD,
        VIZ_SHOWING_FIX
    };
    FixVizState fix_viz_state = VIZ_IDLE;
    double fix_highlight_start_time = 0.0; // Time when overload highlight started
    // --- END NEW ---

    SetTargetFPS(60);

    // --- 5. Main Game Loop ---
    while (!WindowShouldClose())
    {
        switch (currentState)
        {
        case STATE_INPUT:
        {
            // --- Update Input State ---
            framesCounter++;
            if ((framesCounter / 30) % 2 == 0)
                showCursor = true; // Blink cursor every half second
            else
                showCursor = false;

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                activeTextBox = -1; // Deselect by default
                for (int i = 0; i < inputFieldCount; i++)
                {
                    if (CheckCollisionPointRec(GetMousePosition(), textBoxes[i]))
                    {
                        activeTextBox = i;
                        framesCounter = 0; // Reset cursor blink timer
                        break;
                    }
                }
            }

            // Handle keyboard input if a text box is active
            if (activeTextBox != -1)
            {
                int key = GetCharPressed();
                while (key > 0)
                {
                    // Only allow digits
                    if ((key >= '0' && key <= '9'))
                    {
                        if (inputStrings[activeTextBox].length() < 9)
                        { // Limit length
                            inputStrings[activeTextBox] += (char)key;
                        }
                    }
                    key = GetCharPressed(); // Check next character in queue
                }

                if (IsKeyPressed(KEY_BACKSPACE))
                {
                    if (!inputStrings[activeTextBox].empty())
                    {
                        inputStrings[activeTextBox].pop_back();
                    }
                }
            }

            // Check Start Button Click
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), startButton))
            {
                try
                {
                    // --- 1. Parse all inputs (use stoi, catch exceptions) ---
                    int num_power_plant = std::stoi(inputStrings[0]);
                    int num_substation = std::stoi(inputStrings[1]);
                    int num_consumer = std::stoi(inputStrings[2]);
                    resident_demand = std::stoi(inputStrings[3]);
                    hospital_demand = std::stoi(inputStrings[4]);
                    industry_demand = std::stoi(inputStrings[5]);
                    institute_demand = std::stoi(inputStrings[6]);

                    // --- Basic Input Validation ---
                    if (num_power_plant <= 0 || num_substation < 0 || num_consumer <= 0 ||
                        resident_demand <= 0 || hospital_demand <= 0 || industry_demand <= 0 || institute_demand <= 0)
                    {
                        cout << "Error: All inputs must be positive integers (substations can be 0)." << endl;
                        // Optionally show an error message on screen
                    }
                    else
                    {
                        // --- 2. Build and Layout Graph ---
                        cout << "Building graph...\n";
                        G.mapping(num_power_plant, num_substation, num_consumer);
                        cout << "Network successfully built.\n";

                        G.layout_graph();
                        cout << "Graph layout calculated.\n";

                        G.add_realistic_connections(2); // Connect to 1 nearest neighbor
                        cout << "Realistic redundant connections added.\n";

                        // --- 3. Setup Visualization Camera ---
                        camera.target = (Vector2){1000, 1000};
                        camera.offset = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f};
                        camera.rotation = 0.0f;
                        camera.zoom = 0.25f;

                        // --- 4. Switch State ---
                        currentState = STATE_VISUALIZATION;
                        cout << "Starting visualization...\n";
                    }
                }
                catch (const std::invalid_argument &e)
                {
                    cout << "Error: Invalid input. Please enter numbers only." << endl;
                    // Optionally show an error message on screen
                }
                catch (const std::out_of_range &e)
                {
                    cout << "Error: Input number is too large." << endl;
                    // Optionally show an error message on screen
                }
            }

            // --- Draw Input State ---
            BeginDrawing();
            ClearBackground(DARKGRAY);
            DrawText("Power Grid Simulation Setup", screenWidth / 2 - MeasureText("Power Grid Simulation Setup", 30) / 2, 30, 30, WHITE);

            for (int i = 0; i < inputFieldCount; i++)
            {
                // Draw label
                DrawText(labels[i], textBoxes[i].x - labelWidth, textBoxes[i].y + (inputHeight - fontSize) / 2.0f, fontSize, LIGHTGRAY);
                // Draw text box rectangle
                DrawRectangleRec(textBoxes[i], LIGHTGRAY);
                if (activeTextBox == i)
                    DrawRectangleLinesEx(textBoxes[i], 2, RED); // Highlight active box
                else
                    DrawRectangleLinesEx(textBoxes[i], 1, DARKGRAY);

                // Draw text inside box
                DrawText(inputStrings[i].c_str(), (int)(textBoxes[i].x + 5), (int)(textBoxes[i].y + (inputHeight - fontSize) / 2.0f), fontSize, BLACK);

                // Draw blinking cursor
                if (activeTextBox == i && showCursor)
                {
                    int textWidth = MeasureText(inputStrings[i].c_str(), fontSize);
                    DrawRectangle((int)(textBoxes[i].x + 5 + textWidth), (int)(textBoxes[i].y + 4), 2, inputHeight - 8, MAROON);
                }
            }

            // Draw Start Button
            DrawRectangleRec(startButton, MAROON);
            DrawText("START SIMULATION", startButton.x + startButton.width / 2 - MeasureText("START SIMULATION", fontSize) / 2, startButton.y + (startButton.height - fontSize) / 2, fontSize, WHITE);

            EndDrawing();
        }
        break;

        case STATE_VISUALIZATION:
        {
            // --- Pre-populate node lists for update and draw phases ---
            visited_nodes.clear();
            a_nodes_to_draw.clear();
            b_nodes_to_draw.clear();

            for (auto const &map_entry : G.adj_power_substation)
            {
                graph::Vertex_A *parent = map_entry.first;
                if (visited_nodes.find(parent) == visited_nodes.end())
                {
                    a_nodes_to_draw.push_back(parent);
                    visited_nodes.insert(parent);
                }
                for (auto &edge_pair : map_entry.second)
                {
                    graph::Vertex_A *child = edge_pair.first;
                    if (visited_nodes.find(child) == visited_nodes.end())
                    {
                        a_nodes_to_draw.push_back(child);
                        visited_nodes.insert(child);
                    }
                }
            }
            for (auto const &map_entry : G.adj_substion_consumer)
            {
                graph::Vertex_A *parent = map_entry.first;
                if (visited_nodes.find(parent) == visited_nodes.end())
                {
                    a_nodes_to_draw.push_back(parent);
                    visited_nodes.insert(parent);
                }
                for (auto &edge_pair : map_entry.second)
                {
                    graph::Vertex_B *child = edge_pair.first;
                    if (visited_nodes.find(child) == visited_nodes.end())
                    {
                        b_nodes_to_draw.push_back(child);
                        visited_nodes.insert(child);
                    }
                }
            }
            /*Apply any demand reduction from the *previous* frame's failed fixes
               This MUST be called every frame.*/
            G.apply_demand_reduction_updates();

            // --- Update Camera ---
            camera.zoom += ((float)GetMouseWheelMove() * 0.05f);
            if (camera.zoom > 3.0f)
                camera.zoom = 3.0f;
            if (camera.zoom < 0.1f)
                camera.zoom = 0.1f;

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                Vector2 delta = GetMouseDelta();
                delta = Vector2Scale(delta, -1.0f / camera.zoom);
                camera.target = Vector2Add(camera.target, delta);
            }
            if (IsKeyPressed(KEY_I))
            {
                // Clear fix visualization if demand changes
                fix_viz_state = VIZ_IDLE;
                G.edge_being_fixed = nullptr;
                G.fix_path.clear();
                graph::Vertex_B *event_node = G.increase_random_consumer_demand();
                G.check_node_overloads();
                
                G.status_message = "Increased demand at random consumer.";
                G.check_edge_overloads();
                
                // If the function successfully found a node...
                if (event_node != nullptr)
                {
                    // De-select whatever else was selected
                    selected_a_node = nullptr;
                    selected_edge = nullptr;

                    // Set the event node as the new selected node
                    selected_b_node = event_node;
                    camera.target = (Vector2){(float)event_node->x, (float)event_node->y};
                    // Zoom in to a close-up level
                    camera.zoom = 1.5f;
                }
                //G.overloading_edge();
                //G.apply_demand_reduction_updates();
            }
            if (IsKeyPressed(KEY_O))
            {
                // Clear any previous fix visualization first
                fix_viz_state = VIZ_IDLE;
                G.edge_being_fixed = nullptr;
                G.fix_path.clear();
                G.last_demand_increase_consumer = nullptr;
                
                G.fix_all_overloads();
                G.check_node_overloads();

                // Check if a fix was applied and stored
                if (G.edge_being_fixed != nullptr)
                {
                    fix_viz_state = VIZ_SHOWING_OVERLOAD; // Start visualization sequence
                    fix_highlight_start_time = GetTime(); // Record start time
                    cout << "Starting fix visualization for edge: " << G.edge_being_fixed << endl;
                }
                else
                {
                    cout << "Overload check ran, but no fixable overload was processed or stored for visualization." << endl;
                }
                G.apply_demand_reduction_updates();
            }
            if (IsKeyPressed(KEY_U))
            {
                // This calls the new function on the currently selected node
                G.upgrade_selected_node_limit(selected_a_node);
            }
            if (IsKeyPressed(KEY_C))
            {
                G.overloaded_edges.clear();   // Clear the highlights
                G.edge_being_fixed = nullptr; // Clear green edge highlight
                G.fix_path.clear();
                fix_viz_state = VIZ_IDLE;
                G.throttled_nodes_visual.clear();
                G.node_overloads_visual.clear(); 
                cout << "Cleared all highlights." << endl;
            }
            if (IsKeyPressed(KEY_BACKSPACE))
            {
                G = graph();                // Clear graph
                currentState = STATE_INPUT; // Go back to input screen
            }
            // --- Update selection state ---
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 worldMousePos = GetScreenToWorld2D(GetMousePosition(), camera);

                selected_a_node = nullptr;
                selected_b_node = nullptr;
                selected_edge = nullptr;

                bool foundClick = false;

                // Check B-Nodes
                if (camera.zoom >= ZOOM_LOD_CONSUMER)
                {
                    for (auto *node : b_nodes_to_draw)
                    {
                        float radius = 5.0f;
                        if (node->node_type == "hospital" || node->node_type == "industrial" || node->node_type == "institute")
                            radius = 7.0f;

                        if (CheckCollisionPointCircle(worldMousePos, {(float)node->x, (float)node->y}, radius))
                        {
                            selected_b_node = node;
                            foundClick = true;
                            break;
                        }
                    }
                }

                // Check A-Nodes
                if (!foundClick)
                {
                    for (auto *node : a_nodes_to_draw)
                    {
                        float radius = 0.0f;
                        if (node->node_type == "power_plant")
                        {
                            radius = 15.0f;
                        }
                        else if (node->node_type == "substation" && camera.zoom >= ZOOM_LOD_SUBSTATION)
                        {
                            radius = 10.0f;
                        }

                        if (radius > 0.0f && CheckCollisionPointCircle(worldMousePos, {(float)node->x, (float)node->y}, radius))
                        {
                            selected_a_node = node;
                            foundClick = true;
                            break;
                        }
                    }
                }

                const int lineClickThreshold = 5;

                // Check Edges (A -> A)
                if (!foundClick)
                {
                    if (camera.zoom >= ZOOM_LOD_SUBSTATION)
                    {
                        for (auto const &map_entry : G.adj_power_substation)
                        {
                            graph::Vertex_A *parent = map_entry.first;
                            for (auto &edge_pair : map_entry.second)
                            {
                                graph::Vertex_A *child = edge_pair.first;
                                graph::Edge *edge = edge_pair.second;

                                if (CheckCollisionPointLine(worldMousePos,
                                                            {(float)parent->x, (float)parent->y},
                                                            {(float)child->x, (float)child->y},
                                                            lineClickThreshold / camera.zoom))
                                {
                                    selected_edge = edge;
                                    foundClick = true;
                                    break;
                                }
                            }
                            if (foundClick)
                                break;
                        }
                    }
                }

                // Check Edges (A -> B)
                if (!foundClick)
                {
                    if (camera.zoom >= ZOOM_LOD_CONSUMER)
                    {
                        for (auto const &map_entry : G.adj_substion_consumer)
                        {
                            graph::Vertex_A *parent = map_entry.first;
                            for (auto &edge_pair : map_entry.second)
                            {
                                graph::Vertex_B *child = edge_pair.first;
                                graph::Edge *edge = edge_pair.second;

                                if (CheckCollisionPointLine(worldMousePos,
                                                            {(float)parent->x, (float)parent->y},
                                                            {(float)child->x, (float)child->y},
                                                            lineClickThreshold / camera.zoom))
                                {
                                    selected_edge = edge;
                                    foundClick = true;
                                    break;
                                }
                            }
                            if (foundClick)
                                break;
                        }
                    }
                }
                if (fix_viz_state == VIZ_SHOWING_OVERLOAD && (GetTime() - fix_highlight_start_time >= 0.5))
                {
                    fix_viz_state = VIZ_SHOWING_FIX; // Switch to showing the fix path after 0.5 seconds
                    cout << "Switching to fix path visualization." << endl;
                }
            }

            // --- Draw Scene ---
            BeginDrawing();
            ClearBackground(DARKGRAY);

            BeginMode2D(camera);

            // --- Draw Edges (A -> A) ---
            unordered_set<graph::Edge *> path_edges_for_fix_viz;
            if (fix_viz_state == VIZ_SHOWING_FIX)
            {
                for (size_t i = 1; i < G.fix_path.size(); ++i)
                {
                    if (G.fix_path[i].second)
                    {
                        path_edges_for_fix_viz.insert(G.fix_path[i].second);
                    }
                }
            }

            for (auto const &map_entry : G.adj_power_substation)
            {
                graph::Vertex_A *parent = map_entry.first;
                for (auto &edge_pair : map_entry.second)
                {
                    graph::Vertex_A *child = edge_pair.first;
                    graph::Edge *edge = edge_pair.second;

                    Color edgeColor = GRAY;
                    float thickness = 1.0f;
                    bool drawEdge = false;

                    // Determine Base Color
                    if (parent->node_type == "power_plant")
                        edgeColor = YELLOW;
                    else if (parent->node_type == "substation")
                        edgeColor = SKYBLUE;

                    // Check Zoom Level
                    if (camera.zoom >= ZOOM_LOD_SUBSTATION)
                        drawEdge = true;

                    if (drawEdge)
                    {
                        // Get start/end vectors and child radius for the arrow
                        Vector2 startV = {(float)parent->x, (float)parent->y};
                        Vector2 endV = {(float)child->x, (float)child->y};
                        float endRadius = (child->node_type == "power_plant") ? 15.0f : 10.0f;

                        bool is_green_fix_path = (fix_viz_state == VIZ_SHOWING_FIX) &&
                                                 (edge == G.edge_being_fixed || path_edges_for_fix_viz.count(edge));

                        bool is_orange_overload = (fix_viz_state == VIZ_SHOWING_OVERLOAD) &&
                                                  (edge == G.edge_being_fixed);

                        bool is_red_pulsing = G.overloaded_edges.count(edge);
                        // -------------------------

                        if (is_green_fix_path)
                        {
                            /* --- USER REQUEST: Draw fixed path as a thick green arrow ---
                              Determine the correct path direction.*/

                            Vector2 pathStart = startV;
                            Vector2 pathEnd = endV;

                            // Check if the path is running in reverse (child -> parent)
                            bool reversed = false;
                            for (size_t i = 1; i < G.fix_path.size(); ++i)
                            {
                                if (G.fix_path[i].second == edge && G.fix_path[i].first == parent)
                                {
                                    reversed = true;
                                    break;
                                }
                            }
                            if (reversed)
                            {
                                std::swap(pathStart, pathEnd);
                                endRadius = (parent->node_type == "power_plant") ? 15.0f : 10.0f;
                            }

                            DrawArrow(pathStart, pathEnd, endRadius, 4.0f / camera.zoom, GREEN);
                        }
                        else if (is_orange_overload)
                        {
                            // --- USER REQUEST: Draw the edge being fixed as a thick orange ARROW ---
                            DrawArrow(startV, endV, endRadius, 4.0f / camera.zoom, ORANGE);
                        }
                        else if (is_red_pulsing)
                        {
                            // --- USER REQUEST: Draw overloaded edges as pulsating red ARROWS ---
                            thickness = 3.0f + sin(GetTime() * 10.0f) * 1.5f;
                            DrawArrow(startV, endV, endRadius, thickness / camera.zoom, RED);
                        }
                        else
                        {
                            // --- USER REQUEST: Draw normal edges as simple LINES ---
                            DrawLineEx(startV, endV, thickness / camera.zoom, edgeColor);
                        }

                        // Draw Selection Highlight (Yellow Outline) on top
                        if (edge == selected_edge)
                        {
                            DrawLineEx(startV, endV, (thickness + 2.0f) / camera.zoom, YELLOW);
                        }
                    } // End if(drawEdge)
                } // End edge loop
            } // End A->A map loop

            // --- Draw Edges (A -> B) ---
            for (auto const &map_entry : G.adj_substion_consumer)
            {
                graph::Vertex_A *parent = map_entry.first;
                for (auto &edge_pair : map_entry.second)
                {
                    graph::Vertex_B *child = edge_pair.first;
                    graph::Edge *edge = edge_pair.second;

                    if (camera.zoom >= ZOOM_LOD_CONSUMER)
                    {
                        Vector2 startV = {(float)parent->x, (float)parent->y};
                        Vector2 endV = {(float)child->x, (float)child->y};

                        // Determine child radius
                        float endRadius = 5.0f;
                        if (child->node_type != "residential")
                            endRadius = 7.0f;

                        DrawLineEx(startV, endV, 1.0f / camera.zoom, LIGHTGRAY);

                        if (edge == selected_edge)
                        {
                            DrawLineEx(startV, endV, 4.0f / camera.zoom, YELLOW);
                        }
                    }
                }
            }
            // --- Draw Nodes (on top of edges) ---
            for (graph::Vertex_A *node : a_nodes_to_draw)
            {
                if (node->node_type == "power_plant")
                {
                    DrawNodeA(node);
                    if (node == selected_a_node)
                        DrawCircleLines(node->x, node->y, 17.0f, YELLOW);
                }
                else if (node->node_type == "substation" && camera.zoom >= ZOOM_LOD_SUBSTATION)
                {
                    DrawNodeA(node); // Draw blue circle

                    // --- ADD THROTTLE HIGHLIGHT ---
                    if (G.throttled_nodes_visual.count(node))
                    {
                        // Draw a purple circle over the blue one
                        DrawCircle(node->x, node->y, 10.0f, RAYWHITE);
                        DrawCircleLines(node->x, node->y, 10.0f, WHITE);
                    }
                    // -----------------------------
                    if (G.node_overloads_visual.count(node))
                    {
                        // Highlight this node in BLACK as requested
                        DrawCircle(node->x, node->y, 10.0f, BLACK);
                        DrawCircleLines(node->x, node->y, 10.0f, DARKGRAY);
                    }
                    // --- END NEW ---
                    if (node == selected_a_node)
                        DrawCircleLines(node->x, node->y, 12.0f, YELLOW);
                }
            }

            if (camera.zoom >= ZOOM_LOD_CONSUMER)
            {
                for (graph::Vertex_B *node : b_nodes_to_draw)
                {
                    DrawNodeB(node);
                    float radius = (node->node_type == "residential") ? 5.0f : 7.0f;

                    // --- ADD EVENT HIGHLIGHT ---
                    if (node == G.last_demand_increase_consumer)
                    {
                        // Pulsating yellow outline
                        float pulseRadius = radius + 2.0f + sin(GetTime() * 10.0f) * 1.5f;
                        DrawRingLines({(float)node->x, (float)node->y}, pulseRadius - (2.0f / camera.zoom), pulseRadius, 0, 360, 36, Fade(YELLOW, 0.8f));
                    }
                    // --------------------------

                    if (node == selected_b_node)
                    {
                        DrawCircleLines(node->x, node->y, radius + 2.0f, YELLOW);
                    }
                }
            }
            EndMode2D();

            // --- Draw UI (Overlay) ---
            DrawRectangle(0, 0, screenWidth, 35, Fade(BLACK, 0.8f));
            DrawText(G.status_message.c_str(), screenWidth/2 - MeasureText(G.status_message.c_str(), 20)/2, 8, 20, YELLOW);

            DrawText("Power Grid Visualizer", 10, 40, 20, WHITE);
            DrawText("Power Plant  ", 10, 65, 18, WHITE);
            DrawCircle(165, 74, 7.0f, PINK);
            DrawText("Substation   ", 10, 85, 18, WHITE);
            DrawCircle(165, 94, 7.0f, BLUE);
            DrawText("Resident     ", 10, 105, 18, WHITE);
            DrawCircle(165, 114, 7.0f, BROWN);
            DrawText("Hospital     ", 10, 125, 18, WHITE);
            DrawCircle(165, 134, 7.0f, DARKBLUE);
            DrawText("Institute    ", 10, 145, 18, WHITE);
            DrawCircle(165, 154, 7.0f, PURPLE);
            DrawText("Industry     ", 10, 165, 18, WHITE);
            DrawCircle(165, 174, 7.0f, ORANGE);

            DrawText("--- Event Legend ---", 10, 195, 16, YELLOW);
            DrawText("Overloaded Edge", 10, 215, 18, WHITE);
            DrawLineEx({160, 224}, {180, 224}, 4.0f, RED);
            DrawText("Rerouting Path", 10, 235, 18, WHITE);
            DrawLineEx({160, 244}, {180, 244}, 4.0f, GREEN);
            DrawText("Node Overloaded", 10, 255, 18, WHITE);
            DrawCircle(170, 264, 7.0f, BLACK);
            DrawCircleLines(170, 264, 7.0f, DARKGRAY);
            DrawText("Throttled Node", 10, 275, 18, WHITE);
            DrawCircle(170, 284, 7.0f, RAYWHITE);
            DrawCircleLines(170, 284, 7.0f, WHITE);
            DrawFPS(screenWidth - 100, 40);
            DrawRectangle(screenWidth - 255, 35, 250, 130, Fade(BLACK, 0.7f)); 
            DrawText("[I] Increase Random Demand", screenWidth - 250, 40, 18, WHITE);
            DrawText("[O] Fix Edge/Throttle Node", screenWidth - 250, 65, 18, WHITE); 
            DrawText("[U] Upgrade Selected Node", screenWidth - 250, 90, 18, WHITE);  
            DrawText("[C] Clear Highlights", screenWidth - 250, 115, 18, WHITE);       
            DrawText("[BACKSPACE] Main Menu", screenWidth - 250, 140, 18, WHITE);     
            // --- Draw Inspector Panel ---
            if (selected_a_node != nullptr)
            {
                // ---Check if this selected node is overloaded ---
                if (G.node_overloads_visual.count(selected_a_node))
                {
                    // --- Show the special prompt ---
                    string type = "Type: " + selected_a_node->node_type;
                    string status = "STATUS: NODE OVERLOADED";
                    string load = "Load: " + to_string(selected_a_node->current_downstream_demand);
                    string limit = "Limit: " + to_string(selected_a_node->max_limit);
                    string prompt2 = "Press 'U' to upgrade limit";

                    DrawRectangle(10, 320, 270, 140, Fade(BLACK, 0.8f)); // Panel is bigger
                    DrawText("--- SELECTED NODE ---", 15, 325, 16, YELLOW);
                    DrawText(type.c_str(), 15, 345, 18, WHITE);
                    DrawText(status.c_str(), 15, 365, 18, RED);
                    DrawText(load.c_str(), 15, 385, 18, ORANGE);
                    DrawText(limit.c_str(), 15, 405, 18, WHITE);
                    DrawText(prompt2.c_str(), 15, 425, 18, WHITE);
                }
                else
                {
                    // --- Show the normal panel ---
                    string type = "Type: " + selected_a_node->node_type;
                    string load = "Current Load: " + to_string(selected_a_node->current_downstream_demand);
                    string limit = "Max Limit: " + to_string(selected_a_node->max_limit);

                    DrawRectangle(10, 320, 250, 80, Fade(BLACK, 0.7f));
                    DrawText("--- SELECTED NODE ---", 15, 325, 16, YELLOW);
                    DrawText(type.c_str(), 15, 345, 18, WHITE);
                    DrawText(load.c_str(), 15, 365, 18, WHITE);
                    DrawText(limit.c_str(), 15, 385, 18, WHITE);
                }
            }
            else if (selected_b_node != nullptr)
            {
                string type = "Type: " + selected_b_node->node_type;
                string demand = "Demand: " + to_string(selected_b_node->demand);

                DrawRectangle(10, 320, 250, 60, Fade(BLACK, 0.7f));
                DrawText("--- SELECTED NODE ---", 15, 325, 16, YELLOW);
                DrawText(type.c_str(), 15, 345, 18, WHITE);
                DrawText(demand.c_str(), 15, 365, 18, WHITE);
            }
            else if (selected_edge != nullptr)
            {
                string load = "Current Load: " + to_string(selected_edge->current_load);
                string max_load_str = "Max Load: " + to_string(selected_edge->max_load);

                DrawRectangle(10, 320, 250, 80, Fade(BLACK, 0.7f));
                DrawText("--- SELECTED EDGE ---", 15, 325, 16, YELLOW);
                DrawText(load.c_str(), 15, 345, 18, WHITE);
                DrawText(max_load_str.c_str(), 15, 365, 18, WHITE);
            }

            // --- Bottom Status Bar ---
            DrawRectangle(0, screenHeight - 30, screenWidth, 30, Fade(BLACK, 0.8f));
            string time_str = "Time Elapsed: " + to_string((int)GetTime()) + "s";
            string throttled_str = "Throttled Substations: " + to_string(G.throttled_nodes_visual.size());
            string overloads_str = "Overloaded Edges: " + to_string(G.overloaded_edges.size());
            
            DrawText(time_str.c_str(), 20, screenHeight - 22, 18, WHITE);
            DrawText(throttled_str.c_str(), screenWidth/2 - MeasureText(throttled_str.c_str(), 18)/2, screenHeight - 22, 18, (G.throttled_nodes_visual.empty() ? WHITE : RED));
            DrawText(overloads_str.c_str(), screenWidth - MeasureText(overloads_str.c_str(), 18) - 20, screenHeight - 22, 18, (G.overloaded_edges.empty() ? WHITE : RED));

            EndDrawing();
        }
        break;
        }
    }

    // --- 6. Cleanup ---
    CloseWindow();

    }



#endif
