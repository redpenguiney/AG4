#pragma once
#include <vector>
#include <memory>

// Abstract event class. See Event for actual info.
class BaseEvent {
protected:
	class BaseEventInvocation {
	public:
		virtual void RunConnections() = 0;
		virtual ~BaseEventInvocation() = default;
	};

	BaseEvent() = default;

	static std::vector<std::unique_ptr<BaseEventInvocation>>& EventInvocationQueue();

	// events are owned in a static local variable to ensure event/etc. destruction order play nice.
	// You can store events elsewhere if you please, just be careful. (todo: no you can't lol, constructor is private)
	static std::vector<std::unique_ptr<BaseEvent>>& GetEventRepository() {
		static std::vector<std::unique_ptr<BaseEvent>> repo;
		return repo;
	}

public:
	virtual ~BaseEvent() = default;

	// Calls all the functions connected to the events in the queue and empties the queue.  (disregard depth, it's just an implementation detail for recursion.)
	// Do NOT call within an event handler
	static void FlushEventQueue(int depth = 0);

	// neccesary because functional objects connected to events could store things that need singletons to be destructed, yet singletons store events.
	// Disconnects all events.
	//static void Cleanup();

private:

	//// helper for Cleanup()
	//virtual void CleanupConnections() = 0;
	//static inline std::vector<BaseEvent*> events = {};
};

