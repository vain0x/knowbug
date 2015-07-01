// clhsp - declaration about Var

#ifndef IG_CLHSP_VAR_CORE_H
#define IG_CLHSP_VAR_CORE_H

#include "module/defdef.h"
#include "module/mod_cast.h"
#include "module/func_isRange.h"

// 型定義
typedef ushort* label_t;
typedef ushort* csptr_t;
typedef ushort chcode_t;
typedef  short vartype_t;
typedef  short valmode_t;

typedef       void*   valptr_t;
typedef const void* c_valptr_t;
typedef       char*   bufptr_t;
typedef const char* c_bufptr_t;

typedef ushort arridx_t;

// 前方宣言
struct VarProc;

struct ScrVar;
struct ScrValue;
struct LValue;

struct String;
struct ModInst;
struct StArray;
struct StArrRef;
struct Vector;
struct VecRef;
struct Block;
struct PrmStk;

// 変数宣言
extern ScrValue** mem_tval;
extern VarProc* g_varproc;
extern uint     g_cntVartype;

typedef void (* pfVarProcInit_t)(VarProc*);

// その他の定数
static const size_t ArrayDimMax = 3;

//------------------------------------------------
// 組み込み型の型タイプ値
//------------------------------------------------
namespace Vartype
{
	static const vartype_t
		None     = 0,
		Label    = 1,
		Str      = 2,
		Double   = 3,
		Int      = 4,
		Bool     = 4,
		ModInst  = 5,
		Comobj   = 6,
		Variant  = 7,
		LValue   = 8,
		Ref      = LValue,
		Vector   = 9,
		Array    = 10,
		Block    = 11,
		Void     = 12,
	//	Function = 13,
		Userdef  = 13,
		Max      = Userdef;
};

//------------------------------------------------
// 記号のコード値
//------------------------------------------------
enum MarkCode
{
	MarkCode_Top    = 0,
	MarkCode_Top_Op = MarkCode_Top,
	
	// 単項演算子
	MarkCode_Top_OpUni = MarkCode_Top_Op,
	MarkCode_Plus      = MarkCode_Top_OpUni,
	MarkCode_Minus,
	MarkCode_Inv,
	MarkCode_Neg,
	MarkCode_Pack,
	MarkCode_UnPack,
	MarkCode_Max_OpUni,
	
	// 二項演算子
	MarkCode_Top_OpBin = MarkCode_Max_OpUni,
	MarkCode_Add       = MarkCode_Top_OpBin,
	MarkCode_Sub,
	MarkCode_Mul,
	MarkCode_Div,
	MarkCode_Mod,
	MarkCode_And,
	MarkCode_Or,
	MarkCode_Xor,
	MarkCode_ShL,
	MarkCode_ShR,
	MarkCode_LAnd,
	MarkCode_LOr,
	MarkCode_Cmp,
	MarkCode_Eq,
	MarkCode_Ne,
	MarkCode_IdEq,
	MarkCode_IdNe,
	MarkCode_Lt,
	MarkCode_Gt,
	MarkCode_LtEq,
	MarkCode_GtEq,
	MarkCode_Cat,
	MarkCode_Max_OpBin,
	
	// 括弧演算子
	MarkCode_Top_OpBracket = MarkCode_Max_OpBin,
	MarkCode_IdxL          = MarkCode_Top_OpBracket,
	MarkCode_IdxR,
	MarkCode_ArgL,
	MarkCode_ArgR,
	MarkCode_Max_OpBracket,
	
	// 特殊演算子
	MarkCode_Top_OpExtra = MarkCode_Max_OpBracket,
	MarkCode_Dot         = MarkCode_Top_OpExtra,
	MarkCode_Arw,
	MarkCode_Which,
	MarkCode_Max_OpExtra,
	
	// 代入演算子
	MarkCode_Top_OpAssign = MarkCode_Max_OpExtra,
	MarkCode_Set          = MarkCode_Top_OpAssign,
	MarkCode_Swap,
	MarkCode_Inc,
	MarkCode_Dec,
	
