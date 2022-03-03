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
	virtual void MatchOrder() = 0;

	template<typename T>
	friend void RegisterOrder(T* order, OrderList<T>& orderList);

	template<typename T>
	friend void WithdrawOrder(T* order, OrderList<T>& orderList);

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
	OrderBook() {}

	static void MatchOrderWrapper(Order* order) {
		order->MatchOrder();
	}
	
	template<typename T>
	static void RegisterOrderWrapper(T* order, OrderList<T>& orderList) {
		RegisterOrder(order, orderList);
	}

	template<typename T>
	static void WithdrawOrderWrapper(T* order, OrderList<T>& orderList) {
		WithdrawOrder(order, orderList);
	}

// private:
//	OrderList<AskOrder> &askOrderList;
//	OrderList<BidOrder> &bidOrderList;
};

template<typename T>
void RegisterOrder(T* order, OrderList<T>& orderList) {
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
void WithdrawOrder(T* order, OrderList<T>& orderList) {
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

void AskOrder::MatchOrder()
{
	OrderList<BidOrder>& bidOrderList = Singleton<OrderList<BidOrder>>::GetInstance();
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
		iter->second = head;
		if (iter->second == nullptr) bidOrderList.orderLinkList.erase(price);
	}
}

void BidOrder::MatchOrder() {
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
			quantity -= min(quantity, head->quantity);
			head->quantity -= min(quantity, head->quantity);
			if (head->quantity == 0) {
				AskOrder* tmp = head;
				head = head->next;
				delete tmp;
			}
		}
		iter->second = head;
		if (iter->second == nullptr) askOrderList.orderLinkList.erase(price);
		if (iter == askOrderList.orderLinkList.begin()) break;
	}
}

int main()
{
	OrderList<BidOrder>& bidOrderList = Singleton<OrderList<BidOrder>>::GetInstance();
	OrderList<AskOrder>& askOrderList = Singleton<OrderList<AskOrder>>::GetInstance();
	BidOrder* bid1 = new BidOrder(1, 2, 2);
	BidOrder* bid2 = new BidOrder(2, 2, 3);
	OrderBook::RegisterOrderWrapper(bid1, bidOrderList);
	OrderBook::RegisterOrderWrapper(bid2, bidOrderList);

	// BidOrder* bid3 = new BidOrder(2, 3, 5);
	// OrderBook::WithdrawOrderWrapper(bid3, bidOrderList);

	AskOrder* bid4 = new AskOrder(3, 2, 1);
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