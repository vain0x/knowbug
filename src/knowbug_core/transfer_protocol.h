// メッセージ転送プロトコル

#pragma once

#include "encoding.h"

class Tests;

// バッファーからメッセージを取り出す。
// 取り出したら true を返し、バッファーからメッセージを取り除き、ボディー部分を body にコピーする。
extern auto transfer_protocol_parse(std::u8string& body, std::u8string& buffer) -> bool;

extern void transfer_protocol_tests(Tests& tests);
