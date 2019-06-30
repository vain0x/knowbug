// FIXME: 無限のネストに対処
// FIXME: 文字コードの混在に対処

#include "encoding.h"
#include "module/CStrWriter.h"
#include "HspObjects.h"
#include "HspObjectWriter.h"

// -----------------------------------------------
// ヘルパー
// -----------------------------------------------

// FIXME: クローン変数なら & をつける
static void write_array_type(CStrWriter& writer, Utf8StringView const& type_name, HspDimIndex const& lengths) {
	writer.cat(as_native(type_name));

	switch (lengths.dim()) {
	case 0:
		writer.cat("(empty)");
		return;
	case 1:
		// (%d)
		writer.cat("(");
		writer.catSize(lengths.at(0));
		writer.cat(")");
		return;
	default:
		// (%d, %d, ..) (%d in total)
		writer.cat("(");
		for (auto i = std::size_t{}; i < lengths.dim(); i++) {
			if (i != 0) {
				writer.cat(", ");
			}
			writer.catSize(lengths.at(i));
		}
		writer.cat(") (");
		writer.catSize(lengths.size());
		writer.cat(" in total)");
		return;
	}
}

static bool string_is_compact(Utf8StringView const& str) {
	if (str.size() >= 64) {
		return false;
	}

	if (str.find((Utf8Char)'\n') != Utf8StringView::npos) {
		return false;
	}

	return true;
}

static bool object_path_is_compact(HspObjectPath const& path, HspObjects& objects) {
	switch (path.kind()) {
	case HspObjectKind::Label:
	case HspObjectKind::Double:
	case HspObjectKind::Int:
	case HspObjectKind::Unknown:
		return true;

	case HspObjectKind::Str:
		return string_is_compact(path.as_str().value(objects));

	case HspObjectKind::Flex:
		return path.as_flex().is_nullmod(objects);

	default:
		return false;
	}
}

// -----------------------------------------------
// 実装クラス
// -----------------------------------------------

class HspObjectWriterImpl
	: public HspObjectPath::Visitor
{
public:
	class TableForm;
	class BlockForm;
	class FlowForm;

private:
	CStrWriter& writer_;

public:
	explicit HspObjectWriterImpl(HspObjects& objects, CStrWriter& writer);

	auto writer() -> CStrWriter& {
		return writer_;
	}

	auto to_table_form() -> TableForm;

	auto to_block_form() -> BlockForm;

	auto to_flow_form() -> FlowForm;
};

// テーブルフォーム。
// 文字列全体を使って、オブジェクトの詳細情報を表示する。
class HspObjectWriterImpl::TableForm
	: public HspObjectWriterImpl
{
public:
	TableForm(HspObjects& objects, CStrWriter& writer);

	void accept_default(HspObjectPath const& path) override;

	void on_static_var(HspObjectPath::StaticVar const& path) override;

	void on_system_var_list(HspObjectPath::SystemVarList const& path) override;

	void on_log(HspObjectPath::Log const& path) override;

	void on_script(HspObjectPath::Script const& path) override;

	// FIXME: unavailable に対応

	// FIXME: CallFrame ならシグネチャと呼び出し位置とダンプを表示する
};

// ブロックフォーム。
// 数行を使って、オブジェクトの情報を表示する。
// 構築した文字列は、テーブルフォームの内部に埋め込まれる。
class HspObjectWriterImpl::BlockForm
	: public HspObjectWriterImpl
{
public:
	BlockForm(HspObjects& objects, CStrWriter& writer);

	void accept_default(HspObjectPath const& path) override;

	void on_module(HspObjectPath::Module const& path) override;

	void on_static_var(HspObjectPath::StaticVar const& path) override;

	void on_label(HspObjectPath::Label const& path) override;

	void on_str(HspObjectPath::Str const& path) override;

	void on_double(HspObjectPath::Double const& path) override;

	void on_int(HspObjectPath::Int const& path) override;

	void on_flex(HspObjectPath::Flex const& path) override;

	void on_unknown(HspObjectPath::Unknown const& path) override;

private:
	void add_name_children(HspObjectPath const& path);
};

// フローフォーム。
// オブジェクトの情報を簡易的に表示する。
// 基本的に改行を含まない。
class HspObjectWriterImpl::FlowForm
	: public HspObjectWriterImpl
{
public:
	FlowForm(HspObjects& objects, CStrWriter& writer);

	void on_static_var(HspObjectPath::StaticVar const& path) override;

	void on_label(HspObjectPath::Label const& path) override;

	void on_str(HspObjectPath::Str const& path) override;

	void on_double(HspObjectPath::Double const& path) override;

	void on_int(HspObjectPath::Int const& path) override;

	void on_flex(HspObjectPath::Flex const& path) override;

	void on_unknown(HspObjectPath::Unknown const& path) override;
};

