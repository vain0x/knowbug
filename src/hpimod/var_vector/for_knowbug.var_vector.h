// header for knowbug

#ifndef IG_VECTOR_FOR_KNOWBUG_H
#define IG_VECTOR_FOR_KNOWBUG_H

class CVector;

static char const* const vector_vartype_name = "vector_k";

using GetVectorList_t = PVal**(*)( const CVector*, int* );		// HspVarProc::user

#endif
