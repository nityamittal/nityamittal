// Single-floor parking lot.
// Patterns: Singleton (ParkingManager), Strategy (FeeStrategy, PaymentStrategy)
#include<bits/stdc++.h>
using namespace std;

/*
parking lot - user journey:
1. vehicle enters -> gets a spot assigned -> parks; no spot free, no entry
2. comes back to its spot -> pays fee -> exits
3. single floor, 3 vehicle types, rate depends on the type
*/

enum class VehicleType { CAR, BIKE, TRUCK };

string toString(VehicleType t) {
    if (t == VehicleType::CAR) return "Car";
    if (t == VehicleType::BIKE) return "Bike";
    return "Truck";
}

struct Vehicle {
    string plate;
    VehicleType type;

    Vehicle(string plate, VehicleType type) : plate(plate), type(type) {}
};

// ---- Strategy: how the fee is calculated ----
class FeeStrategy {
public:
    virtual ~FeeStrategy() = default;
    virtual double calculateFee(VehicleType type, time_t entryTime, time_t exitTime) const = 0;
};

class HourlyFee : public FeeStrategy {
public:
    double calculateFee(VehicleType type, time_t entryTime, time_t exitTime) const override {
        double hours = max(1.0, ceil(difftime(exitTime, entryTime) / (60.0 * 60))); // minimum 1 hour billed
        double ratePerHour = (type == VehicleType::CAR) ? 10.0 : (type == VehicleType::BIKE) ? 5.0 : 20.0;
        return hours * ratePerHour;
    }
};

// ---- Strategy: how payment is made ----
class PaymentStrategy {
public:
    virtual ~PaymentStrategy() = default;
    virtual void pay(double amount) = 0;
};

class PayByCash : public PaymentStrategy {
public:
    void pay(double amount) override { cout << "Paid in cash = $" << amount << "\n"; }
};

class PayByCard : public PaymentStrategy {
public:
    void pay(double amount) override { cout << "Paid using card = $" << amount << "\n"; }
};

// ---- A single spot. No Ticket class: the spot remembers who's parked and
// ---- when, so the spot id IS your ticket. ----
class ParkingSpot {
public:
    int id;
    VehicleType type;
    bool occupied = false;
    Vehicle parkedVehicle{"", VehicleType::CAR}; // placeholder until occupied
    time_t entryTime = 0;

    ParkingSpot(int id, VehicleType type) : id(id), type(type) {}

    void park(Vehicle v) {
        parkedVehicle = v;
        entryTime = time(nullptr);
        occupied = true;
    }

    void vacate() {
        parkedVehicle = {"", VehicleType::CAR};
        entryTime = 0;
        occupied = false;
    }
};

// ---- Singleton: exactly one lot, one floor's worth of spots. ----
class ParkingManager {
    vector<ParkingSpot> spots_;
    HourlyFee feeStrategy_;

    ParkingManager() = default;

public:
    ParkingManager(const ParkingManager&) = delete;
    ParkingManager& operator=(const ParkingManager&) = delete;

    static ParkingManager& instance() {
        static ParkingManager lot;
        return lot;
    }

    void addSpot(VehicleType type) {
        spots_.emplace_back((int)spots_.size() + 1, type);
    }

    // Returns true and prints where the vehicle parked (remember the spot id,
    // that is your "ticket"), or false if the lot is full for this type.
    bool parkVehicle(Vehicle vehicle) {
        for (auto& spot : spots_) {
            if (spot.occupied || spot.type != vehicle.type) continue;
            spot.park(vehicle);
            cout << toString(vehicle.type) << " " << vehicle.plate
                 << " parked at spot " << spot.id << "\n";
            return true;
        }
        cout << "No free spot for " << toString(vehicle.type) << "!\n";
        return false;
    }

    void exitVehicle(int spotId, PaymentStrategy& payment) {
        for (auto& spot : spots_) {
            if (spot.id != spotId) continue;
            if (!spot.occupied) break;
            double fee = feeStrategy_.calculateFee(spot.type, spot.entryTime, time(nullptr));
            payment.pay(fee);
            spot.vacate();
            return;
        }
        throw runtime_error("Invalid or already-vacant spot");
    }
};

// ---- Demo ----
int main() {
    ParkingManager& lot = ParkingManager::instance();

    lot.addSpot(VehicleType::CAR);    // spot 1
    lot.addSpot(VehicleType::BIKE);   // spot 2
    lot.addSpot(VehicleType::TRUCK);  // spot 3

    Vehicle car("CAR-101", VehicleType::CAR);
    Vehicle bike("BIKE-202", VehicleType::BIKE);

    lot.parkVehicle(car);   // -> spot 1
    lot.parkVehicle(bike);  // -> spot 2

    PayByCard card;
    PayByCash cash;
    lot.exitVehicle(1, card);  // pay for the car
    lot.exitVehicle(2, cash);  // pay for the bike

    return 0;
}
