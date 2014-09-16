// Assoc 実体データクラス

#ifndef IG_CLASS_ASSOC_H
#define IG_CLASS_ASSOC_H

#include <map>
#include <string>

#include "hsp3plugin_custom.h"

//	#define DBGOUT_ASSOC_ADDREF_OR_RELEASE	// AddRef, Release を dbgout で報告する

//------------------------------------------------
// assoc の実体データのクラス
//------------------------------------------------
class CAssoc
{
public:
	using Key_t = std::string;
	using Map_t = std::map<Key_t, PVal*>;

	static int const HSPVAR_SUPPORT_USER_ELEM = HSPVAR_SUPPORT_USER1;	// 要素として生成された PVal*

	//--------------------------------------------
	// メンバ変数
	//--------------------------------------------
private:
	Map_t map_;		// 実データ
	int cnt_;		// 参照カウンタ (0以下で死亡)

	//--------------------------------------------
	// メンバ関数
	//--------------------------------------------
public:
	static CAssoc* New();

	PVal* At    ( Key_t const& key );
	PVal* AtSafe( Key_t const& key ) const;

	void  Chain ( CAssoc const* src );
	void  Copy  ( CAssoc const* src ) { Clear(); Chain(src); }
	void  Clear();

	PVal* Insert( Key_t const& key );
	PVal* Insert( Key_t const& key, PVal* pval );
	void  Remove( Key_t const& key );

	size_t Size() const { return map_.size(); }
	bool Empty() const  { return map_.empty(); }
	bool Exists( Key_t const& key ) const;

	Map_t::iterator begin() { return map_.begin(); }
	Map_t::iterator   end() { return map_.end(); }

private:
	PVal* NewElem();
	PVal* NewElem( int vflag );
	PVal* ImportElem( PVal* pval );
	void DeleteElem( PVal* pval );

	static void Delete( CAssoc* self );

#ifdef DBGOUT_ASSOC_ADDREF_OR_RELEASE
private:
	int id_;
public:
	void AddRef()  { cnt_ ++; dbgout("[%d] ++ → %d", id_, cnt_); }
	void Release() { cnt_ --; dbgout("[%d] -- → %d", id_, cnt_); if ( cnt_ == 0 ) { Delete(this); } }
#else
public:
	void AddRef()  { cnt_ ++; }
	void Release() { cnt_ --; if ( cnt_ == 0 ) { Delete(this); } }
#endif
public:
	static void AddRef ( CAssoc* self ) { if (self) self->AddRef();  }
	static void Release( CAssoc* self ) { if (self) self->Release(); } 

private:
	CAssoc();
	~CAssoc();

	CAssoc( CAssoc const& );
	CAssoc& operator =(CAssoc const& );
};

//*
//------------------------------------------------
// 引数として受け取った CAssoc* を所有するクラス
// 
// @ 一時変数に生成された assoc の次に引数がある場合、
// @	mpval が所有権を失って assoc が消滅するため、
// @	mpval に代わってローカルで assoc を所有する。
// @ex: AssocForeachNext
//------------------------------------------------
class CAssocHolder
{
private:
	CAssoc* const inst_;

public:
	CAssocHolder( CAssoc* inst )
		: inst_(inst)
	{
		CAssoc::AddRef(inst_);
	}
	~CAssocHolder()
	{
		CAssoc::Release( inst_ );
	}

	CAssoc* get() { return inst_; }
	CAssoc const* get() const { return inst_; }

	operator CAssoc*() { return get(); }
	operator CAssoc const*() const { return get(); }

	CAssoc* operator ->() { return inst_; }
	CAssoc const* operator ->() const { return inst_; }

private:
	CAssocHolder();
	CAssocHolder( CAssocHolder const& );
	CAssocHolder& operator =( CAssocHolder& );
};
//*/

#endif
