#include "pch.h"
#include "hash_code.h"
#include "hsx_internals.h"

bool hsx_indexes_equals(HsxIndexes first, HsxIndexes second) {
	static_assert(sizeof(HsxIndexes::data) == sizeof(size_t) * HSX_MAX_DIM);
	return first.dim == second.dim && memcmp(first.data, second.data, sizeof(HsxIndexes::data)) == 0;
}

size_t hsx_indexes_hash(HsxIndexes indexes) {
	auto h = HashCode::from(indexes.dim);
	for (size_t d = 0; d < indexes.dim; d++) {
		h = h.combine(indexes.data[d]);
	}
	return h.value();
}

size_t hsx_indexes_get_total(HsxIndexes indexes) {
	size_t count = 1;
	for (size_t d = 0; d < indexes.dim; d++) {
		count *= indexes.data[d];
	}
	return count;
}
