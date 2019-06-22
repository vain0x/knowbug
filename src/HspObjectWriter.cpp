#include "encoding.h"
#include "module/CStrWriter.h"
#include "HspObjects.h"
#include "HspObjectWriter.h"
#include "CVarinfoText.h"

extern auto stringizeVartype(PVal const* pval) -> string;

// -----------------------------------------------
// テーブルフォーム
// -----------------------------------------------

HspObjectWriter::HspObjectWriter(HspObjects& objects, CStrWriter& writer)
	: Visitor(objects)
	, writer_(writer)
{
}

HspObjectWriter::TableForm::TableForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf)
	: HspObjectWriter(objects, writer)
	, varinf_(varinf)
{
}

void HspObjectWriter::TableForm::on_module(HspObjectPath::Module const& path) {
	auto&& w = writer();
	auto&& objects_ = objects();
	auto&& name = path.name(objects_);

	w.cat("[");
	w.cat(name.data());
	w.catln("]");

	for (auto i = std::size_t{}; i < path.child_count(objects_); i++) {
		auto&& child_path = path.child_at(i, objects_);
		varinf_.to_block_form().accept(*child_path);
	}
}

void HspObjectWriter::TableForm::on_static_var(HspObjectPath::StaticVar const& path) {
	auto pval = objects().static_var_to_pval(path.static_var_id());
	auto name = path.name(objects());

	// 新APIが実装済みのケース
	if (path.type(objects()) == HspType::Int) {
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
		varinf_.to_block_form().accept_children(path);
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

HspObjectWriter::BlockForm::BlockForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf)
	: HspObjectWriter(objects, writer)
	, varinf_(varinf)
{
}

void HspObjectWriter::BlockForm::on_module(HspObjectPath::Module const& path) {
	auto&& name = path.name(objects());

	// (入れ子の)モジュールは名前だけ表示しておく
	writer().catln(name.data());
}

void HspObjectWriter::BlockForm::on_static_var(HspObjectPath::StaticVar const& path) {
	auto&& name = path.name(objects());
	auto short_name = hpiutil::nameExcludingScopeResolution(name.data());

	writer().cat(short_name.data());
	writer().cat("\t= ");

	varinf_.to_flow_form().accept(path);

	writer().catCrlf();
}

void HspObjectWriter::BlockForm::on_element(HspObjectPath::Element const& path) {
	auto& w = writer();
	auto&& name = path.name(objects());

	w.cat(name.data());
	w.cat("\t= ");

	varinf_.to_flow_form().accept(path);

	w.catCrlf();
}

// -----------------------------------------------
// フローフォーム
// -----------------------------------------------

HspObjectWriter::FlowForm::FlowForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf)
	: HspObjectWriter(objects, writer)
	, varinf_(varinf)
{
}

void HspObjectWriter::FlowForm::on_static_var(HspObjectPath::StaticVar const& path) {
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

void HspObjectWriter::FlowForm::on_int(HspObjectPath::Int const& path) {
	writer().cat(strf("%d", path.value(objects())));
}
