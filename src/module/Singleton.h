#pragma once
#include <memory>

template <class T>
class Singleton
{
public:
	static auto instance() -> T&
	{
		static auto instance_ =
			typename T::singleton_pointer_type { T::createInstance() };
		return T::dereference(instance_);
	}
protected:
	Singleton()
	{}

private:
	using singleton_pointer_type = std::unique_ptr<T>;
	static auto createInstance() -> T*
	{
		return new T();
	}
	static auto dereference(singleton_pointer_type const& ptr) -> T&
	{
		return *ptr;
	}
private:
	Singleton(Singleton const&) = delete;
	Singleton& operator=(Singleton const&) = delete;
	Singleton(Singleton&&) = delete;
	Singleton& operator=(Singleton&&) = delete;
};

template<typename T>
struct SharedSingleton
	: public Singleton<T>
{
	static auto make_shared() -> std::shared_ptr<T> const&
	{
		static auto instance_ = std::shared_ptr<T> { new T {} };
		assert(instance_);
		return instance_;
	}
	static auto instance() -> T&
	{
		return *make_shared();
	}
};
