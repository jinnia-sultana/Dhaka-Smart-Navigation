#include "csv_loader.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <iostream>
#include <limits>

#define EARTH_RADIUS 6371000.0
#define M_PI 3.14159265358979323846

using namespace std;

double deg2rad(double deg) { return deg * M_PI / 180.0; }

double haversineDistance(double lat1, double lon1, double lat2, double lon2) {
    double dLat = deg2rad(lat2 - lat1);
    double dLon = deg2rad(lon2 - lon1);
    double a = sin(dLat/2) * sin(dLat/2) +
               cos(deg2rad(lat1)) * cos(deg2rad(lat2)) *
               sin(dLon/2) * sin(dLon/2);
    return EARTH_RADIUS * (2 * atan2(sqrt(a), sqrt(1-a)));
}

CSVLoader::CSVLoader(Graph& g) : graph(g) {
    nodeMap.reserve(60000);
}

string CSVLoader::makeKey(double lat, double lon) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.6f,%.6f", lat, lon);
    return string(buffer);
}

double CSVLoader::haversine(double lat1, double lon1, double lat2, double lon2) const {
    return haversineDistance(lat1, lon1, lat2, lon2);
}

int CSVLoader::findNearestNode(double lat, double lon, double threshold) {
    int nearest = -1;
    double minDist = threshold;
    
    for (const auto& node : graph.nodes) {
        double dist = haversineDistance(lat, lon, node.lat, node.lon);
        if (dist < minDist) {
            minDist = dist;
            nearest = node.id;
        }
    }
    
    if (nearest != -1) {
        cout << "  Found node " << nearest << " at " << minDist << " meters" << endl;
    } else {
        cout << "  No node found within " << threshold << " meters" << endl;
    }
    
    return nearest;
}

int CSVLoader::getOrCreateNode(double lat, double lon, const string& name) {
    string key = makeKey(lat, lon);
    auto it = nodeMap.find(key);
    if (it != nodeMap.end()) return it->second;
    
    int id = graph.addNode(lat, lon, name);
    nodeMap[key] = id;
    if (!name.empty()) stopNameMap[name] = id;
    return id;
}

void CSVLoader::loadDhakaStreets(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) { 
        cerr << "Cannot open: " << filename << endl; 
        return; 
    }
    
    string line;
    int edgesCreated = 0;
    int linesProcessed = 0;
    
    while (getline(file, line)) {
        linesProcessed++;
        if (line.empty()) continue;
        
        stringstream ss(line);
        vector<string> cols;
        string token;
        while (getline(ss, token, ',')) cols.push_back(token);
        
        if (cols.size() < 5) continue;
        
        try {
            double travel_time = stod(cols[cols.size()-1]);
            
            vector<int> nodeIds;
            // CSV format: lon, lat (not lat, lon!)
            for (size_t i = 1; i < cols.size() - 2; i += 2) {
                if (i + 1 >= cols.size() - 2) break;
                double lon = stod(cols[i]);      // First is LONGITUDE
                double lat = stod(cols[i+1]);    // Second is LATITUDE
                int id = getOrCreateNode(lat, lon);  // Store as (lat, lon)
                nodeIds.push_back(id);
            }
            
            if (nodeIds.size() < 2) continue;
            
            // Calculate total distance
            double totalDist = 0;
            vector<double> segmentDists;
            for (size_t i = 0; i < nodeIds.size() - 1; i++) {
                double dist = haversineDistance(
                    graph.nodes[nodeIds[i]].lat, graph.nodes[nodeIds[i]].lon,
                    graph.nodes[nodeIds[i+1]].lat, graph.nodes[nodeIds[i+1]].lon
                );
                segmentDists.push_back(dist);
                totalDist += dist;
            }
            
            // Add edges with proportional time
            for (size_t i = 0; i < nodeIds.size() - 1; i++) {
                double segmentTime = travel_time * (segmentDists[i] / totalDist);
                if (segmentTime < 0.01) segmentTime = 0.01;
                
                graph.addEdge(nodeIds[i], nodeIds[i+1], segmentDists[i], segmentTime, TransportMode::WALK);
                graph.addEdge(nodeIds[i+1], nodeIds[i], segmentDists[i], segmentTime, TransportMode::WALK);
                edgesCreated += 2;
            }
        } catch (...) {}
    }
    file.close();
    cout << "Street: " << edgesCreated << " edges, " << graph.nodes.size() << " nodes from " << linesProcessed << " lines" << endl;
}

