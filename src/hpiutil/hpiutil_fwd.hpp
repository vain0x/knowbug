
#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include "../hspsdk/hsp3plugin.h"
#undef min
#undef max
#undef stat

namespace hpiutil {

static auto const ArrayDimMax = size_t { 4 };

//static auto const BracketIdxL = "(";
//static auto const BracketIdxR = ")";

static auto const HSPVAR_FLAG_COMOBJ = HSPVAR_FLAG_COMSTRUCT;
static auto const HSPVAR_FLAG_VARIANT = 7;

using vartype_t = unsigned short;
using varmode_t = signed short;
using label_t = unsigned short const*; // a.k.a. HSPVAR_LABEL
using csptr_t = unsigned short const*;
using stdat_t = STRUCTDAT const*;
using stprm_t = STRUCTPRM const*;

// HspVarProcの演算関数
using operator_t = void(*)(PDAT*, void const*);

// デバッグウィンドウのコールバック関数

// デバッグウィンドウへの通知ID
enum DebugNotice
{
	DebugNotice_Stop = 0,
	DebugNotice_Logmes = 1,
};

class DInfo;
class SourcePos;

// 定数 /MPTYPE_(\w+)/ の値に対応する適当な名前を得る
extern auto nameFromMPType(int mptype) -> char const*;

// モジュールクラス名を得る (クローンなら末尾に `&` をつける)
extern auto nameFromModuleClass(stdat_t stdat, bool isClone) -> std::string;

/**
エイリアスの名前を得る
index はそのエイリアスの元の引数列における番号。
DInfo からみつからなければ "(i)" が返る。
//*/
extern auto nameFromStPrm(stprm_t stprm, int index, DInfo const& debug_segment) -> std::string;

//文字列リテラル
extern auto literalFormString(char const* s) -> std::string;

//配列添字の文字列の生成
extern auto stringifyArrayIndex(std::vector<int> const& indexes) -> std::string;

//修飾子を取り除いた識別子
extern auto nameExcludingScopeResolution(std::string const& name) -> std::string;

}
