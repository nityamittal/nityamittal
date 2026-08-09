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
    virtual double calculateFee(VehicleType type, time_t entryTime, time_t exitTime) const = 0;
};

class HourlyRateFeeStrategy : public FeeStrategy {
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
