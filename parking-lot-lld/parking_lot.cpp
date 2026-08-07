// ============================================================================
// Parking Lot - Low Level Design (single-file, interview-ready C++17)
//
// Compile:  g++ -std=c++17 -O2 -o parking_lot parking_lot.cpp && ./parking_lot
//
// Design patterns used (kept to only what earns its place):
//   1. Strategy   -> fee calculation per vehicle type, and payment method
//   2. Factory    -> creates the right ParkingSpot for a vehicle type
//   3. Singleton  -> exactly one ParkingLot instance
//   4. Observer   -> DisplayBoard is notified when a spot is taken/freed
//
// Multi-floor support: ParkingLot owns N ParkingFloor objects; each floor
// owns its own spots and knows how to find a free one. ParkingLot just
// delegates across floors (Single Responsibility Principle).
//
// Vehicle-type extension: adding a new vehicle type is a ONE-LINE change
// (add an enum value + a rate entry). There is deliberately no per-type
// Vehicle subclass (BikeVehicle/CarVehicle/...) because vehicles don't
// behave differently from each other here -- only their type tag matters.
// Subclassing them would be the "overkill" the prompt is trying to avoid.
// ============================================================================

#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// Vehicle
// ---------------------------------------------------------------------------

enum class VehicleType { BIKE, CAR, TRUCK };

std::string toString(VehicleType t) {
    switch (t) {
        case VehicleType::BIKE:  return "Bike";
        case VehicleType::CAR:   return "Car";
        case VehicleType::TRUCK: return "Truck";
    }
    return "Unknown";
}

struct Vehicle {
    std::string licensePlate;
    VehicleType type;
    Vehicle(std::string plate, VehicleType t) : licensePlate(std::move(plate)), type(t) {}
};

// ---------------------------------------------------------------------------
// Strategy #1: fee calculation
// ---------------------------------------------------------------------------

class FeeStrategy {
public:
    virtual ~FeeStrategy() = default;
    virtual double calculateFee(VehicleType type, long durationMinutes) const = 0;
};

// Adding a new vehicle type only means adding one entry to this map.
class HourlyRateFeeStrategy : public FeeStrategy {
    std::unordered_map<VehicleType, double> ratePerHour_ = {
        {VehicleType::BIKE, 5.0},
        {VehicleType::CAR, 10.0},
        {VehicleType::TRUCK, 20.0},
    };

public:
    double calculateFee(VehicleType type, long durationMinutes) const override {
        double hours = std::max(1.0, std::ceil(durationMinutes / 60.0)); // min 1 hr billed
        return hours * ratePerHour_.at(type);
    }
};

// ---------------------------------------------------------------------------
// Strategy #2: payment
// ---------------------------------------------------------------------------

class PaymentStrategy {
public:
    virtual ~PaymentStrategy() = default;
    virtual void pay(double amount) = 0;
};

class CashPayment : public PaymentStrategy {
public:
    void pay(double amount) override {
        std::cout << "[Payment] Paid $" << amount << " in cash.\n";
    }
};

class CardPayment : public PaymentStrategy {
public:
    void pay(double amount) override {
        std::cout << "[Payment] Paid $" << amount << " by card.\n";
    }
};

class UpiPayment : public PaymentStrategy {
public:
    void pay(double amount) override {
        std::cout << "[Payment] Paid $" << amount << " via UPI.\n";
    }
};

// ---------------------------------------------------------------------------
// ParkingSpot + Factory
// ---------------------------------------------------------------------------

class ParkingSpot {
    int spotId_;
    VehicleType type_;
    std::shared_ptr<Vehicle> vehicle_;

public:
    ParkingSpot(int id, VehicleType type) : spotId_(id), type_(type) {}

    bool isOccupied() const { return vehicle_ != nullptr; }
    VehicleType type() const { return type_; }
    int id() const { return spotId_; }
    std::shared_ptr<Vehicle> vehicle() const { return vehicle_; }

    void park(std::shared_ptr<Vehicle> v) {
        if (isOccupied()) throw std::runtime_error("Spot " + std::to_string(spotId_) + " already occupied");
        if (v->type != type_) throw std::runtime_error("Vehicle type does not match spot type");
        vehicle_ = std::move(v);
    }

    void vacate() { vehicle_.reset(); }
};

// Factory pattern: hides spot construction. Today it's a one-liner, but it's
// the single place to touch if a spot type ever needs extra setup
// (e.g. EV charging spots) without changing ParkingFloor/ParkingLot.
class ParkingSpotFactory {
public:
    static ParkingSpot create(int id, VehicleType type) {
        return ParkingSpot(id, type);
    }
};

// ---------------------------------------------------------------------------
// Observer: notify a display board when occupancy changes
// ---------------------------------------------------------------------------

class ParkingObserver {
public:
    virtual ~ParkingObserver() = default;
    virtual void onSpotStatusChanged(VehicleType type, int freeCount) = 0;
};

class DisplayBoard : public ParkingObserver {
public:
    void onSpotStatusChanged(VehicleType type, int freeCount) override {
        std::cout << "[DisplayBoard] " << toString(type) << " spots free: " << freeCount << "\n";
    }
};

// ---------------------------------------------------------------------------
// ParkingFloor - owns its own spots
// ---------------------------------------------------------------------------

class ParkingFloor {
    int floorNumber_;
    std::vector<ParkingSpot> spots_;

public:
    explicit ParkingFloor(int floorNumber) : floorNumber_(floorNumber) {}

    int floorNumber() const { return floorNumber_; }

