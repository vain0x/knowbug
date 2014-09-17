// multi - SubCommand header

#ifndef IG_VECTOR_COMMAND_SUB_H
#define IG_VECTOR_COMMAND_SUB_H

#include "hsp3plugin_custom.h"
#include "vt_vector.h"

// vector_t 用の関数
// 置く場所を考えなおす必要あり

extern bool isValidIndex(vector_t const& self, int idx);
extern bool isValidRange(vector_t const& self, size_t iBgn, size_t iEnd);
extern void chainShallow(vector_t& dst, vector_t const& src, std::pair<size_t, size_t> range);
extern void chainDeep(vector_t& dst, vector_t const& src, std::pair<size_t, size_t> range);

#endif
