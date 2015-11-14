// 動的ツリー

// TODO: Singleton にしろ
//	というか所有する CNodeGlobal が singleton であるべきだ

#ifndef IG_CLASS_DATA_TREE_H
#define IG_CLASS_DATA_TREE_H

#include "Node.h"

namespace DataTree
{

class CDataTree
{
public:
	CDataTree();
	~CDataTree() { delete root_; }
	
//	void addVar   ( char const* pName, PVal* pval );
//	void addModule( char const* pName );
	
	tree_t getRoot() const { return root_; }
	
private:
	// usually global module
	tree_t root_;
};

}

typedef DataTree::CDataTree DataTree_t;

#endif
