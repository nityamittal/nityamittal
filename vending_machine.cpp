#include<bits/stdc++.h>
using namespace std;

/*
vending machine - user journey:
1. machine is idle -> user selects an item code
2. item exists and is in stock -> machine waits for money
3. user inserts money (cash/card) -> once balance >= price, item can be dispensed
4. dispense -> item drops, change is returned, machine goes back to idle
5. user can cancel any time before dispensing -> full refund

designs used:
- State    : machine behaviour changes with its state (idle/awaiting payment/ready)
- Strategy : payment mode (cash, card, ... upi later)
- Singleton: one physical machine
extension points: new state (Maintenance), new payment mode, new item, multi-slot inventory
*/

class Item{
    public:
    string code;
    string name;
    double price;

    Item(string code = "", string name = "", double price = 0.0)
        : code(code), name(name), price(price) {}
};

//Strategy Design
class PaymentStrategy{
    public:
    virtual ~PaymentStrategy() = default;
    virtual void pay(double amount) = 0;
};

class PayByCash: public PaymentStrategy{
    public:
    void pay(double amount) override
    {
        cout<<"Inserted cash = $"<<amount<<endl;
    }
};

class PayByCard: public PaymentStrategy{
    public:
    void pay(double amount) override
    {
        cout<<"Swiped card for $"<<amount<<endl;
    }
};

class VendingMachine;

//State Design
class State{
    public:
    virtual ~State() = default;
    virtual string name() const = 0;

    //by default every action is illegal; each state allows only what it supports
    virtual void selectItem(VendingMachine& m, string code) { invalid("selectItem"); }
    virtual void insertMoney(VendingMachine& m, double amount, PaymentStrategy& p) { invalid("insertMoney"); }
    virtual void dispense(VendingMachine& m) { invalid("dispense"); }
    virtual void cancel(VendingMachine& m) { invalid("cancel"); }

    protected:
    void invalid(string action) { throw runtime_error("Cannot "+action+" right now!"); }
};

class IdleState: public State{
    public:
    string name() const override { return "IDLE"; }
    void selectItem(VendingMachine& m, string code) override;
};

class AwaitingPaymentState: public State{
    public:
    string name() const override { return "AWAITING_PAYMENT"; }
    void insertMoney(VendingMachine& m, double amount, PaymentStrategy& p) override;
    void cancel(VendingMachine& m) override;
};

class ReadyToDispenseState: public State{
    public:
    string name() const override { return "READY_TO_DISPENSE"; }
    void dispense(VendingMachine& m) override;
    void cancel(VendingMachine& m) override;
};

//Singleton Design
class VendingMachine{
    vector<Item> items;              //catalog
    map<string,int> stock;           //code -> quantity
    Item selected;                   //current selection
    double balance = 0.0;            //money inserted for current txn

    IdleState idle;
    AwaitingPaymentState awaiting;
    ReadyToDispenseState ready;
    State* state = &idle;

    VendingMachine() = default;

    public:

    static VendingMachine& instance()
    {
        static VendingMachine machine;
        return machine;
    }

    void addItem(Item item, int qty)
    {
        items.push_back(item);
        stock[item.code] += qty;
    }

    //---- user facing API, simply delegates to the current state ----
    void selectItem(string code) { state->selectItem(*this, code); }
    void insertMoney(double amount, PaymentStrategy& p) { state->insertMoney(*this, amount, p); }
    void dispense() { state->dispense(*this); }
    void cancel() { state->cancel(*this); }

    //---- helpers used by the states ----
    Item findItem(string code)
    {
        for(auto& item:items)
            if(item.code==code) return item;
        throw runtime_error("Invalid item code "+code);
    }

    int stockOf(string code) { return stock.count(code)?stock[code]:0; }
    void reduceStock(string code) { stock[code]--; }

    Item currentItem() { return selected; }
    void setCurrentItem(Item item) { selected = item; }

    double getBalance() { return balance; }
    void addBalance(double amount) { balance += amount; }

    double refund()
    {
        double amount = balance;
        balance = 0.0;
        if(amount>0) cout<<"Returned $"<<amount<<endl;
        return amount;
    }

    void reset()
    {
        selected = Item();
        balance = 0.0;
        state = &idle;
    }

    void toIdle() { state = &idle; }
    void toAwaiting() { state = &awaiting; }
    void toReady() { state = &ready; }

    string status() { return state->name(); }
};

void IdleState::selectItem(VendingMachine& m, string code)
{
    Item item = m.findItem(code);
    if(m.stockOf(code)<=0) throw runtime_error("Item "+item.name+" is out of stock!");

    m.setCurrentItem(item);
    cout<<"Selected "<<item.name<<", please pay $"<<item.price<<endl;
    m.toAwaiting();
}

void AwaitingPaymentState::insertMoney(VendingMachine& m, double amount, PaymentStrategy& p)
{
    if(amount<=0) throw runtime_error("Amount must be positive!");

    p.pay(amount);
    m.addBalance(amount);

    double price = m.currentItem().price;
    if(m.getBalance()>=price)
    {
        cout<<"Payment complete, press dispense.\n";
        m.toReady();
    }
    else
        cout<<"Still due = $"<<(price-m.getBalance())<<endl;
}

void AwaitingPaymentState::cancel(VendingMachine& m)
{
    cout<<"Transaction cancelled.\n";
    m.refund();
    m.reset();
}

void ReadyToDispenseState::dispense(VendingMachine& m)
{
    Item item = m.currentItem();
    m.reduceStock(item.code);

    double change = m.getBalance()-item.price;
    cout<<"Dispensing "<<item.name<<endl;
    if(change>0) cout<<"Change = $"<<change<<endl;

    m.reset();
}

void ReadyToDispenseState::cancel(VendingMachine& m)
{
    cout<<"Transaction cancelled.\n";
    m.refund();
    m.reset();
}

int main()
{
    VendingMachine& machine = VendingMachine::instance();

    machine.addItem(Item("A1", "Coke", 25.0), 2);
    machine.addItem(Item("A2", "Chips", 40.0), 1);

    PayByCash cash;
    PayByCard card;

    //happy path with change
    machine.selectItem("A1");
    machine.insertMoney(10.0, cash);
    machine.insertMoney(20.0, cash);
    machine.dispense();

    //cancel path
    machine.selectItem("A2");
    machine.insertMoney(15.0, card);
    machine.cancel();

    //illegal action for the current state
    try{
        machine.dispense();
    }catch(exception& e){
        cout<<"Error: "<<e.what()<<endl;
    }

    //out of stock / invalid code
    try{
        machine.selectItem("Z9");
    }catch(exception& e){
        cout<<"Error: "<<e.what()<<endl;
    }

    cout<<"Machine status = "<<machine.status()<<endl;

    return 0;
}
