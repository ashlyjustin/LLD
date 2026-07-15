#include <iostream>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <string>
using namespace std;

enum class VehicleType { BIKE, CAR, TRUCK };

static string vehicleTypeToString(VehicleType t) {
    switch (t) {
        case VehicleType::BIKE: return "BIKE";
        case VehicleType::CAR: return "CAR";
        case VehicleType::TRUCK: return "TRUCK";
    }
    return "UNKNOWN";
}

struct Vehicle {
    string number;
    VehicleType type;
    Vehicle() = default;
    Vehicle(string n, VehicleType t) : number(move(n)), type(t) {}
};

struct ParkingSpot {
    int floor;
    int row;
    int col;
    VehicleType supportedType;
    optional<Vehicle> vehicle; // engaged when occupied

    ParkingSpot(int f=0,int r=0,int c=0,VehicleType t=VehicleType::CAR)
        : floor(f), row(r), col(c), supportedType(t), vehicle(nullopt) {}

    bool isOccupied() const { return vehicle.has_value(); }
    bool canFit(const Vehicle &v) const { return !isOccupied() && supportedType == v.type; }
    void park(const Vehicle &v) { vehicle = v; }
    void unpark() { vehicle.reset(); }
    string id() const { return to_string(floor)+":"+to_string(row)+":"+to_string(col); }
};

class ParkingFloor {
public:
    int floorNumber;
    vector<vector<ParkingSpot>> spots; // rows x cols

    ParkingFloor(int floorNumber_, int rows, int cols, VehicleType defaultType)
        : floorNumber(floorNumber_), spots(rows, vector<ParkingSpot>(cols)) {
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                spots[r][c] = ParkingSpot(floorNumber_, r, c, defaultType);
            }
        }
    }

    int rows() const { return (int)spots.size(); }
    int cols() const { return spots.empty() ? 0 : (int)spots[0].size(); }

    int countFreeSpots(VehicleType type) const {
        int cnt = 0;
        for (const auto &row : spots) for (const auto &s : row) if (!s.isOccupied() && s.supportedType == type) ++cnt;
        return cnt;
    }
};

// Strategy interface
class ParkingStrategy {
public:
    virtual ParkingSpot* findSpot(const vector<ParkingFloor*> &floors, const Vehicle &v) = 0;
    virtual ~ParkingStrategy() = default;
};

// Nearest-first strategy: iterate floors, rows, cols
class NearestFirstStrategy : public ParkingStrategy {
public:
    ParkingSpot* findSpot(const vector<ParkingFloor*> &floors, const Vehicle &v) override {
        for (auto *f : floors) {
            for (int r = 0; r < f->rows(); ++r) {
                for (int c = 0; c < f->cols(); ++c) {
                    ParkingSpot &s = f->spots[r][c];
                    if (s.canFit(v)) return &s;
                }
            }
        }
        return nullptr;
    }
};

// Farthest-first strategy: iterates floors/rows/cols in reverse order.
// Useful to keep entrance-side spots free for quick turnarounds.
class FarthestFirstStrategy : public ParkingStrategy {
public:
    ParkingSpot* findSpot(const vector<ParkingFloor*> &floors, const Vehicle &v) override {
        for (int fi = (int)floors.size() - 1; fi >= 0; --fi) {
            ParkingFloor *f = floors[fi];
            for (int r = f->rows() - 1; r >= 0; --r) {
                for (int c = f->cols() - 1; c >= 0; --c) {
                    ParkingSpot &s = f->spots[r][c];
                    if (s.canFit(v)) return &s;
                }
            }
        }
        return nullptr;
    }
};

// Least-loaded floor strategy: always picks the floor with the most free spots
// for the given vehicle type, then uses nearest-first within that floor.
// This balances utilization across floors.
class LeastLoadedFloorStrategy : public ParkingStrategy {
public:
    ParkingSpot* findSpot(const vector<ParkingFloor*> &floors, const Vehicle &v) override {
        ParkingFloor *best = nullptr;
        int maxFree = -1;
        for (auto *f : floors) {
            int free = f->countFreeSpots(v.type);
            if (free > maxFree) { maxFree = free; best = f; }
        }
        if (!best) return nullptr;
        for (int r = 0; r < best->rows(); ++r)
            for (int c = 0; c < best->cols(); ++c) {
                ParkingSpot &s = best->spots[r][c];
                if (s.canFit(v)) return &s;
            }
        return nullptr;
    }
};

