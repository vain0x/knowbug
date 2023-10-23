// 開発用のデバッグログを出力する機能を提供する

// 現在の実装では、VisualStudio のデバッグ機能でログ出力を行う
// (デバッガーにアタッチすると「出力」パネルにメッセージが表示される)

#pragma once

#include <cstdarg>

namespace knowbug {

extern void vdebugf(char8_t const* format, va_list vlist);

// デバッグログを出力する
static void debugf(char8_t const* format, ...) {
	va_list vlist;
	va_start(vlist, format);
	vdebugf(format, vlist);
	va_end(vlist);
}

}
