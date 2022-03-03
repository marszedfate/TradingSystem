#include <map>
#include <mutex>
#include <vector>
#include <iostream>
#include <memory>
#include <algorithm>
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
	// virtual void CheckExchange() = 0;
	virtual void RegisterOrder() = 0;
	// virtual void WithdrawOrder() = 0;

	template<typename T>
	friend void WithdrawOrder(Order* order, OrderList<T>& orderList);

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
	
	/*
	void CheckExchange() {
		OrderList<BidOrder> &bidOrderList = Singleton<OrderList<BidOrder>>::GetInstance();
		for (auto iter = bidOrderList.orderLinkList.rbegin(); iter != bidOrderList.orderLinkList.rend(); iter++) {
			int bidPrice = iter->first;
			if (bidPrice < price || quantity == 0) break;
			BidOrder* head = iter->second;
			while (head != nullptr && quantity) {
				quantity -= min(quantity, head->quantity);
				head->quantity -= min(quantity, head->quantity);
				if (head->quantity == 0) {
					BidOrder* tmp = head;
					head = head->next;
					delete tmp;
				}
			}
		}
	}
	*/

	void RegisterOrder() {
		if (!quantity) return;
		OrderList<AskOrder>& askOrderList = Singleton<OrderList<AskOrder>>::GetInstance();
		auto head = askOrderList.orderLinkList[price];
		if (head == nullptr) {
			askOrderList.orderLinkList[price] = this;
		}
		else {
			while (head->next) head = head->next;
			head->next = this;
		}
	}

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

	/*
	void CheckExchange() {
		OrderList<BidOrder> &bidOrderList = Singleton<OrderList<BidOrder>>::GetInstance();
		for (auto iter = bidOrderList.orderLinkList.rbegin(); iter != bidOrderList.orderLinkList.rend(); iter++) {
			int bidPrice = iter->first;
			if (bidPrice < price || quantity == 0) break;
			BidOrder* head = iter->second;
			while (head != nullptr && quantity) {
				quantity -= min(quantity, head->quantity);
				head->quantity -= min(quantity, head->quantity);
				if (head->quantity == 0) {
					BidOrder* tmp = head;
					head = head->next;
					delete tmp;
				}
			}
		}
	}
	*/
	
	void RegisterOrder() {
		if (!quantity) return;
		OrderList<BidOrder> &bidOrderList = Singleton<OrderList<BidOrder>>::GetInstance();
		auto head = bidOrderList.orderLinkList[price];
		if (head == nullptr) {
			bidOrderList.orderLinkList[price] = this;
		}
		else {
			while (head->next) head = head->next;
			head->next = this;
		}
	}

	BidOrder* next;
};

class OrderBook {
public:
	OrderBook() {}

	/*
	void CheckExchange(Order* order) {
		order->CheckExchange();
	}
	*/

	void RegisterOrder(Order* order) {
		order->RegisterOrder();
	}

	template<typename T>
	void WithdrawOrderWrapper(Order* order, OrderList<T>& orderList) {
		WithdrawOrder(order, orderList);
	}
	
	~OrderBook() {

	}
	
// private:
//	OrderList<AskOrder> &askOrderList;
//	OrderList<BidOrder> &bidOrderList;
};

template<typename T>
void WithdrawOrder(Order* order, OrderList<T>& orderList) {
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
		head = nullptr;
		if (orderList.orderLinkList[order->price] == nullptr) orderList.orderLinkList.erase(order->price);
		return;
	}
	cout << "OrderId: " << order->orderId << " doesn't exist\n";
	return;
}

int main()
{
	OrderBook orderBook;
	BidOrder* bid1 = new BidOrder(1, 2, 2);
	BidOrder* bid2 = new BidOrder(2, 3, 5);
	orderBook.RegisterOrder(bid1);
	orderBook.RegisterOrder(bid2);
	BidOrder* bid3 = new BidOrder(2, 3, 5);
	OrderList<BidOrder>& bidOrderList = Singleton<OrderList<BidOrder>>::GetInstance();
	orderBook.WithdrawOrderWrapper(bid3, bidOrderList);
	
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