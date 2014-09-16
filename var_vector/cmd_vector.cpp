// vector - Command code

#include <functional>

#include "vt_vector.h"
#include "cmd_vector.h"
#include "sub_vector.h"

#include "mod_makepval.h"
#include "mod_argGetter.h"
#include "mod_func_result.h"
#include "mod_varutil.h"

using namespace hpimod;

static vector_t g_pResultVector { nullptr };

static void VectorMovingImpl( vector_t& self, int cmd );

//------------------------------------------------
// vector 型の値を返却する
// 
// @ そのまま返却するとスタックに乗る。
//------------------------------------------------
int SetReffuncResult( void** ppResult, vector_t self )
{
	self.beTmpObj();

	g_pResultVector = std::move(self);
	*ppResult = &g_pResultVector;
	return g_vtVector;
}

//------------------------------------------------
// vector 型の値を受け取る
//------------------------------------------------
vector_t code_get_vector()
{
	if ( code_getprm() <= PARAM_END ) puterror( HSPERR_NO_DEFAULT );
	if ( mpval->flag != g_vtVector ) puterror( HSPERR_TYPE_MISMATCH );
	return std::move(VectorTraits::getMaster(mpval));
}

//------------------------------------------------
// vector の内部変数を受け取る
//------------------------------------------------
PVal* code_get_vector_inner()
{
	PVal* const pval = code_get_var();
	if ( pval->flag != g_vtVector ) puterror( HSPERR_TYPE_MISMATCH );

	PVal* const pvInner = VectorTraits::getInnerPVal( pval );
	if ( !pvInner ) puterror( HSPERR_VARIABLE_REQUIRED );
	return pvInner;
}

//------------------------------------------------
// vector の範囲を取り出す
//------------------------------------------------
std::pair<size_t, size_t> code_get_vector_range(vector_t const& self)
{
	bool const bNull = self.isNull();

	int const iBgn = code_getdi(0);
	int const iEnd = code_getdi((!bNull ? self->size() : 0));
	if ( (!bNull && !isValidRange(self, iBgn, iEnd))
		|| (bNull && (iBgn != 0 || iEnd != 0)) ) puterror(HSPERR_ILLEGAL_FUNCTION);
	return { iBgn, iEnd };
}

//#########################################################
//        命令
//#########################################################
//------------------------------------------------
// 破棄
//------------------------------------------------
void VectorDelete()
{
	PVal* const pval = code_get_var();
	if ( pval->flag != g_vtVector ) puterror(HSPERR_TYPE_MISMATCH);

	VectorTraits::getMaster(pval).clear();
	return;
}

//------------------------------------------------
// リテラル式生成
//------------------------------------------------
int VectorMake(void** ppResult)
{
	auto&& self = vector_t::make();

	for ( size_t i = 0; code_isNextArg(); ++i ) {
		int const chk = code_getprm();
		assert(chk != PARAM_END && chk != PARAM_ENDSPLIT);

		self->resize(i + 1);
		if ( chk != PARAM_DEFAULT ) {
			PVal* const pvInner = self->at(i).valuePtr();
			PVal_assign(pvInner, mpval->pt, mpval->flag);
		}
		// else: 初期値のまま
	}

	return SetReffuncResult(ppResult, self);
}

//------------------------------------------------
// スライス
//------------------------------------------------
int VectorSlice(void** ppResult)
{
	auto&& self = code_get_vector();
	auto const&& range = code_get_vector_range(self);

	auto result = vector_t::make();
	chainShallow(result, self, range);
	return SetReffuncResult(ppResult, std::move(result));
}

//------------------------------------------------
// スライス・アウト
//------------------------------------------------
int VectorSliceOut(void** ppResult)
{
	auto&& self = code_get_vector();
	auto const&& range = code_get_vector_range(self);

	size_t const len = self->size();
	size_t const lenRange = range.second - range.first;

	auto result = vector_t::make();
	if ( len > lenRange ) {
		result->reserve(len - lenRange);
		chainShallow(result, self, { 0, range.first });
		chainShallow(result, self, { range.second, len });
	}
	return SetReffuncResult(ppResult, std::move(result));
}

//------------------------------------------------
// 複製
//------------------------------------------------
int VectorDup(void** ppResult)
{
	auto&& src = code_get_vector();
	auto&& range = code_get_vector_range(src);

	// PVal の値を複製して vector をもう一つ作る
	auto self = vector_t::make();

	chainDeep(self, src, range);
	return SetReffuncResult(ppResult, self);
}

