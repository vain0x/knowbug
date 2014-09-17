// prmstack 生成クラス

#ifndef IG_CLASS_PARAMETER_STACK_CREATOR_H
#define IG_CLASS_PARAMETER_STACK_CREATOR_H

/**
@summary:
	prmstk と同じ形式でデータを格納する処理のインターフェースとなるクラス。
**/

#include "hsp3plugin.h"

class CPrmStkCreator
{
private:
	char*  ptr_;		// スタックの先頭へのポインタ
	size_t usingSize_;	// 使用されたサイズ
	size_t bufSize_;	// 確保済みのサイズ

public:
	CPrmStkCreator( void* buf, size_t bufSize )
		: ptr_( reinterpret_cast<char*>( buf ) )
		, bufSize_( bufSize )
		, usingSize_( 0 )
	{ }
	~CPrmStkCreator() { }

	// 取得
	void*  getptr()       const { return ptr_; }
	size_t getBufSize()   const { return bufSize_; }
	size_t getUsingSize() const { return usingSize_; }

	// 格納
	template<typename T> T* pushValue();		// コピーなし
	template<typename T> void pushValue( T&& value );

	void pushPVal( PVal* pval, APTR aptr ) {
		pushValue( pval );
		pushValue( aptr );
	}
	void pushPVal( MPVarData const* pVarDat ) {
		pushValue(*pVarDat);
	}
	void pushThismod( PVal* pval, APTR aptr, int idStDat ) {
		pushValue(MPModVarData { idStDat, MODVAR_MAGICCODE, pval, aptr });
	}

	PVal* pushLocal() {
		// 初期化しない
		return pushValue<PVal>();
	}

private:
	void needSize( size_t sizeAdditional ) {		// 使用するサイズの宣言
		if ( bufSize_ < (usingSize_ + sizeAdditional) ) {
			puterror( HSPERR_OUT_OF_MEMORY );
		}
	}

	void* begin() const { return ptr_; }
	void* end() const { return ptr_ + usingSize_; }

private:
	CPrmStkCreator( CPrmStkCreator const& obj ) = delete;
	CPrmStkCreator& operator = ( CPrmStkCreator const& obj ) = delete;
};

//------------------------------------------------
// 単純に値をプッシュする
//------------------------------------------------
template<typename T>
void CPrmStkCreator::pushValue(T&& value)
{
	using value_type = std::decay_t<T>;

	value_type* const p = pushValue<value_type>();
	*p = std::forward<T>(value);
	return;
}

template<typename T>
T* CPrmStkCreator::pushValue()
{
	needSize(sizeof(T));
	T* const p = reinterpret_cast<T*>(end());
	usingSize_ += sizeof(T);
	return p;
}

//------------------------------------------------
// バッファ所有バージョン
//------------------------------------------------
class CPrmStkCreatorWithBuffer
	: public CPrmStkCreator
{
public:
	CPrmStkCreatorWithBuffer(size_t size)
		: CPrmStkCreator(hspmalloc(size), size)
	{ }
	~CPrmStkCreatorWithBuffer()
	{
		void* const ptr = getptr();
		assert(!!ptr);
		hspfree(ptr);
	}
};

#endif
