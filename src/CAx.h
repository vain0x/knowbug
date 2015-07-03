#ifndef IG_CLASS_AX_H
#define IG_CLASS_AX_H

#include <memory>
#include <map>
#include <algorithm>

#include "hsp3plugin.h"
#include "hpimod/basis.h"

static unsigned short wpeek(unsigned char const* p) { return *reinterpret_cast<unsigned short const*>(p); }
static unsigned int tripeek(unsigned char const* p) { return (p[0] | p[1] << 8 | p[2] << 16); }

namespace hpimod {
	
// ax analyzer
// dinfo を解析してラベル名、パラメータ名を収集する
// 文字列ポインタ(char*)はすべて DInfo セグメント内を指す。
// todo: 今更だが CHsp3Parser などの既存のものを使うべきかもしれない
class CAx
{
private:
	using identTable_t = std::map<int, char const*>;
	using csMap_t = std::map<std::pair<char const*, int>, csptr_t>;

private:
	identTable_t labelNames_;
	identTable_t paramNames_;

	// (未実装)
	// ファイル名, 行番号から cs 位置を特定するためのもの
	// ただしファイル名は di の該当位置へのポインタに限る
	csMap_t csMap_;

public:
	CAx() {
		analyzeDInfo();
	}

private:
	static char const* tryFindIdent(identTable_t const& table, int iparam) {
		auto const iter = table.find(iparam);
		return (iter != table.end()) ? iter->second : nullptr;
	}

public:
	char const* tryFindLabelName(int otindex) const { return tryFindIdent(labelNames_, otindex); }
	char const* tryFindParamName(int stprmidx) const { return tryFindIdent(paramNames_, stprmidx); }

private:
	enum DInfoCtx {	// increment したいので enum class にしない
		DInfoCtx_Default = 0,
		DInfoCtx_LabelNames,
		DInfoCtx_ParamNames,
		DInfoCtx_Max
	};
	static bool dictx_moveNext(int& ctx) { ++ctx; return ctx != DInfoCtx_Max; }

	identTable_t* tryFindIdentTableFromCtx(int dictx) {
		switch ( dictx ) {
			case DInfoCtx_Default:    return nullptr; // 変数名は記録しない
			case DInfoCtx_LabelNames: return &labelNames_;
			case DInfoCtx_ParamNames: return &paramNames_;
			default: throw; //unreachable
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
					if ( !dictx_moveNext(dictx) ) {
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
					if ( auto const tbl = tryFindIdentTableFromCtx(dictx) ) {
						auto const ident = &ctx->mem_mds[tripeek(&ctx->mem_di[i + 1])];
						int const iparam = wpeek(&ctx->mem_di[i + 4]);
						tbl->emplace(iparam, ident);
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
