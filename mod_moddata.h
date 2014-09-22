// #module 関係の処理

#ifndef IG_HPIMOD_MODULE_MODULE_DATA_H
#define IG_HPIMOD_MODULE_MODULE_DATA_H

#include "hsp3plugin_custom.h"

namespace hpimod {

extern void code_expandstruct(void* prmstk, stdat_t stdat, int option);
extern void customstack_delete(stdat_t stdat, void* prmstk);

// openhsp から引用
#define CODE_EXPANDSTRUCT_OPT_NONE 0
#define CODE_EXPANDSTRUCT_OPT_LOCALVAR 1

} // namespace hpimod

#endif
