#include "graph.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define EARTH_RADIUS 6371000 // meters

using namespace std;

// Helper function: haversine distance
static inline float haversine(float lat1, float lon1, float lat2, float lon2) {
    float dLat = (lat2 - lat1) * M_PI / 180.0f;
    float dLon = (lon2 - lon1) * M_PI / 180.0f;
    float lat1r = lat1 * M_PI / 180.0f;
    float lat2r = lat2 * M_PI / 180.0f;
    
    float a = sin(dLat/2) * sin(dLat/2) +
              cos(lat1r) * cos(lat2r) *
              sin(dLon/2) * sin(dLon/2);
    
    return EARTH_RADIUS * (2 * atan2(sqrt(a), sqrt(1-a)));
}

void Graph::updateStats(const Node& node) {
    stats.nodeCount++;
    stats.minLat = min(stats.minLat, node.lat);
    stats.maxLat = max(stats.maxLat, node.lat);
    stats.minLon = min(stats.minLon, node.lon);
    stats.maxLon = max(stats.maxLon, node.lon);
}

void Graph::updateStats(const Edge& edge) {
    stats.edgeCount++;
    stats.totalRoadLength += edge.distance / 1000.0;  // Convert to km
    
    switch(edge.mode) {
        case TransportMode::WALK:
        case TransportMode::RICKSHAW:
        case TransportMode::BIKE:
            stats.walkEdges++;
            break;
        case TransportMode::BUS:
            stats.busEdges++;
            break;
        case TransportMode::METRO:
            stats.metroEdges++;
            break;
        default:
            break;
    }
}

uint32_t Graph::addNode(float lat, float lon, const string& name) {
    uint32_t id = nodes.size();
    
    Node n;
    n.id = id;
    n.lat = lat;
    n.lon = lon;
    n.name = name;
    n.walkableNeighbors = 0;
    
    nodes.push_back(n);
    adj.push_back(vector<Edge>());
    adj.back().reserve(4);  // Typical node has 4 connections
    
    updateStats(n);
    
    return id;
}

void Graph::addEdge(uint32_t from, uint32_t to, float distance, 
                    float travel_time, TransportMode mode, bool bidirectional) {
    // Validate node IDs
    if (from >= nodes.size() || to >= nodes.size()) {
        cerr << "Error: Invalid node IDs in addEdge" << endl;
        return;
    }
    
    // Don't add self-loops
    if (from == to) {
        cerr << "Warning: Attempted to add self-loop edge" << endl;
        return;
    }
    
    // Add forward edge
    Edge e(to, distance, travel_time, mode);
    adj[from].push_back(e);
    updateStats(e);
    
    // Count walkable neighbors for heuristics
    if (mode == TransportMode::WALK) {
        nodes[from].walkableNeighbors++;
    }
    
    // Add to transfer nodes if applicable
    uint8_t modeVal = static_cast<uint8_t>(mode);
    for (int i = 0; i < 5; i++) {
        if (modeVal & (1 << i)) {
            transferNodes[i].push_back(from);
        }
    }
    
    // Add bidirectional edge if requested
    if (bidirectional) {
        Edge e2(from, distance, travel_time, mode);
        adj[to].push_back(e2);
        
        if (mode == TransportMode::WALK) {
            nodes[to].walkableNeighbors++;
        }
    }
}

void Graph::addEdgeWithSpeed(uint32_t from, uint32_t to, float distance,
                             float speedKmph, TransportMode mode, bool bidirectional) {
    // Convert speed from km/h to m/s, then calculate time
    float speedMps = speedKmph * 1000.0f / 3600.0f;
    float travel_time = distance / speedMps;  // time in seconds
    
    // Convert to minutes for consistency
    travel_time /= 60.0f;
    
    addEdge(from, to, distance, travel_time, mode, bidirectional);
}

