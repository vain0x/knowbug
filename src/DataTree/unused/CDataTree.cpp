// DataTree

#include "CDataTree.h"

namespace DataTree
{

//##############################################################################
//                ��`�� : CDataTree
//##############################################################################
//------------------------------------------------
// �\�z
//------------------------------------------------
CDataTree::CDataTree()
	: root_(&CNodeGlobal::getInstance())
{ }

//------------------------------------------------
// ���
//------------------------------------------------
//	CDataTree::~CDataTree() { }

/*
//------------------------------------------------
// �ϐ��ǉ�
// 
// @ �K�v�Ȃ烂�W���[�����ǉ�����
//------------------------------------------------
void CDataTree::addVar( char const* name, PVal* pval )
{
	
	return;
}

//------------------------------------------------
// ���W���[���ǉ�
//------------------------------------------------
void CDataTree::addModule( char const* pName, PVal* pval )
{
	
	return;
}
//*/

}