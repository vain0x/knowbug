
#include <Windows.h>
#include <CommCtrl.h>

#include "module/GuiUtility.h"

#include "main.h"
#include "DebugInfo.h"
#include "dialog.h"
#include "config_mng.h"
#include "Logger.h"
#include "HspObjectPath.h"
#include "HspObjects.h"
#include "HspStaticVars.h"

#include "vartree.h"
#include "CVarinfoText.h"
#include "CVardataString.h"

#include "WrapCall/WrapCall.h"
#include "WrapCall/ModcmdCallInfo.h"

#include "module/supio/supio.h"

// 変数ツリービューに対する操作のラッパー
class VarTreeView {
	HWND hwndVarTree;

public:
	VarTreeView(HWND tree_view_handle)
		: hwndVarTree(tree_view_handle)
	{
	}

	auto selected_item() const -> HTREEITEM {
		return TreeView_GetSelection(hwndVarTree);
	}

	auto insert_item(HTREEITEM hParent, char const* name, VTNodeData* node) -> HTREEITEM {
		auto tvis = TVINSERTSTRUCT{};
		HTREEITEM res;
		HSPAPICHAR* hactmp1;
		tvis.hParent = hParent;
		tvis.hInsertAfter = TVI_LAST;
		tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
		tvis.item.lParam = (LPARAM)node;
		tvis.item.pszText = chartoapichar(const_cast<char*>(name), &hactmp1);
		res = TreeView_InsertItem(hwndVarTree, &tvis);
		freehac(&hactmp1);
		return res;
	}

	void delete_item(HTREEITEM hItem) {
		TreeView_EscapeFocus(hwndVarTree, hItem);
		TreeView_DeleteItem(hwndVarTree, hItem);
	}

	void expand_item(HTREEITEM hParent) {
		TreeView_Expand(hwndVarTree, hParent, TVE_EXPAND);
	}

	void select_item(HTREEITEM hItem) {
		TreeView_SelectItem(hwndVarTree, hItem);
	}

	auto tryGetNodeData(HTREEITEM hItem) const -> optional_ref<VTNodeData> {
		auto const lp = reinterpret_cast<VTNodeData*>(TreeView_GetItemLParam(hwndVarTree, hItem));
		assert(lp);
		return lp;
	}
};

#define hwndVarTree (Dialog::getVarTreeHandle())

#ifdef with_WrapCall
using WrapCall::ModcmdCallInfo;
#endif
using detail::TvObserver;
using detail::VarTreeLogObserver;

static auto makeNodeName(VTNodeData const& node) -> string;
static bool isAutoOpenNode(VTNodeData const& node);

class HspObjectTreeObserver {
public:
	virtual ~HspObjectTreeObserver() {
	}

	virtual void did_create(std::size_t node_id) {
	}

	virtual void will_destroy(std::size_t node_id) {
	}

	virtual void did_focus(std::size_t node_id) {
	}
};

class HspObjectTree {
public:
	class Node;

private:
	HspObjects& objects_;
	std::unordered_map<std::size_t, Node> nodes_;
	std::size_t root_node_id_;
	std::size_t last_node_id_;
	std::vector<std::unique_ptr<HspObjectTreeObserver>> observers_;

public:
	HspObjectTree(HspObjects& objects, std::shared_ptr<HspObjectPath const> root_path)
		: objects_(objects)
		, nodes_()
		, root_node_id_(0)
		, last_node_id_(0)
		, observers_()
	{
		root_node_id_ = create_node(1, root_path);
	}

	void subscribe(std::unique_ptr<HspObjectTreeObserver>&& observer) {
		observers_.emplace_back(std::move(observer));
	}

	auto path(std::size_t node_id) -> std::optional<std::shared_ptr<HspObjectPath const>> {
		if (!nodes_.count(node_id)) {
			assert(false && u8"存在しないノードのパスを要求しています");
			return std::nullopt;
		}

		return std::make_optional(nodes_.at(node_id).path().self());
	}

	// 親ノードのIDを取得する。
	// auto parent(std::size_t node_id) const -> std::optional<std::size_t>;

	// 親ノードの子ノードのうち、自身より1つ上の子ノード (兄ノード?) のIDを取得する。
	auto previous_sibling(std::size_t node_id) const -> std::optional<std::size_t> {
		return std::nullopt;
	}