void CSVLoader::loadMetroRail(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) { 
        cerr << "Cannot open: " << filename << endl; 
        return; 
    }
    
    string line;
    int edgesAdded = 0;
    int linesProcessed = 0;
    
    while (getline(file, line)) {
        linesProcessed++;
        if (line.empty()) continue;
        
        stringstream ss(line);
        vector<string> cols;
        string token;
        while (getline(ss, token, ',')) cols.push_back(token);
        
        if (cols.size() < 5) continue;
        
        try {
            string startStop = cols[cols.size()-2];
            string endStop = cols[cols.size()-1];
            
            vector<int> nodeIds;
            // CSV format: lon, lat
            for (size_t i = 1; i < cols.size() - 2; i += 2) {
                if (i + 1 >= cols.size() - 2) break;
                double lon = stod(cols[i]);      // First is LONGITUDE
                double lat = stod(cols[i+1]);    // Second is LATITUDE
                string name = (nodeIds.empty()) ? startStop : (i + 2 >= cols.size() - 2) ? endStop : "";
                int id = getOrCreateNode(lat, lon, name);
                nodeIds.push_back(id);
            }
            
            if (nodeIds.size() < 2) continue;
            
            for (size_t i = 0; i < nodeIds.size() - 1; i++) {
                double dist = haversineDistance(
                    graph.nodes[nodeIds[i]].lat, graph.nodes[nodeIds[i]].lon,
                    graph.nodes[nodeIds[i+1]].lat, graph.nodes[nodeIds[i+1]].lon
                );
                double time = (dist / 1000.0) / 20.0 * 60.0; // Metro: 20 km/h
                if (time < 0.1) time = 0.1;
                
                graph.addEdge(nodeIds[i], nodeIds[i+1], dist, time, TransportMode::METRO);
                graph.addEdge(nodeIds[i+1], nodeIds[i], dist, time, TransportMode::METRO);
                edgesAdded += 2;
            }
        } catch (...) {}
    }
    file.close();
    cout << "Metro: " << edgesAdded << " edges from " << linesProcessed << " lines" << endl;
}

void CSVLoader::loadBusRoute(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) { 
        cerr << "Cannot open: " << filename << endl; 
        return; 
    }
    
    string line;
    int edgesAdded = 0;
    int linesProcessed = 0;
    
    while (getline(file, line)) {
        linesProcessed++;
        if (line.empty()) continue;
        
        stringstream ss(line);
        vector<string> cols;
        string token;
        while (getline(ss, token, ',')) cols.push_back(token);
        
        if (cols.size() < 5) continue;
        
        try {
            string startStop = cols[cols.size()-2];
            string endStop = cols[cols.size()-1];
            
            vector<int> nodeIds;
            // CSV format: lon, lat
            for (size_t i = 1; i < cols.size() - 2; i += 2) {
                if (i + 1 >= cols.size() - 2) break;
                double lon = stod(cols[i]);      // First is LONGITUDE
                double lat = stod(cols[i+1]);    // Second is LATITUDE
                string name = (nodeIds.empty()) ? startStop : (i + 2 >= cols.size() - 2) ? endStop : "";
                int id = getOrCreateNode(lat, lon, name);
                nodeIds.push_back(id);
            }
            
            if (nodeIds.size() < 2) continue;
            
            for (size_t i = 0; i < nodeIds.size() - 1; i++) {
                double dist = haversineDistance(
                    graph.nodes[nodeIds[i]].lat, graph.nodes[nodeIds[i]].lon,
                    graph.nodes[nodeIds[i+1]].lat, graph.nodes[nodeIds[i+1]].lon
                );
                double time = (dist / 1000.0) / 12.0 * 60.0; // Bus: 12 km/h
                if (time < 0.1) time = 0.1;
                
                graph.addEdge(nodeIds[i], nodeIds[i+1], dist, time, TransportMode::BUS);
                graph.addEdge(nodeIds[i+1], nodeIds[i], dist, time, TransportMode::BUS);
                edgesAdded += 2;
            }
        } catch (...) {}
    }
    file.close();
    cout << "Bus (" << filename << "): " << edgesAdded << " edges from " << linesProcessed << " lines" << endl;
}