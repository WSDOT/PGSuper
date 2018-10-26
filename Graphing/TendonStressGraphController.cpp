///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include "TendonStressGraphController.h"
#include <Graphing\TendonStressGraphBuilder.h>

#include <EAF\EAFUtilities.h>
#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\Selection.h>

#include <Hints.h>

#include <EAF\EAFGraphBuilderBase.h>
#include <EAF\EAFGraphView.h>

#include <PgsExt\BridgeDescription2.h>

IMPLEMENT_DYNCREATE(CTendonStressGraphController,CGirderGraphControllerBase)

CTendonStressGraphController::CTendonStressGraphController():
CGirderGraphControllerBase(false/*don't use ALL_GROUPS*/),
m_DuctIdx(INVALID_INDEX)
{
}

BEGIN_MESSAGE_MAP(CTendonStressGraphController, CGirderGraphControllerBase)
	//{{AFX_MSG_MAP(CTendonStressGraphController)
   ON_CBN_SELCHANGE( IDC_DUCT, OnDuctChanged )
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CTendonStressGraphController::OnInitDialog()
{
   CGirderGraphControllerBase::OnInitDialog();

   FillDuctCtrl();

   return TRUE;
}

IndexType CTendonStressGraphController::GetGraphCount()
{
   return 1;
}

void CTendonStressGraphController::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   if ( lHint == HINT_BRIDGECHANGED )
   {
      // The bridge changed, so reset the controls
      FillGroupCtrl();
      FillGirderCtrl();
      FillIntervalCtrl();
      FillDuctCtrl();
   }
}

DuctIndexType CTendonStressGraphController::GetDuct()
{
   return m_DuctIdx;
}

void CTendonStressGraphController::OnGroupChanged()
{
   FillDuctCtrl();
}

void CTendonStressGraphController::OnGirderChanged()
{
   FillDuctCtrl();
}

void CTendonStressGraphController::OnDuctChanged()
{
   CComboBox* pcbDuct = (CComboBox*)GetDlgItem(IDC_DUCT);
   int curSel = pcbDuct->GetCurSel();
   if ( m_DuctIdx != (DuctIndexType)curSel )
   {
      m_DuctIdx = (DuctIndexType)curSel;
      UpdateGraph();
   }
}

void CTendonStressGraphController::FillDuctCtrl()
{
   GroupIndexType grpIdx = GetGirderGroup();
   GirderIndexType gdrIdx = GetGirder();

   CComboBox* pcbDuct = (CComboBox*)GetDlgItem(IDC_DUCT);
   int curSel = pcbDuct->GetCurSel();
   pcbDuct->ResetContent();

   GET_IFACE(ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(CGirderKey(grpIdx,gdrIdx));
   
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      CString strDuct;
      strDuct.Format(_T("Duct %d"),LABEL_DUCT(ductIdx));
      pcbDuct->AddString(strDuct);
   }

   if ( curSel != CB_ERR && curSel < nDucts)
   {
      curSel = pcbDuct->SetCurSel(curSel);
   }
   else
   {
      curSel = pcbDuct->SetCurSel(0);
   }

   if ( curSel == CB_ERR )
   {
      m_DuctIdx = INVALID_INDEX;
      pcbDuct->EnableWindow(FALSE);
   }
   else
   {
      m_DuctIdx = (DuctIndexType)curSel;
      pcbDuct->EnableWindow(TRUE);
   }
}

#ifdef _DEBUG
void CTendonStressGraphController::AssertValid() const
{
	CGirderGraphControllerBase::AssertValid();
}

void CTendonStressGraphController::Dump(CDumpContext& dc) const
{
	CGirderGraphControllerBase::Dump(dc);
}
#endif //_DEBUG
