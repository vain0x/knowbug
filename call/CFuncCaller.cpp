
#include "CFuncCaller.h"

void CFuncCaller::addArgByVal(PDAT const* pdat, vartype_t vtype)
{
	auto const& prminfo = getPrmInfo();
	int const prmtype = prminfo.getPrmType(cntArgs_);


}

void CFuncCaller::addArgByRef(PVal* pval, APTR aptr)
{
	auto const& prminfo = getPrmInfo();
	int const prmtype = prminfo.getPrmType(cntArgs_);

}

void CFuncCaller::addLocals()
{
	prmstk_.pushLocal();
}