// -----------------------------------------------
// 基底クラスの実装
// -----------------------------------------------

HspObjectWriterImpl::HspObjectWriterImpl(HspObjects& objects, CStrWriter& writer)
	: Visitor(objects)
	, writer_(writer)
{
}

auto HspObjectWriterImpl::to_table_form() -> HspObjectWriterImpl::TableForm {
	return TableForm{ objects(), writer() };
}

auto HspObjectWriterImpl::to_block_form() -> HspObjectWriterImpl::BlockForm {
	return BlockForm{ objects(), writer() };
}

auto HspObjectWriterImpl::to_flow_form() -> HspObjectWriterImpl::FlowForm {
	return FlowForm{ objects(), writer() };
}

// -----------------------------------------------
// テーブルフォーム
// -----------------------------------------------

HspObjectWriterImpl::TableForm::TableForm(HspObjects& objects, CStrWriter& writer)
	: HspObjectWriterImpl(objects, writer)
{
}

void HspObjectWriterImpl::TableForm::accept_default(HspObjectPath const& path) {
	auto&& o = objects();
	auto&& w = writer();

	w.cat("[");
	w.cat(path.name(o));
	w.catln("]");

	to_block_form().accept_children(path);
}

void HspObjectWriterImpl::TableForm::on_static_var(HspObjectPath::StaticVar const& path) {
	auto&& o = objects();
	auto&& w = writer();

	auto&& name = path.name(o);
	auto type = path.type(o);
	auto&& type_name = o.type_to_name(type);
	auto&& metadata = path.metadata(o);

	// 変数に関する情報
	w.cat(u8"変数名: ");
	w.catln(name);

	w.cat(u8"変数型: ");
	write_array_type(w, type_name, metadata.lengths());
	w.catCrlf();

	w.cat(u8"アドレス: ");
	w.catPtr(metadata.data_ptr());
	w.cat(", ");
	w.catPtr(metadata.master_ptr());
	w.catCrlf();

	w.cat(u8"サイズ: ");
	w.catSize(metadata.data_size());
	w.cat(" / ");
	w.catSize(metadata.block_size());
	w.cat(" [byte]");
	w.catCrlf();
	w.catCrlf();

	// 変数の内容に関する情報
	to_block_form().accept_children(path);
	w.catCrlf();

	// メモリダンプ
	w.catDump(metadata.block_ptr(), metadata.block_size());
}

void HspObjectWriterImpl::TableForm::on_system_var_list(HspObjectPath::SystemVarList const& path) {
	writer().catln(u8"[システム変数]");
	to_block_form().accept_children(path);
}

void HspObjectWriterImpl::TableForm::on_log(HspObjectPath::Log const& path) {
	auto&& content = path.content(objects());
	assert((content.empty() || (char)content.back() == '\n') && u8"Log must be end with line break");

	writer().cat(content);
}

void HspObjectWriterImpl::TableForm::on_script(HspObjectPath::Script const& path) {
	auto&& content = path.content(objects());

	writer().catln(content);
}

// -----------------------------------------------
// ブロックフォーム
// -----------------------------------------------

HspObjectWriterImpl::BlockForm::BlockForm(HspObjects& objects, CStrWriter& writer)
	: HspObjectWriterImpl(objects, writer)
{
}

void HspObjectWriterImpl::BlockForm::accept_default(HspObjectPath const& path) {
	add_name_children(path);

	// FIXME: システム変数や引数リストならメモリダンプを出力できる
}

void HspObjectWriterImpl::BlockForm::on_module(HspObjectPath::Module const& path) {
	auto&& name = path.name(objects());

	// (入れ子の)モジュールは名前だけ表示しておく
	writer().catln(name.data());
}

void HspObjectWriterImpl::BlockForm::on_static_var(HspObjectPath::StaticVar const& path) {
	auto name = as_native(path.name(objects()));
	auto short_name = hpiutil::nameExcludingScopeResolution(name);

	writer().cat(short_name);
	writer().cat("\t= ");

	to_flow_form().accept(path);

	writer().catCrlf();
}

void HspObjectWriterImpl::BlockForm::on_label(HspObjectPath::Label const& path) {
	to_flow_form().on_label(path);
	writer().catCrlf();
}

void HspObjectWriterImpl::BlockForm::on_str(HspObjectPath::Str const& path) {
	auto&& w = writer();
	auto&& value = path.value(objects());

	w.catln(value);
}

void HspObjectWriterImpl::BlockForm::on_double(HspObjectPath::Double const& path) {
	auto&& w = writer();
	auto value = path.value(objects());

	w.catln(strf("%.16f", value));
}

void HspObjectWriterImpl::BlockForm::on_int(HspObjectPath::Int const& path) {
	auto&& w = writer();
	auto value = path.value(objects());

	w.catln(strf("%-10d (0x%08X)", value, value));
}