	// あるノードに焦点を当てる。
	// ノードへのパスが存在しなくなっていれば消去して、代わりに親ノードに焦点を当てる。
	// 存在していたら、子ノードの情報を更して、did_focus イベントを発行する。
	void focus(std::size_t node_id) {
		if (!nodes_.count(node_id)) {
			assert(false && u8"存在しないノードにフォーカスしようとしています");
			return;
		}

		if (!node_is_alive(node_id)) {
			auto parent = nodes_.at(node_id).parent();

			remove_node(node_id);
			focus(parent);
			return;
		}

		update_children(node_id);

		for (auto&& observer : observers_) {
			observer->did_focus(node_id);
		}
	}

	void focus_root() {
		focus(root_node_id_);
	}

	void focus_path(HspObjectPath const& path) {
		if (auto&& node_id_opt = find_by_path(path)) {
			focus(*node_id_opt);
		}
	}

private:
	auto objects() const -> HspObjects& {
		return objects_;
	}

	auto find_by_path(HspObjectPath const& path) -> std::optional<std::size_t> {
		if (path.kind() == HspObjectKind::Root) {
			return std::make_optional(root_node_id_);
		}

		if (auto&& node_id_opt = find_by_path(path.parent())) {
			auto&& node = nodes_.at(*node_id_opt);
			for (auto child_id : node.children()) {
				if (nodes_.at(child_id).path().equals(path)) {
					return std::make_optional(child_id);
				}
			}
		}
		return std::nullopt;
	}

	auto create_node(std::size_t parent_id, std::shared_ptr<HspObjectPath const> path) -> std::size_t {
		auto node_id = ++last_node_id_;
		nodes_.emplace(node_id, Node{ parent_id, path });
		return node_id;
	}

	void notify_did_create(std::size_t node_id) {
		auto&& node = nodes_.at(node_id);
		for (auto&& observer : observers_) {
			observer->did_create(node_id);
		}
	}

	void remove_node(std::size_t node_id) {
		if (!nodes_.count(node_id)) {
			assert(false && u8"存在しないノードを削除しようとしています");
			return;
		}

		auto&& node = nodes_.at(node_id);

		// 子ノードをすべて削除する。
		{
			auto&& children = node.children();
			while (!children.empty()) {
				auto child_id = children.back();
				children.pop_back();

				remove_node(child_id);
			}
		}

		// 親ノードの子ノードリストから除外する。
		if (node.parent() != node_id) {
			auto&& parent = nodes_.at(node.parent());
			auto&& siblings = parent.children();
			siblings.erase(std::find(std::begin(siblings), std::end(siblings), node_id));
		}

		for (auto&& observer : observers_) {
			observer->will_destroy(node_id);
		}
		nodes_.erase(node_id);
	}

	// 指定したノードに対応するパスの子要素のうち、
	// 無効なパスに対応する子ノードがあれば削除し、
	// 有効なパスに対応する子ノードがなければ挿入する。
	void update_children(std::size_t node_id) {
		if (!nodes_.count(node_id)) {
			assert(false && u8"存在しないノードの子ノード更新をしようとしています");
			return;
		}

		auto&& children = nodes_.at(node_id).children();
		auto&& path = nodes_.at(node_id).path();
		auto child_count = path.child_count(objects());

		// FIXME: アルゴリズムを改善する
		// 挿入・削除はインデックスの計算がとてもめんどうなので、いまのところは避ける。
		// 子パスと子ノードの前から n 個が一致する、という最大の n を探す。
		// 前から n 個を除く子ノードをすべて削除して、前から n 個を除く子パスを末尾に挿入する。

		auto n = std::size_t{};

		while (n < children.size() && n < child_count) {
			auto&& child_path = path.child_at(n, objects());
			auto&& child_node = nodes_.at(children[n]);

			if (!child_path->equals(child_node.path())) {
				break;
			}

			n++;
		}

		// 削除
		for (auto i = children.size(); i >= 1;) {
			i--;

			if (i < n) {
				break;
			}

			remove_node(children.at(i));
		}

		// 挿入
		for (auto i = n; i < child_count; i++) {
			auto&& child_path = path.child_at(i, objects());

			auto child_node_id = create_node(node_id, child_path);
			children.push_back(child_node_id);
			notify_did_create(child_node_id);
		}
	}

