// WrapCall - sdk struct

#ifndef IG_STRUCT_WRAP_CALL_SDK_H
#define IG_STRUCT_WRAP_CALL_SDK_H

// @ WrapCall 側の情報を公開するための構造体

struct WrapCallSdk
{
	
};

struct WrapCallMethod
{
	void (*AddLog)( const char* log );
	
	void (*BgnCalling)( unsigned int idx, const ModcmdCallInfo* pCallInfo );
	int  (*EndCalling)( unsigned int idx, const ModcmdCallInfo* pCallInfo );
};

#endif
