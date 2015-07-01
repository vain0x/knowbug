// module - cast

#ifndef IG_MODULE_CAST_H
#define IG_MODULE_CAST_H

#include <typeinfo>		// std::bad_cast のため

//------------------------------------------------
// Ｃ形式キャスト [FTM]
//------------------------------------------------
#define FTM_ctype_cast(name)					\
	template<class TTo, class TFrom> inline		\
	TTo name( const TFrom& target )				\
	{											\
		return (TTo)target;						\
	}
	
//------------------------------------------------
// 万能キャスト (Ｃ形式キャスト)
// 
// @ おそらく安全だと思われるときに使用する。
// @	→ otherwise: forcible_cast
//------------------------------------------------
FTM_ctype_cast( almighty_cast );

//------------------------------------------------
// 強引キャスト (Ｃ形式キャスト)
// 
// @ 無理をするときに使用する。
// @	→ otherwise: almighty_cast
//------------------------------------------------
FTM_ctype_cast( forcible_cast );

#undef FTM_ctype_cast

//------------------------------------------------
// 暗黙の型変換を明示する関数
//------------------------------------------------
template<class TTo, class TFrom> inline
TTo implicit_cast( TFrom& target )
{
	return target;
}

//------------------------------------------------
// ポインタ型同士のキャスト
// 
// @ 関連性がなくてもかまわない。
// @ 関数ポインタは受け取れない。
// @prm <T>: void* 型から static_cast で変換できる型。
// @ex: pp = ptr_cast<char**>(s);
//------------------------------------------------
template<class T> inline
T ptr_cast( void* p )
{
	return static_cast<T>( p );
}

// const バージョン
template<class T> inline
T ptr_cast( const void* p )
{
	return static_cast<T>( p );
}

//------------------------------------------------
// ポインタ → 整数へのキャスト
//------------------------------------------------
#ifdef _UINTPTR_T_DEFINED

static inline uintptr_t address_cast( const void* p )
{
	return almighty_cast<uintptr_t>(p);
}

#else

static inline unsigned long address_cast( const void* p )
{
	return forcible_cast<unsigned long>(p);
}

#endif

//------------------------------------------------
// 符号キャスト
// 
// @ex: unsigned int -> signed int
//------------------------------------------------
class bad_unsign_cast;

template<class TUn, class TSg>
static inline TUn unsign_cast( TSg val ) // throw (bad_unsign_cast)
{
	if ( val < 0 ) throw bad_unsign_cast(val);
	return almighty_cast<TUn>( val );
}

/*
template<class TSg, class TUn>
static inline TSg sign_cast( TUn& val ) throw (bad_sign_cast)
{
	if ( val > almighty_cast<TUn>(almighty_cast<TSg>(-1)) ) {
		throw bad_sign_cast;
	}
	return val;
}
//*/

class bad_unsign_cast
	: public std::bad_cast
{
public:
	bad_unsign_cast( int val )
		: mVal( val )
	{ }
	operator int(void) const { return mVal; }
	
private:
	int mVal;
};

/*
//------------------------------------------------
// 明示アップキャスト
//------------------------------------------------
template<class T> inline
T& up_cast( T& obj )
{
	return obj;
}

template<class T> inline
const T& up_cast( const T& obj )
{
	return obj;
}

//*/

#endif
