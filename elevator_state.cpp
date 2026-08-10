#include<bits/stdc++.h>
using namespace std;
/*
elevator - State pattern
IDLE --request--> UP/DOWN --step--> DOORS_OPEN --close--> UP/DOWN/IDLE, IDLE --service--> MAINTENANCE --service--> IDLE
every action returns the id of the NEXT state; the elevator looks that id up and switches.
an action not allowed in a state returns its own id, so nothing changes.
*/
enum StateId { IDLE, UP, DOWN, DOORS_OPEN, MAINTENANCE };
//all the data a state works on
class Cabin{
    public:
    int floor = 0, lo = 0, hi = 0;
    set<int> stops;              //floors still to serve, sorted
};
class State{
    public:
    virtual ~State() = default;
    virtual StateId id() = 0;
    //by default an action is not allowed -> say so and stay in the same state
    virtual StateId request(Cabin& c, int f)  { return no("request floor"); }
    virtual StateId step(Cabin& c)            { return no("step"); }
    virtual StateId closeDoors(Cabin& c)      { return no("close doors"); }
    virtual StateId service(Cabin& c, bool on){ return no("change service mode"); }
    string name() { string n[] = {"IDLE","UP","DOWN","DOORS_OPEN","MAINTENANCE"}; return n[id()]; }
    protected:
    StateId no(string a) { cout<<"Cannot "<<a<<" in "<<name()<<endl; return id(); }
    //queue a floor; ignore junk and the floor we are already on
    void add(Cabin& c, int f)
    {
        if(f<c.lo || f>c.hi) { cout<<"Floor "<<f<<" does not exist\n"; return; }
        if(f==c.floor)       { cout<<"Already at "<<f<<endl; return; }
        c.stops.insert(f);
        cout<<"Queued floor "<<f<<endl;
    }
    //nothing left -> IDLE, else go where the remaining stops are
    StateId next(Cabin& c)
    {
        if(c.stops.empty()) return IDLE;
        return (c.stops.upper_bound(c.floor)!=c.stops.end())?UP:DOWN;
    }
    //one floor of travel in direction d (+1 / -1); returns the state after that move
    StateId move(Cabin& c, int d)
    {
        c.floor += d;
        cout<<"At floor "<<c.floor<<endl;
        if(c.stops.count(c.floor)) { c.stops.erase(c.floor); cout<<"Doors open\n"; return DOORS_OPEN; }
        return next(c);
    }
};
//each state overrides ONLY the actions it allows
class Idle: public State{
    public:
    StateId id() override { return IDLE; }
    StateId request(Cabin& c, int f) override { add(c, f); return next(c); }
    StateId service(Cabin& c, bool on) override { return on?MAINTENANCE:IDLE; }
};
class MovingUp: public State{
    public:
    StateId id() override { return UP; }
    StateId request(Cabin& c, int f) override { add(c, f); return UP; }
    //keep going up while anything is above us - direction preference comes from being in this class
    StateId step(Cabin& c) override { return move(c, +1); }
};
class MovingDown: public State{
    public:
    StateId id() override { return DOWN; }
    StateId request(Cabin& c, int f) override { add(c, f); return DOWN; }
    StateId step(Cabin& c) override { return move(c, -1); }
};
class DoorsOpen: public State{
    public:
    StateId id() override { return DOORS_OPEN; }
    StateId request(Cabin& c, int f) override { add(c, f); return DOORS_OPEN; }
    StateId closeDoors(Cabin& c) override { cout<<"Doors close\n"; return next(c); }
};
class Maintenance: public State{
    public:
    StateId id() override { return MAINTENANCE; }
    StateId service(Cabin& c, bool on) override { if(!on) c.stops.clear(); return on?MAINTENANCE:IDLE; }
};
//Context - owns one object per state and only delegates
class Elevator{
    Cabin cabin;
    Idle idle;  MovingUp up;  MovingDown down;  DoorsOpen doorsOpen;  Maintenance maintenance;
    State* all[5];
    State* cur;
    public:
    Elevator(int start, int lo, int hi)
    {
        cabin.floor = start;
        cabin.lo = lo;
        cabin.hi = hi;
        all[IDLE] = &idle;  all[UP] = &up;  all[DOWN] = &down;
        all[DOORS_OPEN] = &doorsOpen;  all[MAINTENANCE] = &maintenance;
        cur = all[IDLE];
    }
    void request(int f)   { cur = all[cur->request(cabin, f)]; }
    void step()           { cur = all[cur->step(cabin)]; }
    void closeDoors()     { cur = all[cur->closeDoors(cabin)]; }
    void service(bool on) { cur = all[cur->service(cabin, on)]; }
    void status() { cout<<"[floor="<<cabin.floor<<", stops="<<cabin.stops.size()<<", state="<<cur->name()<<"]\n"; }
};
int main()
{
    Elevator e(0, 0, 5);
    e.step();               //rejected, nothing to do yet
    e.request(9);           //rejected, no such floor
    e.request(3);
    e.step();
    e.request(2);           //pressed while moving - picked up on the way
    e.step();
    e.status();             //floor 2, doors open
    e.closeDoors();         //still one stop above -> keeps going up
    e.step();
    e.closeDoors();
    e.status();             //floor 3, IDLE
    e.request(1);
    e.step();
    e.step();
    e.closeDoors();
    e.status();             //floor 1, IDLE
    e.service(true);
    e.request(5);           //rejected, under maintenance
    e.service(false);
    e.status();             //IDLE again
    return 0;
}
