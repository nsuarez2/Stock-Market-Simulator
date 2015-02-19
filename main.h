#ifndef p3_main_h
#define p3_main_h

#include <iostream>
#include <random>
#include <string>
#include <sstream>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <queue>
#include <deque>
#include <unordered_map>
#include <map>

using namespace std;

typedef long long bigInt;

struct Order {
    bigInt price, quantity, time, ID;
    string name, equity;
    bool buy; // 1 means buy, 0 means sell
};

struct buyComp {
    bool operator() (const Order &a, const Order &b) const {
        if (a.price < b.price)
            return true;
        else if (a.price > b.price)
            return false;
        else
            return (a.ID > b.ID);
    }
};
struct sellComp {
    bool operator() (const Order &a, const Order &b) const {
        if (a.price > b.price)
            return true;
        else if (a.price < b.price)
            return false;
        else
            return (a.ID > b.ID);
    }
};

struct option longOpts[] = {
    {"summary", no_argument, NULL, 's'},
    {"verbose", no_argument, NULL, 'v'},
    {"median", no_argument, NULL, 'm'},
    {"transfers", no_argument, NULL, 't'},
    {"insider", required_argument, NULL, 'i'},
    {"ttt", required_argument, NULL, 'g'},
    {"help", no_argument, NULL, 'h'},
};

struct Summary {
    bigInt commissionEarnings = 0;
    bigInt totalMoney = 0;
    bigInt numTrades = 0;
    bigInt numShares = 0;
};
ostream& operator<< (ostream& os, const Summary& sum) {
    return os   << "Commission Earnings: $" << sum.commissionEarnings
    << "\nTotal Amount of Money Transferred: $" << sum.totalMoney
    << "\nNumber of Completed Trades: " << sum.numTrades
    << "\nNumber of Shares Traded: " << sum.numShares << '\n';
}

template<typename T , typename Container, typename Comp = buyComp>
class buyPQ : public priority_queue<T, Container, Comp> {
public:
    Container &owner() {
        return this->c;
    }
};

template<typename T, typename Container, typename Comp = sellComp>
class sellPQ : public priority_queue<T, Container, Comp> {
public:
    Container &owner() {
        return this->c;
    }
};

class buySellBooks{
private:
    buyPQ<Order*, deque<Order>> buying;
    sellPQ<Order*, deque<Order>> selling;
    
public:
    const Order b() { return (buying.top()); }
    const Order s() { return (selling.top()); }
    bool bEmpt() { return buying.empty(); }
    bool sEmpt() { return selling.empty(); }
    bigInt tradePrice() {
        if (buying.top().ID > selling.top().ID)
            return selling.top().price;
        else
            return buying.top().price;
    }
    bigInt tradeAmount() {
        if (buying.top().quantity > selling.top().quantity)
            return selling.top().quantity;
        else
            return buying.top().quantity;
    }
    
    
    void updateOrders(bigInt amount) {
        
        if (buying.top().quantity != amount)
            buying.owner().front().quantity -= amount;
        else {
            buying.pop();
        }
        
        if (selling.top().quantity != amount)
            selling.owner().front().quantity -= amount;
        else {
            selling.pop();
        }
        
    }
    bool match() {
        if (bEmpt() || sEmpt()) {
            return false;
        }
        return (buying.top().price >= selling.top().price);
    }
    void push(Order order) {
        if (order.buy)
            buying.push(order);
        else
            selling.push(order);
    }
};

unordered_map<string, buySellBooks> orderBook;

class Client {
private:
    bigInt numBought;
    bigInt numSold;
    bigInt netValue;
public:
    Client() : numBought(0), numSold(0), netValue(0) {}
    void buy(bigInt num, bigInt valueEach) {
        numBought += num;
        netValue -= (num * valueEach);
    }
    void sell(bigInt num, bigInt valueEach) {
        numSold += num;
        netValue += (num * valueEach);
    }
    friend ostream& operator<<(ostream &os, const Client &c);
};
ostream& operator<<(ostream &os, const Client &c) {
    return os << " bought " << c.numBought << " and sold " << c.numSold
    << " for a net transfer of $" << c.netValue << "\n";
}

class Median {
private:
    bigInt med;
    priority_queue<bigInt*, vector<bigInt>, less<bigInt>> lower;
    priority_queue<bigInt*, vector<bigInt>, greater<bigInt>> higher;
public:
    Median() : med() {}
    Median(bigInt val) : med(val) {}
    void updateMedian(bigInt val) {
        if (val > med) {
            higher.push(val);
            if (higher.size() > lower.size()) {
                lower.push(med);
                med = higher.top();
                higher.pop();
            }
        }
        else {
            lower.push(val);
            if (lower.size() > higher.size() + 1) {
                higher.push(med);
                med = lower.top();
                lower.pop();
            }
        }
    }
    bigInt medVal() {
        if (lower.size() > higher.size())
            return ((lower.top() + med) / 2);
        else
            return med;
    }
};


struct tPair {
    
    string equity;
    
    bigInt timeB = -1;
    bigInt timeS = -1;
    bigInt priceB = 9223372036854775806; // max for bigInt
    bigInt priceS = 0;
    
    bigInt newPriceB = 32767;
    bigInt newTimeB = -2;
    
    bigInt bestDiff() { return (priceS - priceB); }
};

ostringstream os;

bigInt currentTimeStamp = 0;
bool summary=0, verbose=0, median=0, transfers=0, insider=0, ttt = 0;

void readAndCheckTL(Order&, bigInt&, bigInt&);

void pushOrder(Order&, map<string, Median>&, map<string, Client>&);

void analyzeAndExecute(Order&, Summary&, map<string, Median>&,
                       map<string,Client>&, unordered_map<string, int>&,
                       vector<tPair>&);

bool enoughProfit(bigInt, bigInt, bool);

void printMedians(map<string, Median> &);

#endif
