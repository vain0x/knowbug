#pragma once

#include "test_suite.h"

class HspObjectPath;
class HspObjects;
class CStrWriter;

// HSP のオブジェクトの情報を文字列に書き出すもの。
class HspObjectWriter {
	HspObjects& objects_;
	CStrWriter& writer_;

public:
	HspObjectWriter(HspObjects& objects, CStrWriter& writer);

	void write_table_form(HspObjectPath const& path);

	void write_block_form(HspObjectPath const& path);

	void write_flow_form(HspObjectPath const& path);
};

extern void hsp_object_writer_tests(Tests& tests);
