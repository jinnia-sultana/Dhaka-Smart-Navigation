#include "csvParser.h"
#include "nodesAndEdges.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <cctype>
#include <vector>
#include <string>

using namespace std;

//String helpers
void trim_in_place(char *s) {
    if (!s) return;
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n-1])) s[--n] = '\0';
}

int split_csv(char *line, char **tokens, int maxTokens) {
    int count = 0;
    char *save = NULL;
    for (char *tok = strtok_r(line, ",", &save);
         tok && count < maxTokens;
         tok = strtok_r(NULL, ",", &save)) {
        trim_in_place(tok);
        tokens[count++] = tok;
    }
    return count;
}

int is_number_token(const char *s) {
    if (!s) return 0;
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') return 0;
    char *end = NULL;
    strtod(s, &end);
    while (end && *end && isspace((unsigned char)*end)) end++;
    return end && *end == '\0';
}

//Road network
void parseRoadmapCSV(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) { printf("Error opening %s\n", filename); return; }

    char line[MAX_LINE];
    char *tokens[MAX_TOKENS];

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!strlen(line)) continue;
        int count = split_csv(line, tokens, MAX_TOKENS);
        if (count < 6) continue;

        //last 2 cols: altitude, length
        if (!is_number_token(tokens[count-2]) ||
            !is_number_token(tokens[count-1])) continue;

        int coordEnd = count - 2; 
        if ((coordEnd - 1) < 4 || (coordEnd - 1) % 2 != 0) continue;

        // tokens[1..coordEnd-1] = lon/lat pairs
        for (int i = 1; i + 3 <= coordEnd; i += 2) {
            double lon1 = atof(tokens[i]);
            double lat1 = atof(tokens[i+1]);
            double lon2 = atof(tokens[i+2]);
            double lat2 = atof(tokens[i+3]);

            int from = findOrAddNode(lat1, lon1);
            int to   = findOrAddNode(lat2, lon2);
            double dist = haversineDistance(lat1, lon1, lat2, lon2);

            addEdge(from, to,   MODE_WALK, dist);
            addEdge(to,   from, MODE_WALK, dist);
            addEdge(from, to,   MODE_BIKE, dist);
            addEdge(to,   from, MODE_BIKE, dist);
        }
    }
    fclose(f);
    printf("Roads loaded: %d nodes, %d edges\n", numNodes, numEdges);
}

//Bus Metro loader

void parseTransitCSV(const char *filename, Mode mode) {
    FILE *f = fopen(filename, "r");
    if (!f) { printf("Error opening %s\n", filename); return; }

    char line[MAX_LINE];
    char *tokens[MAX_TOKENS];

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!strlen(line)) continue;
        int count = split_csv(line, tokens, MAX_TOKENS);
        if (count < 5) continue;

        const char *startStop = tokens[count-2];
        const char *endStop   = tokens[count-1];

        if (is_number_token(startStop) || is_number_token(endStop)) continue;

        int coordEnd = count - 2;
        int coordLen = coordEnd - 1;
        if (coordLen < 4 || coordLen % 2 != 0) continue;

        int pairCount = coordLen / 2;
        vector<int> lineNodes;

        for (int p = 0; p < pairCount; p++) {
            int ti = 1 + p * 2;
            double lon = atof(tokens[ti]);
            double lat = atof(tokens[ti+1]);

            const char* nm = nullptr;
            if (p == 0) nm = startStop;
            if (p == pairCount-1) nm = endStop;

            int id = findOrAddNode(lat, lon, nm);
            lineNodes.push_back(id);
        }

        for (int i = 0; i + 1 < (int)lineNodes.size(); i++) {
            int u = lineNodes[i], v = lineNodes[i+1];
            double dist = haversineDistance(nodes[u].lat, nodes[u].lon,
                                             nodes[v].lat, nodes[v].lon);
            addEdge(u, v, mode, dist);
            addEdge(v, u, mode, dist);
        }
    }
    fclose(f);
    printf("Transit (%s) loaded\n", filename);
}
void parseMetroCSV(const char *filename) {
    parseTransitCSV(filename, MODE_METRO);
}

void parseBusCSV(const char *filename, Mode busMode) {
    parseTransitCSV(filename, busMode);
}

//KML export
//Path stored start→end, so iterate forward
void exportPathToKML(const vector<int>& path, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) { printf("Failed to open %s\n", filename); return; }

    fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(f, "<kml xmlns=\"http://earth.google.com/kml/2.1\">\n");
    fprintf(f, "<Document>\n");
    fprintf(f, "<Placemark><name>Route</name>\n");
    fprintf(f, "<LineString><tessellate>1</tessellate>\n");
    fprintf(f, "<coordinates>\n");

    //Forward iteration: path[0] = start, path[n-1] = goal
    for (int i = 0; i < (int)path.size(); i++) {
        int id = path[i];
        fprintf(f, "%.6f,%.6f,0\n", nodes[id].lon, nodes[id].lat);
    }

    fprintf(f, "</coordinates>\n</LineString>\n</Placemark>\n");
    fprintf(f, "</Document>\n</kml>\n");
    fclose(f);
    printf("Exported %s (%d points)\n", filename, (int)path.size());
}