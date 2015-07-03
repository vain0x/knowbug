#pragma once
#include <memory>

template <class T>
class Singleton
{
public:
	static T& instance() {
		static typename T::singleton_pointer_type instance_ (T::createInstance());
		return T::dereference(instance_);
	}
protected:
	Singleton() {}

private:
	using singleton_pointer_type = std::unique_ptr<T>;
	inline static T* createInstance() { return new T(); }
	inline static T& dereference(singleton_pointer_type const& ptr) { return *ptr; }

private:
	Singleton(Singleton const&) = delete;
	Singleton& operator=(Singleton const&) = delete;
	Singleton(Singleton&&) = delete;
	Singleton& operator=(Singleton&&) = delete;
};