#include "route_exporter.h"
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace std;

void RouteExporter::exportToJSON(const Graph& graph, const RouteResult& route, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error writing to " << filename << endl;
        return;
    }
    
    file << "[\n";
    for (size_t i = 0; i < route.path.size(); i++) {
        const Node& node = graph.nodes[route.path[i]];
        file << "  [" << fixed << setprecision(6) << node.lat << ", " << node.lon << "]";
        if (i < route.path.size() - 1) file << ",";
        file << "\n";
    }
    file << "]\n";
    file.close();
    
    cout << "Route exported to " << filename << " (" << route.path.size() << " nodes)" << endl;
}