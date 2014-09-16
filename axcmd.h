// AxCmd
// @ HSP中間コードの1単位と同じ情報量を持つ32bit整数値

#ifndef IG_AX_CMD_H
#define IG_AX_CMD_H

#include "hsp3plugin.h"

namespace AxCmd
{

static const int MagicCode = 0x20000000;

inline int make( int type, int code )
{
	return ( MagicCode | ((type & 0x0FFF) << 16) | (code & 0xFFFF) );
}

inline int getType( int id )
{
	return ( (id & MagicCode) ? ((id >> 16) & 0x0FFF) : 0 );
}

inline int getCode( int id )
{
	return ( (id & MagicCode) ? (id & 0xFFFF) : 0 );
}

inline bool isOk(int axcmd)
{
	return ((axcmd & MagicCode) != 0);
}

};

//------------------------------------------------
// 式から axcmd を取り出す
//------------------------------------------------
inline int code_get_axcmd()
{
	int const axcmd = AxCmd::make(*type, *val);		// MagicCode つき
	code_next();

	// 次が文頭や式頭ではなく、')' でもない → 与えられた引数式が2字句以上でできている
	if ( !((*exinfo->npexflg & (EXFLG_1 | EXFLG_2)) != 0 || (*type == TYPE_MARK && *val == ')')) )
		puterror(HSPERR_INVALID_PARAMETER);
	return axcmd;
}

#endif
