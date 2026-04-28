#pragma once
#include <vector>
#include <functional>
#include "base_event.hpp"
#include "log.hpp"
#include <memory>
#include "assert.hpp"
#include <tuple>

// Events of various kinds are triggered by various parts of the AG3 engine (input events, collision events, etc.).
// You can connect functions to those events to make code run when an event is fired, as well as fire custom events.
// Connected functions are not called immediately for performance reasons, but are instead stored in a queue then fired all at once.
// Objects of the same class share the same Event objects; events store which connections are affiliated with which objects.
template <typename Object, typename ... eventArgs>
class Event : public BaseEvent {
public:
	using Ref = std::shared_ptr<Event>;

	friend class Connection;

	// Represents a function's connection to a specific object's event. 
	// When this goes out of scope, it disconnects the function from the event.
	class Connection {
	public:
		Connection(unsigned int id, std::shared_ptr<Object> obj) :
			connectedFunctionId(id), obj(obj)
		{

		}

		~Connection() {

			if (auto lockedEvent = event.lock()) {
				if (!lockedEvent->connectedFunctions) return;
				for (auto it = lockedEvent->connectedFunctions->begin(); it != lockedEvent->connectedFunctions->end(); it++) {
					if (it->first == connectedFunctionId) {
						lockedEvent->connectedFunctions->erase(it);
						return;
					}
				}
			}
		}

		Connection(const Connection&) = delete;
		Connection(Connection&&) = delete;
	private:
		unsigned int connectedFunctionId;
		std::weak_ptr<Object> obj;
	};

	using ConnectableFunctionArgs = std::tuple<eventArgs...>;
	using ConnectableFunction = std::function<void(eventArgs...)>;
	using ConnectableUniversalFunction = std::function<void(Object*, eventArgs...)>;
private:
	class EventInvocation : public BaseEventInvocation {
	public:
		ConnectableFunctionArgs invocationArgs;
		WeakRef event;

		EventInvocation(ConnectableFunctionArgs args, decltype(event) e) : invocationArgs(args), event(e) {}
		virtual ~EventInvocation() = default;

		virtual void RunConnections() override {
			if (event.expired()) return;

			auto eventLock = event.lock();

			// have to take connectedFunctions by value so a function disconnecting itself won't invalidate iterator
			auto connectedFunctions = *(eventLock->connectedFunctions);
			for (auto& f : connectedFunctions) {
				std::apply(f.second, invocationArgs); // std::apply basically calls f using the tuple cfa as a variaidic for us.
			}

		}
	};
public:
	Event() : BaseEvent() {
		connectedFunctions = std::make_shared< std::vector<std::pair<unsigned int, ConnectableFunction>>>();
	}

	Event(const Event&) = delete;
	~Event() {}

	// Adds the event to the queue, meaning that next time FlushQueue() is called, all functions connected to this event will be called.
	void Fire(eventArgs ... args) {
		ConnectableFunctionArgs tupledArgs = std::make_tuple(args...);
		Fire(tupledArgs);
	}

	void Fire(ConnectableFunctionArgs tupledArgs) {
		WeakRef e = dynamic_pointer_cast<Event>(weak_from_this().lock());
		std::unique_ptr<BaseEventInvocation> p = std::unique_ptr<BaseEventInvocation>((BaseEventInvocation*)(new EventInvocation(tupledArgs, e)));
		auto& q = EventInvocationQueue();
		q.push_back(std::move(p));
	}

	// Connects the given function to the event, so that it will be called every time the event is fired.
	// WARNING: if the function is a lambda which captures a shared_ptr, then that shared_ptr gets stored in this event.
	// This connection is permanent and lasts until the event is destroyed. For a temporary connection use ConnectTemporary().
	void Connect(ConnectableFunction function, const std::shared_ptr<Object>& obj) {
		Assert(function != nullptr);
		connectedFunctions->push_back(std::make_pair(LAST_CONNECTION_ID++, function));
	}

	// Like Connect(), but the provided function is connected to every object.
	void ConnectUniversal(ConnectableUniversalFunction function) {
	
	}

	// Connects the given function to the event, so that it will be called every time the event is fired, until the destruction of the returned Connection object.
	std::unique_ptr<Connection> ConnectTemporary(ConnectableFunction function) {
		unsigned int id = LAST_CONNECTION_ID++;
		connectedFunctions->push_back(std::make_pair(id, function));
		WeakRef wthis = std::dynamic_pointer_cast<Event>(shared_from_this()); // sadly we have to use shared_ptr, cast it, then turn it to a weak_ptr because no dynamic pointer cast for weak_ptr. stupid.
		return std::move(std::unique_ptr<Connection>(new Connection(id, wthis)));
	}

	std::unique_ptr<Connection> ConnectTemporaryUniversal()

	// returns true if anything is connected to this event.
	bool HasConnections() {
		return connectedFunctions->size() > 0;
	};

private:
	static inline unsigned int LAST_CONNECTION_ID = 0;	

	virtual void CleanupConnections() override {
		connectedFunctions = nullptr;
	}

	//template <typename T>
	friend class EventRef;

	// shared_ptrs needed so that Flush() can keep working without segfaults, even if the event is destroyed by calling a connected function.
	//std::shared_ptr<std::vector<ConnectableFunctionArgs>> eventInvocations;
	friend class EventInvocation;

	struct ConnectedFunction {
		ConnectableFunction func;
		// std::function can't be compared and we need to be able to differentiate between different functions to disconnect them.
		unsigned int funcId;
	};
	
	std::vector<ConnectedFunction> universallyConnectedFunctions;

};