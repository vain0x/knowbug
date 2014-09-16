// Vector 実体データクラス

#ifndef IG_CLASS_VECTOR_H
#define IG_CLASS_VECTOR_H

// todo: 一般化して VWRC に置き換える。
#include "hsp3plugin_custom.h"
#include "PValRef.h"
using namespace hpimod;

class CVector;
static bool isNull( CVector const* const src );

// AddRef, Release を dbgout で報告する (mpval や CVectorHolder でのカウンタ増減がめんどくさい)
//	#define DBGOUT_VECTOR_ADDREF_OR_RELEASE

//------------------------------------------------
// vector の実体データたる CVector* を所持するクラス
// 
// @ CVector は、「PValRef* の組み合わせと、その順序」を表現する。
// @ 内部変数の参照カウンタを回すために、これ自身も参照型でなければならない。
// @ capacity 0 のときは ptr_ = nullptr でもよい。
//------------------------------------------------
class CVector
{
public:
	using Iter_t = PVal**;
	using CIter_t = PVal* const*;

public:
	static CVector* const Null;
	static CVector* const MagicNull;

	//--------------------------------------------
	// メンバ変数
	//--------------------------------------------
private:
	// 実データへのポインタ
	PValRef** ptr_;

	// 配列バッファの情報
	size_t size_;
	size_t capa_;

	// 参照カウンタ、一時オブジェクトフラグ
	mutable int cntRefed_;
	mutable int cntTmp_;

	//--------------------------------------------
	// メンバ関数
	//--------------------------------------------
public:
	static CVector* New(PVal* pval = nullptr);
	static CVector* NewTemp(PVal* pval = nullptr);

private:
	CVector( PVal* pval = nullptr );
	~CVector();
	void Free();

	static void Delete( CVector* src );

	// コンテナの情報
public:
	size_t Size()     const { return size_; }
	size_t Capacity() const { return capa_; }
	bool   Empty()    const { return size_ == 0; }

	 Iter_t  begin()       { return const_cast<Iter_t>(static_cast<CVector const*>(this)->begin()); }
	 Iter_t    end()       { return begin() + Size(); }
	CIter_t  begin() const { return reinterpret_cast<PVal**>(ptr_); }
	CIter_t    end() const { return begin() + Size(); }
	 Iter_t rbegin()       { return   end() - 1; }
	 Iter_t   rend()       { return begin() - 1; }
	CIter_t rbegin() const { return   end() - 1; }
	CIter_t   rend() const { return begin() - 1; }

	int Compare( CVector const& src ) const;
	static int Compare( CVector const* lhs, CVector const* rhs );

	// 添字
public:
	PVal* operator[]( size_t const idx ) { return At(idx); }
	PVal* At( size_t idx );
	PVal* AtSafe( size_t idx ) const { return ( IsValid(idx) ? AtUnsafe(idx) : nullptr ); }

	bool IsValid( int    idx ) const { return ( 0 <= idx && IsValid( static_cast<size_t>(idx) ) ); }
	bool IsValid( size_t idx ) const { return (idx < Size()); }
	bool IsValid( size_t iBgn, size_t iEnd ) const {
		return ( iBgn <= iEnd && IsValid(iEnd - 1) );
	}

private:
	PVal*& AtUnsafe( size_t idx ) { return begin()[idx]; }
	PVal* const& AtUnsafe( size_t idx ) const { return begin()[idx]; }
	PVal* const& AtFirst() const { assert(!Empty()); return AtUnsafe(0); }
	PVal* const& AtLast()  const { assert(!Empty()); return AtUnsafe( Size() - 1 ); }

	// メモリ管理
public:
	void Alloc( size_t const newSize ) { AllocImpl( newSize, true ); }
	void Expand( size_t const exSize  ) { Alloc( Size() + exSize ); }
	void Reserve( size_t const minCapa ) { if ( minCapa > Capacity() ) { ReserveImpl( minCapa - Capacity() ); } }

private:
	void AllocImpl( size_t const newSize, bool bInit );
	void ExpandImpl( size_t const exSize, bool bInit ) { AllocImpl( Size() + exSize, bInit ); }
	void ReserveImpl( size_t const exSize );
	void SetSizeImpl( size_t const newSize );

	// 要素メモリ管理
private:
	PVal* NewElem( int vflag = HSPVAR_FLAG_INT );
	void  DeleteElem( PVal* pval );

	// 要素増減
public:
	PVal* Insert(size_t idx);
	void  Insert(size_t iBgn, size_t iEnd) { InsertImpl(iBgn, iEnd, true); }
	void  Remove(size_t idx);
	void  Remove(size_t iBgn, size_t iEnd);
	void  Replace( size_t iBgn, size_t iEnd, CVector const* src );
private:
	void  InsertImpl( size_t iBgn, size_t iEnd, bool bInit = false );
	void  RemoveImpl( size_t iBgn, size_t iEnd );
	void  ReplaceImpl( size_t iBgn, size_t iEnd, size_t cntElems );

public:
	PVal* PushFront() { return Insert(0); }
	PVal* PushBack()  { return Insert( Size() ); }

	void PopFront() { Remove( 0 ); }
	void PopBack()  { Remove( Size() - 1 ); }

	// コンテナ管理
public:
	void Move  ( size_t iDst, size_t iSrc );
	void Swap  ( size_t idx1, size_t idx2 );
	void Rotate( int step = 1 );
	void RotateBack( int step = 1 ) { Rotate( -step ); }
	void Reverse();
	void Reverse( size_t iBgn ) { Reverse( iBgn, Size() ); }
	void Reverse( size_t iBgn, size_t iEnd );

	void Clear();

