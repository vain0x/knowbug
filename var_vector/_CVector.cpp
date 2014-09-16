// Vector 実体データクラス

#include "CVector.h"
#include "PValRef.h"

#include "sub_vector.h"
#include "mod_makepval.h"

#include <algorithm>

using namespace hpimod;

#define MALLOC  hspmalloc
#define MEXPAND hspexpand
#define MFREE   hspfree

CVector* const CVector::Null      = (CVector*)HspFalse;
CVector* const CVector::MagicNull = (CVector*)HspTrue;

//------------------------------------------------
// 構築
//------------------------------------------------
CVector::CVector( PVal* pval )
	: ptr_( 0 )
	, cntRefed_( 0 )
	, cntTmp_( 0 )
{
	size_ = 0;
	capa_ = 0;

#ifdef DBGOUT_VECTOR_ADDREF_OR_RELEASE
	static int stt_counter = 0;
	id_ = stt_counter ++;
	dbgout("[%d] new", id_ );
#endif
}

CVector* CVector::New( PVal* pval )
{
	return new( MALLOC( sizeof(CVector) ) ) CVector( pval );
		//new CVector( pval )
}

CVector* CVector::NewTemp(PVal* pval)
{
	auto const&& self = CVector::New(pval);
	self->BecomeTmpObj();
	return self;
}

//------------------------------------------------
// 解体
//------------------------------------------------
CVector::~CVector()
{
	Free();
}

//------------------------------------------------
// 解体処理の実装
//------------------------------------------------
void CVector::Free()
{
	// 内部変数をすべて解放する
	Clear();

	// vector を解放する
	if ( ptr_ ) { MFREE(ptr_); ptr_ = nullptr; }	
	return;
}

void CVector::Delete( CVector* src )
{
	assert( src );
	src->~CVector();
	MFREE(src);
//	delete src;
}

//------------------------------------------------
// 指定要素の PVal* を得る
// 
// @ なければ既定値で追加する。
//------------------------------------------------
PVal* CVector::At( size_t idx )
{
	size_t const len = Size();

	if ( IsValid(idx) ) {
		return AtUnsafe( idx );

	// ない =>> 既定値で追加
	} else {
		Alloc( idx + 1 );
		return AtLast();
	}
}

//------------------------------------------------
// 比較
//------------------------------------------------
int CVector::Compare( CVector const& src ) const
{
	size_t const len    = Size();
	size_t const lenRhs = src.Size();

	if ( len != lenRhs ) return (len < lenRhs ? -1 : 1);

	for ( size_t i = 0; i < len; ++ i ) {
		if ( AtUnsafe(i) != AtUnsafe(i) ) return -1;		// 違ったらとりあえず左のが小さいことにする
	}

	return 0;
}

int CVector::Compare( CVector const* lhs, CVector const* rhs )
{
	bool const bNullLhs = isNull(lhs);
	bool const bNullRhs = isNull(rhs);

	if ( bNullLhs ) {
		return (bNullRhs ? 0 : -1);
	} else if ( bNullRhs ) {
		return (bNullLhs ? 0 :  1);
	} else {
		return lhs->Compare( *rhs );
	}
}

//##########################################################
//        メモリ管理
//##########################################################
//------------------------------------------------
// 指定個数の要素を確保する
//------------------------------------------------
void CVector::AllocImpl( size_t const newSize, bool bInit )
{
	if ( newSize > size_ ) {
		size_t const oldSize = size_;

		Reserve( newSize );
		SetSizeImpl( newSize );

		// 拡張部分を初期化
		if ( bInit ) {
			for ( size_t i = oldSize; i < newSize; ++ i ) {
				AtUnsafe(i) = NewElem();
			}
		}
#ifdef _DEBUG
		else {
			for ( size_t i = oldSize; i < newSize; ++i ) {
				AtUnsafe(i) = AsPVal(PValRef::MagicNull);
			}
		}
#endif
	}

	return;
}