	bool node_is_alive(std::size_t node_id) const {
		auto&& iter = nodes_.find(node_id);
		if (iter == nodes_.end()) {
			assert(false && u8"存在しないノードの生存検査をしようとしています");
			return false;
		}

		auto&& node = iter->second;
		return node.path().is_alive(objects());
	}

public:
	class HspObjectTree::Node {
		std::size_t parent_;
		std::shared_ptr<HspObjectPath const> path_;
		std::vector<std::size_t> children_;

	public:
		Node(std::size_t parent, std::shared_ptr<HspObjectPath const> path)
			: parent_(parent)
			, path_(path)
			, children_()
		{
		}

		// 親ノードのID。ただし、ルートノードなら自身のID。
		auto parent() const -> std::size_t {
			return parent_;
		}

		auto path() const -> HspObjectPath const& {
			return *path_;
		}

		auto children() -> std::vector<std::size_t>& {
			return children_;
		}

		auto children() const -> std::vector<std::size_t> const& {
			return children_;
		}
	};
};

struct VTView::Impl
{
	VTView& self_;

	HspObjectTree object_tree_;

	unordered_map<VTNodeData const*, HTREEITEM> itemFromNode_;

	//ノードごとのビューウィンドウのキャレット位置
	unordered_map<HTREEITEM, int> viewCaret_;

	shared_ptr<TvObserver> observer_;
	shared_ptr<LogObserver> logObserver_;

	HTREEITEM hNodeDynamic_, hNodeScript_, hNodeLog_;

	unordered_map<HTREEITEM, std::size_t> node_ids_;

public:
	auto itemFromNode(VTNodeData const* p) const -> HTREEITEM;

	auto viewCaretFromNode(HTREEITEM hItem) const -> int;
};

struct TvObserver
	: VTNodeData::Observer
{
	VTView::Impl& self;
public:
	TvObserver(VTView::Impl& self);
	void onInit(VTNodeData& node) override;
	void onTerm(VTNodeData& node) override;
};

struct VarTreeLogObserver
	: LogObserver
{
	VTView::Impl& self;
public:
	VarTreeLogObserver(VTView::Impl& self) : self(self) {}
	void did_change() override;
};

class HspObjectTreeObserverImpl
	: public HspObjectTreeObserver
{
	HspObjectTree& tree_;
	HspObjects& objects_;

public:
	HspObjectTreeObserverImpl(HspObjectTree& tree, HspObjects& objects)
		: tree_(tree)
		, objects_(objects)
	{
	}

	void log(std::string&& text) {
		Knowbug::get_logger()->append_line(HspString{ std::move(text) }.to_os_string().as_ref());
	}

	virtual void did_create(std::size_t node_id) {
		auto&& path = tree_.path(node_id);
		if (!path) {
			return;
		}

		auto&& name = (**path).name(objects_);
		log(strf("create '%s' (%d)", name, node_id));
	}

	virtual void will_destroy(std::size_t node_id) {
		auto&& path = tree_.path(node_id);
		if (!path) {
			return;
		}

		auto&& name = (**path).name(objects_);
		log(strf("destroy '%s' (%d)", name, node_id));
	}

	virtual void did_focus(std::size_t node_id) {
		auto&& path = tree_.path(node_id);
		if (!path) {
			return;
		}

		auto&& name = (**path).name(objects_);
		log(strf("focus '%s' (%d)", name, node_id));
	}
};

