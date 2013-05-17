#include "Factory.h"

#include <Core/RTTI.h>

//???RTTI class name CStrID, CSimpleString or nString?

namespace Core
{

CFactory* CFactory::Instance()
{
	static CFactory Singleton;
	return &Singleton;
}
//---------------------------------------------------------------------

void CFactory::Register(const CRTTI& RTTI, const nString& Name, nFourCC FourCC)
{
	n_assert2(!IsRegistered(Name), Name.CStr());
	if (FourCC != 0) n_assert2(!IsRegistered(FourCC), n_fourcctostr(FourCC));
	NameToRTTI.Add(Name, &RTTI);
	if (FourCC != 0) FourCCToRTTI.Add(FourCC, &RTTI);
}
//---------------------------------------------------------------------

CRefCounted* CFactory::Create(const nString& ClassName, void* pParam) const
{
	n_assert2_dbg(IsRegistered(ClassName), ClassName.CStr());
	const CRTTI* pRTTI = GetRTTI(ClassName);
	n_assert_dbg(pRTTI);
	return pRTTI->Create(pParam);
}
//---------------------------------------------------------------------

CRefCounted* CFactory::Create(nFourCC ClassFourCC, void* pParam) const
{
	n_assert2_dbg(IsRegistered(ClassFourCC), n_fourcctostr(ClassFourCC));
	const CRTTI* pRTTI = GetRTTI(ClassFourCC);
	n_assert_dbg(pRTTI);
	return pRTTI->Create(pParam);
}
//---------------------------------------------------------------------

} // namespace Core