bool Graph::canTransfer(uint32_t nodeId, TransportMode mode1, TransportMode mode2) const {
    if (nodeId >= nodes.size()) return false;
    
    bool hasMode1 = false;
    bool hasMode2 = false;
    
    for (const auto& edge : adj[nodeId]) {
        if (edge.mode == mode1) hasMode1 = true;
        if (edge.mode == mode2) hasMode2 = true;
        if (hasMode1 && hasMode2) return true;
    }
    
    return false;
}

vector<uint32_t> Graph::findTransferPoints(TransportMode fromMode, TransportMode toMode) const {
    vector<uint32_t> transferPoints;
    transferPoints.reserve(1000);  // Reserve space
    
    uint8_t fromVal = static_cast<uint8_t>(fromMode);
    uint8_t toVal = static_cast<uint8_t>(toMode);
    
    // Get indices for mode vectors
    int fromIdx = -1, toIdx = -1;
    for (int i = 0; i < 5; i++) {
        if (fromVal & (1 << i)) fromIdx = i;
        if (toVal & (1 << i)) toIdx = i;
    }
    
    if (fromIdx == -1 || toIdx == -1) return transferPoints;
    
    // Find intersection of nodes that have both modes
    const auto& nodesWithFrom = transferNodes[fromIdx];
    const auto& nodesWithTo = transferNodes[toIdx];
    
    // Use set intersection for efficiency
    vector<uint32_t> intersection;
    intersection.reserve(min(nodesWithFrom.size(), nodesWithTo.size()));
    
    // Since vectors are unsorted, we need a different approach
    // For small vectors, brute force is fine
    if (nodesWithFrom.size() * nodesWithTo.size() < 1000000) {
        // Brute force for smaller sets
        unordered_map<uint32_t, bool> fromSet;
        for (uint32_t node : nodesWithFrom) {
            fromSet[node] = true;
        }
        
        for (uint32_t node : nodesWithTo) {
            if (fromSet.count(node)) {
                transferPoints.push_back(node);
            }
        }
    } else {
        // For larger sets, sort and use set_intersection
        vector<uint32_t> sortedFrom = nodesWithFrom;
        vector<uint32_t> sortedTo = nodesWithTo;
        sort(sortedFrom.begin(), sortedFrom.end());
        sort(sortedTo.begin(), sortedTo.end());
        
        set_intersection(
            sortedFrom.begin(), sortedFrom.end(),
            sortedTo.begin(), sortedTo.end(),
            back_inserter(transferPoints)
        );
    }
    
    return transferPoints;
}

vector<uint32_t> Graph::findNodesInRadius(float lat, float lon, float radiusMeters) const {
    vector<uint32_t> result;
    result.reserve(1000);
    
    // Convert radius to approximate degree difference
    float latRadius = radiusMeters / 111000.0f;  // 1 deg lat ≈ 111 km
    float lonRadius = radiusMeters / (111000.0f * cos(lat * M_PI / 180.0f));
    
    float minLat = lat - latRadius;
    float maxLat = lat + latRadius;
    float minLon = lon - lonRadius;
    float maxLon = lon + lonRadius;
    
    // Brute force search (could be optimized with spatial index)
    for (const auto& node : nodes) {
        if (node.lat >= minLat && node.lat <= maxLat &&
            node.lon >= minLon && node.lon <= maxLon) {
            float dist = haversine(lat, lon, node.lat, node.lon);
            if (dist <= radiusMeters) {
                result.push_back(node.id);
            }
        }
    }
    
    return result;
}

uint32_t Graph::findNearestNode(float lat, float lon, float maxDistance) const {
    uint32_t nearest = static_cast<uint32_t>(-1);
    float minDist = maxDistance;
    
    // First check spatial cache
    uint32_t gridCell = nodes.empty() ? 0 : 
        Node(lat, lon, "", 0).getGridCell();
    
    auto it = spatialCache.find(gridCell);
    if (it != spatialCache.end()) {
        // Search cached nodes
        for (uint32_t id : it->second) {
            const auto& node = nodes[id];
            float dist = haversine(lat, lon, node.lat, node.lon);
            if (dist < minDist) {
                minDist = dist;
                nearest = id;
            }
        }
        if (nearest != static_cast<uint32_t>(-1)) {
            return nearest;
        }
    }
    
    // Fallback to full search
    for (const auto& node : nodes) {
        float dist = haversine(lat, lon, node.lat, node.lon);
        if (dist < minDist) {
            minDist = dist;
            nearest = node.id;
        }
    }
    
    return nearest;
}

