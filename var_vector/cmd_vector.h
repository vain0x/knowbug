// vector - Command header

#ifndef IG_VECTOR_COMMAND_H
#define IG_VECTOR_COMMAND_H

#include "hsp3plugin_custom.h"
using namespace hpimod;

#include "vt_vector.h"

extern vector_t code_get_vector();

// コマンド用関数のプロトタイプ宣言
extern void VectorDelete();				// 破棄

extern int VectorMake( void** ppResult );			// as literal
extern int VectorSlice( void** ppResult );
extern int VectorSliceOut( void** ppResult );
extern int VectorDup( void** ppResult );

extern int VectorIsNull( void** ppResult );
extern int VectorVarinfo( void** ppResult );
extern int VectorSize( void** ppResult );

extern void VectorDimtype();
extern void VectorClone();

extern void VectorChain(bool bClear);	// 連結 (or 複写)
#if 0
extern void VectorMoving( int cmd );	// 要素順序操作系
extern int  VectorMovingFunc( void** ppResult, int cmd );

extern void VectorInsert();				// 要素追加
extern void VectorInsert1();
extern void VectorPushFront();
extern void VectorPushBack();
extern void VectorRemove();				// 要素削除
extern void VectorRemove1();
extern void VectorPopFront();
extern void VectorPopBack();
extern void VectorReplace();
extern int VectorInsert( void** ppResult ) ;
extern int VectorInsert1( void** ppResult ) ;
extern int VectorPushFront( void** ppResult );
extern int VectorPushBack( void** ppResult );
extern int VectorRemove( void** ppResult );
extern int VectorRemove1( void** ppResult );
extern int VectorPopFront( void** ppResult );
extern int VectorPopBack( void** ppResult );
extern int VectorReplace( void** ppResult );
#endif

extern int VectorResult( void** ppResult );
extern int VectorExpr( void** ppResult );
extern int VectorJoin( void** ppResult );
extern int VectorAt( void** ppResult );

// 終了時
extern void VectorCmdTerminate();

// システム変数

// 定数
namespace VectorCmdId {
	int const
		Move    = 0x20,
		Swap    = 0x21,
		Rotate  = 0x22,
		Reverse = 0x23;
};

#endif
