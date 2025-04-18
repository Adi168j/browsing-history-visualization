#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <climits>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <regex>

using namespace std;
const long long SESSION_GAP = 30LL * 60 * 1000000; 


struct Visit {
    string domain;
    long long rawTime;
    string readableTime;
    string title;
};

string chromeTimeToReadable(long long chromeTime) {
    const long long EPOCH_DIFF_SECS = 11644473600LL;
    time_t unixTime = chromeTime / 1000000 - EPOCH_DIFF_SECS;
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&unixTime));
    return string(buffer);
}

string extractDomain(const string& url) {
    smatch match;
    regex pattern(R"(https?:\/\/([^\/]+))");
    if (regex_search(url, match, pattern)) {
        return match[1];
    }
    return "";
}

bool compareByTime(const Visit& a, const Visit& b) {
    return a.rawTime < b.rawTime;
}

class Graph {
public:
    map<string, map<string, int>> adj;
    map<string, int> visitCount;
    map<string, string> domainToTitle;
    string outputFolder;

    Graph(const string& folder) : outputFolder(folder) {}

    void addEdge(const string& from, const string& to) {
        adj[from][to]++;
        visitCount[from]++;
    }

    void printGraph() {
        ofstream output(outputFolder + "/graph.txt");
        output << "===================================\n";
        output << "       Transition Graph\n";
        output << "===================================\n\n";
        
        for (const auto& pair : adj) {
            output << pair.first << " -> ";
            for (const auto& neighbor : pair.second) {
                output << neighbor.first << "(" << neighbor.second << ") ";
            }
            output << endl;
        }
        output.close();
    }

    void exportToDot(const string& filename) {
        ofstream dot(outputFolder + "/" + filename);
        dot << "digraph BrowsingGraph {\n";
        for (const auto& [domain, title] : domainToTitle) {
            dot << "\t\"" << domain << "\" [label=\"" << title << "\"];\n";
        }
        for (const auto& [from, edges] : adj) {
            for (const auto& [to, weight] : edges) {
                dot << "\t\"" << from << "\" -> \"" << to << "\" [label=" << weight << "]\n";
            }
        }
        dot << "}\n";
        dot.close();
        
        ofstream info(outputFolder + "/dot_info.txt");
        info << "DOT file saved as '" << filename << "'\n";
        info << "To visualize, run: dot -Tpng " << filename << " -o graph.png\n";
        info << "Then open graph.png to view your browsing graph!\n";
        info.close();
    }

    void showSummary() {
        ofstream output(outputFolder + "/summary.txt");
        output << "===================================\n";
        output << "   Browsing Pattern Analysis Summary\n";
        output << "===================================\n\n";
        
        output << "- Total unique sites: " << visitCount.size() << endl;

        string mostVisited;
        int maxCount = 0;
        for (const auto& [site, count] : visitCount) {
            if (count > maxCount) {
                maxCount = count;
                mostVisited = site;
            }
        }
        output << "- Most visited site: " << mostVisited << " (" << maxCount << " times)";
        if (domainToTitle.count(mostVisited)) {
            output << " — " << domainToTitle[mostVisited];
        }
        output << endl;

        string maxFrom, maxTo;
        int maxTrans = 0;
        for (const auto& [from, edges] : adj) {
            for (const auto& [to, weight] : edges) {
                if (weight > maxTrans) {
                    maxFrom = from;
                    maxTo = to;
                    maxTrans = weight;
                }
            }
        }
        output << "- Most common jump: " << maxFrom << " -> " << maxTo << " (" << maxTrans << " times)" << endl;

        // Top 5 transitions
        vector<tuple<int, string, string>> allTransitions;
        for (const auto& [from, edges] : adj) {
            for (const auto& [to, count] : edges) {
                allTransitions.push_back({count, from, to});
            }
        }

        sort(allTransitions.rbegin(), allTransitions.rend());
        output << "- Top 5 frequent transitions:\n";
        for (int i = 0; i < min(5, (int)allTransitions.size()); ++i) {
            auto [count, from, to] = allTransitions[i];
            output << "  " << i + 1 << ". " << from << " → " << to << " (" << count << " times)" << endl;
        }
        output.close();
    }

    void dfs(const string& start, set<string>& visited, ofstream& output) {
        visited.insert(start);
        output << start << " ";
        for (const auto& [neighbor, _] : adj[start]) {
            if (!visited.count(neighbor)) {
                dfs(neighbor, visited, output);
            }
        }
    }

    void runDFS(const string& start) {
        set<string> visited;
        ofstream output(outputFolder + "/dfs_output.txt");
        output << "===================================\n";
        output << "   Depth-First Search Results\n";
        output << "===================================\n\n";
        
        output << "DFS from " << start << ":\n";
        dfs(start, visited, output);
        output << endl;
        output.close();
    }