	void ChainWithCopy( CVector const& src, size_t iBgn = 0 ) { return ChainImpl( src, iBgn, src.Size(), true  ); }
	void Chain        ( CVector const& src, size_t iBgn = 0 ) { return ChainImpl( src, iBgn, src.Size(), false ); }
	void CopyWithCopy ( CVector const& src, size_t iBgn = 0 ) { return CopyImpl ( src, iBgn, src.Size(), true  ); }
	void Copy         ( CVector const& src, size_t iBgn = 0 ) { return CopyImpl ( src, iBgn, src.Size(), false ); }

	void ChainWithCopy( CVector const& src, size_t iBgn, size_t iEnd ) { if ( src.IsValid(iBgn, iEnd) ) return ChainImpl( src, iBgn, iEnd, true  ); }
	void Chain        ( CVector const& src, size_t iBgn, size_t iEnd ) { if ( src.IsValid(iBgn, iEnd) ) return ChainImpl( src, iBgn, iEnd, false ); }
	void CopyWithCopy ( CVector const& src, size_t iBgn, size_t iEnd ) { if ( src.IsValid(iBgn, iEnd) ) return CopyImpl ( src, iBgn, iEnd, true  ); }
	void Copy         ( CVector const& src, size_t iBgn, size_t iEnd ) { if ( src.IsValid(iBgn, iEnd) ) return CopyImpl ( src, iBgn, iEnd, false ); }

private:
	void ChainImpl( CVector const& src, size_t iBgn, size_t iEnd, bool bCopyElem );
	void CopyImpl ( CVector const& src, size_t iBgn, size_t iEnd, bool bCopyElem ) {
		Clear(); ChainImpl( src, iBgn, iEnd, bCopyElem );
	}

private:
	void MemMove( size_t iDst, size_t iSrc, size_t cnt )
	{
		std::memmove( begin() + iDst, begin() + iSrc, cnt * sizeof(PVal*) );
	}

	// 参照カウンタ
#ifdef DBGOUT_VECTOR_ADDREF_OR_RELEASE
private:
	int id_;
public:
	void AddRef()  { dbgout("[%d] ++ → %d <%d>", id_, ++ cntRefed_, cntTmp_); }
	void Release() { dbgout("[%d] -- → %d <%d>", id_, -- cntRefed_, cntTmp_); if ( cntRefed_ == 0 ) Delete(this); }
	void BecomeTmpObj() { assert(cntTmp_ == 0); dbgout("[%d] ++ → %d <tmpobj>", id_, ++ cntRefed_); cntTmp_ = 1; }
	void BecomeShared() { assert(cntTmp_ == 1); dbgout("[%d] -- → %d <shared>", id_, -- cntRefed_); cntTmp_ = 0; if ( cntRefed_ == 0 ) Delete(this); }
#else
public:
	void AddRef()  { cntRefed_ ++; }
	void Release() { cntRefed_ --; if ( cntRefed_ == 0 ) Delete(this); }
	void BecomeTmpObj() { assert(cntTmp_ == 0); cntTmp_ = 1; AddRef(this); }
	void BecomeShared() { assert(cntTmp_ == 1); cntTmp_ = 0; Release(this); }
#endif
	bool IsTmp() const { return cntTmp_ > 0; }

	static void AddRef    ( CVector* src ) { if ( !isNull(src) ) src->AddRef(); }
	static void Release   ( CVector* src ) { if ( !isNull(src) ) src->Release(); }
	static void BecomeTmpObj(CVector* src) { if ( !isNull(src) ) src->BecomeTmpObj(); }
	static void BecomeShared(CVector* src) { if ( !isNull(src) ) src->BecomeShared(); }

	void ReleaseIfTmpObj() { if ( IsTmp() ) BecomeShared(); }
	static void ReleaseIfTmpObj(CVector* src) { if ( !isNull(src) ) src->ReleaseIfTmpObj(); }

private:
	CVector( CVector const& src ) = delete;
	CVector& operator =(CVector const& src ) = delete;
};

//------------------------------------------------
// マクロ的関数
//------------------------------------------------
static bool isNull( CVector const* const src )
{
	return ( src == CVector::MagicNull || src == CVector::Null );
}
//*
//------------------------------------------------
// 引数として受け取った CVector* を所有するクラス
// 
// @ 一時変数に生成された vector の次に引数がある場合、
// @	mpval が所有権を失って vector が消滅するため、
// @	mpval に代わってローカルで vector を所有する。
//------------------------------------------------
class CVectorHolder
{
private:
	CVector* inst_;

public:
	CVectorHolder()
		: inst_(nullptr)
	{ }
	explicit CVectorHolder( CVector* inst )
		: inst_(inst)
	{
		CVector::AddRef(inst_);
	}
	~CVectorHolder()
	{
		CVector::Release( inst_ );
	}

	CVector* get() { return inst_; }
	CVector const* get() const { return inst_; }

	operator CVector*() const { return inst_; }
	CVector& operator *() const { return *inst_; }
	CVector* operator ->() const { return inst_; }

	CVector* const* operator&() const { return &inst_; }

public:
	// mover
	CVectorHolder& operator =(CVectorHolder&& src) {
		if ( *this != src ) {
			this->~CVectorHolder(); new(this) CVectorHolder(std::move(src));
		}
		return *this;
	}
	CVectorHolder(CVectorHolder&& src) : inst_(src.inst_) {
		src.inst_ = nullptr;
	}

	bool operator ==(CVectorHolder const& rhs) const {
		return (isNull(inst_) && isNull(rhs.inst_)) || (inst_ == rhs.inst_);
	}
	bool operator !=(CVectorHolder const& rhs) const { return !(*this == rhs); }

private:
	CVectorHolder( CVectorHolder const& ) = delete;
	CVectorHolder& operator =( CVectorHolder& ) = delete;
};
//*/
#endif
