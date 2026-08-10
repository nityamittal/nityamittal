#include<bits/stdc++.h>
using namespace std;

/*
vending machine - State pattern
NO_COIN --insertCoin--> HAS_COIN --selectItem--> DISPENSE --dispense--> NO_COIN / SOLD_OUT --refill--> NO_COIN

every action returns the id of the NEXT state; machine looks that id up and switches.
an action not allowed in a state returns its own id, so nothing changes.
*/

enum StateId { NO_COIN, HAS_COIN, DISPENSE, SOLD_OUT };

//all the data a state works on
class Stock{
    public:
    int items = 0, price = 0, coins = 0;
};

class State{
    public:
    virtual ~State() = default;
    virtual StateId id() = 0;

    //by default an action is not allowed -> say so and stay in the same state
    virtual StateId insertCoin(Stock& s, int coin) { return no("insert coin"); }
    virtual StateId selectItem(Stock& s) { return no("select item"); }
    virtual StateId dispense(Stock& s) { return no("dispense"); }
    virtual StateId returnCoin(Stock& s) { return no("return coin"); }
    virtual StateId refill(Stock& s, int qty) { return no("refill"); }

    string name() { string n[] = {"NO_COIN","HAS_COIN","DISPENSE","SOLD_OUT"}; return n[id()]; }

    protected:
    StateId no(string a) { cout<<"Cannot "<<a<<" in "<<name()<<endl; return id(); }
};

//each state overrides ONLY the actions it allows
class NoCoin: public State{
    public:
    StateId id() override { return NO_COIN; }
    StateId insertCoin(Stock& s, int coin) override { s.coins += coin; cout<<"Balance = "<<s.coins<<endl; return HAS_COIN; }
    StateId refill(Stock& s, int qty) override { s.items += qty; return NO_COIN; }
};

class HasCoin: public State{
    public:
    StateId id() override { return HAS_COIN; }
    StateId insertCoin(Stock& s, int coin) override { s.coins += coin; cout<<"Balance = "<<s.coins<<endl; return HAS_COIN; }
    StateId returnCoin(Stock& s) override { cout<<"Returned "<<s.coins<<endl; s.coins = 0; return NO_COIN; }

    StateId selectItem(Stock& s) override
    {
        if(s.coins<s.price)
        {
            cout<<"Need "<<(s.price-s.coins)<<" more\n";
            return HAS_COIN;
        }
        cout<<"Item selected\n";
        return DISPENSE;
    }
};

class Dispensing: public State{
    public:
    StateId id() override { return DISPENSE; }

    StateId dispense(Stock& s) override
    {
        cout<<"Dispensed, change = "<<(s.coins-s.price)<<endl;
        s.coins = 0;
        s.items--;
        return (s.items>0)?NO_COIN:SOLD_OUT;
    }
};

class SoldOut: public State{
    public:
    StateId id() override { return SOLD_OUT; }
    StateId refill(Stock& s, int qty) override { s.items += qty; return NO_COIN; }
};

//Context - owns one object per state and only delegates
class VendingMachine{
    Stock stock;
    NoCoin noCoin;  HasCoin hasCoin;  Dispensing dispensing;  SoldOut soldOut;
    State* all[4];
    State* cur;

    public:
    VendingMachine(int items, int price)
    {
        stock.items = items;
        stock.price = price;
        all[NO_COIN] = &noCoin;  all[HAS_COIN] = &hasCoin;
        all[DISPENSE] = &dispensing;  all[SOLD_OUT] = &soldOut;
        cur = all[(items>0)?NO_COIN:SOLD_OUT];
    }

    void insertCoin(int c) { cur = all[cur->insertCoin(stock, c)]; }
    void selectItem()      { cur = all[cur->selectItem(stock)]; }
    void dispense()        { cur = all[cur->dispense(stock)]; }
    void returnCoin()      { cur = all[cur->returnCoin(stock)]; }
    void refill(int qty)   { cur = all[cur->refill(stock, qty)]; }

    void status() { cout<<"[items="<<stock.items<<", coins="<<stock.coins<<", state="<<cur->name()<<"]\n"; }
};

int main()
{
    VendingMachine m(2, 20);

    m.selectItem();         //rejected, no coin yet
    m.insertCoin(10);
    m.selectItem();         //rejected, not enough
    m.insertCoin(10);
    m.selectItem();
    m.dispense();
    m.status();             //NO_COIN, 1 item left

    m.insertCoin(30);
    m.selectItem();
    m.dispense();           //change = 10
    m.status();             //SOLD_OUT

    m.insertCoin(20);       //rejected
    m.refill(5);
    m.status();             //NO_COIN
    return 0;
}
