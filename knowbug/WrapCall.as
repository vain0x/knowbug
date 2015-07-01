// WrapCall - import header

#ifndef IG_HPI_WRAP_CALL_AS
#define IG_HPI_WRAP_CALL_AS

#ifdef _DEBUG

#uselib "hsp3debug.dll"	// knowbug
#cfunc knowbug_hwnd "_knowbug_hwnd@0"

#uselib "WrapCall.hpi"
#func WrapCall_init "_WrapCallInitialize@4" int
#func WrapCall_term "_WrapCallTerminate@0"

	WrapCall_init knowbug_hwnd()
	
#endif

#endif
