#include "graph.h"
#include <iostream>
#include <cmath>
#include <algorithm>

using namespace std;

int Graph::addNode(double lat, double lon, const string& name) {
    int id = nodes.size();
    Node n;
    n.id = id;
    n.lat = lat;
    n.lon = lon;
    n.name = name;
    nodes.push_back(n);
    adj.push_back(vector<Edge>());
    return id;
}

void Graph::addEdge(int from, int to, double distance, double travel_time, TransportMode mode) {
    if (from >= (int)adj.size() || to >= (int)adj.size()) {
        cerr << "Error: Invalid edge from " << from << " to " << to << endl;
        return;
    }
    adj[from].push_back(Edge(to, distance, travel_time, mode));
}

void Graph::printStats() const {
    cout << "\n=== Graph Statistics ===" << endl;
    cout << "Nodes: " << nodes.size() << endl;
    
    int totalEdges = 0;
    for (const auto& edges : adj) {
        totalEdges += edges.size();
    }
    cout << "Edges: " << totalEdges << endl;
    cout << "========================\n" << endl;
}
