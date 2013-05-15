#pragma once
#ifndef __DEM_L3_AI_VALIDATOR_CAN_TALK_H__
#define __DEM_L3_AI_VALIDATOR_CAN_TALK_H__

#include <AI/SmartObj/Validator.h>

// Checks if the actor can talk with the smart object right now

namespace AI
{

class CValidatorCanTalk: public CValidator
{
	__DeclareClass(CValidatorCanTalk);

public:

	virtual bool IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction);
};

__RegisterClassInFactory(CValidatorCanTalk);

typedef Ptr<CValidatorCanTalk> PValidatorCanTalk;

}

#endif