// PValHolder
#if 0
#ifndef IG_HPIMOD_PVAL_HOLDER_H
#define IG_HPIMOD_PVAL_HOLDER_H

#include "mod_makePVal.h"

//------------------------------------------------
// 変数ホルダー
//------------------------------------------------
class PValHolder
{
public:
	PValHolder( PVal* pval )
	{
		initialize( pval->flag );
		PVal_copy( get(), pval );
	}
	PValHolder( const void* pt, vartype_t type )
	{
		initialize( type );
		PVal_assign( get(), pt, type );
	}
	~PValHolder()
	{
		PVal_free( get() );
	}
	
	PVal* get() { return &mVar; }
	const PVal* get() const { return &mVar; }
	
	PVal* operator ->() { return get(); }
	const PVal* operator ->() const { return get(); }
	
	void RestoreTo( PVal* dst ) { std::swap( *dst, *get() ); }
	
private:
	void initialize( vartype_t type )
	{
		PVal_init( get(), type );
	}
private:
	PVal mVar;		// 本体
	
};

#endif
#endif