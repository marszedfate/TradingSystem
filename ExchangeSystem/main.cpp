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

template<typename T>    // T: "XXXOrder"
class OrderList;

class Order {
public:
	template<typename T>    // T: "XXXOrder"
	void InsertOrder(T* order, OrderList<T>& orderList);

	template<typename T>
	void WithdrawOrder(T* order, OrderList<T>& orderList);

	int orderId;
	int price;
	int quantity;
};

class AskOrder;
class BidOrder;

class AskOrder : public Order {
public:
	AskOrder(int orderId, int price, int quantity) {
		this->orderId = orderId;
		this->price = price;
		this->quantity = quantity;
		this->next = nullptr;
	}
	
	void MatchOrder(OrderList<BidOrder>& bidOrderList);

	AskOrder* next;
};

class BidOrder : public Order {
public:
	BidOrder(int orderId, int price, int quantity) {
		this->orderId = orderId;
		this->price = price;
		this->quantity = quantity;
		this->next = nullptr;
	}

	void MatchOrder(OrderList<AskOrder>& askOrderList);

	BidOrder* next;
};

template<typename T>    // T: "XXXOrder"
class OrderList {
public:
	void PrintOrderList() const {
		cout << "==========\n";
		cout << typeid(T).name() << ":\n";
		for (auto iter = orderLinkList.rbegin(); iter != orderLinkList.rend(); iter++) {
			int price = iter->first;
			cout << price << ": ";
			T* head = iter->second;
			while (head != nullptr) {
				cout << head->quantity << " ";
				head = head->next;
			}
			cout << endl;
		}
		cout << "==========\n";
	}

	~OrderList() {
		/*
		TODO: delete LinkList here
		*/
	}

	map<int, T*> orderLinkList;    // key: price, value: "XXXOrder"s' linklist
};

static class OrderBook {
public:
	template<typename T1, typename T2>
	static void MatchOrderWrapper(T1* order, OrderList<T2>& matchOrderList) {    // (order: askOrder) -> (matchOrderList: bidOrderList)
		order->MatchOrder(matchOrderList);
	}
	
	template<typename T>
	static void InsertOrderWrapper(T* order, OrderList<T>& orderList) {
		order->InsertOrder(order, orderList);
	}

	template<typename T>
	static void WithdrawOrderWrapper(T* order, OrderList<T>& orderList) {
		order->WithdrawOrder(order, orderList);
	}

	static void PrintOrderListWrapper(const OrderList<AskOrder>& askOrderList, const OrderList<BidOrder>& bidOrderList) {
		askOrderList.PrintOrderList();
		bidOrderList.PrintOrderList();
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
		OrderBook::MatchOrderWrapper(bidOrder, askOrderList);
		OrderBook::InsertOrderWrapper(bidOrder, bidOrderList);
	}

	void BXOrderFunc(int orderId, int price, int quantity) {
		BidOrder* bidOrder = new BidOrder(orderId, price, quantity);
		OrderBook::WithdrawOrderWrapper(bidOrder, bidOrderList);
	}

	void SAOrderFunc(int orderId, int price, int quantity) {
		AskOrder* askOrder = new AskOrder(orderId, price, quantity);
		OrderBook::MatchOrderWrapper(askOrder, bidOrderList);
		OrderBook::InsertOrderWrapper(askOrder, askOrderList);
	}

	void SXOrderFunc(int orderId, int price, int quantity) {
		AskOrder* askOrder = new AskOrder(orderId, price, quantity);
		OrderBook::WithdrawOrderWrapper(askOrder, askOrderList);
	}

	void CreateOrder(string orderType, int orderId, int price, int quantity) {
		(this->*(orderFuncMap[orderType]))(orderId, price, quantity);
	}

	~RegisterFactory() {
		/*
		TODO: Delete all "XXXOrder"s here.
		*/
	}

	OrderList<AskOrder> askOrderList;
	OrderList<BidOrder> bidOrderList;

private:
	map<string, void (RegisterFactory::*)(int, int, int)> orderFuncMap;
};

template<typename T>
void Order::InsertOrder(T* order, OrderList<T>& orderList) {
	if (!order->quantity) return;
	auto& orderLinkList = orderList.orderLinkList;
	auto head = orderLinkList[order->price];
	if (head == nullptr) {
		orderLinkList[order->price] = order;
	}
	else {
		while (head->next) head = head->next;
		head->next = order;
	}
}

template<typename T>
void Order::WithdrawOrder(T* order, OrderList<T>& orderList) {
	auto& orderLinkList = orderList.orderLinkList;
	T* head = orderLinkList[order->price];
	if (head == nullptr) {
		cout << "Withdraw failed      , orderId: " << order->orderId << "\n";
		orderLinkList.erase(order->price);
		return;
	}
	if (head->orderId == order->orderId) {
		cout << "Withdraw successfully, orderId: " << order->orderId << ", price: " << order->price << ", quantity: " << head->quantity << endl;
		orderLinkList[order->price] = head->next;
		delete head;
		head = nullptr;
		if (orderLinkList[order->price] == nullptr) orderLinkList.erase(order->price);
		return;
	}
	while (head->next) {
		if (head->next->orderId != order->orderId) {
			head = head->next;
			continue;
		}
		cout << "Withdraw successfully, orderId: " << order->orderId << ", price: " << order->price << ", quantity: " << head->next->quantity << endl;
		auto tmp = head->next;
		head->next = head->next->next;
		delete tmp;
		// head = nullptr;
		if (orderLinkList[order->price] == nullptr) orderLinkList.erase(order->price);
		return;
	}
	cout << "Withdraw failed      , orderId: " << order->orderId << "\n";
	return;
}

void AskOrder::MatchOrder(OrderList<BidOrder>& bidOrderList) {
	vector<int> erasePrice;
	auto& bidOrderLinkList = bidOrderList.orderLinkList;
	for (auto iter = bidOrderLinkList.rbegin(); iter != bidOrderLinkList.rend(); iter++) {
		int bidPrice = iter->first;
		if (bidPrice < price || quantity == 0) break;
		BidOrder* head = iter->second;
		while (head != nullptr && quantity) {
			int minQuantity = min(quantity, head->quantity);
			quantity -= minQuantity;
			head->quantity -= minQuantity;
			cout << "Trade happen         , orderId: " << orderId << ' ' << head->orderId << ", price: " << bidPrice << ", quantity: " << minQuantity << endl;
			if (head->quantity == 0) {
				BidOrder* tmp = head;
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

void BidOrder::MatchOrder(OrderList<AskOrder>& askOrderList) {
	vector<int> erasePrice;
	auto& askOrderLinkList = askOrderList.orderLinkList;
	if (askOrderLinkList.size() == 0) return;
	for (map<int, AskOrder*>::iterator iter = askOrderLinkList.lower_bound(price); true; iter--) {
		if (iter == askOrderLinkList.end()) continue;
		if (quantity == 0) break;
		int askPrice = iter->first;
		if (askPrice > price) {
			if (iter == askOrderLinkList.begin()) break;
			continue;
		}
		AskOrder* head = iter->second;
		while (head != nullptr && quantity) {
			int minQuantity = min(quantity, head->quantity);
			quantity -= minQuantity;
			head->quantity -= minQuantity;
			cout << "Trade happen         , orderId: " << orderId << ' ' << head->orderId << ", price: " << price << ", quantity: " << minQuantity << endl;
			if (head->quantity == 0) {
				AskOrder* tmp = head;
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
	OrderBook::PrintOrderListWrapper(factory.askOrderList, factory.bidOrderList);
}

int main()
{
	input();
	return 0;
}