#pragma once

class HspObjectPath;
class HspObjects;
class CStrWriter;
class CVarinfoText;

// HSP のオブジェクトの情報を文字列に書き出すもの。
class HspObjectWriter {
	HspObjects& objects_;
	CVarinfoText& varinf_;
	CStrWriter& writer_;

public:
	HspObjectWriter(HspObjects& objects, CVarinfoText& varinf, CStrWriter& writer);

	void write_table_form(HspObjectPath const& path);

	void write_block_form(HspObjectPath const& path);

	void write_flow_form(HspObjectPath const& path);
};