//------------------------------------------------
// vector の情報
//------------------------------------------------
int VectorIsNull(void** ppResult)
{
	auto&& self = code_get_vector();
	return SetReffuncResult(ppResult, HspBool(!self.isNull()));
}

int VectorSize(void** ppResult)
{
	auto&& self = code_get_vector();
	assert(!self.isNull());

	return SetReffuncResult(ppResult, static_cast<int>(self->size()));
}

//------------------------------------------------
// 内部変数へのあれこれ
//------------------------------------------------
void VectorDimtype()
{
	PVal* const pvInner = code_get_vector_inner();
	code_dimtype(pvInner);
	return;
}

void VectorClone()
{
	PVal* const pvalSrc = code_get_vector_inner();
	PVal* const pvalDst = code_getpval();

	PVal_cloneVar( pvalDst, pvalSrc );
	return;
}

int VectorVarinfo(void** ppResult)
{
	PVal* const pvInner = code_get_vector_inner();
	return SetReffuncResult(ppResult, code_varinfo(pvInner));
}

//#########################################################
//        コンテナ操作
//#########################################################
//------------------------------------------------
// 連結
//------------------------------------------------
void VectorChain(bool bClear)
{
	auto&& dst = code_get_vector();
	auto&& src = code_get_vector();

	if ( bClear ) dst->clear();

	auto const range = code_get_vector_range(src);
	chainDeep(dst, src, range);
	return;
}

#if 0

//------------------------------------------------
// コンテナ操作処理テンプレート
//------------------------------------------------
// 難しい

//------------------------------------------------
// 要素順序
//------------------------------------------------
void VectorMoving( int cmd )
{
	PVal* const pval = code_get_var();
	if ( pval->flag != g_vtVector ) puterror( HSPERR_TYPE_MISMATCH );

	auto& src = VectorTraits::getMaster(pval);
	if ( !src.isNull() ) puterror( HSPERR_ILLEGAL_FUNCTION );

	VectorMovingImpl( src, cmd );
	return;
}

static void VectorMovingImpl( vector_t& self, int cmd )
{
	switch ( cmd ) {
		case VectorCmdId::Move:
		{
			int const iDst = code_geti();
			int const iSrc = code_geti();
			if ( isValidIndex(self, iDst) || !isValidIndex(self, iSrc) ) puterror( HSPERR_ILLEGAL_FUNCTION );

			std::move(
				self->begin() + iSrc,
				self->begin() + (iSrc + 1),
				self->begin() + iDst
			);
			break;
		}
		case VectorCmdId::Swap:
		{
			int const idx1 = code_geti();
			int const idx2 = code_geti();

			std::swap(self->begin() + idx1, self->begin() + idx2);
			break;
		}
		case VectorCmdId::Rotate:
		{
			int const step = code_getdi(1);
			assert(false);
			//std::rotate(self->begin(), self->end());
			break;
		}
		case VectorCmdId::Reverse:
		{
			auto&& range = code_get_vector_range(self);
			std::reverse(self->begin() + range.first, self->begin() + range.second);
			break;
		}
	}
	return;
}

int VectorMovingFunc( void** ppResult, int cmd )
{
	auto&& src = code_get_vector();
	if ( !src.isNull() ) puterror( HSPERR_ILLEGAL_FUNCTION );

	// 全区間スライス
	auto self = vector_t::make();
	CVector* const self = CVector::NewTemp();
	{
		chain(self, src, { 0, src->size() });
		VectorMovingImpl( self, cmd );
	}

	return SetReffuncResult( ppResult, self );
}

