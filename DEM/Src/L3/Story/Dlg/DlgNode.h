#pragma once
#ifndef __IPG_DLG_NODE_H__
#define __IPG_DLG_NODE_H__

#include <Core/RefCounted.h>

// Dialogue node base. Can be used as start phrase-less node to determine real starting node by links evaluation.
// Does transition along the first valid link.

namespace Story
{
class CDlgLink;
class CActiveDlg;

class CDlgNode: public Core::CRefCounted
{
	DeclareRTTI;
	DeclareFactory(CDlgNode);

public:

	nArray<Ptr<CDlgLink>> Links; //???!!!or nonptr array of structs declared here?!

	virtual void		OnEnter(CActiveDlg& Dlg) {}
	virtual CDlgNode*	Trigger(CActiveDlg& Dlg);
};

RegisterFactory(CDlgNode);

}

#endif
