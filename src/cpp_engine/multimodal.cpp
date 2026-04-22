#include "nodesAndEdges.h"
#include "csvParser.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <queue>
#include <cstring>
#include <algorithm>
#include <fstream>

using namespace std;

#define WALK_SPEED 3.0
#define RICKSHAW_SPEED 8.0
#define BIKE_SPEED 15.0
#define BUS_SPEED 12.0
#define METRO_SPEED 20.0

#define WALK_COST 0.0
#define RICKSHAW_COST 15.0
#define BIKE_COST 18.0
#define BUS_COST 3.0
#define METRO_COST 5.0


double getSpeed(Mode mode) {
    switch(mode) {
        case MODE_WALK: return WALK_SPEED;
        case MODE_RICKSHAW: return RICKSHAW_SPEED;
        case MODE_BIKE: return BIKE_SPEED;
        case MODE_UTTARA:
        case MODE_BIKOLPO: return BUS_SPEED;
        case MODE_METRO: return METRO_SPEED;
        default: return WALK_SPEED;
    }
}

double getCost(Mode mode) {
    switch(mode) {
        case MODE_WALK: return WALK_COST;
        case MODE_RICKSHAW: return RICKSHAW_COST;
        case MODE_BIKE: return BIKE_COST;
        case MODE_UTTARA:
        case MODE_BIKOLPO: return BUS_COST;
        case MODE_METRO: return METRO_COST;
        default: return 0;
    }
}

void exportRouteToJSON(const vector<int>& path, const char* filename) {
    if (path.size() < 2) return;

    ofstream out(filename);
    out << "{\n  \"segments\": [\n";

    Mode curMode = (Mode)-1;
    int segStart = path[0];
    vector<pair<double,double>> coords;

    double segDist = 0, segTime = 0, segCost = 0;
    double totalDist = 0, totalTime = 0, totalCost = 0;
    bool firstSeg = true;

    for (size_t i = 0; i + 1 < path.size(); i++) {
        int u = path[i], v = path[i+1];

        for (int eidx : adj[u]) {
            if (edges[eidx].to == v) {
                Mode m = edges[eidx].mode;
                double d = edges[eidx].distance;
                double dk = d / 1000.0;
                double t = (dk / getSpeed(m)) * 60.0;
                double c = dk * getCost(m);

                //current node's coordinates
                coords.push_back({nodes[u].lat, nodes[u].lon});

                if (curMode == (Mode)-1) {
                    curMode = m;
                    segStart = u;
                }
                else if (m != curMode) {
                    //the end point of segment
                    coords.push_back({nodes[u].lat, nodes[u].lon});
                    
                    if (!firstSeg) out << ",\n";
                    firstSeg = false;

                    const char* fromName = (strlen(nodes[segStart].name) ? nodes[segStart].name : "Point");
                    const char* toName = (strlen(nodes[u].name) ? nodes[u].name : "Point");

                    out << "    {\n";
                    out << "      \"mode\": \"" << getModeName(curMode) << "\",\n";
                    out << "      \"from\": \"" << fromName << "\",\n";
                    out << "      \"to\": \"" << toName << "\",\n";
                    out << "      \"distance\": " << segDist << ",\n";
                    out << "      \"time\": " << segTime << ",\n";
                    out << "      \"cost\": " << segCost << ",\n";
                    out << "      \"coords\": [";

                    for (size_t k = 0; k < coords.size(); k++) {
                        out << "[" << coords[k].first << "," << coords[k].second << "]";
                        if (k + 1 < coords.size()) out << ",";
                    }
                    out << "]\n    }";

                    coords.clear();
                    segDist = 0; segTime = 0; segCost = 0;
                    segStart = u;
                    curMode = m;
                }

                segDist += d;
                segTime += t;
                segCost += c;
                totalDist += d;
                totalTime += t;
                totalCost += c;
                break;
            }
        }
    }

    //final destination coordinate
    if (path.size() > 0) {
        coords.push_back({nodes[path.back()].lat, nodes[path.back()].lon});
    }

    //Last segment
    if (!firstSeg) out << ",\n";
    
    const char* fromName = (strlen(nodes[segStart].name) ? nodes[segStart].name : "Point");
    const char* toName = (strlen(nodes[path.back()].name) ? nodes[path.back()].name : "Destination");
    
    out << "    {\n";
    out << "      \"mode\": \"" << getModeName(curMode) << "\",\n";
    out << "      \"from\": \"" << fromName << "\",\n";
    out << "      \"to\": \"" << toName << "\",\n";
    out << "      \"distance\": " << segDist << ",\n";
    out << "      \"time\": " << segTime << ",\n";
    out << "      \"cost\": " << segCost << ",\n";
    out << "      \"coords\": [";

    for (size_t k = 0; k < coords.size(); k++) {
        out << "[" << coords[k].first << "," << coords[k].second << "]";
        if (k + 1 < coords.size()) out << ",";
    }

    out << "]\n    }\n";
    
    out << "  ],\n";
    out << "  \"total\": {\n";
    out << "    \"dist\": " << totalDist / 1000.0 << ",\n";
    out << "    \"time\": " << totalTime << ",\n";
    out << "    \"cost\": " << totalCost << "\n";
    out << "  }\n";
    out << "}\n";
}