// Even-distribution (round-robin) strategy: cycles through floors on each park
// call so that vehicles spread evenly across all floors.
class EvenDistributionStrategy : public ParkingStrategy {
    int nextFloor = 0; // index into floors vector for the next candidate
public:
    ParkingSpot* findSpot(const vector<ParkingFloor*> &floors, const Vehicle &v) override {
        int n = (int)floors.size();
        for (int i = 0; i < n; ++i) {
            ParkingFloor *f = floors[(nextFloor + i) % n];
            for (int r = 0; r < f->rows(); ++r) {
                for (int c = 0; c < f->cols(); ++c) {
                    ParkingSpot &s = f->spots[r][c];
                    if (s.canFit(v)) {
                        nextFloor = (nextFloor + i + 1) % n; // advance for next call
                        return &s;
                    }
                }
            }
        }
        return nullptr;
    }
};

class ParkingLot {
    vector<unique_ptr<ParkingFloor>> floors_store; // ownership
    vector<ParkingFloor*> floors; // raw pointers for strategy API
    unique_ptr<ParkingStrategy> strategy;
    unordered_map<string, ParkingSpot*> vehicleMap; // vehicle number -> spot pointer

public:
    ParkingLot(unique_ptr<ParkingStrategy> strat) : strategy(move(strat)) {}

    // create and add floor convenience
    void addFloor(int floorNumber, int rows, int cols, VehicleType type) {
        auto pf = make_unique<ParkingFloor>(floorNumber, rows, cols, type);
        floors.push_back(pf.get());
        floors_store.push_back(move(pf));
    }

    bool parkVehicle(const Vehicle &v) {
        if (vehicleMap.find(v.number) != vehicleMap.end()) return false; // already parked
        ParkingSpot *spot = strategy->findSpot(floors, v);
        if (!spot) return false;
        spot->park(v);
        vehicleMap[v.number] = spot;
        return true;
    }

    bool unparkVehicle(const string &vehicleNumber) {
        auto it = vehicleMap.find(vehicleNumber);
        if (it == vehicleMap.end()) return false;
        ParkingSpot *spot = it->second;
        spot->unpark();
        vehicleMap.erase(it);
        return true;
    }

    ParkingSpot* searchVehicle(const string &vehicleNumber) const {
        auto it = vehicleMap.find(vehicleNumber);
        if (it == vehicleMap.end()) return nullptr;
        return it->second;
    }

    int countFreeSpots(int floorNumber, VehicleType type) const {
        for (auto *f : floors) if (f->floorNumber == floorNumber) return f->countFreeSpots(type);
        return 0;
    }

    void printStatus() const {
        cout << "--- ParkingLot Status ---\n";
        for (auto *f : floors) {
            cout << "Floor " << f->floorNumber << " (" << f->rows() << "x" << f->cols() << ")\n";
            for (int r = 0; r < f->rows(); ++r) {
                for (int c = 0; c < f->cols(); ++c) {
                    const ParkingSpot &s = f->spots[r][c];
                    if (s.isOccupied()) cout << "[X]"; else cout << "[ ]";
                }
                cout << '\n';
            }
        }
    }
};

static void runDemo(const string &strategyName, unique_ptr<ParkingStrategy> strat) {
    cout << "\n========== " << strategyName << " ==========\n";
    ParkingLot lot(move(strat));

    // Two CAR floors and one TRUCK floor
    lot.addFloor(0, 3, 4, VehicleType::CAR);
    lot.addFloor(1, 3, 4, VehicleType::CAR);
    lot.addFloor(2, 2, 3, VehicleType::TRUCK);

    Vehicle cars[] = {
        {"CAR-001", VehicleType::CAR},
        {"CAR-002", VehicleType::CAR},
        {"CAR-003", VehicleType::CAR},
        {"CAR-004", VehicleType::CAR},
        {"CAR-005", VehicleType::CAR},
    };
    Vehicle trk1("TRK-001", VehicleType::TRUCK);

    for (auto &c : cars)
        cout << "Park " << c.number << ": " << (lot.parkVehicle(c) ? "OK" : "FAIL") << "\n";
    cout << "Park " << trk1.number << ": " << (lot.parkVehicle(trk1) ? "OK" : "FAIL") << "\n";

    auto spot = lot.searchVehicle("CAR-001");
    if (spot) cout << "CAR-001 at spot " << spot->id() << "\n";

    spot = lot.searchVehicle("CAR-005");
    if (spot) cout << "CAR-005 at spot " << spot->id() << "\n";

    cout << "Free CAR on floor 0: " << lot.countFreeSpots(0, VehicleType::CAR) << "\n";
    cout << "Free CAR on floor 1: " << lot.countFreeSpots(1, VehicleType::CAR) << "\n";
    lot.printStatus();
}

int main() {
    runDemo("NearestFirstStrategy",       make_unique<NearestFirstStrategy>());
    runDemo("FarthestFirstStrategy",      make_unique<FarthestFirstStrategy>());
    runDemo("LeastLoadedFloorStrategy",   make_unique<LeastLoadedFloorStrategy>());
    runDemo("EvenDistributionStrategy",   make_unique<EvenDistributionStrategy>());
    return 0;
}
