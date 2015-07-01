// VarTree

#include "CVarTree.h"

std::string CVarTree::AreaName_Global("*");

//##############################################################################
//                定義部 : CVarTree
//##############################################################################
//------------------------------------------------
// 構築
//------------------------------------------------
CVarTree::CVarTree( const std::string& name, NodeType type )
	: mpChildren( nullptr )
	, mName     ( name )
	, mType     ( type )
{
	if ( mType == NodeType_Module ) {
		mpChildren = new Children_t;
	}
	return;
}

//------------------------------------------------
// 構築 (複写)
//------------------------------------------------
CVarTree::CVarTree( const CVarTree& src )
	: mType ( src.mType )
	, mName ( src.mName )
	, mpChildren( nullptr )
{
	if ( mType == NodeType_Module ) {
		mpChildren = new Children_t;
		
		for ( iterator iter = src.begin()
			; iter != src.end()
			; ++ iter
		) {
			mpChildren->insert(
				Children_t::value_type( iter->getName(), CVarTree( *iter ) )
			);
		}
	}
	return;
}

//------------------------------------------------
// 解体
//------------------------------------------------
CVarTree::~CVarTree()
{
	if ( mpChildren ) {
		delete mpChildren; mpChildren = nullptr;
	}
	return;
}

//------------------------------------------------
// 子ノードを追加する (任意位置)
//------------------------------------------------
void CVarTree::push_child( const char* name, CVarTree::NodeType type )
{
	if ( mType != NodeType_Module ) return;
	
	const char* pModname = strchr( name, '@' );
	
	// スコープ解決がある => 子ノードのモジュールに属す
	if ( pModname ) {
		
		// 属すモジュールを得る
		Children_t::iterator iter = getChildModule( pModname );
		
	//	std::string stmp( name, pModname - name );
		
		iter->second.add( name, type );
		
	// このモジュールに属す
	} else {
		add( name, type );
	}
	
	return;
}

//------------------------------------------------
// 子ノードの、指定したモジュール名のノードを取得する
// 
/// @prm pModname : 先頭は '@' とする
//------------------------------------------------
CVarTree::ChildrenIter_t CVarTree::getChildModule( const char* pModname )
{
	++ pModname;		// '@' はとりあえず無視する
	
	string modname( pModname );
	
	// '@' を探す ( 後ろ優先 )
	const char* pModnameNext( strrchr( pModname, '@' ) );
	
	// '@' がまだある場合、'@' に続く文字列だけにする
	if ( pModnameNext ) {
		modname = (pModnameNext + 1);	// '@' は飛ばす
	}
	
	// モジュール名で検索
	ChildrenIter_t iter( mpChildren->find( modname ) );
	
	// (なければ) モジュール名リストに追加する
	if ( iter == mpChildren->end() ) {
		
		iter = mpChildren->insert(
			Children_t::value_type(
				modname,
				CVarTree( modname, NodeType_Module )
			)
		).first;
	}
	
	// 続きがあれば、さらに探索を続ける
	if ( pModnameNext ) {
		string s( pModname - 1, pModnameNext );		// 先頭の '@' が必要
		iter = iter->second.getChildModule( s.c_str() );
	}
	
	return iter;
}

//------------------------------------------------
// 要素を子ノードとして追加する
// 
// @prm name : スコープ解決 '@' は、絶対にない。
//------------------------------------------------
void CVarTree::add( const char* name, CVarTree::NodeType type )
{
	if ( mType != NodeType_Module ) return;
	
	mpChildren->insert(
		Children_t::value_type( name, CVarTree( name, type ) )
	);
	
	return;
}

//------------------------------------------------
// iterator生成
//------------------------------------------------
CVarTree::iterator CVarTree::begin(void) const
{
	return iterator( const_cast<CVarTree*>( this ) );
}

CVarTree::iterator CVarTree::end(void) const
{
	iterator iter( const_cast<CVarTree*>( this ) );
	
	iter.moveToEnd();
	return iter;
}

//##############################################################################
//                定義部 : CVarTree::iterator
//##############################################################################