vector<int> dijkstra(int start, int goal, double w_time, double w_cost, double w_dist) {
    vector<double> cost(numNodes, INF);
    vector<int> prev(numNodes, -1);
    vector<bool> visited(numNodes, false);
    
    cost[start] = 0;
    
    for (int i = 0; i < numNodes; i++) {
        int u = -1;
        double best = INF;
        for (int j = 0; j < numNodes; j++) {
            if (!visited[j] && cost[j] < best) {
                best = cost[j];
                u = j;
            }
        }
        if (u == -1 || u == goal) break;
        visited[u] = true;
        
        for (int eidx : adj[u]) {
            const Edge& e = edges[eidx];
            double distKm = e.distance / 1000.0;
            double timeCost = (distKm / getSpeed(e.mode)) * 60.0;
            double moneyCost = distKm * getCost(e.mode);
            double edgeCost = timeCost * w_time + moneyCost * w_cost + distKm * w_dist;
            
            if (cost[u] + edgeCost < cost[e.to]) {
                cost[e.to] = cost[u] + edgeCost;
                prev[e.to] = u;
            }
        }
    }
    
    vector<int> path;
    for (int at = goal; at != -1; at = prev[at]) path.push_back(at);
    reverse(path.begin(), path.end());
    return path;
}

void printRouteShort(const vector<int>& path, const char* label) {
    const char* sourceName = nodes[path.front()].name[0] ? nodes[path.front()].name : "Source";
const char* destName   = nodes[path.back()].name[0] ? nodes[path.back()].name : "Destination";
    printf("\n=== %s ===\n", label);

    if (path.empty() || path.size() < 2) {
        printf("No route found.\n");
        return;
    }

    double totalDist = 0, totalTime = 0, totalCost = 0;

    Mode curMode = (Mode)-1;
    double segDist = 0, segTime = 0, segCost = 0;
    int segStart = path[0];
    int segEnd = path[0];

    for (size_t i = 0; i + 1 < path.size(); i++) {
        int u = path[i], v = path[i+1];

        for (int eidx : adj[u]) {
            if (edges[eidx].to == v) {
                Mode m = edges[eidx].mode;
                double d = edges[eidx].distance;
                double dk = d / 1000.0;
                double t = (dk / getSpeed(m)) * 60.0;
                double c = dk * getCost(m);

                if (curMode == (Mode)-1) {
                    curMode = m;
                    segStart = u;
                }
                else if (m != curMode) {
                  char fromBuf[100], toBuf[100];

if (segStart == path.front())
    sprintf(fromBuf, "%s", sourceName);
else if (nodes[segStart].name[0])
    sprintf(fromBuf, "%s", nodes[segStart].name);
else
    sprintf(fromBuf, "(%.6f, %.6f)", nodes[segStart].lon, nodes[segStart].lat);

if (segEnd == path.back())
    sprintf(toBuf, "%s", destName);
else if (nodes[segEnd].name[0])
    sprintf(toBuf, "%s", nodes[segEnd].name);
else
    sprintf(toBuf, "(%.6f, %.6f)", nodes[segEnd].lon, nodes[segEnd].lat);

                    printf("%s from %s to %s: %.0f m, %.1f min, %.1f tk\n",
       getModeName(curMode), fromBuf, toBuf, segDist, segTime, segCost);

                    totalDist += segDist;
                    totalTime += segTime;
                    totalCost += segCost;

                    segDist = segTime = segCost = 0;
                    segStart = u;
                    curMode = m;
                }

                segDist += d;
                segTime += t;
                segCost += c;
                segEnd = v;
                break;
            }
        }
    }

    // Last segment
    char fromBuf[100], toBuf[100];

if (segStart == path.front())
    sprintf(fromBuf, "%s", sourceName);
else if (nodes[segStart].name[0])
    sprintf(fromBuf, "%s", nodes[segStart].name);
else
    sprintf(fromBuf, "(%.6f, %.6f)", nodes[segStart].lon, nodes[segStart].lat);

if (segEnd == path.back())
    sprintf(toBuf, "%s", destName);
else if (nodes[segEnd].name[0])
    sprintf(toBuf, "%s", nodes[segEnd].name);
else
    sprintf(toBuf, "(%.6f, %.6f)", nodes[segEnd].lon, nodes[segEnd].lat);

printf("%s from %s to %s: %.0f m, %.1f min, %.1f tk\n",
       getModeName(curMode), fromBuf, toBuf, segDist, segTime, segCost);

    totalDist += segDist;
    totalTime += segTime;
    totalCost += segCost;

    //TOTAL OUTPUT
    printf("\nTotal Distance: %.2f km\n", totalDist / 1000.0);
    printf("Total Time: %.1f min\n", totalTime);
    printf("Total Cost: %.1f tk\n", totalCost);
}
// Helper: Get allowed modes for a specific route type
unsigned int getAllowedModes(const char* routeType, double directDist = 0) {
    if (strcmp(routeType, "time") == 0) {
        // Time-optimized: Metro + Rickshaw (fastest combo)
        return (1<<MODE_WALK) | (1<<MODE_METRO) | (1<<MODE_RICKSHAW);
    }
    else if (strcmp(routeType, "cost") == 0) {
        // Cost-optimized: Walk + Bus (cheapest combo)
        if (directDist < 2000) {  // < 2km
            return (1<<MODE_WALK);  // Walk only for short trips
        }
        return (1<<MODE_WALK) | (1<<MODE_UTTARA) | (1<<MODE_BIKOLPO);
    }
    // Default: all modes
    return (1<<MODE_WALK) | (1<<MODE_BIKE) | (1<<MODE_RICKSHAW) | 
           (1<<MODE_METRO) | (1<<MODE_UTTARA) | (1<<MODE_BIKOLPO);
}

