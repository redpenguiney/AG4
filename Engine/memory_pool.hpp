#pragma once
#include "assert.hpp"
#include <vector>

//template <typename T>
//concept CanGoInPool = requires (const T & a) { { a.live } -> std::same_as<bool&>; };

// Singleton class. T must have a boolean member "live"
template <typename T, typename...ConstructorArgs>
	//requires CanGoInPool<T>
class MemoryPool {
public:
	static_assert(offsetof(T, live) >= sizeof(T*));

	union StorageType {
		T obj;
		StorageType* nextFree;
		~StorageType() = delete;
	};

	static MemoryPool& Get() {
		static MemoryPool pool;
		return pool;
	}

	T* New(ConstructorArgs... args) {
		T* foundObj = nullptr;

		for (auto& f : firstFree) {
			if (f != nullptr) {
				foundObj = &f->obj;

				// update free list
				f = f->nextFree;
			}
		}
		
		if (!foundObj) {
			// if we didn't find an object, then we must allocate a new page
			StorageType* newPage = reinterpret_cast<StorageType*>(malloc(pageSize));
			for (unsigned i = 0; i < objectsPerPage - 1; i++) {
				newPage[i].nextFree = newPage + i + 1;
				newPage[i].obj.live = false;
			}
			newPage[objectsPerPage - 1].nextFree = nullptr;
			foundObj = &newPage->obj;
			firstFree.push_back(newPage->nextFree);
			pages.push_back(newPage);

		}

		// placement new to call constructor
		new (foundObj) T(args...);

		return foundObj;
	}

	void Destroy(T* obj) {
		obj->live = false;

		// Determine what page the gameobject is stored on
		StorageType* page = nullptr;
		
		unsigned pageI = 0;
		for (unsigned i = 0; i < pages.size(); i++) { // TODO: binary search would be better
			auto& p = pages[i];
			if (obj - &p->obj < objectsPerPage) {
				page = p;
				pageI = i;
				break;
			}
		}
		Assert(page);

		// Update free list
		reinterpret_cast<StorageType*>(obj)->nextFree = firstFree[pageI];
		firstFree[pageI] = reinterpret_cast<StorageType*>(obj);
	}

	~MemoryPool() {
		for (auto& p : pages) {
			free(p);
		}
	}
	
	const std::vector<StorageType*>& GetIterable() { return pages; }	
	static constexpr size_t objectsPerPage = 256;

private:
	MemoryPool() {}
	MemoryPool(const MemoryPool&) = delete;

	static constexpr size_t pageSize = sizeof(T) * objectsPerPage;
	std::vector<StorageType*> pages;
	std::vector<StorageType*> firstFree; // for each page, first unallocated. nullptr if the page is entirely allocated.
};