// knowbug optional module

#ifndef        IG_KNOWBUG_AS
#define global IG_KNOWBUG_AS
#ifdef _DEBUG

#uselib "hsp3debug.dll"
#func knowbug_greet "_knowbug_greet@0"
#func knowbug_namePrms_ "_knowbug_namePrms@64" sptr,sptr,sptr,sptr,sptr,sptr,sptr,sptr,sptr,sptr,sptr,sptr,sptr,sptr,sptr,sptr
#define global knowbug_namePrms(%1, %2=0,%3=0,%4=0,%5=0,%6=0,%7=0,%8=0,%9=0,%10=0,%11=0,%12=0,%13=0,%14=0,%15=0,%16=0) \
	knowbug_namePrms_ %1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15,%16

#module knowbug

#define _empty

// 変数ノードのノード名につく修飾子
#define NamePrefix_Category "+"		// +dyanmic, +sysvar
#define NamePrefix_Var ""
#define NamePrefix_Sysvar "~"
#define NamePrefix_Module "@"
#define NamePrefix_Call   "'"
#define NamePrefix_Result "\""

/*
dim wh_knowbug@knowbug	// この変数名の変数に knowbug のウィンドウハンドルが自動的に格納される
#define whKnowbug wh_knowbug@knowbug

// 通信用ウィンドウメッセージ
#enum DWM_BgnWithScript = (0x400 | 0x90)
#enum DWM_InitConnectWithScript
#enum DWM_SetNodeAnnotation
#enum DWM_SetStPrmNameBegin
#enum DWM_SetStPrmNameEnd
#enum DWM_SetStPrmName

#deffunc knowbug_greet
	sendmsg whKnowbug, DWM_InitConnectWithScript	// 連繋開始を通知する
	sdim stPrmNames@knowbug,, 15
	return

#define global knowbug_namePrms(%1, %2="",%3="",%4="",%5="",%6="",%7="",%8="",%9="",%10="",%11="",%12="",%13="",%14="",%15="",%16="") \
	stPrmNames@knowbug = %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12, %13, %14, %15, %16 :\
	knowbug_setStPrmName_impl (%1), stPrmNames@knowbug;
	
#deffunc knowbug_namePrms_impl str modname, array namelist
	sendmsg whKnowbug, DWM_SetStPrmNameBegin, 0, modname
	foreach namelist
		if ( namelist(cnt) != "" ) { sendmsg whKnowbug, DWM_SetStPrmName, cnt, varptr(namelist(cnt)) }
	loop
	sendmsg whKnowbug, DWM_SetStPrmNameEnd, 0, 0
	return
//*/

;#deffunc knowbug_setNodeAnnotation str prefix, str name, str msg
;	assert
;	return
	
#global

	knowbug_greet

#else	// defined(_DEBUG)

#define knowbug_setAnnotation(%1, %2, %3) :
#define knowbug_namePrms(%1, %2=0,%3=0,%4=0,%5=0,%6=0,%7=0,%8=0,%9=0,%10=0,%11=0,%12=0,%13=0,%14=0,%15=0,%16=0) :

#endif	// defined(_DEBUG)
#endif