void Graph::printStats() const {
    cout << "\n=== Graph Statistics ===" << endl;
    cout << "Nodes: " << stats.nodeCount << endl;
    cout << "Total Edges: " << stats.edgeCount << endl;
    cout << "  Walk/Bike/Rickshaw edges: " << stats.walkEdges << endl;
    cout << "  Bus edges: " << stats.busEdges << endl;
    cout << "  Metro edges: " << stats.metroEdges << endl;
    cout << "Total road length: " << stats.totalRoadLength << " km" << endl;
    cout << "Bounds: Lat(" << stats.minLat << ", " << stats.maxLat 
         << "), Lon(" << stats.minLon << ", " << stats.maxLon << ")" << endl;
    cout << "Memory usage: ~" 
         << (nodes.size() * sizeof(Node) + 
             adj.size() * sizeof(vector<Edge>) +
             stats.edgeCount * sizeof(Edge)) / (1024 * 1024)
         << " MB" << endl;
    cout << "=======================\n" << endl;
}

void Graph::clear() {
    nodes.clear();
    adj.clear();
    transferNodes.clear();
    transferNodes.resize(5);
    spatialCache.clear();
    stats = GraphStats();
}

bool Graph::saveToFile(const string& filename) const {
    ofstream file(filename, ios::binary);
    if (!file.is_open()) return false;
    
    // Write magic number and version
    uint32_t magic = 0x47524150;  // "GRAP"
    uint16_t version = 1;
    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    
    // Write stats
    file.write(reinterpret_cast<const char*>(&stats), sizeof(stats));
    
    // Write nodes
    uint32_t nodeCount = nodes.size();
    file.write(reinterpret_cast<const char*>(&nodeCount), sizeof(nodeCount));
    file.write(reinterpret_cast<const char*>(nodes.data()), nodeCount * sizeof(Node));
    
    // Write edges
    uint32_t edgeCount = stats.edgeCount;
    file.write(reinterpret_cast<const char*>(&edgeCount), sizeof(edgeCount));
    
    // Write adjacency lists
    for (const auto& edges : adj) {
        uint32_t edgeListSize = edges.size();
        file.write(reinterpret_cast<const char*>(&edgeListSize), sizeof(edgeListSize));
        file.write(reinterpret_cast<const char*>(edges.data()), edgeListSize * sizeof(Edge));
    }
    
    return true;
}

bool Graph::loadFromFile(const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) return false;
    
    // Check magic number and version
    uint32_t magic;
    uint16_t version;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    
    if (magic != 0x47524150 || version != 1) return false;
    
    // Clear current data
    clear();
    
    // Read stats
    file.read(reinterpret_cast<char*>(&stats), sizeof(stats));
    
    // Read nodes
    uint32_t nodeCount;
    file.read(reinterpret_cast<char*>(&nodeCount), sizeof(nodeCount));
    nodes.resize(nodeCount);
    file.read(reinterpret_cast<char*>(nodes.data()), nodeCount * sizeof(Node));
    
    // Read edges
    uint32_t edgeCount;
    file.read(reinterpret_cast<char*>(&edgeCount), sizeof(edgeCount));
    
    // Read adjacency lists
    adj.resize(nodeCount);
    for (uint32_t i = 0; i < nodeCount; i++) {
        uint32_t edgeListSize;
        file.read(reinterpret_cast<char*>(&edgeListSize), sizeof(edgeListSize));
        adj[i].resize(edgeListSize);
        file.read(reinterpret_cast<char*>(adj[i].data()), edgeListSize * sizeof(Edge));
    }
    
    return true;
}