void HspObjectWriterImpl::BlockForm::on_flex(HspObjectPath::Flex const& path) {
	auto&& w = writer();
	auto&& o = objects();

	if (path.is_nullmod(o)) {
		w.catln("<null>");
		return;
	}

	// FIXME: クローンなら & をつける
	auto&& module_name = path.module_name(o);

	w.cat(".module = ");
	w.cat(module_name);
	w.catCrlf();

	accept_children(path);
}

void HspObjectWriterImpl::BlockForm::on_unknown(HspObjectPath::Unknown const& path) {
	to_flow_form().on_unknown(path);
	writer().catCrlf();
}

void HspObjectWriterImpl::BlockForm::add_name_children(HspObjectPath const& path) {
	auto&& w = writer();
	auto&& o = objects();
	auto&& name = path.name(o);

	auto child_count = path.child_count(o);
	if (child_count == 0) {
		w.catln(name.data());
		return;
	}

	auto&& first_child = path.child_at(0, o);
	if (child_count == 1 && object_path_is_compact(*first_child, o)) {
		w.cat(name.data());
		w.cat("\t= ");
		to_block_form().accept(*first_child);
		return;
	}

	w.cat(name.data());
	w.catln(":");
	w.indent();
	to_block_form().accept_children(path);
	w.unindent();
}

// -----------------------------------------------
// フローフォーム
// -----------------------------------------------

HspObjectWriterImpl::FlowForm::FlowForm(HspObjects& objects, CStrWriter& writer)
	: HspObjectWriterImpl(objects, writer)
{
}

void HspObjectWriterImpl::FlowForm::on_static_var(HspObjectPath::StaticVar const& path) {
	auto&& w = writer();
	auto type = path.type(objects());
	auto&& type_name = objects().type_to_name(type);
	auto child_count = path.child_count(objects());

	// FIXME: 多次元配列の表示を改善する

	w.cat("<");
	w.cat(as_native(type_name));
	w.cat(">[");

	for (auto i = std::size_t{}; i < child_count; i++) {
		auto&& child = path.child_at(i, objects());

		if (i != 0) {
			w.cat(", ");
		}

		accept(*child);
	}

	w.cat("]");
}

void HspObjectWriterImpl::FlowForm::on_label(HspObjectPath::Label const& path) {
	auto&& o = objects();
	auto&& w = writer();

	if (path.is_null(o)) {
		w.cat("<null-label>");
		return;
	}

	if (auto&& name_opt = path.static_label_name(o)) {
		w.cat("*");
		w.cat(*name_opt);
		return;
	}

	if (auto&& id_opt = path.static_label_id(o)) {
		w.cat("*#");
		w.catSize(*id_opt);
		return;
	}

	w.cat("<label>");
}

void HspObjectWriterImpl::FlowForm::on_str(HspObjectPath::Str const& path) {
	auto&& value = path.value(objects());
	auto&& literal = hpiutil::literalFormString(as_native(value).data());

	writer().cat(literal);
}

void HspObjectWriterImpl::FlowForm::on_double(HspObjectPath::Double const& path) {
	writer().cat(strf("%f", path.value(objects())));
}

void HspObjectWriterImpl::FlowForm::on_int(HspObjectPath::Int const& path) {
	writer().cat(strf("%d", path.value(objects())));
}

void HspObjectWriterImpl::FlowForm::on_flex(HspObjectPath::Flex const& path) {
	auto&& w = writer();
	auto&& o = objects();

	if (path.is_nullmod(o)) {
		w.cat("null");
		return;
	}

	// FIXME: クローンなら & をつける
	auto&& module_name = path.module_name(o);
	w.cat(module_name);
	w.cat("{");

	for (auto i = std::size_t{}; i < path.child_count(o); i++) {
		auto&& child_path = path.child_at(i, o);

		if (i != 0) {
			w.cat(", ");
		}
		accept(*child_path);
	}

	w.cat("}");
}

void HspObjectWriterImpl::FlowForm::on_unknown(HspObjectPath::Unknown const& path) {
	// FIXME: 型名を表示する
	writer().cat("<unknown>");
}

// -----------------------------------------------
// 公開クラス
// -----------------------------------------------

HspObjectWriter::HspObjectWriter(HspObjects& objects, CStrWriter& writer)
	: objects_(objects)
	, writer_(writer)
{
}

void HspObjectWriter::write_table_form(HspObjectPath const& path) {
	HspObjectWriterImpl::TableForm{ objects_, writer_ }.accept(path);
}

void HspObjectWriter::write_block_form(HspObjectPath const& path) {
	HspObjectWriterImpl::BlockForm{ objects_, writer_ }.accept(path);
}

void HspObjectWriter::write_flow_form(HspObjectPath const& path) {
	HspObjectWriterImpl::FlowForm{ objects_, writer_ }.accept(path);
}
