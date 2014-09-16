// CAssoc 実装

#include "CAssoc.h"
#include "mod_makepval.h"

using namespace hpimod;

//------------------------------------------------
// 生成
//------------------------------------------------
CAssoc::CAssoc()
	: map_()
	, cnt_(0)
{
#ifdef DBGOUT_ASSOC_ADDREF_OR_RELEASE
	static int stt_counter = 1;
	id_ = stt_counter ++;
//	dbgout("[%d] new", id_);
#endif
	return;
}

CAssoc* CAssoc::New()
{
	return new CAssoc();
}

//------------------------------------------------
// 破棄
//------------------------------------------------
CAssoc::~CAssoc()
{
	Clear();
	return;
}

void CAssoc::Delete( CAssoc* self )
{
	delete self;
	return;
}

//------------------------------------------------
// 要素アクセス
//------------------------------------------------
PVal* CAssoc::AtSafe( Key_t const& key ) const
{
	auto iter = map_.find( key );
	return ( iter != map_.end() )
		? iter->second
		: nullptr;
}

// 存在しなければ新規要素を挿入する
PVal* CAssoc::At( Key_t const& key )
{
	return Insert( key );
}

//------------------------------------------------
// 要素の存在
//------------------------------------------------
bool CAssoc::Exists( Key_t const& key ) const
{
	return (map_.find(key) != map_.end());
}

//------------------------------------------------
// 要素の生成・削除
//------------------------------------------------
PVal* CAssoc::NewElem()
{
	PVal* pval = (PVal*)hspmalloc( sizeof(PVal) );
	// HSPVAR_SUPPORT_USER_ELEM の付加が義務
	return pval;
}

PVal* CAssoc::NewElem( int vflag )
{
	PVal* pval = NewElem();
	PVal_init( pval, vflag );
	pval->support |= HSPVAR_SUPPORT_USER_ELEM;
	return pval;
}

PVal* CAssoc::ImportElem( PVal* pval )
{
	return pval;
}

void CAssoc::DeleteElem( PVal* pval )
{
	if ( pval->support & HSPVAR_SUPPORT_USER_ELEM ) {
		PVal_free( pval );
		hspfree( pval );
	}
	return;
}

//------------------------------------------------
// 要素の挿入・除去
//------------------------------------------------
PVal* CAssoc::Insert( Key_t const& key )
{
	PVal* pvElem = AtSafe(key);
	if ( pvElem ) return pvElem;

	PVal* const pval = NewElem(HSPVAR_FLAG_INT);
	map_.insert({ key, pval });
	return pval;
}

PVal* CAssoc::Insert( Key_t const& key, PVal* pval )
{
	PVal* pvElem = AtSafe(key);
	if ( pvElem || !pval ) return pvElem;

	map_.insert({ key, pval });
	return pval;
}

void CAssoc::Remove( Key_t const& key )
{
	DeleteElem( AtSafe(key) );
	map_.erase( key );
	return;
}

//------------------------------------------------
// assoc 連結
//
// @ 重複要素は上書きする
//------------------------------------------------
void CAssoc::Chain( CAssoc const* src )
{
	if ( !src ) return;

	for ( auto iter : map_ ) {
		if ( !iter.second ) continue;

		// 重複 => 既存要素を除去する
		auto elem = map_.find( iter.first );
		if ( elem != map_.end() ) {
			Remove( iter.first );
		}

		// 内部変数の複製を作る
		PVal* pvElem;
		if ( iter.second->support & HSPVAR_SUPPORT_USER_ELEM ) {
			pvElem = NewElem( HSPVAR_FLAG_INT );		// 軽量初期化
			PVal_copy( pvElem, iter.second );			// 変数は再確保される
			pvElem->support |= HSPVAR_SUPPORT_USER_ELEM;
		} else {
			pvElem = iter.second;		// 共有
		}

		// キーの同じ要素を生成する
		Insert( iter.first, pvElem );
	}

	return;
}

//------------------------------------------------
// assoc 消去
//
// @ 全ての要素を取り除く。
//------------------------------------------------
void CAssoc::Clear()
{
	for ( auto iter : *this ) {
		DeleteElem( iter.second );
	}
	map_.clear();
	return;
}
