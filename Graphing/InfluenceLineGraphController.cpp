///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "InfluenceLineGraphController.h"
#include <Graphing\InfluenceLineGraphBuilder.h>

#include <EAF\EAFUtilities.h>
#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\Selection.h>

#include <Hints.h>

#include <EAF\EAFGraphBuilderBase.h>
#include <EAF\EAFGraphView.h>

#include <PgsExt\BridgeDescription2.h>

IMPLEMENT_DYNCREATE(CInfluenceLineGraphController,CGirderGraphControllerBase)

CInfluenceLineGraphController::CInfluenceLineGraphController():
CGirderGraphControllerBase(false/*don't use ALL_GROUPS*/)
{
}

BEGIN_MESSAGE_MAP(CInfluenceLineGraphController, CGirderGraphControllerBase)
	//{{AFX_MSG_MAP(CInfluenceLineGraphController)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CInfluenceLineGraphController::OnInitDialog()
{
   FillGraphTypeControl();

   CGirderGraphControllerBase::OnInitDialog();

   // This graph always works in the last interval
   GET_IFACE(IIntervals,pIntervals);
   m_IntervalIdx = pIntervals->GetIntervalCount()-1;

   return TRUE;
}

IndexType CInfluenceLineGraphController::GetGraphCount()
{
   return 1;
}

void CInfluenceLineGraphController::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   if ( lHint == HINT_BRIDGECHANGED )
   {
      // The bridge changed, so reset the controls
      FillGirderCtrl();
   }
}

void CInfluenceLineGraphController::OnGirderChanged()
{
}

void CInfluenceLineGraphController::FillGraphTypeControl()
{
#pragma Reminder("IMPLEMENT: need influence line types")
}

CInfluenceLineGraphBuilder::GraphType CInfluenceLineGraphController::GetGraphType()
{
#pragma Reminder("UPDATE: hard coded type for testing")
   return CInfluenceLineGraphBuilder::Moment;
}

#ifdef _DEBUG
void CInfluenceLineGraphController::AssertValid() const
{
	CGirderGraphControllerBase::AssertValid();
}

void CInfluenceLineGraphController::Dump(CDumpContext& dc) const
{
	CGirderGraphControllerBase::Dump(dc);
}
#endif //_DEBUG
