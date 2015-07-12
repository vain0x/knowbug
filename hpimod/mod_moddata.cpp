// #module 関係

#include <cstring>

#include "mod_moddata.h"
#include "mod_makepval.h"
//#include "stack.h"			// hsp3/stack.h

namespace hpimod {

//------------------------------------------------
// ユーザ定義コマンドの実引数列を取り出す
// 
// @ 引数を prmstk に格納。
//------------------------------------------------
void code_expandstruct(void* prmstk, stdat_t stdat, int option)
{
	stprm_t pStPrm = getSTRUCTPRM( stdat->prmindex );
	
	for ( int i = 0; i < stdat->prmmax; ++ i, ++ pStPrm ) {
		char* const out = reinterpret_cast<char*>(prmstk) + pStPrm->offset;	// スタックの次のポインタ
		
		switch ( pStPrm->mptype ) {
			// int, double, label
			case MPTYPE_INUM:  *reinterpret_cast<int*>(out)     = code_getdi(0); break;
			case MPTYPE_DNUM:  *reinterpret_cast<double*>(out)  = code_getd();   break;
			case MPTYPE_LABEL: *reinterpret_cast<label_t*>(out) = code_getlb();  break;
			
			// str
			case MPTYPE_LOCALSTRING:
			{
				char* const str = code_gets();
				char* const ls  = hspmalloc( (std::strlen(str) + 1) * sizeof(char) );
				std::strcpy( ls, str );
				*reinterpret_cast<char**>(out) = ls;
				break;
			}
			// modvar
			case MPTYPE_MODULEVAR:
			{
				auto const var = reinterpret_cast<MPModVarData*>(out);
				
				var->magic = MODVAR_MAGICCODE;		// マジックコード
				var->subid = pStPrm->subid;			// modvar の種類 Id
				var->aptr  = code_getva( &var->pval );
				break;
			}
			// modinit, modterm => エミュレート不可能
			case MPTYPE_IMODULEVAR:
			case MPTYPE_TMODULEVAR:
				puterror( HSPERR_UNSUPPORTED_FUNCTION );
			//	*(MPModVarData *)out = modvar_init;
				break;
				
			// 参照(var, array)
			case MPTYPE_SINGLEVAR:
			case MPTYPE_ARRAYVAR:
			{
				auto const var = reinterpret_cast<MPVarData*>(out);
				var->aptr = code_getva( &var->pval );
				break;
			}
			// ローカル変数(local)
			case MPTYPE_LOCALVAR:
			{
				PVal* const pval = reinterpret_cast<PVal*>(out);
				
				// ローカル変数の準備も行う場合 (いわゆる variant 引数)
				if ( option & CODE_EXPANDSTRUCT_OPT_LOCALVAR ) {
					int const prm = code_getprm();
					
					if ( prm > PARAM_END ) {	// 成功
						PVal_init( pval, mpval->flag );		// 最小サイズを確保
						PVal_assign( pval, mpval->pt, mpval->flag );
						break;
					}
				//	else	// 省略時
				}
				
				PVal_init( pval, HSPVAR_FLAG_INT );	// 仮の型で初期化
				break;
			}
			
			// 構造体タグ
		//	case MPTYPE_STRUCTTAG: break;
				
			default:
				puterror( HSPERR_INVALID_STRUCT_SOURCE );
		}
	}
	return;
}

//------------------------------------------------
// prmstack を解体する
// 
// @cf. openhsp: hsp3code.cpp/customstack_delete
//------------------------------------------------
void customstack_delete( stdat_t stdat, void* prmstk )
{
	stprm_t pStPrm = getSTRUCTPRM( stdat->prmindex );
	for ( int i = 0; i < stdat->prmmax; ++ i, ++ pStPrm ) {		// パラメーターを取得
		char* const out = reinterpret_cast<char*>(prmstk) + pStPrm->offset;
		
		switch ( pStPrm->mptype ) {
			case MPTYPE_LOCALSTRING:
			{
				char* const ls = *reinterpret_cast<char**>(out);
				hspfree(ls);
				break;
			}
			case MPTYPE_LOCALVAR:
				PVal_free( reinterpret_cast<PVal*>(out) );
				break;
		}
	}
	return;
}


} // namespace hpimod
