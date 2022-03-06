#include <map>
#include <mutex>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <functional>
// #pragma warning: (disable: 4996)
using namespace std;

class OrderList;

class Order {
public:
	Order(int orderId, int price, int quantity) {
		this->orderId = orderId;
		this->price = price;
		this->quantity = quantity;
		this->next = nullptr;
	}

	virtual void MatchOrder(OrderList* orderList) = 0;

	void InsertOrder(OrderList* orderList);

	void WithdrawOrder(OrderList* orderList);

	int orderId;
	int price;
	int quantity;
	Order* next;
};

class AskOrder;
class BidOrder;

class AskOrder : public Order {
public:
	AskOrder(int orderId, int price, int quantity) : Order(orderId, price, quantity) {}
	
	void MatchOrder(OrderList* bidOrderList);
};

class BidOrder : public Order {
public:
	BidOrder(int orderId, int price, int quantity) : Order(orderId, price, quantity) {}

	void MatchOrder(OrderList* askOrderList);
};

class OrderList {
public:
	void PrintOrderList() const {
		for (auto iter = orderLinkList.rbegin(); iter != orderLinkList.rend(); iter++) {
			int price = iter->first;
			cout << price << ": ";
			Order* head = iter->second;
			while (head != nullptr) {
				cout << head->quantity << " ";
				head = head->next;
			}
			cout << endl;
		}
	}

	~OrderList() {
		/*
		TODO: delete LinkList here
		*/
	}

	map<int, Order*> orderLinkList;    // key: price, value: "XXXOrder"s' linklist
};

static class OrderBook {
public:
	static void MatchOrderWrapper(Order* order, OrderList* matchOrderList) {
		order->MatchOrder(matchOrderList);
	}

	static void InsertOrderWrapper(Order* order, OrderList* orderList) {
		order->InsertOrder(orderList);
	}

	static void WithdrawOrderWrapper(Order* order, OrderList* orderList) {
		order->WithdrawOrder(orderList);
	}

	static void PrintOrderListWrapper(const OrderList* askOrderList, const OrderList* bidOrderList) {
		cout << "\n==========\nASK\n";
		askOrderList->PrintOrderList();
		cout << "----------\n";
		bidOrderList->PrintOrderList();
		cout << "BID\n==========\n\n";
	}
};

class RegisterFactory {
public:
	RegisterFactory() {
		orderFuncMap["BA"] = &RegisterFactory::BAOrderFunc;
		orderFuncMap["BX"] = &RegisterFactory::BXOrderFunc;
		orderFuncMap["SA"] = &RegisterFactory::SAOrderFunc;
		orderFuncMap["SX"] = &RegisterFactory::SXOrderFunc;
	};

	void BAOrderFunc(int orderId, int price, int quantity) {
		BidOrder* bidOrder = new BidOrder(orderId, price, quantity);
		OrderBook::MatchOrderWrapper(bidOrder, &askOrderList);
		OrderBook::InsertOrderWrapper(bidOrder, &bidOrderList);
	}

	void BXOrderFunc(int orderId, int price, int quantity) {
		BidOrder* bidOrder = new BidOrder(orderId, price, quantity);
		OrderBook::WithdrawOrderWrapper(bidOrder, &bidOrderList);
	}

	void SAOrderFunc(int orderId, int price, int quantity) {
		AskOrder* askOrder = new AskOrder(orderId, price, quantity);
		OrderBook::MatchOrderWrapper(askOrder, &bidOrderList);
		OrderBook::InsertOrderWrapper(askOrder, &askOrderList);
	}

	void SXOrderFunc(int orderId, int price, int quantity) {
		AskOrder* askOrder = new AskOrder(orderId, price, quantity);
		OrderBook::WithdrawOrderWrapper(askOrder, &askOrderList);
	}

	void CreateOrder(string orderType, int orderId, int price, int quantity) {
		(this->*(orderFuncMap[orderType]))(orderId, price, quantity);
	}

	~RegisterFactory() {
		/*
		TODO: Delete all "XXXOrder"s here.
		*/
	}

	OrderList askOrderList;
	OrderList bidOrderList;

private:
	map<string, void (RegisterFactory::*)(int, int, int)> orderFuncMap;
};

void Order::InsertOrder(OrderList* orderList) {
	if (!this->quantity) return;
	auto& orderLinkList = orderList->orderLinkList;
	auto head = orderLinkList[this->price];
	if (head == nullptr) {
		orderLinkList[this->price] = this;
	}
	else {
		while (head->next) head = head->next;
		head->next = this;
	}
}

