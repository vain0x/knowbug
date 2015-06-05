// ax analyzer
// dinfo を解析してラベル名、パラメータ名を収集する

#ifndef IG_CLASS_AX_H
#define IG_CLASS_AX_H

#include <memory>
#include <map>
#include <algorithm>

//#include "main.h"

#include "hsp3plugin.h"
#include "hpimod/basis.h"

static unsigned short wpeek(unsigned char const* p) { return *reinterpret_cast<unsigned short const*>(p); }
static unsigned int tripeek(unsigned char const* p) { return (p[0] | p[1] << 8 | p[2] << 16); }

namespace hpimod {
class CAx
{
private:
	using identTable_t = std::map<int, char const*>;
	using csMap_t = std::map<std::pair<char const*, int>, csptr_t>;

private:
	identTable_t labelNames;
	identTable_t prmNames;
	
	// ファイル名, 行番号から cs 位置を特定するためのもの (現在は未実装)
	// ただしファイル名は di の該当位置へのポインタに限る
	csMap_t csMap;

public:
	CAx() {
		analyzeDInfo();
	}

private:
	// テーブルから識別子を検索 (failure: nullptr)
	static char const* getIdentName(identTable_t const& table, int iparam) {
		auto const iter = table.find(iparam);
		return (iter != table.end()) ? iter->second : nullptr;
	}

public:
	char const* getLabelName(int otindex) const { return getIdentName(labelNames, otindex); }
	char const* getPrmName(int stprmidx) const { return getIdentName(prmNames, stprmidx); }

private:
	enum DInfoCtx {	// ++ したいので enum class にしない
		DInfoCtx_Default = 0,
		DInfoCtx_LabelNames,
		DInfoCtx_PrmNames,
		DInfoCtx_Max
	};

	identTable_t* getIdentTableFromCtx(int dictx) {
		switch ( dictx ) {
			case DInfoCtx_Default:    return nullptr;	// 変数名は記録しない
			case DInfoCtx_LabelNames: return &labelNames;
			case DInfoCtx_PrmNames:   return &prmNames;
			default: throw;
		}
	}

	// dinfo 解析
	void analyzeDInfo() {
		/*
		csptr_t cur_cs = ctx->mem_mcs;
		char const* cur_fname;
		int cur_line;

		auto const push_point = [this, &cur_fname, &cur_line, &cur_cs]() {
			csMap.insert({ { cur_fname, cur_line }, cur_cs });
		};
		//*/

		int dictx = DInfoCtx_Default;

		for ( int i = 0; i < ctx->hsphed->max_dinfo; ) {
			switch ( ctx->mem_di[i] ) {
				case 0xFF:
					++dictx;	// enum を ++ する業
					if ( dictx == DInfoCtx_Max ) {
						assert(i + 2 == ctx->hsphed->max_dinfo);
						return;
					}
					++i;
					break;

				// ソースファイル指定
				case 0xFE:
				{/*
					int const idxDs = tripeek(&ctx->mem_di[i + 1]);
					int const line = wpeek(&ctx->mem_di[i + 4]);

					if (idxDs != 0) { cur_fname = &ctx->mem_mds[idxDs]; }
					cur_line = line;
					//*/
					i += 6;
					break;
				}
				// 識別子指定
				case 0xFD:
				case 0xFB:
					if ( auto const tbl = getIdentTableFromCtx(dictx) ) {
						auto const ident = &ctx->mem_mds[tripeek(&ctx->mem_di[i + 1])];
						int const iparam = wpeek(&ctx->mem_di[i + 4]);
						tbl->insert({ iparam, ident });
					}
					i += 6;
					break;
				// 次の命令までのCSオフセット値
				case 0xFC:
					//cur_cs += *reinterpret_cast<csptr_t>(&ctx->mem_di[i + 1]);
					//push_point();
					i += 3;
					break;
				default:
					//cur_cs += ctx->mem_di[i];
					//push_point();
					++i;
					break;
			}
		}
	}
};
	
} // namespace hpimod

#endif
