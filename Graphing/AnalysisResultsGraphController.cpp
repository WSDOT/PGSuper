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
#include "AnalysisResultsGraphController.h"
#include <Graphing\AnalysisResultsGraphBuilder.h>

#include <EAF\EAFUtilities.h>
#include <IFace\DocumentType.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Selection.h>

#include <EAF\EAFGraphBuilderBase.h>
#include <EAF\EAFGraphView.h>

#include <PgsExt\BridgeDescription2.h>

IMPLEMENT_DYNCREATE(CAnalysisResultsGraphController,CGirderGraphControllerBase)

CAnalysisResultsGraphController::CAnalysisResultsGraphController():
CGirderGraphControllerBase(),
m_ActionType(actionMoment),
m_AnalysisType(pgsTypes::Simple)
{
}

BEGIN_MESSAGE_MAP(CAnalysisResultsGraphController, CGirderGraphControllerBase)
	//{{AFX_MSG_MAP(CAnalysisResultsGraphController)
   ON_CBN_SELCHANGE( IDC_ACTION, OnActionChanged )

   ON_LBN_SELCHANGE( IDC_LOAD_CASE, OnLoadCaseChanged )

   ON_BN_CLICKED(IDC_SIMPLE,OnAnalysisTypeClicked)
   ON_BN_CLICKED(IDC_SIMPLE2,OnAnalysisTypeClicked)
   ON_BN_CLICKED(IDC_SIMPLE3,OnAnalysisTypeClicked)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CAnalysisResultsGraphController::OnInitDialog()
{
   CGirderGraphControllerBase::OnInitDialog();

   FillActionTypeCtrl();
   CComboBox* pcbAction = (CComboBox*)GetDlgItem(IDC_ACTION);
   pcbAction->SetCurSel(0);
   m_ActionType = actionMoment;

   FillLoadCaseList();

   UpdateAnalysisType();

   return TRUE;
}

ActionType CAnalysisResultsGraphController::GetActionType()
{
   return m_ActionType;
}

pgsTypes::AnalysisType CAnalysisResultsGraphController::GetAnalysisType()
{
   return m_AnalysisType;
}

IDType CAnalysisResultsGraphController::SelectedGraphIndexToGraphID(IndexType graphIdx)
{
   CListBox* plbLoadCases = (CListBox*)GetDlgItem(IDC_LOAD_CASE);

   CArray<int,int> array;
   array.SetSize(graphIdx+1);

   plbLoadCases->GetSelItems((int)(graphIdx+1),(LPINT)array.GetData());

   int idx = array[graphIdx];

   IDType id = (IDType)plbLoadCases->GetItemData(idx);

   return id;
}

IndexType CAnalysisResultsGraphController::GetGraphCount()
{
   CListBox* plbLoadCases = (CListBox*)GetDlgItem(IDC_LOAD_CASE);
   IndexType count = plbLoadCases->GetSelCount();

   return count;
}

void CAnalysisResultsGraphController::OnActionChanged()
{
   CComboBox* pcbAction = (CComboBox*)GetDlgItem(IDC_ACTION);

   int curSel = pcbAction->GetCurSel();
   ActionType actionType = ActionType(curSel);
   if ( m_ActionType == actionType )
   {
      // action type didn't change
      return;
   }

   m_ActionType = actionType;

   FillLoadCaseList();

   UpdateGraph();
}

void CAnalysisResultsGraphController::OnLoadCaseChanged()
{
   UpdateGraph();
}

void CAnalysisResultsGraphController::OnAnalysisTypeClicked()
{
   int id = GetCheckedRadioButton(IDC_SIMPLE,IDC_SIMPLE3);
   pgsTypes::AnalysisType analysisType;
   switch (id)
   {
   case IDC_SIMPLE:
      analysisType = pgsTypes::Simple;
      break;

   case IDC_SIMPLE2:
      analysisType = pgsTypes::Continuous;
      break;

   case IDC_SIMPLE3:
      analysisType = pgsTypes::Envelope;
      break;

   default:
      ATLASSERT(false); // is there a new analysis type?
   }

   if ( m_AnalysisType == analysisType )
      return;

   m_AnalysisType = analysisType;

   UpdateGraph();
}

void CAnalysisResultsGraphController::OnGirderChanged()
{
   CGirderGraphControllerBase::OnGirderChanged();

   // The available graphs depend on the specific girder selected
   ((CAnalysisResultsGraphBuilder*)GetGraphBuilder())->UpdateGraphDefinitions();

   FillLoadCaseList();
}

void CAnalysisResultsGraphController::OnIntervalChanged()
{
   CGirderGraphControllerBase::OnIntervalChanged();
   FillLoadCaseList();
}

