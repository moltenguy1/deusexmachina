#include "ControlLayout.h"

#include <Input/InputServer.h>
#include <Input/Events/KeyDown.h>
#include <Input/Events/KeyUp.h>
#include <Input/Events/CharInput.h>
#include <Input/Events/MouseBtnDown.h>
#include <Input/Events/MouseBtnUp.h>
#include <Input/Events/MouseDoubleClick.h>
#include <Input/Events/MouseWheel.h>
#include <Events/EventServer.h>

namespace Input
{

bool CControlLayout::Init(const Data::CParams& Desc)
{
	Data::PParams Mappings;

	EventMappings.Clear();
	if (Desc.Get<Data::PParams>(Mappings, CStrID("Events")) && Mappings->GetCount())
	{
		CInputMappingEvent* pNew = EventMappings.Reserve(Mappings->GetCount());
		for (int i = 0; i < Mappings->GetCount(); ++i, ++pNew)
		{
			Data::CParam& Prm = Mappings->Get(i);
			if (Prm.IsA<int>())
			{
				// Syntax shortcut for KeyUp events
				if (!pNew->Init(Prm.GetName(), Prm.GetValue<int>())) FAIL;
			}
			else if (!pNew->Init(Prm.GetName(), *Prm.GetValue<Data::PParams>())) FAIL;
		}
	}

	StateMappings.Clear();
	if (Desc.Get<Data::PParams>(Mappings, CStrID("States")) && Mappings->GetCount())
	{
		CInputMappingState* pNew = StateMappings.Reserve(Mappings->GetCount());
		for (int i = 0; i < Mappings->GetCount(); ++i, ++pNew)
		{
			Data::CParam& Prm = Mappings->Get(i);
			if (!pNew->Init(Prm.GetName(), *Prm.GetValue<Data::PParams>())) FAIL;
		}
	}

	Enabled = false;

	OK;
}
//---------------------------------------------------------------------

void CControlLayout::Enable()
{
	if (Enabled) return;
	for (CArray<CInputMappingEvent>::CIterator It = EventMappings.Begin(); It != EventMappings.End(); It++)
		It->Enable();
	for (CArray<CInputMappingState>::CIterator It = StateMappings.Begin(); It != StateMappings.End(); It++)
		It->Enable();
	Enabled = true;
}
//---------------------------------------------------------------------

void CControlLayout::Disable()
{
	if (!Enabled) return;
	for (CArray<CInputMappingEvent>::CIterator It = EventMappings.Begin(); It != EventMappings.End(); It++)
		It->Disable();
	for (CArray<CInputMappingState>::CIterator It = StateMappings.Begin(); It != StateMappings.End(); It++)
		It->Disable();
	Enabled = false;
}
//---------------------------------------------------------------------

void CControlLayout::Reset()
{
	//!!!WRITE IT!
}
//---------------------------------------------------------------------

} // namespace Input
