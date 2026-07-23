//#pragma once
//#include <concepts>
//#include <vector>
//#include <memory>
//#include <plf_colony.h>
//
//class Gameobject;
//
//template <typename T>
//concept DerivedFromGameobject = std::is_base_of<Gameobject, T>::value;
//
//class BaseGameobjectContainer {
//	virtual ~BaseGameobjectContainer() = default;
//
//	
//};
//
//template <DerivedFromGameobject T>
//class GameobjectContainer {
//public:
//	// doesn't use static locals, destruction only occurs when that of GameobjetManager does.
//	static GameobjectContainer& Get();
//	plf::colony<T> objs;
//
//	template <DerivedFromGameobject U, typename Func>
//	void MaybeIterate(Func f) {
//		constexpr if (std::is_base_of_v<U, T>) {
//			for (auto& o: objs) f(o);
//		}
//	}
//
//	virtual ~BaseGameobjectContainer() = default;
//private:
//	static inline GameobjectContainer* singleton = nullptr;
//};
//
//// Enables arbitrary subclassing of Gameobject without those who subclass Gameobject having to worry about setting up lifetime boilerplate or not being iterated over.
//class GameobjectManager {
//public:
//	static GameobjectManager& Get();
//
//	template <DerivedFromGameobject T, typename Func> 
//	void Iterate(Func f) {
//
//	}
//
//private:
//	std::vector<std::unique_ptr<BaseGameobjectContainer>> containers;
//	
//	template <DerivedFromGameobject T>
//	friend class GameobjectContainer;
//};
//
//// doesn't use static locals, destruction only occurs when that of GameobjetManager does.
//template<DerivedFromGameobject T>
//inline GameobjectContainer<T>& GameobjectContainer<T>::Get() {
//	if (!singleton) {
//		singleton = new GameobjectContainer();
//		GameobjectManager::Get().containers.emplace_back(singleton);
//	}
//	return *singleton;
//}
