///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

#include <EAF\EAFUtilities.h>
#include <IFace\Project.h>
#include <IFace\PointOfInterest.h>
#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\Selection.h>

#include <PgsExt\BridgeDescription2.h>

#include "LocationGraphController.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CLocationGraphController,CEAFGraphControlWindow)

CLocationGraphController::CLocationGraphController() :
m_GirderKey(0,0)
{
   m_bAlwaysSelect = TRUE;
}

BEGIN_MESSAGE_MAP(CLocationGraphController, CEAFGraphControlWindow)
	//{{AFX_MSG_MAP(CLocationGraphController)
   ON_CBN_SELCHANGE( IDC_GROUP, OnGroupChanged )
   ON_CBN_SELCHANGE( IDC_GIRDER, OnGirderChanged )
   ON_CBN_SELCHANGE( IDC_POI, OnLocationChanged )
   ON_BN_CLICKED(IDC_TIME_LOG,OnXAxis)
   ON_BN_CLICKED(IDC_TIME_LINEAR,OnXAxis)
   ON_BN_CLICKED(IDC_INTERVALS,OnXAxis)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CLocationGraphController::AlwaysSelect(BOOL bAlwaysSelect)
{
   m_bAlwaysSelect = bAlwaysSelect;
}

BOOL CLocationGraphController::AlwaysSelect() const
{
   return m_bAlwaysSelect;
}

BOOL CLocationGraphController::OnInitDialog()
{
   CEAFGraphControlWindow::OnInitDialog();

   EAFGetBroker(&m_pBroker);

   // Set initial value based on the current selection
   GET_IFACE(ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();

   if ( selection.Type == CSelection::Girder || selection.Type == CSelection::Segment )
   {
      m_GirderKey.groupIndex = selection.GroupIdx;
      m_GirderKey.girderIndex = selection.GirderIdx;
   }

   CheckRadioButton(IDC_TIME_LINEAR,IDC_INTERVALS,IDC_TIME_LOG);
   FillGroupCtrl(true);
   FillGirderCtrl(true);
   FillLocationCtrl();
   return TRUE;
}

void CLocationGraphController::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   CEAFGraphControlWindow::OnUpdate(pSender,lHint,pHint);
   FillGroupCtrl();
   FillGirderCtrl();
   FillLocationCtrl();
}

const CGirderKey& CLocationGraphController::GetGirderKey() const
{
   return m_GirderKey;
}

void CLocationGraphController::SelectGirder(const CGirderKey& girderKey)
{
   if (!m_GirderKey.IsEqual(girderKey))
   {
      m_GirderKey = girderKey;
      CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
      pcbGroup->SetCurSel((int)m_GirderKey.groupIndex);

      FillGirderCtrl();
      FillLocationCtrl();

      UpdateGraph();
   }
}

const pgsPointOfInterest& CLocationGraphController::GetLocation() const
{
   return m_Poi;
}

void CLocationGraphController::SelectLocation(const pgsPointOfInterest& poi)
{
   IDType newID = poi.GetID();
   ATLASSERT(newID != INVALID_ID);

   const CSegmentKey& currSegmentKey = m_Poi.GetSegmentKey();
   const CSegmentKey& newSegmentKey = poi.GetSegmentKey();
   if (currSegmentKey.groupIndex != newSegmentKey.groupIndex || currSegmentKey.girderIndex != newSegmentKey.girderIndex)
   {
      // the group or the girder is changing... update the combo boxes
      // so they match the poi
      m_GirderKey = newSegmentKey;
      FillGroupCtrl(true);
      FillGirderCtrl(true);
      FillLocationCtrl();
   }

   CComboBox* pcbLocation = (CComboBox*)GetDlgItem(IDC_POI);
   int count = pcbLocation->GetCount();
   for (int idx = 0; idx < count; idx++)
   {
      IDType id = (IDType)pcbLocation->GetItemData(idx);
      if (id == newID)
      {
         m_Poi = poi;
         pcbLocation->SetCurSel(idx);

         UpdateGraph();
         break;
      }
   }
}