	// 複合代入演算子 (二項演算子の数と同じだけ確保しておく)
	MarkCode_Top_OpAssignEx,
	MarkCode_Max_OpAssignEx
		= MarkCode_Top_OpAssignEx
		+ MarkCode_Max_OpBin
		- MarkCode_Top_OpBin,
	MarkCode_Bias_OpAssignEx
		= MarkCode_Top_OpAssignEx
		- MarkCode_Top_OpBin,
	
	MarkCode_Max_OpAssign = MarkCode_Max_OpAssignEx,
	MarkCode_Max_Op       = MarkCode_Max_OpAssign,
	
	// 文タイプ
	MarkCode_Top_Sttm = MarkCode_Max_Op,
	MarkCode_SttmExpr = MarkCode_Top_Sttm,
	MarkCode_SttmAssign,
	MarkCode_SttmInv,
	MarkCode_Max_Sttm,
	
	// その他の記号
	MarkCode_Top_Other = MarkCode_Max_Sttm,
	MarkCode_Omission  = MarkCode_Top_Other,
	MarkCode_VecL,
	MarkCode_VecR,
	MarkCode_BlkL,
	MarkCode_BlkR,
//	MarkCode_Function,
	MarkCode_Max_Other,
	
	// 終了
	MarkCode_Max = MarkCode_Max_Other,
};

typedef MarkCode MarkCode_t;

//------------------------------------------------
// 型変換レベル (二項演算時)
// 
// @ 後で追加されるかもしれないため、間隔を空ける。
//------------------------------------------------
enum tagCnvLv
{
	CnvLv_Invalid = -1,
	CnvLv_Min     =  0,
	CnvLv_Int     = 100,
	CnvLv_Double  = 120,
	CnvLv_Str     = 130,
	CnvLv_Array   = 140,
	CnvLv_Vector  = 150,
	CnvLv_Void    = 0x7FFF,
	CnvLv_Max     = CnvLv_Void,
};
typedef enum tagCnvLv CnvLv_t;
inline bool operator < ( CnvLv_t a, CnvLv_t b ) { return (short)a < (short)b; }

//------------------------------------------------
// 変数型の構造体
//------------------------------------------------
struct VarProc
{
	vartype_t vt;			// 型タイプ値 (ランタイム側で設定する)
	vartype_t aftertype;	// 演算後のタイプ値
	ushort version;			// 型タイプランタイムバージョン(0x100 = 1.0)
	ushort support;			// サポート状況フラグ
	ushort elemsize;		// 1 要素のサイズ (byte)
	short lvCnv;			// 型変換レベル (負 => 変換不可)
	
	char* vartype_name;		// 型名へのポインタ
	void* userdata;			// (任意で使用可能、ランタイムは干渉しない)
	
public:
	void (*pf_first)(void);		// (永遠の先頭メンバ)
	
	// 値の情報を取得する
	bool (*GetUsing)( c_valptr_t p );				// varuse()
	bool (*IsNull  )( c_valptr_t p );				// isNull()
	uint (*Length  )( c_valptr_t p, uint idxDim );	// length()
	
	// Buffer
	bufptr_t (*GetBufPtr) (   valptr_t p );
	size_t   (*GetBufSize)( c_valptr_t p );
	
	// 実体値の初期化
	void (*Alloc  )( valptr_t p, c_valptr_t src );
	void (*Nullify)( valptr_t p );
	void (*Clone  )( valptr_t p, bufptr_t pBuf, size_t size );
	
	// 処理
	void (*AllocBlock)( valptr_t p, size_t size );
	void (*ReDim)( valptr_t p, uint len, uint idxDim );
	
	// 型変換用
	valptr_t (*Cnv      )( c_valptr_t src, vartype_t vtFrom );
	valptr_t (*CnvCustom)( c_valptr_t src, vartype_t vtDest );
	
	// 代入関数
	void (*Assign)( valptr_t p, c_valptr_t src, vartype_t );
	void (*Copy  )( valptr_t p, c_valptr_t src );
	
