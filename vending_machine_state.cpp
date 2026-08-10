#include<bits/stdc++.h>
using namespace std;

/*
vending machine - State pattern
NO_COIN --insertCoin--> HAS_COIN --selectItem--> DISPENSE --dispense--> NO_COIN / SOLD_OUT --refill--> NO_COIN
every action returns the NEXT state; machine only delegates, states hold the logic.
anything not allowed in a state returns the same state.
*/

class VendingMachine;

class State{
    public:
    virtual ~State() = default;
    virtual string name() = 0;
    virtual State* insertCoin(VendingMachine& m, int coin) { return no("insert coin"); }
    virtual State* selectItem(VendingMachine& m) { return no("select item"); }
    virtual State* dispense(VendingMachine& m) { return no("dispense"); }
    virtual State* returnCoin(VendingMachine& m) { return no("return coin"); }
    virtual State* refill(VendingMachine& m, int qty) { return no("refill"); }

    protected:
    State* no(string a) { cout<<"Cannot "<<a<<" in "<<name()<<endl; return this; }
};

//each state overrides ONLY the actions it allows
struct NoCoin: State{
    string name() override { return "NO_COIN"; }
    State* insertCoin(VendingMachine& m, int coin) override;
    State* refill(VendingMachine& m, int qty) override;
};
struct HasCoin: State{
    string name() override { return "HAS_COIN"; }
    State* insertCoin(VendingMachine& m, int coin) override;
    State* selectItem(VendingMachine& m) override;
    State* returnCoin(VendingMachine& m) override;
};
struct Dispensing: State{
    string name() override { return "DISPENSE"; }
    State* dispense(VendingMachine& m) override;
};
struct SoldOut: State{
    string name() override { return "SOLD_OUT"; }
    State* refill(VendingMachine& m, int qty) override;
};

//Context
class VendingMachine{
    State* state;
    public:
    int items, price, coins = 0;
    NoCoin noCoin;  HasCoin hasCoin;  Dispensing dispensing;  SoldOut soldOut;

    VendingMachine(int items, int price): items(items), price(price)
    {
        state = (items>0)?(State*)&noCoin:(State*)&soldOut;
    }

    void insertCoin(int c) { state = state->insertCoin(*this, c); }
    void selectItem()      { state = state->selectItem(*this); }
    void dispense()        { state = state->dispense(*this); }
    void returnCoin()      { state = state->returnCoin(*this); }
    void refill(int qty)   { state = state->refill(*this, qty); }

    void status() { cout<<"[items="<<items<<", coins="<<coins<<", state="<<state->name()<<"]\n"; }
};

State* NoCoin::insertCoin(VendingMachine& m, int coin)
{
    m.coins += coin;
    cout<<"Balance = "<<m.coins<<endl;
    return &m.hasCoin;
}
State* NoCoin::refill(VendingMachine& m, int qty) { m.items += qty; return this; }

State* HasCoin::insertCoin(VendingMachine& m, int coin)
{
    m.coins += coin;
    cout<<"Balance = "<<m.coins<<endl;
    return this;                                    //more coins, same state
}
State* HasCoin::selectItem(VendingMachine& m)
{
    if(m.coins<m.price)
    {
        cout<<"Need "<<(m.price-m.coins)<<" more\n";
        return this;
    }
    cout<<"Item selected\n";
    return &m.dispensing;
}
State* HasCoin::returnCoin(VendingMachine& m)
{
    cout<<"Returned "<<m.coins<<endl;
    m.coins = 0;
    return &m.noCoin;
}

State* Dispensing::dispense(VendingMachine& m)
{
    int change = m.coins-m.price;
    m.coins = 0;
    m.items--;
    cout<<"Dispensed, change = "<<change<<endl;
    return (m.items>0)?(State*)&m.noCoin:(State*)&m.soldOut;
}

State* SoldOut::refill(VendingMachine& m, int qty) { m.items += qty; return &m.noCoin; }

int main()
{
    VendingMachine m(2, 20);

    m.selectItem();                     //rejected, no coin yet
    m.insertCoin(10);
    m.selectItem();                     //rejected, not enough
    m.insertCoin(10);
    m.selectItem();
    m.dispense();
    m.status();                         //NO_COIN, 1 item left

    m.insertCoin(30);
    m.selectItem();
    m.dispense();                       //change = 10
    m.status();                         //SOLD_OUT

    m.insertCoin(20);                   //rejected
    m.refill(5);
    m.status();                         //NO_COIN
    return 0;
}
