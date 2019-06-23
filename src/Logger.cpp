//! ログ関連

#include <cassert>
#include <fstream>
#include "encoding.h"
#include "Logger.h"

Logger::~Logger() {
	do_auto_save();
}

auto Logger::content() const -> OsStringView {
	return content_.as_ref();
}

void Logger::set_observer(std::weak_ptr<LogObserver> observer) {
	observer_ = observer;
}

void Logger::enable_auto_save(OsStringView const& file_path) {
	assert(auto_save_path_ == nullptr);
	auto_save_path_ = std::make_unique<OsString>(file_path.to_owned());
}

void Logger::append(OsStringView const& msg) {
	content_ += msg.data();
	notify();
}

void Logger::append_line(OsStringView const& msg) {
	content_ += msg.data();
	content_ += TEXT("\r\n");
	notify();
}

void Logger::append_warning(OsStringView const& msg, OsStringView const& execution_location) {
	auto text = OsStringView{ TEXT("warning: ") }.to_owned();
	text += msg.data();
	text += TEXT("\r\nCurInf:");
	text += execution_location.data();
	text += TEXT("\r\n");

	append(text.as_ref());
}

void Logger::clear() {
	content_.clear();
}

bool Logger::save(OsStringView const& file_path) {
	auto content = content_.to_utf8_string();

	auto file_stream = std::ofstream{ file_path.data() };
	file_stream.write(content.data(), content.size());
	return file_stream.good();
}

void Logger::do_auto_save() {
	if (content_.empty()) {
		return;
	}

	if (!auto_save_path_ || auto_save_path_->empty()) {
		return;
	}

	auto ok = save(auto_save_path_->as_ref());

	// NOTE: この関数はアプリケーションの終了中に呼ばれるので、エラーがあっても報告できない。
	assert(ok);

	auto_save_path_ = nullptr;
}

void Logger::notify() {
	if (auto observer = observer_.lock()) {
		observer->did_change();
	}
}