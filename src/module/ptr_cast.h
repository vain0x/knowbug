#ifndef IG_POINTER_CAST_H
#define IG_POINTER_CAST_H

/// ポインタ型同士のキャスト
template<class T>
auto ptr_cast(void* p) -> T
{
	return static_cast<T>(p);
}

template<class T>
auto ptr_cast(void const* p) -> T
{
	return static_cast<T>(p);
}

template<class T,
	class U = std::remove_pointer_t<T>>
auto cptr_cast(void const* p) -> U const*
{
	static_assert(std::is_pointer<T>::value, "");

	return static_cast<U const*>(p);
}

/// ポインタ→整数
#ifdef _UINTPTR_T_DEFINED
static auto address_cast(void const* p) -> uintptr_t
{
	return (uintptr_t)(p);
}
#else
static auto address_cast(void const* p) -> unsigned long
{
	return (unsigned long)(p);
}
#endif

#endif
