#include "main.h"

using namespace std;

int main(int argc, char ** argv) {
    std::ios_base::sync_with_stdio(false);
    
    /// flag stuff
    Summary sum;
    map<string, Client> clients;
    map<string, Median> medians;
    unordered_map<string, int> tt;
    vector <tPair> ttvec;
    
    int opt = 0, index = 0;
    while((opt = getopt_long (argc, argv, "svmti:g:h", longOpts, &index)) != -1) {
        string insiderName = "INSIDER_";
        switch(opt) {
            case 's': {
                summary = 1;
                break;}
            case 'v': {
                verbose = 1;
                break;
            }
            case 'm': {
                median = 1;
                break;
            }
            case 't': {
                transfers = 1;
                break;
            }
            case 'i': {
                insider = 1;
                clients.insert(pair<string, Client>(insiderName + optarg, Client()));
                break;
            }
            case 'g': {
                ttt = 1;
                tPair t;
                t.equity = optarg;
                ttvec.push_back(t);
                tt.insert(pair<string, int>(optarg, ttvec.size() - 1));
                break;
            }
            case 'h': {
                
                // ----------------- CHANGE THIS -----------------------
                cout << "Flag -s enables summary output\nFlaf -v enables verbos"
                << "e output\nFlag -m enables medians output\nFlag t enables tr"
                << "ansfers output\nFlag -i followed by an equity name enables "
                << "an insider for that equity. There can be more than one\nFla"
                << "g -g followed by an equity name enables time travelers for "
                << "that equity\n" << endl;
                exit(0);
            }
        }
    }
    // FOR XCODE TESTING
    //ifstream arq(getenv("test6"));
    //cin.rdbuf(arq.rdbuf());
    
    string inputMode;
    bigInt lastTime = 0, ID = 0;
    Order order;
    
    cin >> inputMode;
    if (inputMode == "TL") {
        
        readAndCheckTL(order, lastTime, ID);
        
        while (cin.good()) {
            
            pushOrder(order, medians, clients);
            
            analyzeAndExecute(order, sum, medians, clients, tt, ttvec);
            
            readAndCheckTL(order, lastTime, ID);
        }
        
    }
    else if (inputMode == "PR") {
        
        string trash;
        char lastClient = 0, lastEquity = 0;
        double arrivalRate = 0;
        unsigned int seed = 0, numOrders = 0;
        
        cin >> trash >> seed >> trash >> numOrders >> trash >> lastClient
        >> trash >> lastEquity >> trash >> arrivalRate;
        
        bigInt timestampGen = 0;
        
        mt19937 gen(seed);
        uniform_int_distribution<char> client('a', lastClient);
        uniform_int_distribution<char> equity('A', lastEquity);
        exponential_distribution<> arrivals(arrivalRate);
        bernoulli_distribution buy_or_sell;
        uniform_int_distribution<> price(2, 11);
        uniform_int_distribution<> quantity(1, 30);
        
        while (ID < numOrders) {
            
            order.time = timestampGen;
            timestampGen += static_cast<int>(floor(arrivals(gen) + .5));
            order.name = string("C_") + client(gen);
            order.buy = (buy_or_sell(gen) ? 1 : 0);
            order.equity = string("E_") + equity(gen);
            order.price = 5 * price(gen);
            order.quantity = quantity(gen);
            order.ID = ID;
            ID++;
            
            pushOrder(order, medians, clients);
            
            analyzeAndExecute(order, sum, medians, clients, tt, ttvec);
            
        }
        
    }
    else exit(1);
    
    
    
    
    if (median) printMedians(medians);
    
    os << "---End of Day---\n";
    
    if (summary)
        os << sum;
    
    if (transfers)
        for (auto i: clients)
            os << i.first << i.second;
    
    if (ttt) {
        for (auto i: ttvec) {
            if ((i.timeS < 0) || (i.timeB < 0))  {
                i.timeS = -1;
                i.timeB = -1;
            }
            os << "Time travelers would buy " << i.equity << " at time: "
            << i.timeB << " and sell it at time: "
            << i.timeS << "\n";
        }
    }
    
    cout << os.str();
    
    return 0;
}

