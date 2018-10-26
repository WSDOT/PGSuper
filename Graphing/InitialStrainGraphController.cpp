///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include "InitialStrainGraphController.h"
#include <Graphing\InitialStrainGraphBuilder.h>

#include <EAF\EAFUtilities.h>
#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\Selection.h>

#include <Hints.h>

#include <EAF\EAFGraphBuilderBase.h>
#include <EAF\EAFGraphView.h>

#include <PgsExt\BridgeDescription2.h>

IMPLEMENT_DYNCREATE(CInitialStrainGraphController,CIntervalGirderGraphControllerBase)

CInitialStrainGraphController::CInitialStrainGraphController():
CIntervalGirderGraphControllerBase(false/*don't use ALL_GROUPS*/)
{
   m_GroupIdx = 0;
}

BEGIN_MESSAGE_MAP(CInitialStrainGraphController, CIntervalGirderGraphControllerBase)
	//{{AFX_MSG_MAP(CInitialStrainGraphController)
   ON_CBN_SELCHANGE( IDC_GRAPH_TYPE, OnGraphTypeChanged)
   ON_CONTROL_RANGE( BN_CLICKED, IDC_CREEP, IDC_RELAXATION, OnEffectTypeChanged)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CInitialStrainGraphController::OnInitDialog()
{
   CIntervalGirderGraphControllerBase::OnInitDialog();

   FillGraphType();

   return TRUE;
}

void CInitialStrainGraphController::FillGraphType()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GRAPH_TYPE);
   pCB->AddString(_T("Initial Strain"));
   pCB->AddString(_T("Initial Curvature"));
   pCB->AddString(_T("Restraining Force"));
   pCB->AddString(_T("Restraining Moment"));

   pCB->SetCurSel(0);
}

IndexType CInitialStrainGraphController::GetGraphCount()
{
   return 1;
}

bool CInitialStrainGraphController::Creep()
{
   return IsDlgButtonChecked(IDC_CREEP) == BST_CHECKED ? true : false;
}

bool CInitialStrainGraphController::Shrinkage()
{
   return IsDlgButtonChecked(IDC_SHRINKAGE) == BST_CHECKED ? true : false;
}

bool CInitialStrainGraphController::Relaxation()
{
   return IsDlgButtonChecked(IDC_RELAXATION) == BST_CHECKED ? true : false;
}

void CInitialStrainGraphController::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   CIntervalGirderGraphControllerBase::OnUpdate(pSender,lHint,pHint);

   if ( lHint == HINT_BRIDGECHANGED )
   {
      // The bridge changed, so reset the controls
      FillGirderCtrl();
   }
}

void CInitialStrainGraphController::OnGraphTypeChanged()
{
   UpdateGraph();
}

void CInitialStrainGraphController::OnEffectTypeChanged(UINT nIDC)
{
   UpdateGraph();
}

CInitialStrainGraphBuilder::GraphType CInitialStrainGraphController::GetGraphType()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GRAPH_TYPE);
   int curSel = pCB->GetCurSel();
   return (CInitialStrainGraphBuilder::GraphType)curSel;
}

#ifdef _DEBUG
void CInitialStrainGraphController::AssertValid() const
{
	CIntervalGirderGraphControllerBase::AssertValid();
}

void CInitialStrainGraphController::Dump(CDumpContext& dc) const
{
	CIntervalGirderGraphControllerBase::Dump(dc);
}
#endif //_DEBUG
