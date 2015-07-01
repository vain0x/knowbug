// ax structure analyzed

#ifndef IG_CLASS_AX_H
#define IG_CLASS_AX_H

#include <memory>
#include <map>
#include <vector>
#include <algorithm>

#include "main.h"

static unsigned short wpeek(unsigned char const* p) { return *reinterpret_cast<unsigned short const*>(p); }
static unsigned int tripeek(unsigned char const* p) { return (p[0] | p[1] << 8 | p[2] << 16); }

class CAx {
private:
	typedef std::map<int, const char*> identTable_t;
	typedef std::map<std::pair<const char*, int>, csptr_t> csMap_t;

private:
	identTable_t labelNames;
	identTable_t prmNames;
	csMap_t csMap;	// ファイル名, 行番号から cs 位置を特定する

public:
	CAx()
	{
		analyzeDInfo();
	}

private:
	static const char* getIdentName(identTable_t const& table, int iparam) {
		auto const iter = table.find(iparam);
		return (iter != table.end()) ? iter->second : nullptr;
	}

public:
	const char* getLabelName(int otindex) const { return getIdentName(labelNames, otindex); }
	const char* getPrmName(int stprmidx) const { return getIdentName(prmNames, stprmidx); }

private:
	enum DInfoCtx {
		DInfoCtx_Default = 0,
		DInfoCtx_LabelNames,
		DInfoCtx_PrmNames,
		DInfoCtx_Max
	};

	// dinfo 解析
	void analyzeDInfo() {
		csptr_t cur_cs = ctx->mem_mcs;
		const char* cur_fname;
		int cur_line;

		auto const push_point = [this, &cur_fname, &cur_line, &cur_cs] {
			csMap.insert(
				csMap_t::value_type(std::pair<const char*, int>(cur_fname, cur_line), cur_cs)
			);
		};

		int dictx = DInfoCtx_Default;

		for (int i = 0; i < ctx->hsphed->max_dinfo;) {
			switch (ctx->mem_di[i]) {
				case 0xFF:
					++dictx;	// enum を ++ する業
					if (dictx == DInfoCtx_Max) return;
					++i;
					break;

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

				case 0xFD:
				case 0xFB:
					identTable_t* p;
					switch (dictx) {
						case DInfoCtx_Default:    p = nullptr;     break;	// 変数名は記録しない
						case DInfoCtx_LabelNames: p = &labelNames; break;
						case DInfoCtx_PrmNames:   p = &prmNames;   break;
						default: throw;
					}

					if (p) {
						auto const ident = &ctx->mem_mds[tripeek(&ctx->mem_di[i + 1])];
						int const iparam = wpeek(&ctx->mem_di[i + 4]);
						p->insert(identTable_t::value_type(iparam, ident));
					}
					i += 6;
					break;

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
		return;
	}


};

#endif
