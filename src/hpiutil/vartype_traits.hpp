
#pragma once

namespace hpiutil {

template<typename T>
struct scalar_type_tag
{
	using valptr_type = T*;
	using const_valptr_type = T const*;
	using valref_type = T&;
	using const_valref_type = T const&;
	using master_type = nullptr_t;

	static auto deref(const_valptr_type pdat) -> const_valref_type { return *pdat; }
	static auto deref(valptr_type pdat)       ->       valref_type { return *pdat; }
};

namespace internal_vartype_tags {

struct vtLabel  : scalar_type_tag<label_t> {};
struct vtDouble : scalar_type_tag<double> {};
struct vtInt    : scalar_type_tag<int> {};
struct vtStruct : scalar_type_tag<FlexValue> {};

struct vtStr
{
	using valptr_type = char*;
	using const_valptr_type = char const*;
	using valref_type = char*;
	using const_valref_type = char const*;
	using master_type = char**;

	static auto deref(const_valptr_type pdat) -> const_valref_type { return pdat; }
	static auto deref(valptr_type pdat)       ->       valref_type { return pdat; }
};

} // namespace internal_vartype_tags

using namespace internal_vartype_tags;

// elementary type functions

template<typename Tag> using valptr_t       = typename Tag::valptr_type;
template<typename Tag> using const_valptr_t = typename Tag::const_valptr_type;
template<typename Tag> using valref_t       = typename Tag::valref_type;
template<typename Tag> using const_valref_t = typename Tag::const_valref_type;
template<typename Tag> using master_t       = typename Tag::master_type;

// Cast PDAT* <--> valptr_t

template<typename Tag>
auto asPDAT(const_valptr_t<Tag> pdat) -> PDAT const*
{
	return reinterpret_cast<PDAT const*>(pdat);
}

template<typename Tag>
auto asPDAT(valptr_t<Tag> pdat) -> PDAT*
{
	return reinterpret_cast<PDAT*>(pdat);
}

template<typename Tag>
auto asValptr(PDAT const* pdat) -> const_valptr_t<Tag>
{
	return reinterpret_cast<const_valptr_t<Tag>>(pdat);
}

template<typename Tag>
auto derefValptr(valptr_t<Tag> pdat) -> valref_t<Tag>
{ return Tag::deref(pdat); }

template<typename Tag>
auto derefValptr(const_valptr_t<Tag> pdat) -> const_valref_t<Tag>
{ return Tag::deref(pdat); }

template<typename Tag>
auto derefValptr(PDAT* pdat) -> valref_t<Tag>
{ return derefValptr<Tag>(asValptr<Tag>(pdat)); }

template<typename Tag>
auto derefValptr(PDAT const* pdat) -> const_valref_t<Tag>
{ return derefValptr<Tag>(asValptr<Tag>(pdat)); }

} // namespace hpiutil
