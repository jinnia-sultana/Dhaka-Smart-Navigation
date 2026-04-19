#include <iostream>
#include <chrono>
#include <iomanip>
#include "graph.h"
#include "csv_loader.h"
#include "routing.h"
#include "route_exporter.h"

using namespace std;
using namespace chrono;

int main() {
    cout << "\n=====================================\n";
    cout << "  Smart Multi-Modal Navigation System\n";
    cout << "=====================================\n\n";

    Graph graph;
    CSVLoader loader(graph);

    auto start = high_resolution_clock::now();

    cout << "Loading Dhaka streets...\n";
    loader.loadDhakaStreets("../data/Roadmap-Dhaka.csv");

    cout << "Loading metro routes...\n";
    loader.loadMetroRail("../data/Routemap-DhakaMetroRail.csv");

    cout << "Loading bus routes...\n";
    loader.loadBusRoute("../data/Routemap-UttaraBus.csv");
    loader.loadBusRoute("../data/Routemap-BikolpoBus.csv");

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<seconds>(end - start);

    cout << "\nLoaded in " << duration.count() << " seconds\n";
    graph.printStats();

    RoutingParams params;
    cout << "\nEnter preferences (0-100):\n";
    cout << "Time importance: "; cin >> params.w_time;
    cout << "Cost importance: "; cin >> params.w_cost;
    cout << "Distance importance: "; cin >> params.w_distance;
    params.normalize();

    double slat, slon, dlat, dlon;
    char comma;
    cout << "\nStart (lat,lon): "; cin >> slat >> comma >> slon;
    cout << "Destination (lat,lon): "; cin >> dlat >> comma >> dlon;

    // Find nearest nodes
    int start_id = loader.findNearestNode(slat, slon, 500.0);
    int end_id = loader.findNearestNode(dlat, dlon, 500.0);

    if (start_id == -1) {
        cout << "\nNo node found near start location!\n";
        return 1;
    }
    if (end_id == -1) {
        cout << "\nNo node found near destination!\n";
        return 1;
    }

    cout << "\nStart node: " << start_id << endl;
    cout << "End node: " << end_id << endl;

    Router router(graph);
    auto route_start = high_resolution_clock::now();
    RouteResult result = router.findRoute(start_id, end_id, params);
    auto route_end = high_resolution_clock::now();

    if (result.path.empty()) {
        cout << "\nNo route found!\n";
        return 1;
    }

    cout << "\nRoute found in " << duration_cast<milliseconds>(route_end - route_start).count() << " ms\n";
    cout << "Nodes in path: " << result.path.size() << endl;
    cout << "Total Distance: " << fixed << setprecision(2) << result.total_distance << " m (" << result.total_distance/1000 << " km)\n";
    cout << "Total Time: " << result.total_time << " minutes\n";
    cout << "Total Cost: " << result.total_cost << " tk\n";

    // Display route breakdown by transport mode
    cout << "\n=== ROUTE BREAKDOWN ===" << endl;
    
    if (result.segments.empty()) {
        cout << "No segments found - check routing.cpp for segment building" << endl;
    } else {
        for (size_t i = 0; i < result.segments.size(); i++) {
            string modeName;
            switch(result.segments[i].mode) {
                case TransportMode::WALK: modeName = "WALK"; break;
                case TransportMode::RICKSHAW: modeName = "RICKSHAW"; break;
                case TransportMode::BIKE: modeName = "BIKE"; break;
                case TransportMode::BUS: modeName = "BUS"; break;
                case TransportMode::METRO: modeName = "METRO"; break;
                default: modeName = "UNKNOWN"; break;
            }
            
            cout << "  " << i+1 << ". " << modeName 
                 << " - Distance: " << result.segments[i].distance << " m"
                 << ", Time: " << result.segments[i].time << " min"
                 << ", Cost: " << result.segments[i].cost << " tk" << endl;
        }
    }
    
    cout << "======================" << endl;

    RouteExporter::exportToJSON(graph, result, "../web/route.json");
    cout << "\nRoute exported to ../web/route.json\n";
    cout << "Open ../web/index.html in browser\n";

    return 0;
}