VTView::VTView(hpiutil::DInfo const& debug_segment, HspObjects& objects, HspStaticVars& static_vars)
	: debug_segment_(debug_segment)
	, objects_(objects)
	, static_vars_(static_vars)
	, p_(new Impl { *this, HspObjectTree{ objects, HspObjectPath::get_root().self() } })
{
	// Register observers
	p_->observer_ = std::make_shared<TvObserver>(*p_);
	VTNodeData::registerObserver(p_->observer_);

	p_->logObserver_ = std::make_shared<VarTreeLogObserver>(*p_);
	VTRoot::log().setLogObserver(p_->logObserver_);

	// Initialize tree
	VTRoot::instance().updateDeep();

#ifdef with_WrapCall
	p_->hNodeDynamic_ = p_->itemFromNode(&VTRoot::dynamic());
#endif
	p_->hNodeScript_  = p_->itemFromNode(&VTRoot::script());
	p_->hNodeLog_     = p_->itemFromNode(&VTRoot::log());

	// 新APIの部分
	auto observer = std::unique_ptr<HspObjectTreeObserver>{ std::make_unique<HspObjectTreeObserverImpl>(p_->object_tree_, objects) };
	p_->object_tree_.subscribe(std::move(observer));
	p_->object_tree_.focus_root();
}

VTView::~VTView()
{
}

auto VTView::Impl::itemFromNode(VTNodeData const* p) const -> HTREEITEM
{
	auto const iter = itemFromNode_.find(p);
	return (iter != itemFromNode_.end()) ? iter->second : nullptr;
}

TvObserver::TvObserver(VTView::Impl& self)
	: self(self)
{
	self.itemFromNode_[&VTRoot::instance()] = TVI_ROOT;
}

void TvObserver::onInit(VTNodeData& node)
{
	auto tv = VarTreeView{ hwndVarTree };

	auto const parent = node.parent();
	if ( ! parent ) return; // VTRoot

	auto const hParent = self.itemFromNode(parent);
	assert(hParent != nullptr);

	auto const hItem = tv.insert_item(hParent, makeNodeName(node).c_str(), &node);

	assert(self.itemFromNode_[&node] == nullptr);
	self.itemFromNode_[&node] = hItem;

	self.viewCaret_.erase(hItem);

	if ( isAutoOpenNode(*parent) ) {
		tv.expand_item(hParent);
	}
}

void TvObserver::onTerm(VTNodeData& node)
{
	auto tv = VarTreeView{ hwndVarTree };

	if ( auto const hItem = self.itemFromNode(&node) ) {
		self.itemFromNode_[&node] = nullptr;
		tv.delete_item(hItem);
	}
}

void VarTreeLogObserver::did_change()
{
	auto tv = VarTreeView{ hwndVarTree };

	if ( tv.selected_item() == self.hNodeLog_ ) {
		Dialog::View::update();
	}
}

void VTView::update()
{
#ifdef with_WrapCall
	VTRoot::dynamic().updateDeep();
#endif

	Dialog::View::update();
}

auto VTView::tryGetNodeData(HTREEITEM hItem) const -> optional_ref<VTNodeData> {
	return VarTreeView{ hwndVarTree }.tryGetNodeData(hItem);
}

// ノードに対応する文字列を得る
auto VTView::getItemVarText(HTREEITEM hItem) const -> std::unique_ptr<OsString>
{
	class GetText
		: public VTNodeData::Visitor
	{
		CVarinfoText varinf;
		unique_ptr<OsString> result;

	public:
		GetText(CVarinfoText&& varinf)
			: varinf(std::move(varinf))
		{
		}

		void fModule(VTNodeModule const& node) override
		{
			varinf.add(*node.path());
		}
		void fVar(VTNodeVar const& node) override
		{
			auto const& path = node.path();
			if (path) {
				varinf.add(*path);
				return;
			}

			varinf.addVar(node.pval(), node.name().c_str());
		}
		void fSysvarList(VTNodeSysvarList const&) override
		{
			varinf.addSysvarsOverview();
		}
		void fLog(VTNodeLog const& node) override
		{
			result = std::make_unique<OsString>(node.content().to_owned()); // FIXME: 無駄なコピー
		}
		void fScript(VTNodeScript const& node) override
		{
			if ( auto p = node.fetchScriptAll(g_dbginfo->curPos().fileRefName()) ) {
				result = std::move(p);
			} else {
				result = std::make_unique<OsString>(HspStringView{ g_dbginfo->getCurInfString().data() }.to_os_string());
			}
		}
		void fGeneral(VTNodeGeneral const&) override
		{
			varinf.addGeneralOverview();
		}
		void fSysvar(VTNodeSysvar const& node) override
		{
			varinf.addSysvar(node.id());
		}
#ifdef with_WrapCall
		void fDynamic(VTNodeDynamic const&) override
		{
			varinf.addCallsOverview();
		}
		void fInvoke(VTNodeInvoke const& node) override
		{
			varinf.addCall(node.callinfo());
		}
#endif
		auto apply(VTNodeData const& node) -> unique_ptr<OsString>
		{
			node.acceptVisitor(*this);
			return result
				? std::move(result)
				: std::make_unique<OsString>(HspStringView{ varinf.getString().data() }.to_os_string());
		}
	};

	if ( auto node = tryGetNodeData(hItem) ) {
		return GetText{ CVarinfoText{ debug_segment_, objects_, static_vars_ } }.apply(*node);
	} else {
		return std::make_unique<OsString>(TEXT("(not_available)"));
	}
}

