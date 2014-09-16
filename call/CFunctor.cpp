// 関数子オブジェクト

#include "CCaller.h"
#include "CBound.h"
#include "CStreamCaller.h"
#include "CLambda.h"

#include "CFunctor.h"

#include "cmd_sub.h"

#include <vector>
#include <algorithm>
#include <typeinfo>

using namespace hpimod;

//------------------------------------------------
// 構築 (AxCmd)
// 
// @ axcmd
//------------------------------------------------
CFunctor::CFunctor( int _axcmd )
	 : type( FuncType_Deffid )
	 , deffid( 0 )
{
	assert( AxCmd::isOk(_axcmd) );

	switch ( AxCmd::getType(_axcmd) ) {
		case TYPE_LABEL:
		{
			type = FuncType_Label;
			lb   = hpimod::getLabel( AxCmd::getCode(_axcmd) );
			break;
		}
		case TYPE_MODCMD:
			deffid = AxCmd::getCode( _axcmd );
			break;
		default: puterror( HSPERR_TYPE_MISMATCH );
	}

	return;
}

//------------------------------------------------
// 構築 (特殊関数子)
//------------------------------------------------
CFunctor::CFunctor( exfunctor_t _ex )
	: type( FuncType_Ex )
	, ex( _ex )
{
	if ( !ex ) type = FuncType_None;
	ex->AddRef();
	return;
}

//------------------------------------------------
// 破棄
//------------------------------------------------
CFunctor::~CFunctor()
{
	clear();
	return;
}

//------------------------------------------------
// 初期化
//------------------------------------------------
void CFunctor::clear()
{
	switch ( type ) {
		case FuncType_None: return;
		case FuncType_Label:
		case FuncType_Deffid:
			break;
		case FuncType_Ex:
			FunctorEx_Release( ex );
			break;
		default: throw;
	}

	type = FuncType_None;
	return;
}

//------------------------------------------------
// 使用状況
//------------------------------------------------
int CFunctor::getUsing() const
{
	switch ( type ) {
		case FuncType_None:   return 0;
		case FuncType_Label:  return ( lb ? 1 : 0 );
		case FuncType_Deffid: return 1;
		case FuncType_Ex:     return ( ex ? ex->getUsing() : 0 );
		default:
			throw;
	}
}

//------------------------------------------------
// 仮引数リストを得る
//------------------------------------------------
CPrmInfo const& CFunctor::getPrmInfo() const
{
	switch ( type ) {
		case FuncType_Label:  return GetPrmInfo( lb );
		case FuncType_Deffid: return GetPrmInfo( getSTRUCTDAT(deffid) );
		case FuncType_Ex:     return ex->getPrmInfo();
		default: throw;
	}
}

//------------------------------------------------
// ラベル取得
//------------------------------------------------
label_t CFunctor::getLabel() const
{
	switch ( type ) {
		case FuncType_Label:  return lb;
		case FuncType_Deffid: return hpimod::getLabel( getSTRUCTDAT(deffid)->otindex );
		case FuncType_Ex:     return ex->getLabel();
		default: throw;
	}
//	return nullptr;
}

//------------------------------------------------
// 定義IDを得る
//------------------------------------------------
int CFunctor::getAxCmd() const
{
	switch ( type ) {
		case FuncType_Label:  break;
		case FuncType_Deffid: return AxCmd::make( TYPE_MODCMD, deffid );
		case FuncType_Ex:     return ex->getAxCmd();
		default: throw;
	}
	return 0;
}

//------------------------------------------------
// 
//------------------------------------------------

//------------------------------------------------
// 呼び出す
//------------------------------------------------
void CFunctor::call( CCaller& caller )
{
	switch ( type ) {
		case FuncType_Label:
		case FuncType_Deffid:
			return caller.getCall().callLabel( getLabel() );

		case FuncType_Ex:
			return ex->call( caller );

		default:
			return puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}
}

//------------------------------------------------
// 
//------------------------------------------------


//------------------------------------------------
// 比較
// 
// @ 大小関係を一意に決定できればよい。
//------------------------------------------------
int CFunctor::compare( CFunctor const& obj ) const
{
	if ( type != obj.type ) return type - obj.type;

	switch ( type ) {
		case FuncType_Label:  return ( lb     - obj.lb );
		case FuncType_Deffid: return ( deffid - obj.deffid );
		case FuncType_Ex:
			{
				// 実際の型の違いを優先
				auto& t0 = typeid(*ex);
				auto& t1 = typeid(*obj.ex);
				if ( t0 != t1 ) return t0.hash_code() - t1.hash_code();
			}
			return ex->compare( obj.ex );
		default:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );
			throw;
	}
}

//------------------------------------------------
// 複写
//------------------------------------------------
CFunctor& CFunctor::copy( CFunctor const& src )
{
	clear();

	if ( src.getUsing() == 0 ) {
		return *this;
	}

	type = src.type;

	switch ( type ) {
		case FuncType_Label:  lb     = src.lb;     break;
		case FuncType_Deffid: deffid = src.deffid; break;
		case FuncType_Ex:
			ex = src.ex;
			FunctorEx_AddRef( ex );
			break;
		default: throw;
	}

	return *this;
}
