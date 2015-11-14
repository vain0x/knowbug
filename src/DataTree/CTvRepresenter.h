// ツリービュー表現者

#pragma once

#include <CommCtrl.h>
#include <memory>

#include "ITreeVisitor.h"
#include "ITreeObserver.h"

namespace DataTree
{
	class CTvRepresenter
	{
	public:
		CTvRepresenter(HWND hTreeView);
		~CTvRepresenter();

	public:
		friend class CTvAppendObserver;
		friend class CTvRemoveObserver;

		// 追加を検知する
		// 実際に追加される直前に呼ばれる
		class CTvAppendObserver
			: public ITreeObserver
		{
			friend class CTvRepresenter;
			CTvRepresenter* p_;
			CTvRepresenter* getOwner() const { return p_; }
			CTvAppendObserver(CTvRepresenter* p) : p_(p) { }

		public:
			void visit0(ITree* t) override { t->acceptVisitor(*this); }

			void visit(CLoopNode* t)        override { visit1(t); }
			void visit(CNodeModule* t)		override { visit1(t); }
			void visit(CNodeVarArray* t)	override { visit1(t); }
			void visit(CNodeVarElem* t)		override { visit1(t); }
			void visit(CNodeVarHolder* t)	override { visit1(t); }
			void visit(CNodeModInst* t)		override { visit1(t); }
			void visit(CNodeModInstNull* t) override { visit1(t); }
			void visit(CNodePrmStk* t)		override { visit1(t); }
			void visit(CNodeLabel* t)		override { visit1(t); }
			void visit(CNodeString* t)		override { visit1(t); }
			void visit(CNodeDouble* t)		override { visit1(t); }
			void visit(CNodeInt* t)			override { visit1(t); }
			void visit(CNodeUnknown* t)		override { visit1(t); }

			void visit1(INodeContainer*);
			void visit1(ILeaf*) { }
		};

		// 削除を検知する
		// 解体処理が始まる直前に呼ばれる
		// このノードが持っている子ノードは既にこの通知を送られて削除されている、はず
		class CTvRemoveObserver
			: public ITreeObserver
		{
			friend class CTvRepresenter;
			CTvRepresenter* p_;
			CTvRepresenter* getOwner() const { return p_; }
			CTvRemoveObserver(CTvRepresenter* p) : p_(p) { }

		public:
			void visit0(ITree*) override;

			void visit(CLoopNode* t)        override { visit1(t); }
			void visit(CNodeModule* t)		override { visit1(t); }
			void visit(CNodeVarArray* t)	override { visit1(t); }
			void visit(CNodeVarElem* t)		override { visit1(t); }
			void visit(CNodeVarHolder* t)	override { visit1(t); }
			void visit(CNodeModInst* t)		override { visit1(t); }
			void visit(CNodeModInstNull* t) override { visit1(t); }
			void visit(CNodePrmStk* t)		override { visit1(t); }
			void visit(CNodeLabel* t)		override { visit1(t); }
			void visit(CNodeString* t)		override { visit1(t); }
			void visit(CNodeDouble* t)		override { visit1(t); }
			void visit(CNodeInt* t)			override { visit1(t); }
			void visit(CNodeUnknown* t)		override { visit1(t); }

			void visit1(INodeContainer*);
			void visit1(ILeaf*) { }
		};

	private:
		void CTvRepresenter::insertItem(tree_t node, string name, HTREEITEM hParent, HTREEITEM hInsertAfter);

	private:
		// メンバ隠蔽
		struct Impl;
		Impl* m;

	public:
		// ノードからビューノードを探す (見つからなければ nullptr)
		HTREEITEM findTvItem(tree_t node) const;

		// ビューノードからノードを探す (見つからなければ nullptr)
		tree_t findNode(HTREEITEM hItem) const;

		template<class TNode> TNode* findNode(HTREEITEM node) const {
			return dynamic_cast<TNode*>(findNode(node));
		}
	};

}