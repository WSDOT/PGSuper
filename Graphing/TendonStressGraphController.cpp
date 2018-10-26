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
#include "TendonStressGraphController.h"
#include <Graphing\TendonStressGraphBuilder.h>

#include <EAF\EAFUtilities.h>
#include <IFace\DocumentType.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Selection.h>

#include <EAF\EAFGraphBuilderBase.h>
#include <EAF\EAFGraphView.h>

#include <PgsExt\BridgeDescription2.h>

IMPLEMENT_DYNCREATE(CTendonStressGraphController,CGirderGraphControllerBase)

CTendonStressGraphController::CTendonStressGraphController():
CGirderGraphControllerBase(),
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
   CComboBox* pcbDuct = (CComboBox*)GetDlgItem(IDC_DUCT);
   pcbDuct->SetCurSel(0);
   m_DuctIdx = 0;

   return TRUE;
}

IndexType CTendonStressGraphController::GetGraphCount()
{
   return 1;
}

DuctIndexType CTendonStressGraphController::GetDuct()
{
   return m_DuctIdx;
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

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
   const CPTData* pPTData = pGirder->GetPostTensioning();
   
   pcbDuct->ResetContent();
   DuctIndexType nDucts = pPTData->GetDuctCount();
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      CString strDuct;
      strDuct.Format(_T("Duct %d"),LABEL_DUCT(ductIdx));
      pcbDuct->AddString(strDuct);
   }

   if ( curSel != CB_ERR && curSel < nDucts)
   {
      pcbDuct->SetCurSel(curSel);
   }
   else
   {
      pcbDuct->SetCurSel(0);
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
