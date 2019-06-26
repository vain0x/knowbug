//! ログ関連

#pragma once

#include <memory>
#include "encoding.h"

class Logger;
class LogObserver;

class LogObserver {
public:
	virtual ~LogObserver()
	{
	}

	virtual void did_change() = 0;
};

// FIXME: ロガーが2つあるので統合する

class Logger {
	// ログの中身。
	OsString content_;

	// ログに追記するたびに行う処理。
	std::weak_ptr<LogObserver> observer_;

	// 自動保存されるファイルのパス。
	std::unique_ptr<OsString> auto_save_path_;

public:
	~Logger();

	void set_observer(std::weak_ptr<LogObserver> observer);

	void enable_auto_save(OsStringView const& file_path);

	auto content() const ->OsStringView;

	void append(OsStringView const& msg);

	void append_line(OsStringView const& msg);

	void append_warning(OsStringView const& msg, OsStringView const& execution_location);

	void clear();

	// ログを保存する。成功したら true。
	bool save(OsStringView const& file_path);

private:
	void do_auto_save();

	void notify();
};