// Modified Dijkstra with mode filtering
vector<int> dijkstraFiltered(int start, int goal, double w_time, double w_cost, double w_dist, 
                              unsigned int allowedModes) {
    vector<double> cost(numNodes, INF);
    vector<int> prev(numNodes, -1);
    vector<bool> visited(numNodes, false);
    
    cost[start] = 0;
    
    for (int i = 0; i < numNodes; i++) {
        int u = -1;
        double best = INF;
        for (int j = 0; j < numNodes; j++) {
            if (!visited[j] && cost[j] < best) {
                best = cost[j];
                u = j;
            }
        }
        if (u == -1 || u == goal) break;
        visited[u] = true;
        
        for (int eidx : adj[u]) {
            const Edge& e = edges[eidx];
            
            // Skip if mode not allowed
            if (!(allowedModes & (1 << (int)e.mode))) continue;
            
            double distKm = e.distance / 1000.0;
            double timeCost = (distKm / getSpeed(e.mode)) * 60.0;
            double moneyCost = distKm * getCost(e.mode);
            double edgeCost = timeCost * w_time + moneyCost * w_cost + distKm * w_dist;
            
            if (cost[u] + edgeCost < cost[e.to]) {
                cost[e.to] = cost[u] + edgeCost;
                prev[e.to] = u;
            }
        }
    }
    
    vector<int> path;
    if (cost[goal] == INF) return path;
    for (int at = goal; at != -1; at = prev[at]) path.push_back(at);
    reverse(path.begin(), path.end());
    return path;
}

