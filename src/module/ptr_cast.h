#ifndef IG_POINTER_CAST_H
#define IG_POINTER_CAST_H

/// ポインタ型同士のキャスト
template<class T>
T ptr_cast(void* p)
{
	return static_cast<T>(p);
}

template<class T>
T ptr_cast(void const* p)
{
	return static_cast<T>(p);
}

template<class T,
	class U = std::remove_pointer_t<T>>
U const* cptr_cast(void const* p)
{
	static_assert(std::is_pointer<T>::value, "");
	
	return static_cast<U const*>(p);
}

/// ポインタ→整数
#ifdef _UINTPTR_T_DEFINED
static uintptr_t address_cast(void const* p)
{
	return (uintptr_t)(p);
}
#else
static unsigned long address_cast(void const* p)
{
	return ctype_cast<unsigned long>(p);
}
#endif

#endif
