// スクリプト側との通信用

#ifndef IG_KNOWBUG_WITH_SCRIPT_H
#define IG_KNOWBUG_WITH_SCRIPT_H

#include "main.h"

#ifdef with_Script

extern char const* getNodeAnnotation(char const* name);
extern char const* getStPrmName( const stprm_t stprm );

extern void termConnectWithScript();
//static char const* const WindowHandleHolderName = "wh_knowbug@knowbug";

// スクリプト側から呼ばれる関数
EXPORT void WINAPI knowbug_greet();

/*
EXPORT void WINAPI knowbug_namePrms(char const* nameStDat,
	char const* p1, char const* p2, char const* p3, char const* p4,
	char const* p5, char const* p6, char const* p7, char const* p8,
	char const* p9, char const* p10, char const* p11, char const* p12,
	char const* p13, char const* p14, char const* p15);		// HPIでやれ
//*/

#else

static char const* getNodeAnnotation(char const*) { return nullptr; }
//static char const* getStPrmName( const stprm_t stprm ) { return nullptr; }

#endif

#endif
