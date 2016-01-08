
#pragma once

#include <map>
#include <unordered_map>
#include <unordered_set>
#include "hpiutil.hpp"

namespace hpiutil {
	
static auto wpeek(unsigned char const* p) -> unsigned short
{
	return *reinterpret_cast<unsigned short const*>(p);
}

static auto tripeek(unsigned char const* p) -> unsigned int
{
	return (p[0] | p[1] << 8 | p[2] << 16);
}

class DInfo
{
	DInfo()
	{
		parse();
	}

public:
	static auto instance() -> DInfo&;

private:
	using ident_table_t = std::unordered_map<int, char const*>;
	using cs_map_t = std::map<std::pair<char const*, int>, csptr_t>;

	std::unordered_set<std::string> fileRefNames_;
	ident_table_t labelNames_;
	ident_table_t paramNames_;
	cs_map_t csMap_;

public:
	auto tryFindIdent(ident_table_t const& table, int iparam) const -> char const*
	{
		auto const iter = table.find(iparam);
		return (iter != table.end()) ? iter->second : nullptr;
	}

	auto tryFindLabelName(int otIndex) const -> char const*
	{
		return tryFindIdent(labelNames_, otIndex);
	}

	auto tryFindParamName(int stprmIndex) const -> char const*
	{
		return tryFindIdent(paramNames_, stprmIndex);
	}

	auto fileRefNames() const -> decltype(fileRefNames_) const&
	{
		return fileRefNames_;
	}

	void parse()
	{
		auto tryFindIdentTableFromCtx = [&](int dictx) -> ident_table_t*
		{
			switch ( dictx ) {
				case 0: return nullptr; // 変数名は記録しない
				case 1: return &labelNames_;
				case 2: return &paramNames_;
				default: throw; //unreachable
			}
		};

		auto cur_cs = ctx->mem_mcs;
		auto cur_fname = static_cast<char const*>(nullptr);
		auto cur_line = 0;

		auto pushPoint = [&]() {
			csMap_.emplace(std::make_pair(cur_fname, cur_line), cur_cs);
		};

		auto dictx = 0; // Default context

		for ( auto i = 0; i < ctx->hsphed->max_dinfo; ) {
			switch ( ctx->mem_di[i] ) {
				case 0xFF: // 文脈の区切り
					dictx++;
					i++;
					break;

				case 0xFE: // ソースファイル指定
				{
					auto const idxDs = tripeek(&ctx->mem_di[i + 1]);
					auto const line = wpeek(&ctx->mem_di[i + 4]);

					if ( idxDs != 0 ) {
						cur_fname = strData(idxDs);
						fileRefNames_.emplace(cur_fname);
					}
					cur_line = line;
					i += 6;
					break;
				}
				// 識別子指定
				case 0xFD:
				case 0xFB:
					if ( auto const tbl = tryFindIdentTableFromCtx(dictx) ) {
						auto const ident = strData(tripeek(&ctx->mem_di[i + 1]));
						auto const iparam = wpeek(&ctx->mem_di[i + 4]);
						tbl->emplace(iparam, ident);
					}
					i += 6;
					break;

				case 0xFC: // 次の命令までのCSオフセット値
					cur_cs += *reinterpret_cast<csptr_t>(&ctx->mem_di[i + 1]);
					pushPoint();
					i += 3;
					break;
				default:
					cur_cs += ctx->mem_di[i];
					pushPoint();
					i++;
					break;
			}
		}
	}
};

} // namespace hpiutil
