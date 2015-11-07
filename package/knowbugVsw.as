// knowbug VardataStringWriter

#ifndef IG_KNOWBUG_VARDATA_STRING_WRITER_AS
#define IG_KNOWBUG_VARDATA_STRING_WRITER_AS

#ifdef _DEBUG

#module knowbugVsw vsw_

#ifdef __hsp64__@
 #uselib "hsp3debug_64.dll"
 #func global knowbugVsw_catLeaf       "knowbugVsw_catLeaf"      int, sptr, sptr
 #func global knowbugVsw_catLeafExtra  "knowbugVsw_catLeafExtra" int, sptr, sptr
 #func global knowbugVsw_catAttribute  "knowbugVsw_catAttribute" int, sptr, sptr
 #func global knowbugVsw_catNodeBegin  "knowbugVsw_catNodeBegin" int, sptr, sptr
 #func global knowbugVsw_catNodeEnd    "knowbugVsw_catNodeEnd"   int, sptr
 #func global knowbugVsw_addVar        "knowbugVsw_addVar@"      int, sptr, pval
 #func global knowbugVsw_addVarScalar  "knowbugVsw_addVarScalar" int, sptr, pval, int
 #func global knowbugVsw_addVarArray   "knowbugVsw_addVarArray"  int, sptr, pval
 #func global knowbugVsw_addValue      "knowbugVsw_addValue"     int, sptr, int, int
 #func global knowbugVsw_addSysvar     "knowbugVsw_addSysvar"    int, sptr

 #cfunc knowbugVsw_newTreeformedWriter "knowbugVsw_newTreeformedWriter"
 #cfunc knowbugVsw_newLineformedWriter "knowbugVsw_newLineformedWriter"
 #func  knowbugVsw_deleteWriter "knowbugVsw_deleteWriter" int
 #cfunc knowbugVsw_dataPtr      "knowbugVsw_dataPtr" int, int
#else //defined(__hsp64__@)
 #uselib "hsp3debug.dll"
 #func global knowbugVsw_catLeaf       "_knowbugVsw_catLeaf@12"      int, sptr, sptr
 #func global knowbugVsw_catLeafExtra  "_knowbugVsw_catLeafExtra@12" int, sptr, sptr
 #func global knowbugVsw_catAttribute  "_knowbugVsw_catAttribute@12" int, sptr, sptr
 #func global knowbugVsw_catNodeBegin  "_knowbugVsw_catNodeBegin@12" int, sptr, sptr
 #func global knowbugVsw_catNodeEnd    "_knowbugVsw_catNodeEnd@8"    int, sptr
 #func global knowbugVsw_addVar        "_knowbugVsw_addVar@12"       int, sptr, pval
 #func global knowbugVsw_addVarScalar  "_knowbugVsw_addVarScalar@16" int, sptr, pval, int
 #func global knowbugVsw_addVarArray   "_knowbugVsw_addVarArray@12"  int, sptr, pval
 #func global knowbugVsw_addValue      "_knowbugVsw_addValue@12"     int, sptr, int, int
 #func global knowbugVsw_addSysvar     "_knowbugVsw_addSysvar@8"     int, sptr

 #cfunc knowbugVsw_newTreeformedWriter "_knowbugVsw_newTreeformedWriter@0"
 #cfunc knowbugVsw_newLineformedWriter "_knowbugVsw_newLineformedWriter@0"
 #func  knowbugVsw_deleteWriter "_knowbugVsw_deleteWriter@4" int
 #cfunc knowbugVsw_dataPtr      "_knowbugVsw_dataPtr@8" int, int
#endif //defined(__hsp64__@)

#modinit int isLineformed
	if ( isLineformed ) {
		vsw_ = knowbugVsw_newLineformedWriter()
	} else {
		vsw_ = knowbugVsw_newTreeformedWriter()
	}
	assert vsw_
	return
	
#modterm
	knowbugVsw_deleteWriter vsw_
	vsw_ = 0
	return
	
#modfunc kvsw_dup var buf,  local dataPtr, local len
	dataPtr = knowbugVsw_dataPtr(vsw_, varptr(len))
	dupptr buf, dataPtr, len + 1, 2
	return
	
#modcfunc kvsw_getString  local buf
	kvsw_dup thismod, buf
	return buf
	
#modfunc kvsw_var var v, str name
	knowbugVsw_addVar vsw_, name, v
	return
	
#modfunc kvsw_array array arr, str name
	knowbugVsw_addVarArray vsw_, name, arr
	return
	
#modfunc kvsw_sysvar str name
	knowbugVsw_addSysvar vsw_, name
	return
	
#global

#endif // defined(_DEBUG)

#endif