//------------------------------------------------
// バッファを拡張する
//------------------------------------------------
void CVector::ReserveImpl( size_t const exSize )
{
	capa_ += exSize;

	if ( !ptr_ ) {
		capa_ = std::max( exSize, 64 / sizeof(PVal*) );		// hspmalloc は最小でも 64 バイトとるので
		ptr_ = reinterpret_cast<decltype(ptr_)>( MALLOC( capa_ * sizeof(PVal*) ));
	} else {
		ptr_ = reinterpret_cast<decltype(ptr_)>( MEXPAND((char*)ptr_, capa_ * sizeof(PVal*)) );
	}
	return;
}

//------------------------------------------------
// size_ の値を変更する
//------------------------------------------------
void CVector::SetSizeImpl( size_t const newSize )
{
	size_ = newSize;
//	if ( mpvOwn ) mpvOwn->len[1] = newSize;
	return;
}

//------------------------------------------------
// 要素を生成・削除する
//------------------------------------------------
PVal* CVector::NewElem( int vflag )
{
	PValRef* const pval = PValRef::New(vflag);
	PValRef::AddRef( pval );
	return AsPVal( pval );
}

void  CVector::DeleteElem( PVal* pval )
{
	if ( !pval ) return;
	PValRef::Release( AsPValRef(pval) );
	return;
}

//##########################################################
//        コンテナ操作
//##########################################################
//------------------------------------------------
// 全消去
//------------------------------------------------
void CVector::Clear()
{
	for ( size_t i = 0; i < Size(); ++ i ) {
		DeleteElem( AtUnsafe(i) );
	}

	SetSizeImpl( 0 );
	return;
}

//------------------------------------------------
// 連結
// 
// @ src が持つ要素 [iBgn, iEnd) を this にも追加する。
// @prm bCopyElem: PVal を複製するか否か
//------------------------------------------------
void CVector::ChainImpl( CVector const& src, size_t iBgn, size_t iEnd, bool bCopyElem )
{
	assert( src.IsValid( iBgn, iEnd ) );

	size_t const offset    = Size();
	size_t const lenAppend = iEnd - iBgn;

	// バッファ拡張 (拡張部分を初期化しない)
	ExpandImpl( lenAppend, false );

	// 右辺の持つすべての要素を追加する
	for ( size_t i = 0; i < lenAppend; ++ i ) {
		auto& pvDst =     AtUnsafe(offset + i);
		auto& pvSrc = src.AtUnsafe(iBgn + i);

		// 複製する場合
		if ( bCopyElem ) {
			pvDst = NewElem();
		//	memset( pvDst, 0, sizeof(PVal) );
			PVal_copy( pvDst, pvSrc );

		// 参照共有のみの場合
		} else {
			pvDst = PValRef::AddRef( pvSrc );
		}
	}
	return;
}

//------------------------------------------------
// 挿入
//------------------------------------------------
PVal* CVector::Insert( size_t idx )
{
	InsertImpl( idx, idx + 1, true );
	return AtUnsafe(idx);
}

void CVector::InsertImpl( size_t iBgn, size_t iEnd, bool bInit )
{
	// 自動拡張
	if ( !IsValid(iBgn) ) {
		AllocImpl( iEnd, bInit );

	// スペースを空ける
	} else {
		ExpandImpl( iEnd - iBgn, false );			// 枠を広げる (初期化しない)

		// 挿入される場所を空ける
		MemMove( iEnd, iBgn, (Size() - iBgn) );		// 後方にずらす

		// 初期化する
		if ( bInit ) {
			for ( size_t i = iBgn; i < iEnd; ++ i ) {
				AtUnsafe(i) = NewElem();
			}
		}
	}
	return;
}

//------------------------------------------------
// 除去
//------------------------------------------------
void CVector::Remove( size_t idx )
{
	RemoveImpl( idx, idx + 1 );

	// [idx] を詰める (実質的除去)
	MemMove( idx, idx + 1, (Size() - (idx + 1)) );
	SetSizeImpl( size_ - 1 );	// 要素数調整

	return;
}

void CVector::Remove( size_t iBgn, size_t iEnd )
{
	assert( IsValid( iBgn, iEnd ) );

	RemoveImpl( iBgn, iEnd );

	// [iBgn, iEnd) を詰める (実質的除去)
	MemMove( iBgn, iEnd, (Size() - iEnd) );
	SetSizeImpl( size_ - (iEnd - iBgn) );	// 要素数調整
	return;
}