void CAnalysisResultsGraphController::FillActionTypeCtrl()
{
   CComboBox* pcbGraphType = (CComboBox*)GetDlgItem(IDC_ACTION);

   int curSel = pcbGraphType->GetCurSel();

   pcbGraphType->ResetContent();

   // this must match the order of the GT_ constants defined in the header file
   pcbGraphType->AddString(_T("Moment"));
   pcbGraphType->AddString(_T("Shear"));
   pcbGraphType->AddString(_T("Displacement"));
   pcbGraphType->AddString(_T("Stress"));

   if ( curSel == CB_ERR || pcbGraphType->GetCount() <= curSel)
      pcbGraphType->SetCurSel(0);
   else
      pcbGraphType->SetCurSel(curSel);
}

void CAnalysisResultsGraphController::FillLoadCaseList()
{
   CAnalysisResultsGraphBuilder* pGraphBuilder = (CAnalysisResultsGraphBuilder*)GetGraphBuilder();

   CListBox* plbLoadCases = (CListBox*)GetDlgItem(IDC_LOAD_CASE);

   // capture the current selection
   int selCount = plbLoadCases->GetSelCount();
   CArray<int,int> selItemIndices;
   selItemIndices.SetSize(selCount);
   plbLoadCases->GetSelItems(selCount,selItemIndices.GetData());

   CStringArray selItems;
   int i;
   for ( i = 0; i < selCount; i++ )
   {
      CString strItem;
      plbLoadCases->GetText(selItemIndices[i],strItem);
      selItems.Add( strItem );
   }

   // refill control
   plbLoadCases->ResetContent();
   
   std::vector<std::pair<CString,IDType>> lcNames( pGraphBuilder->GetLoadCaseNames(GetInterval(),GetActionType()) );

   std::vector< std::pair<CString,IDType> >::iterator iter(lcNames.begin());
   std::vector< std::pair<CString,IDType> >::iterator end(lcNames.end());
   for ( ; iter != end; iter++ )
   {
      CString& lcName( (*iter).first );
      IDType  lcID   = (*iter).second;
      
      int idx = plbLoadCases->AddString(lcName);
      plbLoadCases->SetItemData(idx,(DWORD_PTR)lcID);
   }

   // reselect anything that was previously selected
   for ( i = 0; i < selCount; i++ )
   {
      CString strItem = selItems[i];
      int idx = plbLoadCases->FindStringExact(-1,strItem);
      if ( idx != LB_ERR )
         plbLoadCases->SetSel(idx);
   }
}

void CAnalysisResultsGraphController::UpdateAnalysisType()
{
   GET_IFACE(IDocumentType,pDocType);
   if ( pDocType->IsPGSpliceDocument() )
   {
      GetDlgItem(IDC_ANALYSISTYPE)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SIMPLE)->ShowWindow(SW_HIDE);  // Simple Span
      GetDlgItem(IDC_SIMPLE2)->ShowWindow(SW_HIDE); // Simple Spans made Continuous
      GetDlgItem(IDC_SIMPLE3)->ShowWindow(SW_HIDE); // Envelope
      m_AnalysisType = pgsTypes::Continuous;
   }
   else
   {
      GET_IFACE(ISpecification,pSpec);
      m_AnalysisType = pSpec->GetAnalysisType();

      int idx;
      switch( m_AnalysisType )
      {
      case pgsTypes::Simple:
         idx = IDC_SIMPLE;
         GetDlgItem(IDC_SIMPLE)->EnableWindow(TRUE);
         GetDlgItem(IDC_SIMPLE2)->EnableWindow(FALSE);
         GetDlgItem(IDC_SIMPLE3)->EnableWindow(FALSE);
         break;

      case pgsTypes::Continuous:
         idx = IDC_SIMPLE2;
         GetDlgItem(IDC_SIMPLE)->EnableWindow(FALSE);
         GetDlgItem(IDC_SIMPLE2)->EnableWindow(TRUE);
         GetDlgItem(IDC_SIMPLE3)->EnableWindow(FALSE);
         break;

      case pgsTypes::Envelope:
         idx = IDC_SIMPLE3;
         GetDlgItem(IDC_SIMPLE)->EnableWindow(TRUE);
         GetDlgItem(IDC_SIMPLE2)->EnableWindow(TRUE);
         GetDlgItem(IDC_SIMPLE3)->EnableWindow(TRUE);
         break;

      default:
         ATLASSERT(false); // is there a new analysis type?
      }

      CheckRadioButton(IDC_SIMPLE,IDC_SIMPLE3,idx );
   }
}

#ifdef _DEBUG
void CAnalysisResultsGraphController::AssertValid() const
{
	CGirderGraphControllerBase::AssertValid();
}

void CAnalysisResultsGraphController::Dump(CDumpContext& dc) const
{
	CGirderGraphControllerBase::Dump(dc);
}
#endif //_DEBUG
