# 🌐 Browsing Insight Tool

A Flask web app that analyzes your **Chrome browsing history** using a C++ analyzer backend and visualizes key insights such as most visited sites, browsing paths, transition graphs, and more.

---

## 📁 Features

- Upload your Chrome history database
- Converts SQLite data to CSV
- Runs a C++ analysis program
- Displays:
  - Summary info
  - DFS traversal
  - Transition graph
  - Shortest browsing paths
  - Most visited domains
  - Frequent transitions
  - Common browsing paths
- Visualized using interactive tables and bar charts

---

##  Quick Start
1. Install Python Dependencies
pip install flask pandas
2. Compile the C++ Analyzer
 On Windows (with MinGW or similar installed):g++ analyzer.cpp -o analyzer.exe
On Linux or macOS:g++ analyzer.cpp -o analyzer


This will generate:
analyzer.exe on Windows
analyzer on Linux/macOS

4. Run the Flask App
   python server.py
   Then open your browser and go to:
   http://127.0.0.1:5000/


##Output Files Generated
Each run creates a timestamped folder like outputs/run_1713450000/ containing:


File	Description

summary.txt	        Summary of browsing data
dfs_output.txt	    DFS traversal of visited domains
graph.txt	          Transition graph info
shortest_path.txt	  Shortest path between nodes
visit_counts.csv	  Top visited domains
transitions.csv	    Frequently followed links
frequent_paths.csv	Common visit sequences

## How to Get Your  History File
Browser | Path
Chrome | C:\Users\<User>\AppData\Local\Google\Chrome\User Data\Default\History
Edge | C:\Users\<User>\AppData\Local\Microsoft\Edge\User Data\Default\History
Brave | C:\Users\<User>\AppData\Local\BraveSoftware\Brave-Browser\User Data\Default\History
Opera | C:\Users\<User>\AppData\Roaming\Opera Software\Opera Stable\History

##Instead of uploading your actual History file directly, it's recommended to make a copy of itand then upload it 

##Temporary files are stored under:
uploads/ — for uploaded .sqlite files and converted .csv
outputs/ — for C++ analysis results

Delete these folders periodically to save space.
