// Single-floor parking lot.
// Patterns: Singleton (ParkingLot), Strategy (FeeStrategy, PaymentStrategy)
#include<bits/stdc++.h>
using namespace std;

enum class VehicleType { BIKE, CAR, TRUCK };

string toString(VehicleType t) {
    if (t == VehicleType::BIKE) return "Bike";
    if (t == VehicleType::CAR) return "Car";
    return "Truck";
}

struct Vehicle {
    string plate;
    VehicleType type;
    Vehicle(string plate, VehicleType t) : plate(plate), type(t) {}
};

// ---- Strategy: how fee is calculated ----
class FeeStrategy {
public:
    virtual ~FeeStrategy() = default;
    virtual double calculateFee(VehicleType type, long durationMinutes) const = 0;
};

class HourlyRateFeeStrategy : public FeeStrategy {
    unordered_map<VehicleType, double> ratePerHour_ = {
        {VehicleType::BIKE, 5.0}, {VehicleType::CAR, 10.0}, {VehicleType::TRUCK, 20.0},
    };
public:
    double calculateFee(VehicleType type, long durationMinutes) const override {
        double hours = max(1.0, ceil(durationMinutes / 60.0)); // minimum 1 hour billed
        return hours * ratePerHour_.at(type);
    }
};

// ---- Strategy: how payment is made ----
class PaymentStrategy {
public:
    virtual ~PaymentStrategy() = default;
    virtual void pay(double amount) = 0;
};

class CashPayment : public PaymentStrategy {
public:
    void pay(double amount) override { cout << "Paid $" << amount << " in cash.\n"; }
};

class CardPayment : public PaymentStrategy {
public:
    void pay(double amount) override { cout << "Paid $" << amount << " by card.\n"; }
};

// ---- A single parking spot. No Ticket class: the spot itself remembers
// ---- who's parked and when, so the spotId IS your ticket. ----
class ParkingSpot {
public:
    int id;
    VehicleType type;
    bool occupied = false;
    Vehicle parkedVehicle{"", VehicleType::CAR}; // placeholder until occupied
    time_t entryTime = 0;

    ParkingSpot(int id, VehicleType type) : id(id), type(type) {}

    void park(Vehicle v) {
        if (occupied) throw runtime_error("Spot already occupied");
        if (v.type != type) throw runtime_error("Vehicle type does not match spot type");
        parkedVehicle = v;
        entryTime = time(nullptr);
        occupied = true;
    }

    void vacate() { occupied = false; }
};

// ---- Singleton: exactly one parking lot, one floor's worth of spots. ----
class ParkingLot {
    vector<ParkingSpot> spots_;
    HourlyRateFeeStrategy feeStrategy_;

    ParkingLot() = default;

    // Internal lookups: the caller only ever deals in spot ids.
    ParkingSpot* findFreeSpot(VehicleType type) {
        for (auto& s : spots_)
            if (!s.occupied && s.type == type) return &s;
        return nullptr;
    }

    ParkingSpot* findSpotById(int spotId) {
        for (auto& s : spots_)
            if (s.id == spotId) return &s;
        return nullptr;
    }

public:
    ParkingLot(const ParkingLot&) = delete;
    ParkingLot& operator=(const ParkingLot&) = delete;

    static ParkingLot& instance() {
        static ParkingLot lot;
        return lot;
    }

    void addSpot(VehicleType type) {
        spots_.emplace_back((int)spots_.size() + 1, type);
    }

    // Returns true and prints where the vehicle parked (remember the spot id,
    // that is your "ticket"), or false if the lot is full for this type.
    bool parkVehicle(Vehicle vehicle) {
        ParkingSpot* spot = findFreeSpot(vehicle.type);
        if (!spot) {
            cout << "No free spot for " << toString(vehicle.type) << "!\n";
            return false;
        }
        spot->park(vehicle);
        cout << toString(vehicle.type) << " " << vehicle.plate
             << " parked at spot " << spot->id << "\n";
        return true;
    }

    void exitVehicle(int spotId, PaymentStrategy& payment) {
        ParkingSpot* spot = findSpotById(spotId);
        if (!spot || !spot->occupied) throw runtime_error("Invalid or already-vacant spot");

        long durationMinutes = (long)difftime(time(nullptr), spot->entryTime) / 60;
        double fee = feeStrategy_.calculateFee(spot->type, durationMinutes);
        payment.pay(fee);
        spot->vacate();
    }
};

// ---- Demo ----
int main() {
    ParkingLot& lot = ParkingLot::instance();

    lot.addSpot(VehicleType::CAR);    // spot 1
    lot.addSpot(VehicleType::BIKE);   // spot 2
    lot.addSpot(VehicleType::TRUCK);  // spot 3

    Vehicle car("CAR-101", VehicleType::CAR);
    Vehicle bike("BIKE-202", VehicleType::BIKE);

    lot.parkVehicle(car);   // -> spot 1
    lot.parkVehicle(bike);  // -> spot 2

    CardPayment card;
    CashPayment cash;
    lot.exitVehicle(1, card);  // pay for the car
    lot.exitVehicle(2, cash);  // pay for the bike

    return 0;
}
