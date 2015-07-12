// hsp code

#ifndef IG_CLASS_HSP_CODE_H
#define IG_CLASS_HSP_CODE_H

#include <memory>
#include <vector>

#include "hsp3plugin_custom.h"
#include "HspAllocator.h"

namespace hpimod
{

namespace Detail {
	template<typename T> using myvector_t = std::vector<T, HspAllocator<T>>;

	class AlignedPlainStorage
	{
		using buf_t = myvector_t<char>;
		myvector_t<buf_t> list_;

		buf_t& allocNew(size_t size);
	public:
		int push(size_t size, size_t alignment, void const* _base);
	};
}

class CHspCode
{
	using label_t = hpimod::label_t;
	using code_t = unsigned short;

public:
	CHspCode()
		: code_()
		, dslist_()
		, len_ { 0 }
	{
		code_.reserve(64);
	}

	// 取得系
	// Remark: code の増減により label の値は変化しうる。offset は変化しない。
	label_t getlb() const { return code_.data(); }
	size_t getCurrentOffset() const { return len_; }

	// 追加系
	void put( int _type, int _code, int _exflg );

	void putVal( label_t     v, int exflg = 0 ) { put( TYPE_LABEL,  reinterpret_cast<int>(v), exflg ); }
	void putVal( int         v, int exflg = 0 ) { put( TYPE_INUM,   v,           exflg ); }
	void putVal( char const* v, int exflg = 0 ) { put( TYPE_STRING, putDsVal(v), exflg ); }
	void putVal( double      v, int exflg = 0 ) { put( TYPE_DNUM,   putDsVal(v), exflg ); }
	void putVar( PVal const* v, int exflg = 0 );

	void putOmt() { put( TYPE_MARK, '?', EXFLG_2 ); }
	void putReturn() { put( TYPE_PROGCMD, 0x002, EXFLG_1 ); }

private:
	// CS バッファの確保要求
	void needCsBuf( size_t sizeAppend );

	int putDsVal(char const* str);
	int putDsVal(double d);
public:
	int putDsStPrm(STRUCTPRM const& stprm);

private:
	Detail::myvector_t<code_t> code_;
	size_t len_;

	Detail::AlignedPlainStorage dslist_;
};

} // namespace hpimod

#endif

// 余りもの
#if 0
{
	// (*type == TYPE_STRUCT)
	// 構造体パラメータが実際に指しているものをコードに追加する
	auto const pStPrm = &ctx->mem_minfo[ *val ];			
	char* out = (char*)ctx->prmstack;
	if ( !out ) puterror( HSPERR_ILLEGAL_FUNCTION );
	if ( pStPrm->subid != STRUCTPRM_SUBID_STACK ) {		// メンバ変数
		auto thismod = (MPModVarData*)out;
		out = (char*)((FlexValue*)PVal_getptr( thismod->pval, thismod->aptr ))->ptr;
	}
	;
	// 引数を展開
	([&body]( char* ptr, int mptype ) {
		switch ( mptype ) {
			case MPTYPE_SINGLEVAR:		// 変数要素 => 変数(aptr)の形で出力
			{
				auto vardata = reinterpret_cast<MPVarData*>(ptr);
				auto pval    = vardata->pval;
				body.putVar( pval );
				;
				if ( vardata->aptr != 0 ) {
					body.put( TYPE_MARK, '(', 0 );
					if ( pval->len[2] == 0 ) {
						// 一次元
						body.putVal( vardata->aptr );
					} else {
						// 多次元 => APTR分解
						int idx[4] = {0};
						GetIndexFromAptr( pval, vardata->aptr, idx );
						for ( int i = 0; i < 4 && idx[i] != 0; ++ i ) {
							body.putVal( idx[i], (i == 0 ? 0 : EXFLG_2) );
						}
					}
					body.put( TYPE_MARK, ')', 0 );
				}
				break;
			}
			case MPTYPE_ARRAYVAR:
				return body.putVar( reinterpret_cast<MPVarData*>(ptr)->pval );
			case MPTYPE_LOCALVAR:
				return body.putVar( reinterpret_cast<PVal*>(ptr) );
			;
			case MPTYPE_LABEL:       return body.putVal( *(label_t*)ptr );
			case MPTYPE_LOCALSTRING: return body.putVal( *(char**)ptr );
			case MPTYPE_DNUM: return body.putVal( *reinterpret_cast<double*>(ptr) );
			case MPTYPE_INUM: return body.putVal( *reinterpret_cast<int*>(ptr) );
			;
			default:
				dbgout("mptype = %d", mptype );
				break;
		};
	})( out + pStPrm->offset, pStPrm->mptype );
	;
	code_next();
}

#endif