void CVector::RemoveImpl( size_t iBgn, size_t iEnd )
{
	// [iBgn, iEnd) を解放 (不定値になるので注意)
	for ( size_t i = iBgn; i < iEnd; ++ i ) {
		DeleteElem( AtUnsafe(i) );
	}
	return;
}

//------------------------------------------------
// 置換
//------------------------------------------------
void CVector::Replace( size_t iBgn, size_t iEnd, CVector const* src )
{
	assert( IsValid(iBgn, iEnd) );

	size_t const cntElems = (src ? src->Size() : 0);

	ReplaceImpl( iBgn, iEnd, cntElems );
	
	// 参照複写
	for ( size_t i = 0; i < cntElems; ++ i ) {
		assert(!!src);

		// lhs は ReplaceImpl にて解放済み、不定値
		AtUnsafe(iBgn + i) = PValRef::AddRef(src->AtUnsafe(i));
	}
	return;
}

void CVector::ReplaceImpl( size_t iBgn, size_t iEnd, size_t cntElems )
{
	size_t const lenRange = iEnd - iBgn;

	if ( lenRange < cntElems ) {
		// 区間の元々の要素を除去する (詰めない)
		RemoveImpl( iBgn, iEnd );

		// 区間に収まらない分の空間を確保する (初期化しない)
		InsertImpl( iEnd, iBgn + cntElems, false );

	} else if ( lenRange > cntElems ) {
		size_t const lenShrink = lenRange - cntElems;
		RemoveImpl(iBgn, iEnd - lenShrink);

		// 区間の方が大きいなら、縮めておく
		Remove(iEnd - lenShrink, iEnd);
	}
	return;
}

//##########################################################
//        順番操作
//##########################################################
// 移動
void CVector::Move( size_t iDst, size_t iSrc )
{
	if ( Size() < 2 || iDst == iSrc ) return;
	if ( !(IsValid(iDst) && IsValid(iSrc)) ) throw std::bad_exception("Invalid vector-index(es)");

	// 移動元の値を保存する
	PVal* const tmp = AtUnsafe(iSrc);

	// 移動する
	if ( iSrc < iDst ) {
		MemMove( iSrc, iSrc + 1, (iDst - iSrc) );	// 前 → 後
	} else {
		MemMove( iDst + 1, iDst, (iSrc - iDst) );	// 前 ← 後
	}

	AtUnsafe(iDst) = tmp;
	return;
}

// 交換
void CVector::Swap( size_t idx1, size_t idx2 )
{
	if ( Size() < 2 || idx1 == idx2 ) return;
	if ( !(IsValid(idx1) && IsValid(idx2)) ) throw std::bad_exception("Invalid vector-index(es)");

	std::swap( AtUnsafe(idx1), AtUnsafe(idx2) );
	return;
}

// 巡回
void CVector::Rotate( int step_ )
{
	size_t const len = Size();
	size_t const step = ((step_ % len) + len) % len;	// [0, len) に納まる剰余
	if ( len < 2 || step == 0 ) return;

	std::rotate( begin(), (begin() + step), end() );
	return;
}

// 反転
void CVector::Reverse()
{
	if ( Size() < 2 ) return;
	std::reverse( begin(), end() );		// 全区間
	return;
}

void CVector::Reverse( size_t iBgn, size_t iEnd )
{
	if ( Size() < 2 || iBgn == iEnd ) return;

	if ( iBgn > iEnd ) {
		if ( iEnd < 2 ) throw std::bad_exception("Invalid vector-reverse interval");
		return Reverse( iEnd + 1, iBgn + 1 );			// assert( iBgn < iEnd ), assert( 0 < iEnd )
	}
	if ( IsValid(iEnd - 1) ) throw std::bad_exception("Invalid vector-reverse interval");

	std::reverse( (begin() + iBgn), (begin() + iEnd) );	// 区間 [iBgn, iEnd)
	return;
}
