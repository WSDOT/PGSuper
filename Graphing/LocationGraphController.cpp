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

#include <EAF\EAFUtilities.h>
#include <IFace\Project.h>
#include <IFace\PointOfInterest.h>
#include <PgsExt\BridgeDescription2.h>

#include "LocationGraphController.h"

IMPLEMENT_DYNAMIC(CLocationGraphController,CEAFGraphControlWindow)

CLocationGraphController::CLocationGraphController() :
m_GirderKey(0,0)
{
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

BOOL CLocationGraphController::OnInitDialog()
{
   CEAFGraphControlWindow::OnInitDialog();

   EAFGetBroker(&m_pBroker);

   CheckRadioButton(IDC_TIME_LINEAR,IDC_INTERVALS,IDC_TIME_LOG);

   return TRUE;
}

void CLocationGraphController::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   FillGroupCtrl();
   FillGirderCtrl();
   FillLocationCtrl();
}

CGirderKey CLocationGraphController::GetGirderKey()
{
   return m_GirderKey;
}

pgsPointOfInterest CLocationGraphController::GetLocation()
{
   return m_Poi;
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

void CLocationGraphController::FillGroupCtrl()
{
   CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
   int curSel = pcbGroup->GetCurSel();
   pcbGroup->ResetContent();

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GroupIndexType nGroups = pIBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CString strItem;
      strItem.Format(_T("Group %d"),LABEL_GROUP(grpIdx));
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

void CLocationGraphController::FillGirderCtrl()
{
   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   int curSel = pcbGirder->GetCurSel();
   pcbGirder->ResetContent();

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
   else if ( nGirders < m_GirderKey.girderIndex )
   {
      curSel = pcbGirder->GetCount()-1;
      m_GirderKey.girderIndex = nGirders-1;
   }

   pcbGirder->SetCurSel(curSel);
}

void CLocationGraphController::FillLocationCtrl()
{
   CComboBox* pcbLocation = (CComboBox*)GetDlgItem(IDC_POI);
   int curSel = pcbLocation->GetCurSel();
   pcbLocation->ResetContent();
   
   GET_IFACE(IPointOfInterest,pPoi);
   std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(CSpanKey(ALL_SPANS,m_GirderKey.girderIndex)));

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest& poi(*iter);

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
      pcbLocation->SetCurSel(0);
      m_Poi = vPoi.front();
   }
   else
   {
      pcbLocation->SetCurSel(curSel);
      m_Poi = vPoi[curSel];
   }
}

int CLocationGraphController::GetXAxisType()
{
   int id = GetCheckedRadioButton(IDC_TIME_LINEAR,IDC_INTERVALS);
   int axisType;
   switch(id)
   {
   case IDC_TIME_LINEAR:
      axisType = X_AXIS_TIME_LINEAR;
      break;

   case IDC_TIME_LOG:
      axisType = X_AXIS_TIME_LOG;
      break;

   case IDC_INTERVALS:
      axisType = X_AXIS_INTEGER;
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
