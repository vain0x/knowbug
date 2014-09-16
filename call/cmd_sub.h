// Call - SubCommand header

#ifndef IG_CALL_SUB_COMMAND_H
#define IG_CALL_SUB_COMMAND_H

#include "hsp3plugin_custom.h"
#include "mod_argGetter.h"
#include "mod_makepval.h"
#include "axcmd.h"

#include "CCall.h"
#include "CFunctor.h"
#include "cmd_call.h"

#include "CPrmInfo.h"

//################################################
//    Call â∫êøÇØä÷êî
//################################################
// âºà¯êîÉäÉXÉgä÷òA
extern void DeclarePrmInfo(label_t lb, CPrmInfo&& prminfo);
extern CPrmInfo const& GetPrmInfo(label_t);
extern CPrmInfo const& GetPrmInfo(stdat_t);

extern int code_get_prmtype(int deftype = PRM_TYPE_NONE);
extern CPrmInfo::prmlist_t code_get_prmlist();
extern CPrmInfo code_get_prminfo();

// ÇªÇÃëº
extern int GetPrmType( char const* s );

extern void* GetReferedPrmstk( stprm_t pStPrm);

//################################################
//    â∫êøÇØä÷êî
//################################################
template<class T>
bool numrg(T const& val, T const& min, T const& max)
{
	return (min <= val && val <= max);
}

// à¯êîéÊìæ
extern CFunctor code_get_functor();

#endif
