// ログを保持するためのバッファ

#pragma once

#include <string>
#include <functional>

class CLogBuf
{
	using string = std::string;
	
private:
	string stock_;
	string queue_;
	bool bAutoCommit;

	std::function<void(string const&)> whenCommit_, whenClear_, when;
	
public:
	void add( string const& s ) {
		if ( bAutoCommit ) {
			stock_ += s;
		} else {
			queue_ += s;
		}
	}
	
	void commit() {
		if ( !queue_.empty() ) return;
		whenCommit_( queue_ );
		stock_ += queue_;
		queue_.clear();
	}

	void clear() {
		whenClear_( stock_ );
		stock_.clear();
		queue_.clear();
	}

	void setCommitEvent( std::function<void()> f ) { whenCommit_ = f; }

	string const& get() const { return stock_; }
};