//------------------------------------------------
// 要素: 追加, 除去
// 
// @t-prm idProc: ここで使うのみ。
// @	0: Insert
// @	1: Insert1
// @	2: PushFront
// @	3: PushBack
// @	4: Remove
// @	5: Remove1
// @	6: PopFront
// @	7: PopBack
// @	8: Replace
//------------------------------------------------
template<int idProc>
static void VectorElemProcImpl( CVector* self )
{
	switch ( idProc ) {
		// 区間アクセス => 区間が必要; 特に insert => 初期値リストを取る (省略可)
		case 0:
		case 4:
		{
			int const iBgn = code_geti();
			int const iEnd = code_geti();
			if ( iBgn == iEnd ) break;

			if ( idProc == 0 ) {
				self->Insert( iBgn, iEnd );

				// 初期値リスト (省略可)
				bool   const bReversed = (iBgn > iEnd);
				size_t const cntElems  = ( !bReversed ? iEnd - iBgn : iBgn - iEnd );
				for ( size_t i = 0; i < cntElems; ++ i ) {
					int const chk = code_getprm();
					if ( chk <= PARAM_END ) {
						if ( chk == PARAM_DEFAULT ) continue; else break;
					}
					PVal_assign( self->AtSafe( (!bReversed ? iBgn + i : iBgn - i) ), mpval->pt, mpval->flag );
				}

			} else {
				self->Remove( iBgn, iEnd );
			}
			break;
		}
		// 単一アクセス => 添字が必要; 特に insert1 => 初期値を取る (省略可)
		case 1:
		case 5:
		{
			int const idx = code_geti();

			if ( idProc == 1 ) {
				PVal* const pvdat = self->Insert( idx );

				// 初期値
				if ( code_getprm() > PARAM_END ) {
					PVal_assign( pvdat, mpval->pt, mpval->flag );
				}

			} else {
				self->Remove( idx );
			}
			break;
		}
		// push => 初期値を取る (省略可)
		case 2:
		case 3:
		{
			PVal* const pvdat = (idProc == 2)
				? self->PushFront()
				: self->PushBack();

			if ( code_getprm() > PARAM_END )  {
				PVal_assign( pvdat, mpval->pt, mpval->flag );
			}
			break;
		}
		// pop
		case 6: self->PopFront(); break;
		case 7: self->PopBack();  break;

		default:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}

	return;
}

// 命令
template<int idProc>
static void VectorElemProc()
{
	auto&& self = code_get_vector();
	if ( isNull( self ) ) puterror( HSPERR_ILLEGAL_FUNCTION );

	VectorElemProcImpl<idProc>( self );
	return;
}

void VectorInsert()    { VectorElemProc<0>(); }
void VectorInsert1()   { VectorElemProc<1>(); }
void VectorPushFront() { VectorElemProc<2>(); }
void VectorPushBack()  { VectorElemProc<3>(); }
void VectorRemove()    { VectorElemProc<4>(); }
void VectorRemove1()   { VectorElemProc<5>(); }
void VectorPopFront()  { VectorElemProc<6>(); }
void VectorPopBack()   { VectorElemProc<7>(); }

// 関数
template<int idProc>
static int VectorElemProc( void** ppResult )
{
	auto&& src = code_get_vector();
	if ( isNull( src ) ) puterror( HSPERR_ILLEGAL_FUNCTION );

	CVector* const self = CVector::NewTemp();		// 同値な一時オブジェクトを生成
	self->Chain( *src );

	VectorElemProcImpl<idProc>( self );

	return SetReffuncResult( ppResult, self );
}

int VectorInsert   ( void** ppResult ) { return VectorElemProc<0>( ppResult ); }
int VectorInsert1  ( void** ppResult ) { return VectorElemProc<1>( ppResult ); }
int VectorPushFront( void** ppResult ) { return VectorElemProc<2>( ppResult ); }
int VectorPushBack ( void** ppResult ) { return VectorElemProc<3>( ppResult ); }
int VectorRemove   ( void** ppResult ) { return VectorElemProc<4>( ppResult ); }
int VectorRemove1  ( void** ppResult ) { return VectorElemProc<5>( ppResult ); }
int VectorPopFront ( void** ppResult ) { return VectorElemProc<6>( ppResult ); }
int VectorPopBack  ( void** ppResult ) { return VectorElemProc<7>( ppResult ); }

//------------------------------------------------
// 要素置換
//------------------------------------------------
// 命令
void VectorReplace()
{
	PVal* const pval = code_get_var();
	if ( pval->flag != g_vtVector ) puterror( HSPERR_TYPE_MISMATCH );

	auto const self = Vector_getPtr(pval);
	if ( isNull( self ) ) puterror( HSPERR_ILLEGAL_FUNCTION );

	int const iBgn = code_getdi( 0 );
	int const iEnd = code_getdi( self->Size() );
	if ( !self->IsValid(iBgn, iEnd) ) puterror( HSPERR_ILLEGAL_FUNCTION );

	auto&& src = code_get_vector();	// null-able

	self->Replace( iBgn, iEnd, src );
	return;
}

