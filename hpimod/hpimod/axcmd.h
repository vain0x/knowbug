// AxCmd
// HSP中間コードの1単位と同じ情報量を持ちたい32bit整数値
// TYPE_INT は code 値に 32bit 使うので所持できない。

#ifndef IG_AX_CMD_H
#define IG_AX_CMD_H

#include "hsp3plugin_custom.h"
#include "mod_argGetter.h"

namespace AxCmd
{

static int const MagicCode  = 0x40000000;
static int const Mask_Magic = 0xC;
static int const Mask_Type  = 0x3F;
static int const Mask_Code  = 0x00FFFFFF;

inline int make( int type, int code )
{
	assert(((type &~ Mask_Type) == 0)
		&& ((code & ~Mask_Code) == 0));
	return (MagicCode | ((type & Mask_Type) << 24) | (code & Mask_Code));
}

inline bool isOk(int axcmd)
{
	return ((axcmd & MagicCode) != 0);
}

inline int getType( int id )
{
	assert(isOk(id));
	return ((id >> 24) & Mask_Type);
}

inline int getCode( int id )
{
	assert(isOk(id));
	return (id & Mask_Code);
}

};

//------------------------------------------------
// 式から axcmd を取り出す
//------------------------------------------------
static inline int code_get_axcmd()
{
	if ( *type == TYPE_INUM ) puterror(HSPERR_TYPE_MISMATCH);

	int const axcmd = AxCmd::make(*type, *val);		// MagicCode つき
	hpimod::code_get_singleToken();
	return axcmd;
}

#endif
