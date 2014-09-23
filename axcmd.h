// AxCmd
// HSP中間コードの1単位と同じ情報量を持ちたい32bit整数値
// TYPE_INT は code 値に 32bit 使うので所持できない。

#ifndef IG_AX_CMD_H
#define IG_AX_CMD_H

#include "hsp3plugin_custom.h"
#include "mod_argGetter.h"

namespace AxCmd
{

static int const MagicCode = 0x20000000;

inline int make( int type, int code )
{
	assert(type != TYPE_INT);
	return ( MagicCode | ((type & 0xFF) << 24) | (code & 0xFFFFFF) );
}

inline int getType( int id )
{
	return ( (id & MagicCode) ? ((id >> 24) & 0xFF) : 0 );
}

inline int getCode( int id )
{
	return ( (id & MagicCode) ? (id & 0xFFFFFF) : 0 );
}

inline bool isOk(int axcmd)
{
	return ((axcmd & MagicCode) != 0);
}

};

//------------------------------------------------
// 式から axcmd を取り出す
//------------------------------------------------
static inline int code_get_axcmd()
{
	if ( *type == TYPE_INT ) puterror(HSPERR_TYPE_MISMATCH);

	int const axcmd = AxCmd::make(*type, *val);		// MagicCode つき
	hpimod::code_get_singleToken();
	return axcmd;
}

#endif