// 関数
int VectorReplace( void** ppResult )
{
	auto&& self = code_get_vector();
	if ( isNull( self ) ) puterror( HSPERR_ILLEGAL_FUNCTION );

	int const iBgn = code_getdi(0);
	int const iEnd = code_getdi(self->Size());
	if ( !self->IsValid(iBgn, iEnd) ) puterror( HSPERR_ILLEGAL_FUNCTION );

	auto&& src = code_get_vector();	// null-able

	if ( !self->IsValid(iBgn, iEnd) ) {
		return SetReffuncResult( ppResult, CVector::Null );

	} else {
		// 同値な一時オブジェクトを生成する
		CVector* const result = CVector::NewTemp();
		{
			result->Chain(*self);
			result->Replace(iBgn, iEnd, src);
		}
		return SetReffuncResult( ppResult, result );
	}
}
#endif

//#########################################################
//        関数
//#########################################################
//------------------------------------------------
// 内部変数の情報を得る
//------------------------------------------------
//------------------------------------------------
// vector 返却関数
//------------------------------------------------
static int const VectorResultExprMagicNumber = 0x31EC100A;

int VectorResult( void** ppResult )
{
	g_pResultVector = code_get_vector();

	return SetReffuncResult(ppResult, VectorResultExprMagicNumber);
}

int VectorExpr( void** ppResult )
{
	// ここで VectorResult() が実行されるはず
	if ( code_geti() != VectorResultExprMagicNumber ) puterror(HSPERR_ILLEGAL_FUNCTION);

	return SetReffuncResult( ppResult, g_pResultVector );
}

//------------------------------------------------
// 文字列結合(Join)
//------------------------------------------------
int VectorJoin( void** ppResult )
{
	auto&& self = code_get_vector();

	char const* const _splitter = code_getds(", ");
	char splitter[0x80];
	strcpy_s(splitter, _splitter);
	size_t const lenSplitter = std::strlen(splitter);

	char const* const _leftBracket = code_getds("");
	char leftBracket[0x10];
	strcpy_s(leftBracket, _leftBracket);
	size_t const lenLeftBracket = std::strlen(leftBracket);

	char const* const _rightBracket = code_getds("");
	char rightBracket[0x10];
	strcpy_s(rightBracket, _rightBracket);
	size_t const lenRightBracket = std::strlen(rightBracket);

	// 文字列化処理
	std::function<void(vector_t&, char*, int, size_t&)> impl
		= [&](vector_t const& self, char* buf, int bufsize, size_t& idx)
	{
		std::strncpy(&buf[idx], leftBracket, lenLeftBracket);
		idx += lenLeftBracket;

		// foreach
		for ( size_t i = 0; i < self->size(); ++ i ) {
			if ( i != 0 ) {
				// 区切り文字
				std::strncpy( &buf[idx], splitter, lenSplitter );
				idx += lenSplitter;
			}

			PVal* const pvdat = self->at(i).valuePtr();

			if ( pvdat->flag == g_vtVector ) {
				impl( VectorTraits::getMaster(pvdat), buf, bufsize, idx );

			} else {
				// 文字列化して連結
				char const* const pStr = (char const*)Valptr_cnvTo((PDAT*)pvdat->pt, pvdat->flag, HSPVAR_FLAG_STR);
				size_t const lenStr = std::strlen( pStr );
				std::strncpy( &buf[idx], pStr, lenStr );
				idx += lenStr;
			}
		}

		std::strncpy(&buf[idx], rightBracket, lenRightBracket);
		idx += lenRightBracket;
	};

	auto const lambda = [&self, &impl](char* buf, int bufsize) {
		size_t idx = 0;				// 結合後の文字列の長さ
		impl( self, buf, bufsize, idx );
		buf[idx ++] = '\0';
	};

	return SetReffuncResultString( ppResult, lambda );
}

//------------------------------------------------
// 添字関数
//------------------------------------------------
int VectorAt( void** ppResult )
{
	auto&& self = code_get_vector();

	int vtype;
	if ( void* const pResult = Vector_indexRhs(self, &vtype) ) {
		*ppResult = pResult;
		return vtype;
	} else {
		return SetReffuncResult(ppResult, self);
	}
}


//------------------------------------------------
// 終了時
//------------------------------------------------
void VectorCmdTerminate()
{
	g_pResultVector.clear();
	return;
}