void CLocationGraphController::OnGroupChanged()
{
   CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
   int curSel = pcbGroup->GetCurSel();
   GroupIndexType grpIdx = (GroupIndexType)(pcbGroup->GetItemData(curSel));
   if ( grpIdx != m_GirderKey.groupIndex )
   {
      m_GirderKey.groupIndex = grpIdx;
      FillGirderCtrl();
      FillLocationCtrl();
      UpdateGraph();
   }
}

void CLocationGraphController::OnGirderChanged()
{
   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   int curSel = pcbGirder->GetCurSel();
   GirderIndexType gdrIdx = (GirderIndexType)(pcbGirder->GetItemData(curSel));
   if ( gdrIdx != m_GirderKey.girderIndex )
   {
      m_GirderKey.girderIndex = gdrIdx;
      FillLocationCtrl();
      UpdateGraph();
   }
}

void CLocationGraphController::OnLocationChanged()
{
   // Get the new poi
   CComboBox* pcbLocation = (CComboBox*)GetDlgItem(IDC_POI);
   int curSel = pcbLocation->GetCurSel();
   PoiIDType poiID = (PoiIDType)(pcbLocation->GetItemData(curSel));
   if ( poiID != m_Poi.GetID() )
   {
      GET_IFACE(IPointOfInterest,pPOI);
      m_Poi = pPOI->GetPointOfInterest(poiID);

      // Update the graph
      UpdateGraph();
   }
}

void CLocationGraphController::OnXAxis()
{
   UpdateGraph();
}

void CLocationGraphController::FillGroupCtrl(bool bInit)
{
   CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
   int curSel = pcbGroup->GetCurSel();
   pcbGroup->ResetContent();

   if ( bInit )
   {
      curSel = (int)m_GirderKey.groupIndex;
   }

   GET_IFACE(IDocumentType,pDocType);
   CString strGroupLabel(pDocType->IsPGSuperDocument() ? _T("Span") : _T("Group"));

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GroupIndexType nGroups = pIBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CString strItem;
      strItem.Format(_T("%s %d"),strGroupLabel,LABEL_GROUP(grpIdx));
      int idx = pcbGroup->AddString(strItem);
      pcbGroup->SetItemData(idx,(DWORD_PTR)grpIdx);
   }

   if ( curSel == CB_ERR )
   {
      curSel = 0;
      m_GirderKey.groupIndex = 0;
   }
   else if ( nGroups < m_GirderKey.groupIndex )
   {
      curSel = pcbGroup->GetCount()-1;
      m_GirderKey.groupIndex = nGroups-1;
   }

   pcbGroup->SetCurSel(curSel);
}

void CLocationGraphController::FillGirderCtrl(bool bInit)
{
   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   int curSel = pcbGirder->GetCurSel();
   pcbGirder->ResetContent();

   if ( bInit )
   {
      curSel = (int)m_GirderKey.girderIndex;
   }
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GirderIndexType nGirders = pIBridgeDesc->GetGirderGroup(m_GirderKey.groupIndex)->GetGirderCount();
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      CString strItem;
      strItem.Format(_T("Girder %s"),LABEL_GIRDER(gdrIdx));
      int idx = pcbGirder->AddString(strItem);
      pcbGirder->SetItemData(idx,(DWORD_PTR)gdrIdx);
   }

   if ( curSel == CB_ERR )
   {
      curSel = 0;
      m_GirderKey.girderIndex = 0;
   }
   else if ( nGirders-1 < m_GirderKey.girderIndex )
   {
      curSel = (int)(nGirders-1);
      m_GirderKey.girderIndex = nGirders-1;
   }

   pcbGirder->SetCurSel(curSel);
}