void analyzeAndExecute(Order &order, Summary &sum, map<string, Median> &medians,
                       map<string, Client> &clients, unordered_map<string, int> &tt,
                       vector<tPair> &ttvec) {
    
    buySellBooks &here = orderBook[order.equity];
    
    if (here.match()) {
        bigInt price = here.tradePrice();
        bigInt amount = here.tradeAmount();
        
        if (verbose) {
            os << here.b().name << " purchased " << amount
            << " shares of " << order.equity << " from "
            << here.s().name << " for $" << price
            << "/share\n";
        }
        
        if (transfers) {
            clients[here.b().name].buy(amount, price);
            clients[here.s().name].sell(amount, price);
        }
        
        if (median || (clients.find("INSIDER_" + order.equity) != clients.end())) {
            if (medians.find(order.equity) == medians.end())
                medians.insert(pair<string, Median>(order.equity, Median(price)));
            else
                medians[order.equity].updateMedian(price);
        }
        
        if (summary) {
            sum.commissionEarnings += (((price * amount) / 100) * 2);
            sum.totalMoney += (price * amount);
            sum.numTrades++;
            sum.numShares += amount;
        }
        
        here.updateOrders(amount);
        
        if (here.match())
            analyzeAndExecute(order, sum, medians, clients, tt, ttvec);
    }
    
    if (ttt && (tt.find(order.equity) != tt.end())) {
        
        tPair &travel = ttvec[tt[order.equity]];
        
        if (!order.buy) {
            if (travel.timeB == -1) {
                travel.timeB = order.time;
                travel.priceB = order.price;
            }
            else if (order.price < travel.priceB) {
                if (order.price < travel.newPriceB) {
                    if (order.time == travel.timeS) {
                        travel.timeB = order.time;
                        travel.priceB = order.price;
                    }
                    else {
                        travel.newPriceB = order.price;
                        travel.newTimeB = order.time;
                    }
                }
            }
        }
        else if ((order.buy) && (travel.timeB > -1)) {
            if (order.price > travel.priceS) {
                
                travel.priceS = order.price;
                travel.timeS = order.time;
                
                if ((order.price - travel.newPriceB) >
                    travel.bestDiff()) {
                    
                    travel.priceB = travel.newPriceB;
                    travel.timeB = travel.newTimeB;
                }
            }
            else if ((order.price - travel.newPriceB) > travel.bestDiff()) {
                
                travel.priceB = travel.newPriceB;
                travel.timeB = travel.newTimeB;
                travel.priceS = order.price;
                travel.timeS = order.time;
            }
            
        }
    }
    
    if (insider) {
        if ((clients.find("INSIDER_" + order.equity) != clients.end()) &&
            (medians.find(order.equity) != medians.end())) {
            
            if ((!here.sEmpt()) &&
                (enoughProfit(medians[order.equity].medVal(),
                              here.s().price, 1))) {
                Order o;
                o.time = currentTimeStamp;
                o.price = here.s().price;
                o.quantity = here.s().quantity;
                o.name = ("INSIDER_" + order.equity);
                o.equity = order.equity;
                o.buy = 1;
                
                here.push(o);
                analyzeAndExecute(o, sum, medians, clients, tt, ttvec);
            }
            
            if ((!here.bEmpt()) &&
                (enoughProfit(medians[order.equity].medVal()
                              , here.b().price, 0))) {
                Order o;
                o.time = currentTimeStamp;
                o.price = here.b().price;
                o.quantity = here.b().quantity;
                o.name = ("INSIDER_" + order.equity);
                o.equity = order.equity;
                o.buy = 0;
                
                here.push(o);
                analyzeAndExecute(o, sum, medians, clients, tt, ttvec);
            }
            
        }
    }
    
}

void pushOrder(Order &order, map<string, Median> &medians, map<string,
               Client> &clients){
    
    if (order.time != currentTimeStamp) {
        if (median)
            printMedians(medians);
        currentTimeStamp = order.time;
    }
    
    orderBook[order.equity].push(order);
    
    if (transfers)
        if (clients.find(order.name) == clients.end())
            clients.insert(pair<string, Client>(order.name, Client()));
    
    
    
}

void readAndCheckTL(Order &o, bigInt &lastTime, bigInt &ID) {
    
    
    char symbol;
    string buySell;
    
    // ----------------TimeStamp---------------------
    cin >> o.time;
    if (!cin.good()) {
        return;
    }
    if ((o.time < 0) || (o.time < lastTime))
        exit(1);
    else
        lastTime = o.time;
    
    // ----------------ClientName--------------------
    cin >> o.name;
    for (auto i : o.name)
        if (!isalnum(i))
            if (i != '_')
                exit(1);
    
    // ----------------BuySell------------------------
    cin >> buySell;
    if (buySell == "BUY")
        o.buy = 1;
    else if (buySell == "SELL")
        o.buy = 0;
    else
        exit(1);
    
    // ----------------EquitySymbol-------------------
    cin >> o.equity;
    if (o.equity.length() > 5)
        exit(1);
    for (auto i : o.equity)
        if (!isalnum(i))
            if ((i != '_') && (i != '.'))
                exit(1);
    
    // ----------------$$$$$$$$$$---------------------
    cin >> symbol;
    if (symbol != '$')
        exit(1);
    
    // ----------------Price--------------------------
    cin >> o.price;
    if (o.price < 1)
        exit(1);
    
    // ----------------##########---------------------
    cin >> symbol;
    if (symbol != '#')
        exit(1);
    
    // ----------------Quantity-----------------------
    cin >> o.quantity;
    
    o.ID = ID;
    ID++;
}

bool enoughProfit(bigInt med, bigInt val, bool buy) {
    
    double tenPercentOfMed = double(med) / 10;
    if (buy) return ((med - val) > tenPercentOfMed);
    else return ((val - med) > tenPercentOfMed);
    
}

void printMedians(map<string, Median> &medians) {
    for (auto i : medians) {
        os << "Median match price of " << i.first << " at time "
        << currentTimeStamp << " is $" << i.second.medVal() << "\n";
    }
}
