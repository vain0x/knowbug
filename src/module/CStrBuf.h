// 文字列バッファ

#ifndef IG_CLASS_STRING_BUFFER_H
#define IG_CLASS_STRING_BUFFER_H

#include <string>
#include <cstdarg>

namespace detail
{
	using std::string;

#if 0
	class stringRef
	{
	private:
		char const* cstr_;
	public:
		stringRef(char const* cstr) : cstr_(cstr) { }
		stringRef(string const& s) : cstr_(s.c_str()) { }
		char const* operator*() { return cstr_; }
	};
#endif

	class CStrBuf
	{
	public:
		CStrBuf();
		CStrBuf(size_t lenLimit);
		CStrBuf(CStrBuf const& src) : CStrBuf(src.getLimit()) { *mpBuf = src.get(); }
		CStrBuf(CStrBuf&& src) : CStrBuf(src.getLimit()) { mpBuf = src.mpBuf; src.mpBuf = nullptr; }
		~CStrBuf();

	public:
		static int const stc_cntUnitIndent = 1;
		static char const* const stc_warning;	// "(too long)"
		static size_t const stc_warning_length = 10;	//= std::strlen(stc_warning);

		void setLenLimit(int lenLimit);
		void reserve(size_t additionalCap);

		void cat(char const* s);
		void catln(char const* s) { cat(s); catCrlf(); }
		void catCrlf();
		void catDump(void const* data, size_t bufsize);

		void cat(string const& s) { cat(s.c_str()); }
		void catln(string const& s) { catln(s.c_str()); }

		string const& get() const { return *mpBuf; }
		int getLimit() const { return mlenLimit; }

	private:
		void catDumpImpl(void const* data, size_t bufsize);
	private:
		string* mpBuf;
		int mlenLimit;
	};

	// ツリー状文字列 (入れ子構造を字下げで表現する)
	// name = value
	// name: ...
	class CTreeformStrBuf
		: public CStrBuf
	{
	public:
		CTreeformStrBuf(size_t lenLimit)
			: CStrBuf(lenLimit)
			, mlvNest(0)
		{ }

	public:
		void incNest() { ++mlvNest; }
		void decNest() { --mlvNest; }

		void catIndent() { cat(getIndent()); }

		// name = value
		void catLeaf(char const* name, char const* value) {
			catIndent();
			cat(name); cat(" = "); cat(value);
			catCrlf();
		}
		void catLeaf(string const& name, char const* value) {
			catLeaf(name.c_str(), value);
		}
		// name : (state)
		void catLeafExtra(char const* name, char const* state) {
			catIndent();
			cat(name); cat(" : ("); cat(state); cat(")");
			catCrlf();
		}
		void catLeafExtra(string const& name, char const* state) { catLeafExtra(name.c_str(), state); }
		// name:
		//     ....
		void catNodeBegin(char const* name) {
			catIndent();
			cat(name); cat(":");
			catCrlf();
			incNest();
		}
		void catNodeBegin(string const& name) { catNodeBegin(name.c_str()); }
		void catNodeEnd() { decNest(); }

		// .attr = value
		void catAttribute(char const* name, char const* value) {
			catLeaf("." + string(name), value);
		}

		string getIndent() const { return string(mlvNest * stc_cntUnitIndent, '\t'); }
		int getNest() const { return mlvNest; }
		bool inifiniteNesting() const { return mlvNest > 512; }	// 無限ネストから保護

	private:
		int mlvNest;
	};

} // namespace detail

using detail::CStrBuf;
using detail::CTreeformStrBuf;

#endif
