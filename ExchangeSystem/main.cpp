#include <map>
#include <mutex>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <functional>
using namespace std;

template<typename T>
class Singleton
{
public:
	static T& GetInstance()
	{
		static T instance;
		return instance;
	}

	Singleton(T&&) = delete;
	Singleton(const T&) = delete;
	void operator= (const T&) = delete;

protected:
	Singleton() = default;
	virtual ~Singleton() = default;
};

template<typename T>    // T: "XXXOrder"
class OrderList {
public:
	void PrintOrderList() {
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

	map<int, T*> orderLinkList;    // key: price, value: "XXXOrder"s' linklist
};

class Order {
public:
	virtual void MatchOrder() = 0;

	template<typename T>    // T: "XXXOrder"
	void InsertOrder(T* order, OrderList<T>& orderList);

	template<typename T>
	void WithdrawOrder(T* order, OrderList<T>& orderList);

	int orderId;
	int price;
	int quantity;
};

class AskOrder : public Order {
public:
	AskOrder(int orderId, int price, int quantity) {
		this->orderId = orderId;
		this->price = price;
		this->quantity = quantity;
		this->next = nullptr;
	}
	
	void MatchOrder(); 

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

	void MatchOrder();

	BidOrder* next;
};

static class OrderBook {
public:
	static void MatchOrderWrapper(Order* order) {
		order->MatchOrder();
	}
	
	template<typename T>
	static void InsertOrderWrapper(T* order, OrderList<T>& orderList) {
		order->InsertOrder(order, orderList);
	}

	template<typename T>
	static void WithdrawOrderWrapper(T* order, OrderList<T>& orderList) {
		order->WithdrawOrder(order, orderList);
	}

	static void PrintOrderListWrapper() {
		OrderList<BidOrder>& bidOrderList = Singleton<OrderList<BidOrder>>::GetInstance();
		OrderList<AskOrder>& askOrderList = Singleton<OrderList<AskOrder>>::GetInstance();
		bidOrderList.PrintOrderList();
		askOrderList.PrintOrderList();
	}

// private:
//	OrderList<AskOrder> &askOrderList;
//	OrderList<BidOrder> &bidOrderList;
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
		OrderList<BidOrder>& bidOrderList = Singleton<OrderList<BidOrder>>::GetInstance();
		BidOrder* bidOrder = new BidOrder(orderId, price, quantity);
		OrderBook::MatchOrderWrapper(bidOrder);
		OrderBook::InsertOrderWrapper(bidOrder, bidOrderList);
	}

	void BXOrderFunc(int orderId, int price, int quantity) {
		OrderList<BidOrder>& bidOrderList = Singleton<OrderList<BidOrder>>::GetInstance();
		BidOrder* bidOrder = new BidOrder(orderId, price, quantity);
		OrderBook::WithdrawOrderWrapper(bidOrder, bidOrderList);
	}

	void SAOrderFunc(int orderId, int price, int quantity) {
		OrderList<AskOrder>& askOrderList = Singleton<OrderList<AskOrder>>::GetInstance();
		AskOrder* askOrder = new AskOrder(orderId, price, quantity);
		OrderBook::MatchOrderWrapper(askOrder);
		OrderBook::InsertOrderWrapper(askOrder, askOrderList);
	}

	void SXOrderFunc(int orderId, int price, int quantity) {
		OrderList<AskOrder>& askOrderList = Singleton<OrderList<AskOrder>>::GetInstance();
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

private:
	map<string, void (RegisterFactory::*)(int, int, int)> orderFuncMap;
};

template<typename T>
void Order::InsertOrder(T* order, OrderList<T>& orderList) {
	if (!order->quantity) return;
	auto head = orderList.orderLinkList[order->price];
	if (head == nullptr) {
		orderList.orderLinkList[order->price] = order;
	}
	else {
		while (head->next) head = head->next;
		head->next = order;
	}
}

template<typename T>
void Order::WithdrawOrder(T* order, OrderList<T>& orderList) {
	T* head = orderList.orderLinkList[order->price];
	if (head == nullptr) {
		cout << "Withdraw failed      , orderId: " << order->orderId << "\n";
		orderList.orderLinkList.erase(order->price);
		return;
	}
	if (head->orderId == order->orderId) {
		cout << "Withdraw successfully, orderId: " << order->orderId << ", price: " << order->price << ", quantity: " << head->quantity << endl;
		orderList.orderLinkList[order->price] = head->next;
		delete head;
		head = nullptr;
		if (orderList.orderLinkList[order->price] == nullptr) orderList.orderLinkList.erase(order->price);
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
		if (orderList.orderLinkList[order->price] == nullptr) orderList.orderLinkList.erase(order->price);
		return;
	}
	cout << "Withdraw failed      , orderId: " << order->orderId << "\n";
	return;
}

void AskOrder::MatchOrder()
{
	vector<int> erasePrice;
	OrderList<BidOrder>& bidOrderList = Singleton<OrderList<BidOrder>>::GetInstance();
	for (auto iter = bidOrderList.orderLinkList.rbegin(); iter != bidOrderList.orderLinkList.rend(); iter++) {
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
		bidOrderList.orderLinkList.erase(*iter);
	}
}

void BidOrder::MatchOrder() {
	vector<int> erasePrice;
	OrderList<AskOrder>& askOrderList = Singleton<OrderList<AskOrder>>::GetInstance();
	if (askOrderList.orderLinkList.size() == 0) return;
	for (map<int, AskOrder*>::iterator iter = askOrderList.orderLinkList.lower_bound(price); true; iter--) {
		if (iter == askOrderList.orderLinkList.end()) continue;
		if (quantity == 0) break;
		int askPrice = iter->first;
		if (askPrice > price) {
			if (iter == askOrderList.orderLinkList.begin()) break;
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
		if (iter == askOrderList.orderLinkList.begin()) break;
	}
	for (auto iter = erasePrice.begin(); iter != erasePrice.end(); iter++) {
		askOrderList.orderLinkList.erase(*iter);
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
		for (int i = 0, cnt = 0; i < orderRecord.size(); i++) {
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
	OrderBook::PrintOrderListWrapper();
}

int main()
{
	input();
	return 0;
}