	// 演算関数
	typedef void (*pfBinOp_t)( valptr_t, c_valptr_t );
	
	void (*AddI)( valptr_t lhs, c_valptr_t rhs );
	void (*SubI)( valptr_t lhs, c_valptr_t rhs );
	void (*MulI)( valptr_t lhs, c_valptr_t rhs );
	void (*DivI)( valptr_t lhs, c_valptr_t rhs );
	void (*ModI)( valptr_t lhs, c_valptr_t rhs );
	
	void (*AndI)( valptr_t lhs, c_valptr_t rhs );
	void (*OrI) ( valptr_t lhs, c_valptr_t rhs );
	void (*XorI)( valptr_t lhs, c_valptr_t rhs );
	void (*RrI) ( valptr_t lhs, c_valptr_t rhs );
	void (*LrI) ( valptr_t lhs, c_valptr_t rhs );
	
	int (*IdNeI)( c_valptr_t lhs, c_valptr_t rhs );
	int (*NeI  )( c_valptr_t lhs, c_valptr_t rhs );
	int (*CmpI )( c_valptr_t lhs, c_valptr_t rhs );
	
	void (*CatI)( valptr_t lhs, c_valptr_t rhs );
	
	void (*PlusI  )( valptr_t p );		// 正符号
	void (*MinusI )( valptr_t p );		// 負符号
	void (*InvI   )( valptr_t p );		// 反転
	void (*NegI   )( valptr_t p );		// 否定
	
	// 特殊演算
	valptr_t (*IndexerI)( valptr_t p );		// operator []
	valptr_t (*FunctorI)( valptr_t p );		// operator ()
	valptr_t (*MethodI )( valptr_t lhs, c_valptr_t rhs, vartype_t vtRhs, int bArglist );
	
	// その他
	int (*Construct)(void);			// 構築処理関数 (現在停止中)
	
	bool (*ToDbgString)( char* buf, c_valptr_t src );
	
	// (永遠の最終メンバ)
	void (*pf_final)(void);
};

//------------------------------------------------
// 変数型の性質を示すフラグ
//------------------------------------------------
namespace VarSupport
{
	static const int
		Storage     = 0x0001,		// 固定長ストレージサポート
		FlexStorage = 0x0002,		// 可変長ストレージサポート
		FixedArray  = 0x0004,		// 固定長の静的配列にできる
		FlexArray   = 0x0008,		// 可変長な静的配列にできる
		Buffer      = 0x0020,		// bufptr_t に変換できる
		NoConvert   = 0x0040,		// 代入時に型変換されない
		Varuse      = 0x0080,		// varuse() のチェックを有効にする
		Reference   = 0x0100,		// 参照型
	//	Container   = 0x0200,		// コンテナ型
		User1       = 0x4000,		// ユーザー定義フラグ1
		User2       = 0x8000;		// ユーザー定義フラグ2
};

//------------------------------------------------
// 値モード
//------------------------------------------------
namespace Valmode
{
	static const valmode_t
		None  = 0,
		Inst  = 1,
		Clone = 2;
};

//------------------------------------------------
// 変数
// 
// @ 値の入れ物でしかない。
//------------------------------------------------
struct ScrVar
{
	ScrValue* pVal;
};

//------------------------------------------------
// 「値」データ
// 
// @ これ自身は実体ではなく、実体への参照、値データへのポインタを持つ。
// @	そのため、ScrValue の複製を memcpy で作っても問題ない。
//------------------------------------------------
struct ScrValue
{
	vartype_t vt;
	ushort flag;			// フラグ (ScrValue::Flag::_)
	
	// 実体データへのポインタ
	valptr_t valptr;
	
	// 実体データを参照するための領域 (inst_t)
	union {
		char valptr_inst[1];	// 実体領域を指す valptr
		void*    pObj;			// (for VarSupport::Reference)
		
		label_t lb;				// for vt label
		double  dval;			// for vt double
		int     ival;			// for vt int
		
		String*  sptr;			// for vt str
		ModInst* modinst;		// for vt struct
		Vector*  vec;			// for vt vector
		StArray* starr;			// for vt array
		
