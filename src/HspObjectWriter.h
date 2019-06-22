#pragma once

#include "HspObjectPath.h"

class CStrWriter;
class CVarinfoText;

class HspObjectWriter
	: public HspObjectPath::Visitor
{
public:
	class TableForm;
	class BlockForm;
	class FlowForm;

private:
	CStrWriter& writer_;

public:
	explicit HspObjectWriter(HspObjects& objects, CStrWriter& writer);

	auto writer() -> CStrWriter& {
		return writer_;
	}
};

// テーブルフォーム。
// 文字列全体を使って、オブジェクトの詳細情報を表示する。
class HspObjectWriter::TableForm
	: public HspObjectWriter
{
public:
	TableForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf);

	void on_module(HspObjectPath::Module const& path) override;

	void on_static_var(HspObjectPath::StaticVar const& path) override;

private:
	CVarinfoText& varinf_;
};

// ブロックフォーム。
// 数行を使って、オブジェクトの情報を表示する。
// 構築した文字列は、テーブルフォームの内部に埋め込まれる。
class HspObjectWriter::BlockForm
	: public HspObjectWriter
{
public:
	BlockForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf);

	void on_module(HspObjectPath::Module const& path) override;

	void on_static_var(HspObjectPath::StaticVar const& path) override;

	void on_element(HspObjectPath::Element const& path) override;

private:
	CVarinfoText& varinf_;
};

// フローフォーム。
// オブジェクトの情報を簡易的に表示する。
// 基本的に改行を含まない。
class HspObjectWriter::FlowForm
	: public HspObjectWriter
{
public:
	FlowForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf);

	void on_static_var(HspObjectPath::StaticVar const& path) override;

	void on_int(HspObjectPath::Int const& path) override;

private:
	CVarinfoText& varinf_;
};