void CLocationGraphController::FillLocationCtrl()
{
   CComboBox* pcbLocation = (CComboBox*)GetDlgItem(IDC_POI);
   int curSel = pcbLocation->GetCurSel();
   pcbLocation->ResetContent();

   GET_IFACE(IBridge,pBridge);
   SpanIndexType startSpanIdx, endSpanIdx;
   pBridge->GetGirderGroupSpans(m_GirderKey.groupIndex,&startSpanIdx,&endSpanIdx);

   
   GET_IFACE(IPointOfInterest,pPoi);
   PoiList vPoi;
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      PoiList vSpanPoi;
      pPoi->GetPointsOfInterest(CSpanKey(spanIdx, m_GirderKey.girderIndex), &vSpanPoi);
      vPoi.insert(vPoi.end(),vSpanPoi.begin(),vSpanPoi.end());
   }

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   for (const pgsPointOfInterest& poi : vPoi)
   {
      CSpanKey spanKey;
      Float64 Xspan;
      pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

      CString strItem;
      std::_tstring strAttributes = poi.GetAttributes(POI_SPAN,false);
      if ( strAttributes.size() == 0 )
      {
         strItem.Format(_T("Span %d, %s"),
            LABEL_SPAN(spanKey.spanIndex),
            FormatDimension(Xspan,pDisplayUnits->GetSpanLengthUnit())
            );
      }
      else
      {
         strItem.Format(_T("Span %d, %s (%s)"),
            LABEL_SPAN(spanKey.spanIndex),
            FormatDimension(Xspan,pDisplayUnits->GetSpanLengthUnit()),
            poi.GetAttributes(POI_SPAN,false).c_str()
            );
      }

      int idx = pcbLocation->AddString(strItem);
      pcbLocation->SetItemData(idx,poi.GetID());
   }


   if ( curSel == CB_ERR || pcbLocation->GetCount() <= curSel )
   {
      if ( m_bAlwaysSelect )
      {
         pcbLocation->SetCurSel(0);
         m_Poi = vPoi.front();
      }
      else
      {
         m_Poi = pgsPointOfInterest();
      }
   }
   else
   {
      pcbLocation->SetCurSel(curSel);
      m_Poi = vPoi[curSel];
   }
}

void CLocationGraphController::SetXAxisType(int type)
{
   int nIDC = IDC_TIME_LOG;
   switch(type)
   {
   case X_AXIS_TIME_LINEAR: nIDC = IDC_TIME_LINEAR; break;
   case X_AXIS_TIME_LOG: nIDC = IDC_TIME_LOG; break;
   case X_AXIS_INTERVAL: nIDC = IDC_INTERVALS; break;
   default:
      ATLASSERT(false);
      nIDC = IDC_TIME_LOG;
   }

   CheckRadioButton(IDC_TIME_LINEAR, IDC_INTERVALS, nIDC);
   UpdateGraph();
}

int CLocationGraphController::GetXAxisType() const
{
   int nIDC = GetCheckedRadioButton(IDC_TIME_LINEAR,IDC_INTERVALS);
   ATLASSERT(nIDC != 0); // 0 means nothing is selected
   int axisType;
   switch(nIDC)
   {
   case IDC_TIME_LINEAR:
      axisType = X_AXIS_TIME_LINEAR;
      break;

   case IDC_TIME_LOG:
      axisType = X_AXIS_TIME_LOG;
      break;

   case IDC_INTERVALS:
      axisType = X_AXIS_INTERVAL;
      break;

   default:
      ATLASSERT(false);
      axisType = X_AXIS_TIME_LOG;
   }
   return axisType;
}

#ifdef _DEBUG
void CLocationGraphController::AssertValid() const
{
	CEAFGraphControlWindow::AssertValid();
}

void CLocationGraphController::Dump(CDumpContext& dc) const
{
	CEAFGraphControlWindow::Dump(dc);
}
#endif //_DEBUG
