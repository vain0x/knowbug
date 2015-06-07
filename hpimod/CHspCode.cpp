// hsp code

#include <memory>
#include <vector>

#include "hsp3plugin_custom.h"
#include "HspAllocator.h"

#include "CHspCode.h"

namespace hpimod
{

namespace Detail
{
	AlignedPlainStorage::buf_t& AlignedPlainStorage::allocNew(size_t size)
	{
		list_.emplace_back(size);
		assert(list_.back().size() == size);
		return list_.back();
	}

	// データバッファを確保する
	/**
	このバッファに書き込んだものにデストラクタは起動されない。
	base に対する alignment を考慮して確保する。その相対位置の整数値 n が返る。
	つまり、&base[n * alignment] == 確保したバッファへのポインタ。
	//*/
	int AlignedPlainStorage::push(size_t size, size_t alignment, void const* _base)
	{
		assert(alignment > 0);
		auto& it = allocNew(size + alignment - 1);

		char* top = it.data();
		auto const base = reinterpret_cast<char const*>(_base);
		int const dif = top - base;
		size_t const padding =
			(dif <= 0)
			? ((-dif) % alignment)
			: (alignment - (dif % alignment)) % alignment;

		top += padding;
		assert((top - base) % alignment == 0 && padding < alignment);
		return (top - base) / alignment;
	}
}

void CHspCode::put( int _type, int _code, int _exflg )
{
	int const lead = ((_type & CSTYPE) | _exflg);

	// 16 bit
	if ( static_cast<unsigned int>(_code) < 0x10000 ) {
		needCsBuf(4);

		code_[len_ ++] = lead;
		code_[len_ ++] = _code;

	// 32 bit
	} else {
		needCsBuf(8);

		code_[len_ ++] = ( lead | EXFLG_3 );
		*reinterpret_cast<int*>(&code_[len_]) = _code;
		len_ += 2;
	}
	return;
}

void CHspCode::putVar( PVal const* v, int exflg )
{
	// v === ctx->mem_var mod (sizeof(PVal)) でなかったら不味い
	assert( (v - ctx->mem_var) % sizeof(PVal) == 0 );
	put( TYPE_VAR, (v - ctx->mem_var) / sizeof(PVal), exflg );
}

// CS バッファの確保要求
void CHspCode::needCsBuf( size_t sizeAppend )
{
	auto const size = code_.size();
	if ( size - len_ <= sizeAppend ) {
		code_.reserve((size + sizeAppend) << 2);
		code_.resize(size + sizeAppend);
	}
	return;
}

int CHspCode::putDsVal(char const* str)
{
	size_t const size = std::strlen(str) + 1;
	int const iDs = dslist_.push(size, sizeof(char), ctx->mem_mds);
	strcpy_s(&ctx->mem_mds[iDs], size, str);
	return iDs;
}
int CHspCode::putDsVal(double d)
{
	int const iDs = dslist_.push(sizeof(double), sizeof(double), ctx->mem_mds);
	*reinterpret_cast<double*>(&ctx->mem_mds[iDs]) = d;
	return iDs;
}

int CHspCode::putDsStPrm(STRUCTPRM const& stprm)
{
	int const idx = dslist_.push(sizeof(STRUCTPRM), sizeof(STRUCTPRM), ctx->mem_minfo);
	auto const it = &ctx->mem_minfo[idx];
	*it = stprm;
	return idx;
}

} // namespace hpimod

