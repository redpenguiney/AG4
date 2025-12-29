#pragma once
#include <vector>
#include <memory>

// Abstract event class. See Event for actual info.
class BaseEvent : public std::enable_shared_from_this<BaseEvent> {
protected:
	class BaseEventInvocation {
	public:
		virtual void RunConnections() = 0;
		virtual ~BaseEventInvocation() = default;
	};

	BaseEvent();
	virtual ~BaseEvent();

	static std::vector<std::unique_ptr<BaseEventInvocation>>& EventInvocationQueue();

public:

	// Calls all the functions connected to the events in the queue and empties the queue.  (disregard depth, it's just an implementation detail for recursion.)
	// Do NOT call within an event handler
	static void FlushEventQueue(int depth = 0);

	// neccesary because functional objects connected to events could store things that need singletons to be destructed, yet singletons store events.
	// Disconnects all events.
	static void Cleanup();

private:
	friend class std::shared_ptr<BaseEvent>;

	// helper for Cleanup()
	virtual void CleanupConnections() = 0;
	static inline std::vector<BaseEvent*> events = {};
};