		LValue*  lval;			// for vt lval
	};
	
public:
	struct Flag {
		// dynamic (ScrValue::flag (max 16bit))
		static const int
			None    = 0x0000,
			NoTmp   = 0x0001,	// 一時変数ではない
			Local   = 0x0002,	// 動的ローカル変数
			Clone   = 0x0004;	// クローン ( valptr != valptr_inst )
		//	RefElem = 0x0008	// 添字括弧付きで参照されている
	};
};

//------------------------------------------------
// str 型の実体
//------------------------------------------------
struct String
{
	char* s;
	valmode_t mode;
	short    _padding;
	size_t bufsize;
	
public:
	char* getptr(void) const { return s; }
};

//------------------------------------------------
// modinst 型の実体
//------------------------------------------------
struct ModInst
{
	ushort id_finfo;
	ushort id_minfo;
	PrmStk* prmstk;
};

//------------------------------------------------
// vector 型への参照
//------------------------------------------------
struct Vector;
struct VecElem;

struct VecRef
{
	Vector* pInst;
	int idx;
};

//------------------------------------------------
// array 型の要素への参照
//------------------------------------------------
struct StArrRef
{
	StArray* pInst;
	uint idx;
};

//------------------------------------------------
// 実体への参照 (クローン)
//------------------------------------------------
struct ClonePtr
{
	vartype_t vt;
	valptr_t p;
};

//------------------------------------------------
// バッファへの参照 (クローン)
//------------------------------------------------
struct CloneBuf
{
	vartype_t vt;
	bufptr_t pBuf;
	size_t size;
};

//------------------------------------------------
// lval 型の実体
//------------------------------------------------
struct LValue
{
	int reftype;
	
	union {
		ScrVar*    var;
		ScrValue* pVal;
	//	CloneBuf clnbuf;
		ClonePtr clnptr;
		VecRef   vecRef;
		StArrRef arrRef;
	};
};

namespace RefType
{
	static const int
		None    = 0,
		Var     = 1,
		Value   = 2,
	//	ClnBuf  = 3,		// Clone-Buf
		ClnPtr  = 4,
		ArrElem = 5,
		VecElem = 6;
};

//------------------------------------------------
// 型変換可能か
//------------------------------------------------
inline bool LValue_isVtConvertable( const LValue* lval )
{
	return lval->reftype != RefType::ArrElem;
}

//##############################################################################
//                関数宣言
//##############################################################################
//**********************************************************
//        全体的
//**********************************************************
void HspVarCoreInit(void);
void HspVarCoreBye(void);
void HspVarCoreResetVartype( int expand );
int HspVarCoreAddType(void);
void HspVarCoreRegisterType( vartype_t vt, pfVarProcInit_t func );
VarProc* HspVarCoreSeekProc( const char* name );

extern ScrValue* HspVarCoreGetTValue( vartype_t vt );

extern void HspVarCorePutInvalid(void);

//**********************************************************
//        Alloc, Free 系統
//**********************************************************
void HspVarCoreAllocVar  ( ScrVar*    var, vartype_t vt, c_valptr_t ptr = NULL );
void HspVarCoreAllocValue( ScrValue* pVal, vartype_t vt, const ScrValue* pVal2 = NULL );
void HspVarCoreAllocValue( ScrValue* pVal, vartype_t vt, c_valptr_t pSrc );

void HspVarCoreAllocArray( ScrValue* pVal, vartype_t vt, uint len1 = 0, uint len2 = 0, uint len3 = 0 );
void HspVarCoreReDimArray( ScrValue* pVal, uint len, uint idxDim = 0 );

ScrValue* HspVarCoreCnvValue( ScrValue* pVal, vartype_t vt );
valptr_t HspVarCoreCnvPtr( c_valptr_t src, vartype_t vtSrc, vartype_t vtDst );
valptr_t HspVarCoreCnvPtr( const ScrValue* pVal, vartype_t vtDst );