    void dijkstra(const string& start, const string& end) {
        map<string, int> dist;
        map<string, string> prev;
        set<string> visited;

        for (auto& pair : adj) dist[pair.first] = INT_MAX;
        dist[start] = 0;

        using pii = pair<int, string>;
        priority_queue<pii, vector<pii>, greater<pii>> pq;
        pq.push({0, start});

        while (!pq.empty()) {
            string curr = pq.top().second;
            pq.pop();

            if (visited.count(curr)) continue;
            visited.insert(curr);

            for (const auto& [neighbor, weight] : adj[curr]) {
                float cost = 1/weight;
                if (dist[curr] + cost < dist[neighbor]) {
                    dist[neighbor] = dist[curr] + cost;
                    prev[neighbor] = curr;
                    pq.push({dist[neighbor], neighbor});
                }
            }
        }

        ofstream output(outputFolder + "/shortest_path.txt");
        output << "===================================\n";
        output << "   Shortest Path Analysis\n";
        output << "===================================\n\n";
        
        if (dist[end] == INT_MAX) {
            output << "No path found from " << start << " to " << end << endl;
            output.close();
            return;
        }

        vector<string> path;
        for (string at = end; !at.empty(); at = prev[at]) {
            path.push_back(at);
        }
        reverse(path.begin(), path.end());

        output << "Shortest path from " << start << " to " << end << ":\n";
        for (const string& p : path) {
            output << p;
            if (p != end) output << " -> ";
        }
        output << "\nHops: " << path.size() - 1 << endl;
        output.close();
    }
  
    
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <csv_file_path> [output_folder]" << endl;
        return 1;
    }

    string csvPath = argv[1];
    string outputPath = (argc >= 3) ? argv[2] : "outputs";

    ifstream file(csvPath);
    if (!file.is_open()) {
        cerr << "Error: Could not open " << csvPath << endl;
        return 1;
    }

    // Create header file
    ofstream headerFile(outputPath + "/header.txt");
    headerFile << "===================================\n";
    headerFile << "   Browsing Pattern Analyzer\n";
    headerFile << "===================================\n";
    headerFile.close();

    vector<Visit> visits;
    string line;
    getline(file, line); // skip header

    while (getline(file, line)) {
        stringstream ss(line);
        string url, title, visitCountStr, lastVisitStr;
        getline(ss, url, ',');
        getline(ss, title, ',');
        getline(ss, visitCountStr, ',');
        getline(ss, lastVisitStr, ',');

        string domain = extractDomain(url);
        if (domain.empty()) continue;

        long long rawTime;
        try {
            rawTime = stoll(lastVisitStr);
        } catch (...) {
            continue;
        }

        string readable = chromeTimeToReadable(rawTime);
        visits.push_back({domain, rawTime, readable, title});
    }

    sort(visits.begin(), visits.end(), compareByTime);

    Graph g(outputPath);
    for (const Visit& v : visits) {
        if (!g.domainToTitle.count(v.domain)) {
            g.domainToTitle[v.domain] = v.title;
        }
    }

    for (size_t i = 1; i < visits.size(); ++i) {
        string from = visits[i - 1].domain;
        string to = visits[i].domain;
    
        long long timeDiff = visits[i].rawTime - visits[i - 1].rawTime;
    
        if (from != to && timeDiff <= SESSION_GAP) {
            g.addEdge(from, to);
        }
    }
    

    g.showSummary();
    g.printGraph();

    if (!visits.empty()) {
        g.runDFS(visits.front().domain);
    }

    string dotFilename = "browsing_graph.dot";
    g.exportToDot(dotFilename);

    // Automatic Dijkstra on top 2 sites
    vector<pair<int, string>> sites;
    for (const auto& [site, count] : g.visitCount) {
        sites.push_back({count, site});
    }
    sort(sites.rbegin(), sites.rend());

    if (sites.size() >= 2) {
        string source = sites[0].second;
        string target = sites[1].second;
        g.dijkstra(source, target); 
        
        
    }

 
    ofstream visitCsv(outputPath + "/visit_counts.csv");
    visitCsv << "domain,count\n";
    for (const auto& [site, count] : g.visitCount) {
        visitCsv << site << "," << count << "\n";
    }
    visitCsv.close();

    ofstream transCsv(outputPath + "/transitions.csv");
    transCsv << "from,to,count\n";
    for (const auto& [from, edges] : g.adj) {
        for (const auto& [to, weight] : edges) {
            transCsv << from << "," << to << "," << weight << "\n";
        }
    }
    transCsv.close();

    cout << outputPath << endl;
    return 0;
}