int main() {
    printf("\n============================================\n");
    printf("  Smart Multi-Modal Navigation For Dhaka\n");
    printf("============================================\n\n");

    system("del ..\\web\\*.kml 2>nul");
    system("del ..\\web\\*.json 2>nul");

    printf("Loading data...\n");
    parseRoadmapCSV("../data/Roadmap-Dhaka.csv");
    parseMetroCSV("../data/Routemap-DhakaMetroRail.csv");
    parseBusCSV("../data/Routemap-UttaraBus.csv", MODE_UTTARA);
    parseBusCSV("../data/Routemap-BikolpoBus.csv", MODE_BIKOLPO);
    connectTransitToStreets(300.0);

    printf("\nGraph: %d nodes, %d edges\n", numNodes, numEdges);

    double w_time, w_cost, w_dist;
    printf("\nSET YOUR PREFERENCES (0-100)\n");
    printf("Time: "); scanf("%lf", &w_time);
    printf("Cost: "); scanf("%lf", &w_cost);
    printf("Dist: "); scanf("%lf", &w_dist);

    double tot = w_time + w_cost + w_dist;
    if (tot > 0) { w_time /= tot; w_cost /= tot; w_dist /= tot; }

    double slat, slon, dlat, dlon;
    printf("\nENTER YOUR LOCATIONS\n");
    printf("Start (lat,lon): "); scanf("%lf,%lf", &slat, &slon);
    printf("Destination (lat,lon): "); scanf("%lf,%lf", &dlat, &dlon);

    int sid = findNearestNode(slat, slon);
    int did = findNearestNode(dlat, dlon);
    if (strlen(nodes[sid].name) == 0) strcpy(nodes[sid].name, "Source");
if (strlen(nodes[did].name) == 0) strcpy(nodes[did].name, "Destination");

    printf("\nStart: %s\n", nodes[sid].name);
    printf("Destination: %s\n", nodes[did].name);
      double directDist = haversineDistance(slat, slon, dlat, dlon);
    printf("Direct distance: %.0f m\n", directDist);

    printf("\nCalculating routes...\n");

   // Route 1: User preference (all modes)
    unsigned int allModes = (1<<MODE_WALK) | (1<<MODE_BIKE) | (1<<MODE_RICKSHAW) | 
                            (1<<MODE_METRO) | (1<<MODE_UTTARA) | (1<<MODE_BIKOLPO);
    vector<int> path1 = dijkstraFiltered(sid, did, w_time, w_cost, w_dist, allModes);
    
    // Route 2: Time-optimized (Metro + Rickshaw for speed)
    unsigned int timeModes = (1<<MODE_WALK) | (1<<MODE_METRO) | (1<<MODE_RICKSHAW);
    vector<int> path2 = dijkstraFiltered(sid, did, 0.8, 0.1, 0.1, timeModes);
    
    // Route 3: Cost-optimized (Walk + Bus, or Walk only if <2km)
    unsigned int costModes;
    if (directDist < 2000) {
        costModes = (1<<MODE_WALK);  // Walk only for short trips
    } else {
        costModes = (1<<MODE_WALK) | (1<<MODE_UTTARA) | (1<<MODE_BIKOLPO);
    }
    vector<int> path3 = dijkstraFiltered(sid, did, 0.1, 0.8, 0.1, costModes);
    
    // Fallbacks
    if (path1.empty()) path1 = dijkstra(sid, did, w_time, w_cost, w_dist);
    if (path2.empty()) path2 = dijkstra(sid, did, 0.8, 0.1, 0.1);
    if (path3.empty()) path3 = dijkstra(sid, did, 0.1, 0.8, 0.1);

    printRouteShort(path1, "ROUTE 1 (Your Preferences)");
    printRouteShort(path2, "ROUTE 2 (Time Optimized)");
    printRouteShort(path3, "ROUTE 3 (Cost Optimized)");


    if (!path1.empty()) exportRouteToJSON(path1, "../web/route1.json");
    if (!path2.empty()) exportRouteToJSON(path2, "../web/route2.json");
    if (!path3.empty()) exportRouteToJSON(path3, "../web/route3.json");

    if (!path1.empty()) exportPathToKML(path1, "../web/route1.kml");
    if (!path2.empty()) exportPathToKML(path2, "../web/route2.kml");
    if (!path3.empty()) exportPathToKML(path3, "../web/route3.kml");

     printf("\nFiles exported to web/\n");
    printf("Open web/index.html to view routes.\n\n");
    return 0;
}