//**********************************************************
//        変数操作
//**********************************************************
void HspVarCoreCloneVar( ScrVar* varDst, ScrVar* varSrc );
void HspVarCoreClonePtr( ScrValue* pVal, vartype_t vt, bufptr_t pBuf, size_t size );
void HspVarCoreAssign( valptr_t p, vartype_t vtDst, c_valptr_t src, vartype_t vtSrc );
void HspVarCoreAssign( ScrValue* pVal, c_valptr_t src, vartype_t vt );
void HspVarCoreAssign(   LValue* lval, c_valptr_t src, vartype_t vt );
void HspVarCoreNullify( ScrValue* pVal );
void HspVarCoreCopy( valptr_t p, c_valptr_t src, vartype_t vt );
void HspVarCoreCopy( ScrValue* pValDst, ScrValue* pValSrc );
void HspVarCoreAssignSwap( LValue* lvalLeft, LValue* lvalRight );

//**********************************************************
//        ScrValue 操作
//**********************************************************
extern ScrValue* ScrValue_expand_lval( ScrValue* pVal );
extern valptr_t  ScrValue_getptr( ScrValue* pVal );
extern vartype_t ScrValue_getvt ( ScrValue* pVal );

//**********************************************************
//        LValue 操作
//**********************************************************
extern ScrValue* LValue_get   ( LValue* self );
extern valptr_t  LValue_getptr( LValue* self );
extern vartype_t LValue_getvt ( LValue* self );

extern void LValue_assign( LValue* self, c_valptr_t src, vartype_t vt );

//**********************************************************
//        
//**********************************************************
extern int  HspVarCoreCmp( const ScrValue* pValLeft, const ScrValue* pValRight );
extern bool HspVarCoreNe ( const ScrValue* pValLeft, const ScrValue* pValRight );
extern int  HspVarCoreCmp( vartype_t vt, c_valptr_t pLeft, c_valptr_t pRight );
extern bool HspVarCoreNe ( vartype_t vt, c_valptr_t pLeft, c_valptr_t pRight );

#define HspVarCoreGetUsing( vt, p ) HspVarCoreGetProc(vt)->GetUsing( p )
#define HspVarCoreIsNull( vt, p )   HspVarCoreGetProc(vt)->IsNull( p )

#define HspVarCoreOperate2( func_ident, vt, p, src ) ( HspVarCoreGetProc(vt)->func_ident##I(p, src) )
#define HspVarCoreCat( pVal, src ) HspVarCoreOperate2( Cat, pVal, src )
#define HspVarCoreDot( pVal, src ) HspVarCoreOperate2( Dot, pVal, src )

#define HspVarCoreCnv( vtFrom, vtDest, p ) g_varproc[(vtDest)].Cnv( (p), (vtFrom) )		// in1->in2の型にin3ポインタを変換する
#define HspVarCoreConstruct(flag) g_varproc[(flag)].Construct()

#define HspVarCoreGetBufPtr(p, vt) ( HspVarCoreGetProc(vt)->GetBufPtr(p)  )
#define HspVarCoreGetBufSize(p, vt) ( HspVarCoreGetProc(vt)->GetBufSize(p) )
#define HspVarCoreGetBlockSize( pv, in1, out ) ( HspVarCoreGetProc(vt)->GetBlockSize( pv, in1, out ) )
#define HspVarCoreAllocBlock( p, vt, size ) ( HspVarCoreGetProc(vt)->AllocBlock( p, size ) )
#define HspVarCoreReDim( p, vt, len, idxDim ) ( HspVarCoreGetProc(vt)->ReDim( p, len, idxDim ) )

//##############################################################################
//                inline 関数
//##############################################################################

inline VarProc* HspVarCoreGetProc( vartype_t vt )
{
	return &g_varproc[vt];
}

//------------------------------------------------
// 変換 : bool -> int
//------------------------------------------------
namespace ChBool
{
	static const int
		True( -1 ), Success( True  ),
		False( 0 ), Failure( False );
}

#define FTM_chbool( type, expr ) \
	inline int chbool( const type& val ) { return ( expr ? ChBool::True : ChBool::False ); }

