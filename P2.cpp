#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <mutex>
#include <condition_variable>
#include <algorithm> 

using namespace std;

// --- Concurrency and Synchronization ---
mutex coutMutex;          // For synchronized console output
mutex statsMutex;         // For dungeon statistics
mutex playerMutex;        // For protecting player counters
condition_variable cv;    // For signaling player availability

// --- Global Configuration (Player counters are the "buffer") ---
long long numDungeons = 0, numTanks = 0, numHealers = 0, numDPS = 0;
int minDungeonTime = 0, maxDungeonTime = 0;
bool simulationOver = false; 

// --- Dungeon Statistics ---
struct DungeonStats {
    bool active;
    int partiesServed;
    long long totalTime;
    DungeonStats() : active(false), partiesServed(0), totalTime(0) {}
};
vector<DungeonStats> instanceStats;


// --- Utility Functions ---

// Print status of all dungeon instances.
void printDungeonStatuses() {
    lock_guard<mutex> lock(statsMutex);
    cout << "\nCurrent Dungeons Status:" << endl;
    for (size_t i = 0; i < instanceStats.size(); i++) {
        cout << "Dungeon " << i + 1 << ": " 
             << (instanceStats[i].active ? "active" : "empty") << endl;
    }
}

void trim(string &s) {
    size_t start = s.find_first_not_of(" \t");
    size_t end = s.find_last_not_of(" \t");
    s = (start == string::npos) ? "" : s.substr(start, end - start + 1);
}

// --- Configuration Reading ---

bool readConfigFile() {
    const string fileName = "config.txt";
    ifstream file(fileName);
    if (!file.is_open()) {
        cerr << "Error: Unable to open configuration file: " << fileName << endl;
        return false;
    }
    if (file.peek() == EOF) {
        cerr << "Error: Configuration file is empty." << endl;
        return false;
    }
    string line;
    while (getline(file, line)) {
        size_t pos = line.find('=');
        if (pos == string::npos) continue;
        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);
        trim(key);
        trim(value);
        try {
            if (key == "n") {
                numDungeons = stoll(value); 
                if (numDungeons < 1) {
                    cerr << "Error: n (max concurrent instances) must be at least 1." << endl;
                    return false;
                }
            } else if (key == "t") {
                numTanks = stoll(value); 
                if (numTanks < 1) {
                    cerr << "Error: t (number of tank players) must be at least 1." << endl;
                    return false;
                }
            } else if (key == "h") {
                numHealers = stoll(value); 
                if (numHealers < 1) {
                    cerr << "Error: h (number of healer players) must be at least 1." << endl;
                    return false;
                }
            } else if (key == "d") {
                numDPS = stoll(value); 
                if (numDPS < 1) {
                    cerr << "Error: d (number of DPS players) must be at least 1." << endl;
                    return false;
                }
            } else if (key == "t1") {
                minDungeonTime = stoi(value);
                if (minDungeonTime <= 0) {
                    cerr << "Error: t1 (min time) must be greater than 0." << endl;
                    return false;
                }
            } else if (key == "t2") {
                maxDungeonTime = stoi(value);
                if (maxDungeonTime <= 0) {
                    cerr << "Error: t2 (max time) must be greater than 0." << endl;
                    return false;
                }
                if (maxDungeonTime < minDungeonTime) {
                    cerr << "Error: t2 (max time) must be greater than or equal to t1." << endl;
                    return false;
                }
            } else {
                cout << "Warning: Unknown configuration key \"" << key << "\". Skipping." << endl;
            }
        } catch (const out_of_range& oor) {
            cerr << "Error: Value for key \"" << key << "\" is out of range." << endl;
            return false;
        } catch (...) {
            cerr << "Error: Invalid value for key \"" << key << "\"." << endl;
            return false;
        }
    }
    return true;
}

// --- Dungeon Logic (Consumer) ---

// Dungeon instance thread: waits for players, forms parties, and simulates runs.
void dungeonConsumer(int instanceId) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(minDungeonTime, maxDungeonTime);

    while (true) {
        unique_lock<mutex> lock(playerMutex);
        
        cv.wait(lock, [] {
            return (numTanks >= 1 && numHealers >= 1 && numDPS >= 3) || simulationOver;
        });

        
        if (simulationOver) {
            break; 
        }

        if (numTanks < 1 || numHealers < 1 || numDPS < 3) {
            simulationOver = true;
            cv.notify_all(); 
            break; 
        }

        // --- A party can be formed. Consume players (decrement counters). ---
        numTanks--;
        numHealers--;
        numDPS -= 3;
        
        if (numTanks < 1 || numHealers < 1 || numDPS < 3) {
            simulationOver = true;
            cv.notify_all(); 
        }

        lock.unlock(); 

        // --- Simulate Dungeon Run ---
        {
            lock_guard<mutex> statsLock(statsMutex);
            instanceStats[instanceId].active = true;
        }
        {
            lock_guard<mutex> coutLock(coutMutex);
            cout << "\nQueueing up players for Dungeon Instance " << instanceId + 1 << endl;
            printDungeonStatuses();
        }

        int dungeonTime = dis(gen);
        this_thread::sleep_for(chrono::seconds(dungeonTime));

        // --- Update Stats Post-Run ---
        {
            lock_guard<mutex> statsLock(statsMutex);
            instanceStats[instanceId].partiesServed++;
            instanceStats[instanceId].totalTime += dungeonTime;
            instanceStats[instanceId].active = false;
        }
        {
            lock_guard<mutex> coutLock(coutMutex);
            cout << "\nDungeon " << instanceId + 1 << " finished. Status:" << endl;
            printDungeonStatuses();
        }
    }
    
}

// --- Main Function (Producer) ---

int main() {
    cout << "Reading config from config.txt" << endl;
    if (!readConfigFile()) {
        return 1;
    }

    int numDungeonsToRun = 0;
    try {
        if (numDungeons > std::numeric_limits<int>::max()) {
            cout << "Warning: Number of dungeons exceeds system limits, clamping to max int." << endl;
            numDungeonsToRun = std::numeric_limits<int>::max();
        } else {
            numDungeonsToRun = static_cast<int>(numDungeons);
        }
    } catch(...) {
        cerr << "Could not convert numDungeons to a runnable integer value." << endl;
        return 1;
    }


    instanceStats.resize(numDungeonsToRun);
    vector<thread> dungeonThreads;

    cout << "\nStarting dungeon instances..." << endl;
    
    for (int i = 0; i < numDungeonsToRun; i++) {
        dungeonThreads.emplace_back(dungeonConsumer, i);
    }
    
    cv.notify_all(); 

    for (auto &th : dungeonThreads) {
        if (th.joinable())
            th.join();
    }
    
    cout << "\n--- Simulation Finished ---\n" << endl;
    cout << "Dungeon Instance Summary:" << endl;
    long long totalCountPartiesServed = 0;
    for (int i = 0; i < numDungeonsToRun; i++) {
        totalCountPartiesServed += instanceStats[i].partiesServed;
        cout << "Dungeon " << i + 1 
             << " served " << instanceStats[i].partiesServed 
             << " parties, total time = " << instanceStats[i].totalTime 
             << " seconds." << endl;
    }
    cout << "\nTotal count of parties served: " << totalCountPartiesServed << endl;
    
    cout << "\nLeftover players:" << endl;
    cout << "Tanks: " << numTanks << endl;
    cout << "Healers: " << numHealers << endl;
    cout << "DPS: " << numDPS << endl;
    
    return 0;
}
