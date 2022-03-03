#include <map>
#include <mutex>
#include <vector>
#include <iostream>
#include <memory>
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

template<typename T>
class OrderList {
public:
	map<int, T*> orderLinkList;    // key: price, value: orders' linklist
};

class Order {
public:
	virtual void MatchOrder() = 0;

	template<typename T>
	void RegisterOrder(T* order, OrderList<T>& orderList);

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

class OrderBook {
public:
	OrderBook() = default;

	static void MatchOrderWrapper(Order* order) {
		order->MatchOrder();
	}
	
	template<typename T>
	static void RegisterOrderWrapper(T* order, OrderList<T>& orderList) {
		order->RegisterOrder(order, orderList);
	}

	template<typename T>
	static void WithdrawOrderWrapper(T* order, OrderList<T>& orderList) {
		order->WithdrawOrder(order, orderList);
	}

// private:
//	OrderList<AskOrder> &askOrderList;
//	OrderList<BidOrder> &bidOrderList;
};

class RegisterFactory {
public:
	RegisterFactory() {
		orderFuncMap["BA"] = this->BAOrderFunc;
		orderFuncMap["BX"] = this->BXOrderFunc;
		orderFuncMap["SA"] = this->SAOrderFunc;
		orderFuncMap["SX"] = this->SXOrderFunc;
	};

	void BAOrderFunc(int orderId, int price, int quantity) {
		OrderList<BidOrder>& bidOrderList = Singleton<OrderList<BidOrder>>::GetInstance();
		BidOrder* bidOrder = new BidOrder(orderId, price, quantity);
		OrderBook::MatchOrderWrapper(bidOrder);
		OrderBook::RegisterOrderWrapper(bidOrder, bidOrderList);
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
		OrderBook::RegisterOrderWrapper(askOrder, askOrderList);
	}

	void SXOrderFunc(int orderId, int price, int quantity) {
		OrderList<AskOrder>& askOrderList = Singleton<OrderList<AskOrder>>::GetInstance();
		AskOrder* askOrder = new AskOrder(orderId, price, quantity);
		OrderBook::WithdrawOrderWrapper(askOrder, askOrderList);
	}

	void CreateOrder(string orderType, int orderId, int price, int quantity) {
		(*(orderFuncMap[orderType]))(this, orderId, price, quantity);
	}

private:
	std::map<string, void (*)(RegisterFactory*, int, int, int)> orderFuncMap;
};

template<typename T>
void Order::RegisterOrder(T* order, OrderList<T>& orderList) {
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
		cout << "OrderId: " << order->orderId << " doesn't exist\n";
		return;
	}
	if (head->orderId == order->orderId) {
		cout << "OrderId: " << order->orderId << " successfully withdraw, price: " << order->price << " ,quantity: " << head->quantity << endl;
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
		cout << "OrderId: " << order->orderId << " successfully withdraw, price: " << order->price << " ,quantity: " << head->next->quantity << endl;
		auto tmp = head->next;
		head->next = head->next->next;
		delete tmp;
		// head = nullptr;
		if (orderList.orderLinkList[order->price] == nullptr) orderList.orderLinkList.erase(order->price);
		return;
	}
	cout << "OrderId: " << order->orderId << " doesn't exist\n";
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
	for (map<int, AskOrder*>::iterator iter = askOrderList.orderLinkList.lower_bound(price); iter != askOrderList.orderLinkList.end(); iter--) {
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

int main()
{
	OrderList<BidOrder>& bidOrderList = Singleton<OrderList<BidOrder>>::GetInstance();
	OrderList<AskOrder>& askOrderList = Singleton<OrderList<AskOrder>>::GetInstance();
	/*
	BidOrder* bid1 = new BidOrder(1, 2, 2);
	BidOrder* bid2 = new BidOrder(2, 2, 3);
	OrderBook::RegisterOrderWrapper(bid1, bidOrderList);
	OrderBook::RegisterOrderWrapper(bid2, bidOrderList);

	// BidOrder* bid3 = new BidOrder(2, 3, 5);
	// OrderBook::WithdrawOrderWrapper(bid3, bidOrderList);

	AskOrder* bid4 = new AskOrder(3, 1, 6);
	OrderBook::MatchOrderWrapper(bid4);
	*/

	AskOrder* bid1 = new AskOrder(1, 2, 2);
	AskOrder* bid2 = new AskOrder(2, 4, 3);
	OrderBook::RegisterOrderWrapper(bid1, askOrderList);
	OrderBook::RegisterOrderWrapper(bid2, askOrderList);

	// BidOrder* bid3 = new BidOrder(2, 3, 5);
	// OrderBook::WithdrawOrderWrapper(bid3, bidOrderList);

	BidOrder* bid4 = new BidOrder(3, 3, 6);
	OrderBook::MatchOrderWrapper(bid4);

	for (auto iter = bidOrderList.orderLinkList.begin(); iter != bidOrderList.orderLinkList.end(); iter++) {
		cout << iter->first << ": \n";
		BidOrder* head = iter->second;
		while (head) {
			cout << head->orderId << ' ' << head->price << ' ' << head->quantity << "\n";
			head = head->next;
		}
	}
	return 0;
}