FTM_chbool(  bool, val );
FTM_chbool(   int, val == 0    );
FTM_chbool( void*, val == NULL );

//------------------------------------------------
// 型空間 ( Namespace-Template-Macro )
//------------------------------------------------
#define NTM_Vt(_ident, _inst_t, _vptr_t) \
	namespace Vt##_ident {				\
		typedef _inst_t inst_t;			\
		typedef _vptr_t vptr_t;			\
		FTM_VtN_getInst( inst_t );		\
	}

#define FTM_VtN_getInst( type ) \
	inline       type& get(   valptr_t p ) { return *ptr_cast<      type*>(p); }	\
	inline const type& get( c_valptr_t p ) { return *ptr_cast<const type*>(p); }

NTM_Vt( Label,   label_t , label_t*  );
NTM_Vt( Str,      String*,  String** );
NTM_Vt( Double,   double ,  double*  );
NTM_Vt( Int,         int ,     int*  );
NTM_Vt( ModInst, ModInst*, ModInst** );
NTM_Vt( LValue,   LValue*,  LValue** );
NTM_Vt( Block,     Block*,   Block** );
//NTM_Vt( Closure, Closure*, Closure** );

//------------------------------------------------
// 演算子の種類
//------------------------------------------------
static inline bool isUnaryOperator(int op)
{
	return isRange<int>( op, MarkCode_Top_OpUni, MarkCode_Max_OpUni - 1 );
}

static inline bool isBinaryOperator(int op)
{
	return isRange<int>( op, MarkCode_Top_OpBin, MarkCode_Max_OpBin - 1 );
}

static inline bool isRelativeOperator(int op)
{
	return isRange<int>( op, MarkCode_Eq, MarkCode_GtEq );
}

static inline bool isAssignExOperator(int op)
{
	return isRange<int>( op, MarkCode_Top_OpAssignEx, MarkCode_Max_OpAssignEx - 1 );
}

static inline bool isAssignOperator(int op)
{
	// AssignEx も含む
	return isRange<int>( op, MarkCode_Top_OpAssign, MarkCode_Max_OpAssign - 1 );
}

static inline bool isLBracketOperator(int op)
{
	return ( op == MarkCode_IdxL || op == MarkCode_ArgL );
}

static inline bool isRBracketOperator(int op)
{
	return ( op == MarkCode_IdxR || op == MarkCode_ArgR );
}

static inline bool isLBracket(int op)
{
	return isLBracketOperator(op) || op == MarkCode_VecL;
}

static inline bool isRBracket(int op)
{
	return isRBracketOperator(op) || op == MarkCode_VecR;
}

static inline bool isBracketOperator(int op)
{
	return ( isLBracketOperator(op) || isRBracketOperator(op) );
}

static inline bool isBracket(int op)
{
	return ( isLBracket(op) || isRBracket(op) );
}

//------------------------------------------------
// Clone かどうか
//------------------------------------------------
static inline bool isClone( const ScrValue* pVal )
{
	return ( pVal->flag & ScrValue::Flag::Clone ) != 0;
}

//------------------------------------------------
// Temporary かどうか
//------------------------------------------------
static inline bool isTmpValue( const ScrValue* pVal )
{
	return !( pVal->flag & ScrValue::Flag::NoTmp || isClone(pVal) );
}

//------------------------------------------------
// 左辺値かどうか
//------------------------------------------------
static inline bool isLValue( const ScrValue* pVal )
{
	return ( !isTmpValue(pVal) || pVal->vt == Vartype::LValue );
}

//------------------------------------------------
// 複合代入演算子にする
//------------------------------------------------
static inline MarkCode_t opAssignEx( MarkCode_t mark )
{
	return ( isBinaryOperator(mark) )
		? (MarkCode_t)( mark + MarkCode_Bias_OpAssignEx )
		: mark
	;
}

static inline MarkCode_t opBinary( MarkCode_t mark )
{
	return ( isAssignExOperator(mark) )
		? (MarkCode_t)( mark - MarkCode_Bias_OpAssignEx )
		: mark
	;
}

#endif
