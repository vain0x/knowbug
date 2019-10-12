#include "pch.h"
#include "encoding.h"
#include "HspObjects.h"
#include "HspObjectWriter.h"
#include "string_writer.h"

static auto const MAX_CHILD_COUNT = std::size_t{ 300 };

// -----------------------------------------------
// ヘルパー
// -----------------------------------------------

// 文字が印字可能か雑に検査する。
static auto char_can_print(char b) -> bool {
	return b != 0x7f && (b == '\t' || b == '\n' || b == '\r' || (unsigned char)b >= 32);
}

static auto char_need_escape(char b) -> bool {
	return b == '\t' || b == '\r' || b == '\n' || b == '\\' || b == '"';
}

static auto string_need_escape(Utf8StringView const& str) -> bool {
	return std::any_of(
		str.begin(),
		str.end(),
		[](Utf8Char c) {
			return char_need_escape((char)c);
		});
}

static auto str_to_string(HspStr const& str) -> std::optional<std::string_view> {
	auto end = std::find(str.begin(), str.end(), '\0');

	if (!std::all_of(str.begin(), end, char_can_print)) {
		return std::nullopt;
	}

	auto count = (std::size_t)(end - str.begin());
	return std::string_view{ str.begin(), count };
}

static auto str_to_utf8(HspStr const& str) -> std::optional<Utf8String> {
	auto&& string_opt = str_to_string(str);
	if (!string_opt) {
		return std::nullopt;
	}

	return to_utf8(as_hsp(*string_opt));
}

static bool string_is_multiline(std::string_view const& str) {
	return std::find(str.begin(), str.end(), '\n') != str.end();
}

static bool str_is_compact(HspStr const& str) {
	auto&& string_opt = str_to_string(str);
	return string_opt && string_opt->size() < 64 && !string_is_multiline(*string_opt);
}

// 文字列をリテラル形式で書く。
static void write_string_as_literal(CStrWriter& w, HspStr const& str) {
	auto&& string_opt = str_to_string(str);
	if (!string_opt) {
		w.cat(u8"<バイナリ>");
		return;
	}

	auto text = to_utf8(as_hsp(*string_opt));

	if (!string_need_escape(text)) {
		w.cat(u8"\"");
		w.cat(text);
		w.cat(u8"\"");
		return;
	}

	w.cat(u8"\"");

	auto i = std::size_t{};
	while (i < text.size()) {
		auto c = (char)text[i];

		if (c == '\t') {
			w.cat(u8"\\t");
			i++;
			continue;
		}

		if (c == '\r') {
			i++;
			continue;
		}

		if (c == '\n') {
			w.cat(u8"\\n");
			i++;
			continue;
		}

		if (c == '\\' || c == '"') {
			w.cat(u8"\\");
			w.cat(text.substr(i, 1));

			i++;
			continue;
		}

		w.cat(text.substr(i, 1));
		i++;
	}

	w.cat(u8"\"");
}

static void write_var_mode(CStrWriter& writer, HspVarMode var_mode) {
	switch (var_mode) {
	case HspVarMode::None:
		writer.cat(u8" (empty)");
		break;
	case HspVarMode::Alloc:
		break;
	case HspVarMode::Clone:
		writer.cat(u8" (dup)");
		break;
	default:
		assert(false && u8"Unknown VarMode");
		break;
	}
}

static void write_array_type(CStrWriter& writer, Utf8StringView const& type_name, HspVarMode var_mode, HspDimIndex const& lengths) {
	// 例: int(2, 3) (6 in total) (dup)

	writer.cat(as_native(type_name));

	if (lengths.dim() == 1) {
		// (%d)
		writer.cat(u8"(");
		writer.catSize(lengths.at(0));
		writer.cat(u8")");
	} else {
		// (%d, %d, ..) (%d in total)
		writer.cat(u8"(");
		for (auto i = std::size_t{}; i < lengths.dim(); i++) {
			if (i != 0) {
				writer.cat(u8", ");
			}
			writer.catSize(lengths.at(i));
		}
		writer.cat(u8") (");
		writer.catSize(lengths.size());
		writer.cat(u8" in total)");
	}

	write_var_mode(writer, var_mode);
}