void Order::WithdrawOrder(OrderList* orderList) {
	auto& orderLinkList = orderList->orderLinkList;
	Order* head = orderLinkList[this->price];
	if (head == nullptr) {
		cout << "Withdraw failed      , orderId: " << this->orderId << "\n";
		orderLinkList.erase(this->price);
		return;
	}
	if (head->orderId == this->orderId) {
		cout << "Withdraw successfully, orderId: " << this->orderId << "\n";
		orderLinkList[this->price] = head->next;
		delete head;
		head = nullptr;
		if (orderLinkList[this->price] == nullptr) orderLinkList.erase(this->price);
		return;
	}
	while (head->next) {
		if (head->next->orderId != this->orderId) {
			head = head->next;
			continue;
		}
		cout << "Withdraw successfully, orderId: " << this->orderId << "\n";
		auto tmp = head->next;
		head->next = head->next->next;
		delete tmp;
		// head = nullptr;
		if (orderLinkList[this->price] == nullptr) orderLinkList.erase(this->price);
		return;
	}
	cout << "Withdraw failed      , orderId: " << this->orderId << "\n";
	return;
}

void AskOrder::MatchOrder(OrderList* bidOrderList) {
	vector<int> erasePrice;
	auto& bidOrderLinkList = bidOrderList->orderLinkList;
	for (auto iter = bidOrderLinkList.rbegin(); iter != bidOrderLinkList.rend(); iter++) {
		int bidPrice = iter->first;
		if (bidPrice < price || quantity == 0) break;
		Order* head = iter->second;
		while (head != nullptr && quantity) {
			int minQuantity = min(quantity, head->quantity);
			quantity -= minQuantity;
			head->quantity -= minQuantity;
			cout << "Trade happen         , orderId: " << orderId << ' ' << head->orderId << ", price: " << bidPrice << ", quantity: " << minQuantity << endl;
			if (head->quantity == 0) {
				Order* tmp = head;
				head = head->next;
				delete tmp;
			}
		}
		iter->second = head;
		if (iter->second == nullptr) erasePrice.push_back(bidPrice);
	}
	for (auto iter = erasePrice.begin(); iter != erasePrice.end(); iter++) {
		bidOrderLinkList.erase(*iter);
	}
}

void BidOrder::MatchOrder(OrderList* askOrderList) {
	vector<int> erasePrice;
	auto& askOrderLinkList = askOrderList->orderLinkList;
	if (askOrderLinkList.size() == 0) return;
	for (map<int, Order*>::iterator iter = askOrderLinkList.lower_bound(price); true; iter--) {
		if (iter == askOrderLinkList.end()) continue;
		if (quantity == 0) break;
		int askPrice = iter->first;
		if (askPrice > price) {
			if (iter == askOrderLinkList.begin()) break;
			continue;
		}
		Order* head = iter->second;
		while (head != nullptr && quantity) {
			int minQuantity = min(quantity, head->quantity);
			quantity -= minQuantity;
			head->quantity -= minQuantity;
			cout << "Trade happen         , orderId: " << head->orderId << ' ' << orderId << ", price: " << price << ", quantity: " << minQuantity << endl;
			if (head->quantity == 0) {
				Order* tmp = head;
				head = head->next;
				delete tmp;
			}
		}
		iter->second = head;
		if (iter->second == nullptr) erasePrice.push_back(askPrice);
		if (iter == askOrderLinkList.begin()) break;
	}
	for (auto iter = erasePrice.begin(); iter != erasePrice.end(); iter++) {
		askOrderLinkList.erase(*iter);
	}
}

void input() {
	ifstream in("in.txt");
	string orderRecord;
	if (!in.is_open()) {
		cout << "no file open\n";
		return;
	}
	RegisterFactory factory;
	while (getline(in, orderRecord))
	{
		int indexOfComma[4];
		for (int i = 0, cnt = 0; i < (int)orderRecord.size(); i++) {
			if (orderRecord[i] == ',') indexOfComma[cnt++] = i;
		}
		string orderType(""); 
		orderType += orderRecord[indexOfComma[1] + 1];
		orderType += orderRecord[0];
		int orderId = stoi(orderRecord.substr(indexOfComma[0] + 1, indexOfComma[1] - indexOfComma[0] - 1));
		int quantity = stoi(orderRecord.substr(indexOfComma[2] + 1, indexOfComma[3] - indexOfComma[2] - 1));
		int price = stoi(orderRecord.substr(indexOfComma[3] + 1));
		factory.CreateOrder(orderType, orderId, price, quantity);
	}
	OrderBook::PrintOrderListWrapper(&factory.askOrderList, &factory.bidOrderList);
}

int main()
{
	input();
	return 0;
}