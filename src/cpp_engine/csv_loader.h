#ifndef CSV_LOADER_H
#define CSV_LOADER_H

#include "graph.h"
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class CSVLoader {
private:
    Graph& graph;
    unordered_map<string, int> nodeMap;
    unordered_map<string, int> stopNameMap;
    
    string makeKey(double lat, double lon);
    double haversine(double lat1, double lon1, double lat2, double lon2) const;
    vector<pair<double, double>> parseCoordinates(const vector<string>& cols, size_t startIdx, size_t endIdx);

public:
    CSVLoader(Graph& g);
    
    void loadDhakaStreets(const string& filename);
    void loadMetroRail(const string& filename);
    void loadBusRoute(const string& filename);
    
    int getOrCreateNode(double lat, double lon, const string& name = "");
    int findNearestNode(double lat, double lon, double threshold = 50.0);  // Make public
    
    void reserve(size_t count) { nodeMap.reserve(count); }
};

#endif