static auto write_var_metadata_on_table(CStrWriter& w, Utf8StringView const& type_name, HspVarMetadata const& metadata) {
	w.cat(u8"変数型: ");
	write_array_type(w, type_name, metadata.mode(), metadata.lengths());
	w.catCrlf();

	w.cat(u8"アドレス: ");
	w.catPtr(metadata.data_ptr());
	w.cat(u8", ");
	w.catPtr(metadata.master_ptr());
	w.catCrlf();

	w.cat(u8"サイズ: ");
	w.catSize(metadata.data_size());
	w.cat(u8" / ");
	w.catSize(metadata.block_size());
	w.cat(u8" [byte]");
	w.catCrlf();
}

static bool object_path_is_compact(HspObjectPath const& path, HspObjects& objects) {
	switch (path.kind()) {
	case HspObjectKind::Label:
	case HspObjectKind::Double:
	case HspObjectKind::Int:
	case HspObjectKind::Unknown:
		return true;

	case HspObjectKind::Str:
		return str_is_compact(path.as_str().value(objects));

	case HspObjectKind::Flex:
		{
			auto&& is_nullmod_opt = path.as_flex().is_nullmod(objects);
			return !is_nullmod_opt || *is_nullmod_opt;
		}

	default:
		return false;
	}
}

static void write_flex_module_name(CStrWriter& w, Utf8StringView const& module_name, std::optional<bool> is_clone_opt) {
	w.cat(module_name);

	// クローンなら印をつける。
	// NOTE: この関数が呼ばれているということは、flex が nullmod か否か判定できたということなので、
	//		 クローン変数か分からない (is_clone_opt == nullopt) ということは考えづらい。
	if (is_clone_opt && *is_clone_opt) {
		w.cat(u8"&");
	}
}

static void write_signature(CStrWriter& w, std::vector<Utf8StringView> const& param_type_names) {
	// FIXME: 命令ならカッコなし、関数ならカッコあり？

	w.cat(u8"(");

	for (auto i = std::size_t{}; i < param_type_names.size(); i++) {
		if (i != 0) {
			w.cat(u8", ");
		}

		w.cat(param_type_names.at(i));
	}

	w.cat(u8")");
}

