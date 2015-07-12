
#ifndef IG_HPIMOD_STRINGIZATION_H
#define IG_HPIMOD_STRINGIZATION_H

#include <string>
#include <vector>

namespace hpimod {

extern char const* nameFromMPType(int mptype);

//文字列リテラル
extern std::string literalFormString(char const* s);

//配列添字の文字列の生成
extern std::string stringizeArrayIndex(std::vector<int> const& indexes);

//修飾子を取り除いた識別子
extern std::string nameExcludingScopeResolution(std::string const& name);

} //namespace hpimod

#endif
