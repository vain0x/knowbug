// スクリプト側との通信用

#ifndef IG_KNOWBUG_WITH_SCRIPT_H
#define IG_KNOWBUG_WITH_SCRIPT_H

#include "main.h"

#ifdef with_Script

extern const char* getNodeAnnotation(const char* name);
extern const char* getStPrmName( const STRUCTPRM* stprm );

extern void termConnectWithScript();
//static const char* const WindowHandleHolderName = "wh_knowbug@knowbug";

// スクリプト側から呼ばれる関数
EXPORT void WINAPI knowbug_greet();

/*
EXPORT void WINAPI knowbug_namePrms(const char* nameStDat,
	const char* p1, const char* p2, const char* p3, const char* p4,
	const char* p5, const char* p6, const char* p7, const char* p8,
	const char* p9, const char* p10, const char* p11, const char* p12,
	const char* p13, const char* p14, const char* p15);		// HPIでやれ
//*/

#else

static const char* getNodeAnnotation(const char*) { return nullptr; }
//static const char* getStPrmName( const STRUCTPRM* stprm ) { return nullptr; }

#endif

#endif