    void addSpot(VehicleType type) {
        int id = floorNumber_ * 1000 + static_cast<int>(spots_.size()) + 1;
        spots_.push_back(ParkingSpotFactory::create(id, type));
    }

    ParkingSpot* findFreeSpot(VehicleType type) {
        for (auto& spot : spots_)
            if (!spot.isOccupied() && spot.type() == type) return &spot;
        return nullptr;
    }

    ParkingSpot* findSpotById(int spotId) {
        for (auto& spot : spots_)
            if (spot.id() == spotId) return &spot;
        return nullptr;
    }

    int countFree(VehicleType type) const {
        int count = 0;
        for (auto& spot : spots_)
            if (!spot.isOccupied() && spot.type() == type) ++count;
        return count;
    }

    std::vector<ParkingSpot>& spots() { return spots_; }
};

// ---------------------------------------------------------------------------
// Ticket
// ---------------------------------------------------------------------------

struct Ticket {
    int ticketId;
    int spotId;
    std::shared_ptr<Vehicle> vehicle;
    std::chrono::steady_clock::time_point entryTime;
};

// ---------------------------------------------------------------------------
// ParkingLot - Singleton, coordinates floors + tickets
// ---------------------------------------------------------------------------

class ParkingLot {
    std::vector<ParkingFloor> floors_;
    std::unordered_map<int, Ticket> activeTickets_;
    std::vector<std::shared_ptr<ParkingObserver>> observers_;
    std::unique_ptr<FeeStrategy> feeStrategy_;
    int nextTicketId_ = 1;

    ParkingLot() : feeStrategy_(std::make_unique<HourlyRateFeeStrategy>()) {}

    void notifyObservers(VehicleType type) {
        int free = 0;
        for (auto& floor : floors_) free += floor.countFree(type);
        for (auto& obs : observers_) obs->onSpotStatusChanged(type, free);
    }

public:
    ParkingLot(const ParkingLot&) = delete;
    ParkingLot& operator=(const ParkingLot&) = delete;

    static ParkingLot& instance() {
        static ParkingLot lot; // thread-safe in C++11+
        return lot;
    }

    ParkingFloor& addFloor() {
        floors_.emplace_back(static_cast<int>(floors_.size()) + 1);
        return floors_.back();
    }

    void addObserver(std::shared_ptr<ParkingObserver> obs) { observers_.push_back(std::move(obs)); }

    // Returns ticket id, or -1 if the lot is full for this vehicle type.
    int parkVehicle(std::shared_ptr<Vehicle> vehicle) {
        for (auto& floor : floors_) {
            ParkingSpot* spot = floor.findFreeSpot(vehicle->type);
            if (!spot) continue;

            spot->park(vehicle);
            int ticketId = nextTicketId_++;
            activeTickets_[ticketId] = Ticket{ticketId, spot->id(), vehicle, std::chrono::steady_clock::now()};

            std::cout << "[ParkingLot] " << toString(vehicle->type) << " " << vehicle->licensePlate
                      << " parked at floor " << floor.floorNumber() << ", spot " << spot->id()
                      << " (ticket #" << ticketId << ")\n";

            notifyObservers(vehicle->type);
            return ticketId;
        }
        std::cout << "[ParkingLot] No free spot for " << toString(vehicle->type) << "!\n";
        return -1;
    }

    void exitVehicle(int ticketId, PaymentStrategy& payment) {
        auto it = activeTickets_.find(ticketId);
        if (it == activeTickets_.end()) throw std::runtime_error("Invalid or already-closed ticket");

        Ticket& ticket = it->second;
        auto duration = std::chrono::duration_cast<std::chrono::minutes>(
            std::chrono::steady_clock::now() - ticket.entryTime).count();
        double fee = feeStrategy_->calculateFee(ticket.vehicle->type, duration);

        payment.pay(fee);

        for (auto& floor : floors_) {
            if (ParkingSpot* spot = floor.findSpotById(ticket.spotId)) {
                VehicleType type = spot->type();
                spot->vacate();
                notifyObservers(type);
                break;
            }
        }
        activeTickets_.erase(it);
    }
};

// ---------------------------------------------------------------------------
// Demo
// ---------------------------------------------------------------------------

int main() {
    ParkingLot& lot = ParkingLot::instance();
    lot.addObserver(std::make_shared<DisplayBoard>());

    // Floor 1: 2 car spots, 2 bike spots
    ParkingFloor& floor1 = lot.addFloor();
    floor1.addSpot(VehicleType::CAR);
    floor1.addSpot(VehicleType::CAR);
    floor1.addSpot(VehicleType::BIKE);
    floor1.addSpot(VehicleType::BIKE);

    // Floor 2: 1 truck spot, 1 car spot
    ParkingFloor& floor2 = lot.addFloor();
    floor2.addSpot(VehicleType::TRUCK);
    floor2.addSpot(VehicleType::CAR);

    auto car1 = std::make_shared<Vehicle>("CAR-101", VehicleType::CAR);
    auto bike1 = std::make_shared<Vehicle>("BIKE-202", VehicleType::BIKE);
    auto truck1 = std::make_shared<Vehicle>("TRUCK-303", VehicleType::TRUCK);

    int carTicket = lot.parkVehicle(car1);
    int bikeTicket = lot.parkVehicle(bike1);
    int truckTicket = lot.parkVehicle(truck1);

    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // simulate parked duration

    CardPayment card;
    UpiPayment upi;
    CashPayment cash;

    lot.exitVehicle(carTicket, card);
    lot.exitVehicle(bikeTicket, upi);
    lot.exitVehicle(truckTicket, cash);

    return 0;
}
