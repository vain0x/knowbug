// opex - command header

#ifndef IG_COMMAND_OPEX_H
#define IG_COMMAND_OPEX_H

#include "hsp3plugin_custom.h"

extern void assign();
extern int  assign( void** ppResult );

extern void swap();
extern int  swap( void** ppResult );

extern void clone();
extern int  clone( void** ppResult );

extern int  castTo( void** ppResult );

extern void memberOf();
extern int  memberOf(void** ppResult);
extern void memberClone();

extern int shortLogOp( void** ppResult, bool bAnd );
extern int   cmpLogOp( void** ppResult, bool bAnd );

extern int which(void** ppResult);
extern int what(void** ppResult);

extern int exprs( void** ppResult );

extern int kw_constptr( void** ppResult );

// íËêî
enum OPTYPE
{
	OPTYPE_MIN = 0,
	OPTYPE_ADD = OPTYPE_MIN,
	OPTYPE_SUB,
	OPTYPE_MUL,
	OPTYPE_DIV,
	OPTYPE_MOD,
	OPTYPE_AND,
	OPTYPE_OR,
	OPTYPE_XOR,
	OPTYPE_EQ,
	OPTYPE_NE,
	OPTYPE_GT,
	OPTYPE_LT,
	OPTYPE_GTEQ,
	OPTYPE_LTEQ,
	OPTYPE_RR,
	OPTYPE_LR,
	OPTYPE_MAX
};

namespace OpexCmd
{
	int const ConstPtr = 0x200;
}

#endif