static void write_source_location(
	CStrWriter& w,
	std::optional<Utf8StringView> const& file_name_opt,
	std::optional<std::size_t> const& line_index_opt
) {
	// 例: #12 hoge.hsp
	if (line_index_opt) {
		// 1-indexed
		w.cat(u8"#");
		w.catSize(*line_index_opt + 1);
		w.cat(u8" ");
	}
	if (file_name_opt) {
		w.cat(*file_name_opt);
	} else {
		w.cat(u8"???");
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

	void accept_children(HspObjectPath const& path) override;

	void on_static_var(HspObjectPath::StaticVar const& path) override;

	void on_param(HspObjectPath::Param const& path) override;

	void on_call_frame(HspObjectPath::CallFrame const& path) override;

	void on_general(HspObjectPath::General const& path) override;

	void on_log(HspObjectPath::Log const& path) override;

	void on_script(HspObjectPath::Script const& path) override;

	void on_unavailable(HspObjectPath::Unavailable const& path) override;

private:
	void write_name(HspObjectPath const& path);
};

// ブロックフォーム。
// 数行を使って、オブジェクトの情報を表示する。
// 構築した文字列は、テーブルフォームの内部に埋め込まれる。
class HspObjectWriterImpl::BlockForm
	: public HspObjectWriterImpl
{
public:
	BlockForm(HspObjects& objects, CStrWriter& writer);

	void accept(HspObjectPath const& path) override;

	void accept_default(HspObjectPath const& path) override;

	void accept_children(HspObjectPath const& path) override;

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

	void accept_children(HspObjectPath const& path) override;

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

void HspObjectWriterImpl::TableForm::write_name(HspObjectPath const& path) {
	auto&& o = objects();
	auto&& w = writer();

	w.cat(u8"[");
	w.cat(path.name(o));
	w.catln(u8"]");
}

void HspObjectWriterImpl::TableForm::accept_default(HspObjectPath const& path) {
	auto&& o = objects();
	auto&& w = writer();

	if (path.child_count(o) == 0) {
		write_name(path);
		to_block_form().accept(path);
		return;
	}

	write_name(path);
	to_block_form().accept_children(path);

	auto&& memory_view_opt = path.memory_view(o);
	if (memory_view_opt) {
		w.catCrlf();
		w.catDump(memory_view_opt->data(), memory_view_opt->size());
		w.catCrlf();
	}
}

void HspObjectWriterImpl::TableForm::accept_children(HspObjectPath const& path) {
	auto&& w = writer();
	auto&& o = objects();

	auto child_count = path.child_count(o);
	for (auto i = std::size_t{}; i < std::min(MAX_CHILD_COUNT, child_count); i++) {
		accept(*path.child_at(i, o));
	}

	if (child_count >= MAX_CHILD_COUNT) {
		w.cat(u8".. (合計");
		w.catSize(child_count);
		w.catln(u8" 件)");
	}
}

void HspObjectWriterImpl::TableForm::on_static_var(HspObjectPath::StaticVar const& path) {
	auto&& o = objects();
	auto&& w = writer();

	auto&& type_name = path.type_name(o);
	auto&& metadata = path.metadata(o);

	// 変数に関する情報
	write_name(path);

	write_var_metadata_on_table(w, type_name, metadata);
	w.catCrlf();

	// 変数の内容に関する情報
	to_block_form().accept_children(path);
	w.catCrlf();

	// メモリダンプ
	w.catDump(metadata.block_ptr(), metadata.block_size());
}

void HspObjectWriterImpl::TableForm::on_param(HspObjectPath::Param const& path) {
	auto&& o = objects();
	auto&& w = writer();

	if (auto && metadata_opt = path.var_metadata(o)) {
		auto&& metadata = *metadata_opt;
		auto&& type_name = o.type_to_name(metadata.type());
		auto&& name = path.name(o);

		write_name(path);
		write_var_metadata_on_table(w, type_name, metadata);
		w.catCrlf();

		to_block_form().accept_children(path);
		w.catCrlf();

		w.catDump(metadata.block_ptr(), metadata.block_size());
		w.catCrlf();
		return;
	}

	accept_default(path);
}

void HspObjectWriterImpl::TableForm::on_call_frame(HspObjectPath::CallFrame const& path) {
	auto&& o = objects();
	auto&& w = writer();

	auto&& signature_opt = path.signature(o);
	auto&& full_path_opt = path.full_path(o);
	auto&& line_index_opt = path.line_index(o);
	auto&& memory_view_opt = path.memory_view(o);

	write_name(path);

	w.cat(u8"呼び出し位置: ");
	write_source_location(w, full_path_opt, line_index_opt);
	w.catCrlf();

	if (signature_opt) {
		w.cat(u8"シグネチャ: ");
		write_signature(w, *signature_opt);
		w.catCrlf();
	}

	w.catCrlf();

	to_block_form().accept_children(path);

	if (memory_view_opt) {
		w.catCrlf();

		w.catDump(memory_view_opt->data(), memory_view_opt->size());
		w.catCrlf();
	}
}

void HspObjectWriterImpl::TableForm::on_general(HspObjectPath::General const& path) {
	auto&& content = path.content(objects());

	write_name(path);
	writer().cat(content);
}

void HspObjectWriterImpl::TableForm::on_log(HspObjectPath::Log const& path) {
	auto&& content = path.content(objects());
	assert((content.empty() || (char)content.back() == '\n') && u8"Log must be end with line break");

	write_name(path);
	writer().cat(content);
}

void HspObjectWriterImpl::TableForm::on_script(HspObjectPath::Script const& path) {
	auto&& content = path.content(objects());

	// NOTE: 行番号がズレないようにスクリプト以外を描画しない。
	// write_name(path);

	writer().catln(content);
}

void HspObjectWriterImpl::TableForm::on_unavailable(HspObjectPath::Unavailable const& path) {
	auto&& w = writer();
	auto&& reason = path.reason();

	write_name(path);
	w.cat(u8"理由: ");
	w.catln(reason);
}

// -----------------------------------------------
// ブロックフォーム
// -----------------------------------------------

HspObjectWriterImpl::BlockForm::BlockForm(HspObjects& objects, CStrWriter& writer)
	: HspObjectWriterImpl(objects, writer)
{
}

void HspObjectWriterImpl::BlockForm::accept(HspObjectPath const& path) {
	if (writer().is_full()) {
		return;
	}

	Visitor::accept(path);
}

void HspObjectWriterImpl::BlockForm::accept_default(HspObjectPath const& path) {
	add_name_children(path);
}

void HspObjectWriterImpl::BlockForm::accept_children(HspObjectPath const& path) {
	auto&& w = writer();
	auto&& o = objects();

	auto child_count = path.child_count(o);
	for (auto i = std::size_t{}; i < std::min(MAX_CHILD_COUNT, child_count); i++) {
		accept(*path.child_at(i, o));
	}

	if (child_count >= MAX_CHILD_COUNT) {
		w.cat(u8".. (合計 ");
		w.catSize(child_count);
		w.catln(u8" 件)");
	}
}

void HspObjectWriterImpl::BlockForm::on_module(HspObjectPath::Module const& path) {
	auto&& name = path.name(objects());

	// (入れ子の)モジュールは名前だけ表示しておく
	writer().catln(name.data());
}

void HspObjectWriterImpl::BlockForm::on_static_var(HspObjectPath::StaticVar const& path) {
	auto&& o = objects();
	auto&& w = writer();

	auto&& name = path.name(o);
	auto short_name = var_name_to_bare_ident(name);

	w.cat(short_name);
	w.cat(u8"\t= ");

	to_flow_form().accept(path);

	w.catCrlf();
}

void HspObjectWriterImpl::BlockForm::on_label(HspObjectPath::Label const& path) {
	to_flow_form().on_label(path);
	writer().catCrlf();
}

void HspObjectWriterImpl::BlockForm::on_str(HspObjectPath::Str const& path) {
	auto&& w = writer();
	auto&& str = path.value(objects());
	auto&& text_opt = str_to_string(str);

	if (!text_opt) {
		w.catln(u8"<バイナリ>");
		return;
	}

	auto text = to_utf8(as_hsp(*text_opt));
	w.catln(text);
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

	auto&& is_nullmod_opt = path.is_nullmod(o);
	if (!is_nullmod_opt) {
		w.catln(u8"<unavailable>");
		return;
	}

	if (*is_nullmod_opt) {
		w.catln(u8"<null>");
		return;
	}

	auto&& module_name = path.module_name(o);
	auto&& is_clone_opt = path.is_clone(o);

	w.cat(u8".module = ");
	write_flex_module_name(w, module_name, is_clone_opt);
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
		w.cat(u8"\t= ");
		to_block_form().accept(*first_child);
		return;
	}

	w.cat(name.data());
	w.catln(u8":");
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

void HspObjectWriterImpl::FlowForm::accept_children(HspObjectPath const& path) {
	auto&& w = writer();
	auto&& o = objects();

	auto child_count = path.child_count(o);
	for (auto i = std::size_t{}; i < std::min(MAX_CHILD_COUNT, child_count); i++) {
		if (i != 0) {
			w.cat(u8", ");
		}

		accept(*path.child_at(i, o));
	}

	if (child_count >= MAX_CHILD_COUNT) {
		w.cat(u8"; ..");
	}
}

void HspObjectWriterImpl::FlowForm::on_static_var(HspObjectPath::StaticVar const& path) {
	auto&& w = writer();
	auto type = path.type(objects());
	auto&& type_name = objects().type_to_name(type);
	auto child_count = path.child_count(objects());

	// FIXME: 多次元配列の表示を改善する

	w.cat(u8"<");
	w.cat(as_native(type_name));
	w.cat(u8">[");
	accept_children(path);
	w.cat(u8"]");
}

void HspObjectWriterImpl::FlowForm::on_label(HspObjectPath::Label const& path) {
	auto&& o = objects();
	auto&& w = writer();

	if (path.is_null(o)) {
		w.cat(u8"<null-label>");
		return;
	}

	if (auto&& name_opt = path.static_label_name(o)) {
		w.cat(u8"*");
		w.cat(*name_opt);
		return;
	}

	w.cat(u8"<label>");
}

void HspObjectWriterImpl::FlowForm::on_str(HspObjectPath::Str const& path) {
	auto&& value = path.value(objects());

	write_string_as_literal(writer(), value);
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

	auto&& is_nullmod_opt = path.is_nullmod(o);
	if (!is_nullmod_opt) {
		w.cat(u8"<unavailable>");
		return;
	}

	if (*is_nullmod_opt) {
		w.cat(u8"null");
		return;
	}

	auto&& module_name = path.module_name(o);
	auto&& is_clone_opt = path.is_clone(o);
	write_flex_module_name(w, module_name, is_clone_opt);
	w.cat(u8"{");

	for (auto i = std::size_t{}; i < path.child_count(o); i++) {
		auto&& child_path = path.child_at(i, o);

		if (i != 0) {
			w.cat(u8", ");
		}
		accept(*child_path);
	}

	w.cat(u8"}");
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

// -----------------------------------------------
// テスト
// -----------------------------------------------

static void write_string_as_literal_tests(Tests& tests) {
	auto&& suite = tests.suite(u8"write_string_as_literal");

	auto write = [&](auto&& str) {
		auto w = CStrWriter{};
		write_string_as_literal(w, Slice<char>{ (char const*)str.data(), str.size() });
		return as_utf8(w.finish());
	};

	suite.test(
		u8"エスケープなし",
		[&](TestCaseContext& t) {
			return t.eq(write(as_utf8(u8"Hello, world!")), as_utf8(u8"\"Hello, world!\""));
		});

	suite.test(
		u8"エスケープあり",
		[&](TestCaseContext& t) {
			return t.eq(
				write(as_utf8(u8"\\1234:\r\n\tThe \"One Two Three Four\" festival.")),
				as_utf8(u8"\"\\\\1234:\\n\\tThe \\\"One Two Three Four\\\" festival.\"")
			);
		});

	suite.test(
		u8"バイナリ",
		[&](TestCaseContext& t) {
			return t.eq(write(as_utf8("\x01\x02\x03")), as_utf8(u8"<バイナリ>"));
		});
}

static void write_array_type_tests(Tests& tests) {
	auto&& suite = tests.suite(u8"write_array_type");

	auto write = [&](auto&& type_name, auto&& var_mode, auto&& lengths) {
		auto w = CStrWriter{};
		write_array_type(w, type_name, var_mode, lengths);
		return as_utf8(w.finish());
	};

	suite.test(
		u8"1次元配列",
		[&](TestCaseContext& t) {
			return t.eq(
				write(as_utf8(u8"int"), HspVarMode::Alloc, HspDimIndex{ 1, { 3 } }),
				as_utf8(u8"int(3)")
			);
		});

	suite.test(
		u8"2次元配列",
		[&](TestCaseContext& t) {
			return t.eq(
				write(as_utf8(u8"str"), HspVarMode::Alloc, HspDimIndex{ 2, { 2, 3 } }),
				as_utf8(u8"str(2, 3) (6 in total)")
			);
		});

	suite.test(
		u8"クローン変数",
		[&](TestCaseContext& t) {
			return t.eq(
				write(as_utf8(u8"int"), HspVarMode::Clone, HspDimIndex{ 1, { 4 } }),
				as_utf8(u8"int(4) (dup)")
			);
		});
}

static void write_source_location_tests(Tests& tests) {
	auto&& suite = tests.suite(u8"write_source_location");

	auto write = [&](auto&& ...args) {
		auto w = CStrWriter{};
		write_source_location(w, args...);
		return as_utf8(w.finish());
	};

	suite.test(
		u8"ファイル名と行番号があるケース",
		[&](TestCaseContext& t) {
			auto actual = write(
				std::make_optional(as_utf8(u8"foo.hsp")),
				std::make_optional(std::size_t{ 42 })
			);
			return t.eq(actual, as_utf8(u8"#43 foo.hsp"));
		});

	suite.test(
		u8"行番号がないケース",
		[&](TestCaseContext& t) {
			auto actual = write(
				std::make_optional(as_utf8(u8"foo.hsp")),
				std::nullopt
			);
			return t.eq(actual, as_utf8(u8"foo.hsp"));
		});

	suite.test(
		u8"ファイル名がないケース",
		[&](TestCaseContext& t) {
			auto actual = write(
				std::nullopt,
				std::make_optional(std::size_t{ 42 })
			);
			return t.eq(actual, as_utf8(u8"#43 ???"));
		});

	suite.test(
		u8"ファイル名も行番号もないケース",
		[&](TestCaseContext& t) {
			auto actual = write(
				std::nullopt,
				std::nullopt
			);
			return t.eq(actual, as_utf8(u8"???"));
		});
}

void hsp_object_writer_tests(Tests& tests) {
	write_string_as_literal_tests(tests);
	write_array_type_tests(tests);
	write_source_location_tests(tests);
}
