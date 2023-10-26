// 開発用のデバッグログを出力する機能を提供する

// 現在の実装では、VisualStudio のデバッグ機能でログ出力を行う
// (デバッガーにアタッチすると「出力」パネルにメッセージが表示される)

#pragma once

// デバッグログを出力する
void debugf(char8_t const* format, ...);
