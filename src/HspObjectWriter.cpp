#include "encoding.h"
#include "module/CStrWriter.h"
#include "HspObjects.h"
#include "HspObjectWriter.h"
#include "CVarinfoText.h"

extern auto stringizeVartype(PVal const* pval) -> string;

// -----------------------------------------------
// ヘルパー
// -----------------------------------------------

static bool string_is_compact(char const* str) {
	for (auto i = std::size_t{}; i < 64; i++) {
		if (str[i] == '\0') {
			return true;
		}
		if (str[i] == '\n') {
			return false;
		}
	}
	return false;
}

static bool object_path_is_compact(HspObjectPath const& path, HspObjects& objects) {
	switch (path.kind()) {
	case HspObjectKind::Str:
		return string_is_compact(path.as_str().value(objects));

	case HspObjectKind::Int:
		return true;

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
	CVarinfoText& varinf_;
	CStrWriter& writer_;

public:
	explicit HspObjectWriterImpl(HspObjects& objects, CVarinfoText& varinf, CStrWriter& writer);

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
	TableForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf);

	void on_module(HspObjectPath::Module const& path) override;

	void on_static_var(HspObjectPath::StaticVar const& path) override;
};

// ブロックフォーム。
// 数行を使って、オブジェクトの情報を表示する。
// 構築した文字列は、テーブルフォームの内部に埋め込まれる。
class HspObjectWriterImpl::BlockForm
	: public HspObjectWriterImpl
{
public:
	BlockForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf);

	void on_module(HspObjectPath::Module const& path) override;

	void on_static_var(HspObjectPath::StaticVar const& path) override;

	void on_element(HspObjectPath::Element const& path) override;

	void on_param(HspObjectPath::Param const& path) override;

	void on_str(HspObjectPath::Str const& path) override;

	void on_int(HspObjectPath::Int const& path) override;

	void on_flex(HspObjectPath::Flex const& path) override;

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
	FlowForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf);

	void on_static_var(HspObjectPath::StaticVar const& path) override;

	void on_str(HspObjectPath::Str const& path) override;

	void on_int(HspObjectPath::Int const& path) override;

	void on_flex(HspObjectPath::Flex const& path) override;
};

// -----------------------------------------------
// 基底クラスの実装
// -----------------------------------------------

HspObjectWriterImpl::HspObjectWriterImpl(HspObjects& objects, CVarinfoText& varinf, CStrWriter& writer)
	: Visitor(objects)
	, varinf_(varinf)
	, writer_(writer)
{
}

auto HspObjectWriterImpl::to_table_form() -> HspObjectWriterImpl::TableForm {
	return TableForm{ objects(), writer(), varinf_ };
}

auto HspObjectWriterImpl::to_block_form() -> HspObjectWriterImpl::BlockForm {
	return BlockForm{ objects(), writer(), varinf_ };
}

auto HspObjectWriterImpl::to_flow_form() -> HspObjectWriterImpl::FlowForm {
	return FlowForm{ objects(), writer(), varinf_ };
}

// -----------------------------------------------
// テーブルフォーム
// -----------------------------------------------

HspObjectWriterImpl::TableForm::TableForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf)
	: HspObjectWriterImpl(objects, varinf, writer)
{
}

void HspObjectWriterImpl::TableForm::on_module(HspObjectPath::Module const& path) {
	auto&& w = writer();
	auto&& objects_ = objects();
	auto&& name = path.name(objects_);

	w.cat("[");
	w.cat(name.data());
	w.catln("]");

	for (auto i = std::size_t{}; i < path.child_count(objects_); i++) {
		auto&& child_path = path.child_at(i, objects_);
		to_block_form().accept(*child_path);
	}
}

void HspObjectWriterImpl::TableForm::on_static_var(HspObjectPath::StaticVar const& path) {
	auto pval = objects().static_var_path_to_pval(path);
	auto&& name = path.name(objects());
	auto type = path.type(objects());

	// 新APIが実装済みのケース
	if (type == HspType::Str || type == HspType::Int || type == HspType::Struct) {
		auto&& w = writer();
		auto&& metadata = path.metadata(objects());

		// 変数に関する情報
		w.cat("変数名: ");
		w.catln(name);

		w.cat("変数型: ");
		w.catln(stringizeVartype(pval));

		w.cat("アドレス: ");
		w.catPtr(metadata.data_ptr());
		w.cat(", ");
		w.catPtr(metadata.master_ptr());
		w.catCrlf();

		w.cat("サイズ: ");
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
		return;
	}

	// 旧APIにフォールバック
	varinf_.addVar(pval, name.data());
}

// -----------------------------------------------
// ブロックフォーム
// -----------------------------------------------

HspObjectWriterImpl::BlockForm::BlockForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf)
	: HspObjectWriterImpl(objects, varinf, writer)
{
}

void HspObjectWriterImpl::BlockForm::on_module(HspObjectPath::Module const& path) {
	auto&& name = path.name(objects());

	// (入れ子の)モジュールは名前だけ表示しておく
	writer().catln(name.data());
}

void HspObjectWriterImpl::BlockForm::on_static_var(HspObjectPath::StaticVar const& path) {
	auto&& name = path.name(objects());
	auto short_name = hpiutil::nameExcludingScopeResolution(name.data());

	writer().cat(short_name.data());
	writer().cat("\t= ");

	to_flow_form().accept(path);

	writer().catCrlf();
}

void HspObjectWriterImpl::BlockForm::on_element(HspObjectPath::Element const& path) {
	add_name_children(path);
}

void HspObjectWriterImpl::BlockForm::on_param(HspObjectPath::Param const& path) {
	add_name_children(path);
}

void HspObjectWriterImpl::BlockForm::on_str(HspObjectPath::Str const& path) {
	auto&& w = writer();
	auto&& value = path.value(objects());

	w.catln(value);
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

	auto&& module_name = path.module_name(o);

	w.cat(".module = ");
	w.cat(module_name);
	w.catCrlf();

	accept_children(path);
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

HspObjectWriterImpl::FlowForm::FlowForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf)
	: HspObjectWriterImpl(objects, varinf, writer)
{
}

void HspObjectWriterImpl::FlowForm::on_static_var(HspObjectPath::StaticVar const& path) {
	auto&& w = writer();
	auto type = path.type(objects());
	auto&& type_name = objects().type_name(type);
	auto child_count = path.child_count(objects());

	// FIXME: 多次元配列の表示を改善する

	w.cat("<");
	w.cat(type_name.data());
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

void HspObjectWriterImpl::FlowForm::on_str(HspObjectPath::Str const& path) {
	auto&& value = path.value(objects());
	auto&& literal = hpiutil::literalFormString(value);

	writer().cat(literal);
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

	w.cat("<struct>");
}

// -----------------------------------------------
// 公開クラス
// -----------------------------------------------

HspObjectWriter::HspObjectWriter(HspObjects& objects, CVarinfoText& varinf, CStrWriter& writer)
	: objects_(objects)
	, varinf_(varinf)
	, writer_(writer)
{
}

void HspObjectWriter::write_table_form(HspObjectPath const& path) {
	HspObjectWriterImpl::TableForm{ objects_, writer_, varinf_ }.accept(path);
}

void HspObjectWriter::write_block_form(HspObjectPath const& path) {
	HspObjectWriterImpl::BlockForm{ objects_, writer_, varinf_ }.accept(path);
}

void HspObjectWriter::write_flow_form(HspObjectPath const& path) {
	HspObjectWriterImpl::FlowForm{ objects_, writer_, varinf_ }.accept(path);
}
