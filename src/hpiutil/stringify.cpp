
#include <sstream>
#include "hpiutil.hpp"
#include "DInfo.hpp"

namespace hpiutil {
	
namespace detail {

template<typename T>
static auto indexFrom(std::vector_view<T> const& v, T const* p) -> ptrdiff_t
{
	return (v.begin() <= p && p < v.end())
		? std::distance(v.begin(), p)
		: (-1);
}

} // namespace detail

auto DInfo::instance() -> DInfo&
{
	static auto const inst = std::unique_ptr<DInfo> { new DInfo {} };
	return *inst;
}

auto nameFromStaticVar(PVal const* pval) -> char const*
{
	auto const index = detail::indexFrom(staticVars(), pval);
	return (index >= 0)
		? exinfo->HspFunc_varname(static_cast<int>(index))
		: nullptr;
}

auto nameFromModuleClass(stdat_t stdat, bool isClone) -> std::string
{
	auto modclsName = std::string { STRUCTDAT_name(stdat) };
	return (isClone
		? modclsName + "&"
		: modclsName);
}

auto nameFromStPrm(stprm_t stprm, int idx) -> std::string
{
	auto const subid = detail::indexFrom(minfo(), stprm);
	if ( subid >= 0 ) {
		if ( auto const name = DInfo::instance().tryFindParamName(subid) ) {
			return nameExcludingScopeResolution(name);

		// thismod 引数
		} else if ( stprm->mptype == MPTYPE_MODULEVAR || stprm->mptype == MPTYPE_IMODULEVAR || stprm->mptype == MPTYPE_TMODULEVAR ) {
			return "thismod";
		}
	}
	return stringifyArrayIndex({ idx });
}

auto nameFromLabel(label_t lb) -> std::string
{
	auto const otIndex = detail::indexFrom(labels(), lb);
	auto buf = std::array<char, 64> {};
	if ( auto const name = DInfo::instance().tryFindLabelName(otIndex) ) {
		std::sprintf(buf.data(), "*%s", name);
	} else {
		std::sprintf(buf.data(), "label(%p)", static_cast<void const*>(lb));
	}
	return std::string { buf.data() };
}

auto nameFromMPType(int mptype) -> char const*
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

auto literalFormString(char const* src) -> std::string
{
	auto const maxlen = size_t { (std::strlen(src) * 2) + 2 };
	auto buf = std::vector<char>(maxlen + 1, '\0');
	auto idx = size_t { 0 };

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

auto stringifyArrayIndex(std::vector<int> const& indexes) -> std::string
{
	auto os = std::ostringstream {};
	os << "(";
	for ( auto i = size_t { 0 }; i < indexes.size(); ++i ) {
		if ( i != 0 ) os << ", ";
		os << indexes[i];
	}
	os << ")";
	return os.str();
}

auto nameExcludingScopeResolution(std::string const& name) -> std::string
{
	auto indexScopeRes = name.find('@');
	return (indexScopeRes != std::string::npos
		? name.substr(0, indexScopeRes)
		: name);
}

// MEMO: DInfoにアクセスするためにここにあるが stringify ではない
auto fileRefNames() -> std::unordered_set<std::string> const&
{
	return DInfo::instance().fileRefNames();
}

} // namespace hpiutil
