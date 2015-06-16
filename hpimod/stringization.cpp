
#include <string>
#include <vector>
#include <sstream>

#include "hsp3plugin_custom.h"
#include "stringization.h"

namespace hpimod {

char const* nameFromMPType(int mptype)
{
	switch ( mptype ) {
		case MPTYPE_NONE:        return "none";
		case MPTYPE_STRUCTTAG:   return "structtag";

		case MPTYPE_LABEL:       return "label";
		case MPTYPE_DNUM:        return "double";
		case MPTYPE_STRING:
		case MPTYPE_LOCALSTRING: return "str";
		case MPTYPE_INUM:        return "int";
		case MPTYPE_PVARPTR: // #dllfunc
		case MPTYPE_VAR:
		case MPTYPE_SINGLEVAR:   return "var";
		case MPTYPE_ARRAYVAR:    return "array";
		case MPTYPE_LOCALVAR:    return "local";
		case MPTYPE_MODULEVAR:   return "thismod"; //or "modvar"
		case MPTYPE_IMODULEVAR:  return "modinit";
		case MPTYPE_TMODULEVAR:  return "modterm";

#if 0
		case MPTYPE_IOBJECTVAR:  return "comobj";
		//case MPTYPE_LOCALWSTR:   return "";
		//case MPTYPE_FLEXSPTR:    return "";
		//case MPTYPE_FLEXWPTR:    return "";
		case MPTYPE_FLOAT:       return "float";
		case MPTYPE_PPVAL:       return "pval";
		case MPTYPE_PBMSCR:      return "bmscr";
		case MPTYPE_PTR_REFSTR:  return "prefstr";
		case MPTYPE_PTR_EXINFO:  return "pexinfo";
		case MPTYPE_PTR_DPMINFO: return "pdpminfo";
		case MPTYPE_NULLPTR:     return "nullptr";
#endif
		default: return "unknown";
	}
}

//------------------------------------------------
// 文字列を文字列リテラルの形式に変換する
//------------------------------------------------
std::string literalFormString(char const* src)
{
	size_t const maxlen = (std::strlen(src) * 2) + 2;
	std::vector<char> buf; buf.resize(maxlen + 1);
	size_t idx = 0;

	buf[idx++] = '\"';

	for ( int i = 0;; ++i ) {
		char const c = src[i];

		if ( c == '\0' ) {
			break;

		} else if ( c == '\\' || c == '\"' ) {
			buf[idx++] = '\\';
			buf[idx++] = c;

		} else if ( c == '\t' ) {
			buf[idx++] = '\\';
			buf[idx++] = 't';

		} else if ( c == '\r' || c == '\n' ) {
			if ( c == '\r' && src[i + 1] == '\n' ) { // CRLF
				i++;
			}
			buf[idx++] = '\\';
			buf[idx++] = 'n';

		} else {
			buf[idx++] = c;
		}
	}

	buf[idx++] = '\"';
	buf[idx++] = '\0';
	return std::string { buf.data() };
}

//------------------------------------------------
// 配列添字の文字列
//------------------------------------------------
std::string stringizeArrayIndex(std::vector<int> const& indexes)
{
	std::ostringstream os;
	os << BracketIdxL;
	for ( size_t i = 0; i < indexes.size(); ++i ) {
		if ( i != 0 ) os << ' ';
		os << indexes[i];
	}
	os << BracketIdxR;
	return os.str();
}

//------------------------------------------------
// スコープ解決を取り除いた名前
//------------------------------------------------
std::string nameExcludingScopeResolution(std::string const& name)
{
	size_t const idxScopeResolution = name.find('@');
	return (idxScopeResolution != std::string::npos
		? name.substr(0, idxScopeResolution)
		: name);
}

} //namespace hpimod