void VTView::saveCurrentViewCaret(int vcaret)
{
	auto tv = VarTreeView{ hwndVarTree };

	if ( auto const hItem = tv.selected_item() ) {
		p_->viewCaret_[hItem] = vcaret;
	}
}

auto VTView::Impl::viewCaretFromNode(HTREEITEM hItem) const -> int
{
	auto iter = viewCaret_.find(hItem);
	return (iter != viewCaret_.end() ? iter->second : 0);
}

void VTView::selectNode(VTNodeData const& node)
{
	auto tv = VarTreeView{ hwndVarTree };

	if ( auto const hItem = p_->itemFromNode(&node) ) {
		tv.select_item(hItem);
	}
}

void VTView::updateViewWindow()
{
	auto tv = VarTreeView{ hwndVarTree };

	auto const hItem = tv.selected_item();
	if ( hItem ) {
		static auto stt_prevSelection = HTREEITEM { nullptr };
		if ( hItem == stt_prevSelection ) {
			Dialog::View::saveCurrentCaret();
		} else {
			stt_prevSelection = hItem;
		}

		auto varinfoText = getItemVarText(hItem);
		Dialog::View::setText(varinfoText->as_ref());

		//+script ノードなら現在の実行位置を選択
		if ( hItem == p_->hNodeScript_ ) {
			auto const iLine = g_dbginfo->curPos().line();
			Dialog::View::scroll(std::max(0, iLine - 3), 0);
			Dialog::View::selectLine(iLine);

		//+log ノードの自動スクロール
		} else if ( hItem == p_->hNodeLog_
			&& g_config->scrollsLogAutomatically
			) {
			Dialog::View::scrollBottom();

		} else {
			Dialog::View::scroll(p_->viewCaretFromNode(hItem), 0);
		}

		// 新APIの試験
		{
			class FocusOnNode
				: public VTNodeData::Visitor
			{
				HspObjectTree& object_tree_;

			public:
				FocusOnNode(HspObjectTree& object_tree)
					: object_tree_(object_tree)
				{
				}

				void fModule(VTNodeModule const& node) override
				{
					object_tree_.focus_path(*node.path());
				}
				void fVar(VTNodeVar const& node) override
				{
					auto const& path = node.path();
					if (path) {
						object_tree_.focus_path(*node.path());
						return;
					}
				}
			};

			if (auto node = tv.tryGetNodeData(hItem)) {
				node->acceptVisitor(FocusOnNode{ p_->object_tree_ });
			}
		}
	}
}

// ノードにつけるべき名前
auto makeNodeName(VTNodeData const& node) -> string
{
	struct matcher : VTNodeData::Visitor
	{
		auto apply(VTNodeData const& node) -> string
		{
			result = node.name(); // default
			node.acceptVisitor(*this);
			return std::move(result);
		}

		void fInvoke(VTNodeInvoke const& node) override { result = "\'" + node.name(); }
	private:
		string result;
	};

	return matcher {}.apply(node);
}

// 自動的に開くべきノードか？
static bool isAutoOpenNode(VTNodeData const& node)
{
	struct matcher : VTNodeData::Visitor
	{
		bool apply(VTNodeData const& node)
		{
			result = true; // default
			node.acceptVisitor(*this);
			return result;
		}

		void fModule(VTNodeModule const& node) override
		{
			result = (node.name() == "@");
		}
		void fSysvarList(VTNodeSysvarList const&) override
		{
			result = false;
		}
	private:
		bool result;
	};

	return matcher {}.apply(node);
}
