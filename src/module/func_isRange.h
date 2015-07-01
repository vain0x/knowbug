// isRange 関数

#ifndef IG_FUNCTION_IS_RANGE_H
#define IG_FUNCTION_IS_RANGE_H

//------------------------------------------------
// 範囲
//------------------------------------------------
template<class T> inline
bool isRange(const T& obj, const T& minval, const T& maxval)
{
	return ( minval <= obj && obj <= maxval );
}

// unsigned char 用
template bool isRange<unsigned char>(const unsigned char&, const unsigned char&, const unsigned char&);

static bool (*const isRangeUChar)(const unsigned char&, const unsigned char&, const unsigned char&)
	= isRange<unsigned char>;

#endif
