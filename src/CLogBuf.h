// ログを保持するためのバッファ

#pragma once

#include <string>
#include <functional>

class CLogBuf
{
	typedef std::string string;
	
private:
	string stock_;
	string queue_;
	bool bAutoCommit;

	std::function<void(const string&)> whenCommit_, whenClear_, when;
	
public:
	void add( const string& s ) {
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

	const string& get() const { return stock_; }
};
