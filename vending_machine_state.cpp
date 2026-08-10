#include<bits/stdc++.h>
using namespace std;

/*
vending machine - State design pattern

states : NO_COIN -> HAS_COIN -> DISPENSE -> (NO_COIN | SOLD_OUT), SOLD_OUT --refill--> NO_COIN
actions: insertCoin, selectItem, dispense, returnCoin, refill

rule: every action RETURNS the next state, machine just stores whatever it gets back.
      an action that is not allowed in a state returns the same state (with a message).
      machine is a dumb object - it only delegates, states hold all the logic.
*/

class VendingMachine;

//State
class VendingState{
    public:
    virtual ~VendingState() = default;
    virtual string name() = 0;

    //by default an action is not allowed -> say why and stay in the same state
    virtual VendingState* insertCoin(VendingMachine& m, int coin) { return reject("insert coin"); }
    virtual VendingState* selectItem(VendingMachine& m) { return reject("select item"); }
    virtual VendingState* dispense(VendingMachine& m) { return reject("dispense"); }
    virtual VendingState* returnCoin(VendingMachine& m) { return reject("return coin"); }
    virtual VendingState* refill(VendingMachine& m, int qty) { return reject("refill"); }

    protected:
    VendingState* reject(string action)
    {
        cout<<"Cannot "<<action<<" in "<<name()<<" state\n";
        return this;
    }
};

//each concrete state overrides ONLY the actions it supports
class NoCoinState: public VendingState{
    public:
    string name() override { return "NO_COIN"; }
    VendingState* insertCoin(VendingMachine& m, int coin) override;
    VendingState* refill(VendingMachine& m, int qty) override;
};

class HasCoinState: public VendingState{
    public:
    string name() override { return "HAS_COIN"; }
    VendingState* insertCoin(VendingMachine& m, int coin) override;
    VendingState* selectItem(VendingMachine& m) override;
    VendingState* returnCoin(VendingMachine& m) override;
};

class DispenseState: public VendingState{
    public:
    string name() override { return "DISPENSE"; }
    VendingState* dispense(VendingMachine& m) override;
};

class SoldOutState: public VendingState{
    public:
    string name() override { return "SOLD_OUT"; }
    VendingState* refill(VendingMachine& m, int qty) override;
};

//Context
class VendingMachine{
    VendingState* state;
    int items;
    int price;
    int coins = 0;

    public:
    //one object per state, created once and reused
    NoCoinState noCoin;
    HasCoinState hasCoin;
    DispenseState dispensing;
    SoldOutState soldOut;

    VendingMachine(int items, int price): items(items), price(price)
    {
        state = (items>0)?(VendingState*)&noCoin:(VendingState*)&soldOut;
    }

    //---- delegate everything to the current state, store the returned state ----
    void insertCoin(int coin) { state = state->insertCoin(*this, coin); }
    void selectItem() { state = state->selectItem(*this); }
    void dispense() { state = state->dispense(*this); }
    void returnCoin() { state = state->returnCoin(*this); }
    void refill(int qty) { state = state->refill(*this, qty); }

    //---- helpers the states operate on ----
    int getPrice() { return price; }
    int getCoins() { return coins; }
    void addCoins(int c) { coins += c; }
    void clearCoins() { coins = 0; }
    int getItems() { return items; }
    void addItems(int n) { items += n; }
    void removeItem() { items--; }

    void status() { cout<<"   [items="<<items<<", coins=$"<<coins<<", state="<<state->name()<<"]\n"; }
};

VendingState* NoCoinState::insertCoin(VendingMachine& m, int coin)
{
    m.addCoins(coin);
    cout<<"Coin inserted, balance = $"<<m.getCoins()<<endl;
    return &m.hasCoin;
}

VendingState* NoCoinState::refill(VendingMachine& m, int qty)
{
    m.addItems(qty);
    cout<<"Refilled, items = "<<m.getItems()<<endl;
    return this;                        //machine is still idle
}

VendingState* HasCoinState::insertCoin(VendingMachine& m, int coin)
{
    m.addCoins(coin);
    cout<<"Coin inserted, balance = $"<<m.getCoins()<<endl;
    return this;                        //more coins are fine, no state change
}

VendingState* HasCoinState::selectItem(VendingMachine& m)
{
    if(m.getCoins()<m.getPrice())
    {
        cout<<"Insufficient funds, need $"<<(m.getPrice()-m.getCoins())<<" more\n";
        return this;
    }
    cout<<"Item selected\n";
    return &m.dispensing;
}

VendingState* HasCoinState::returnCoin(VendingMachine& m)
{
    cout<<"Returned $"<<m.getCoins()<<endl;
    m.clearCoins();
    return &m.noCoin;
}

VendingState* DispenseState::dispense(VendingMachine& m)
{
    int change = m.getCoins()-m.getPrice();
    m.clearCoins();
    m.removeItem();

    cout<<"Item dispensed";
    if(change>0) cout<<", change = $"<<change;
    cout<<endl;

    return (m.getItems()>0)?(VendingState*)&m.noCoin:(VendingState*)&m.soldOut;
}

VendingState* SoldOutState::refill(VendingMachine& m, int qty)
{
    m.addItems(qty);
    cout<<"Refilled, items = "<<m.getItems()<<endl;
    return &m.noCoin;
}

int main()
{
    VendingMachine machine(2, 20);      //2 water bottles, $20 each
    machine.status();

    cout<<"\n1. select item without any coin\n";
    machine.selectItem();               //not allowed, stays in NO_COIN

    cout<<"\n2. insert $10 and try to select\n";
    machine.insertCoin(10);
    machine.selectItem();               //insufficient, stays in HAS_COIN

    cout<<"\n3. insert $10 more, select and dispense\n";
    machine.insertCoin(10);
    machine.selectItem();
    machine.dispense();
    machine.status();                   //back to NO_COIN, 1 item left

    cout<<"\n4. buy the last item with $30 (change expected)\n";
    machine.insertCoin(30);
    machine.selectItem();
    machine.dispense();
    machine.status();                   //SOLD_OUT now

    cout<<"\n5. try to use a sold out machine, then refill\n";
    machine.insertCoin(20);             //not allowed
    machine.refill(5);
    machine.status();                   //NO_COIN again

    cout<<"\n6. insert a coin and take it back\n";
    machine.insertCoin(20);
    machine.returnCoin();
    machine.status();

    return 0;
}
