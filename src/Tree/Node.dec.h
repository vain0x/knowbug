#pragma once

namespace DataTree
{

class ITree;
class INode;
class ILeaf;

using tree_t = ITree*;

class NodeLoop;

class NodeGlobal;
class NodeModule;
//class NodeSysvarList;
//class NodeCallList;

class NodeArray;

class NodeValue;
class NodeLabel;
class NodeString;
class NodeDouble;
class NodeInt;
class NodeModInst;
class NodeUnknown;

class NodePrmStk;

}
