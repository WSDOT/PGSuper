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
#include "StressHistoryGraphController.h"
#include <Graphing\StressHistoryGraphBuilder.h>

#include <EAF\EAFUtilities.h>
#include <IFace\DocumentType.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Selection.h>
#include <IFace\PointOfInterest.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>

#include <EAF\EAFGraphBuilderBase.h>
#include <EAF\EAFGraphView.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderPointOfInterest.h>

#include <PGSuperUnits.h>

IMPLEMENT_DYNCREATE(CStressHistoryGraphController,CEAFGraphControlWindow)

CStressHistoryGraphController::CStressHistoryGraphController() :
m_GirderKey(0,0)
{
}

BEGIN_MESSAGE_MAP(CStressHistoryGraphController, CEAFGraphControlWindow)
	//{{AFX_MSG_MAP(CStressHistoryGraphController)
   ON_CBN_SELCHANGE( IDC_GROUP, OnGroupChanged )
   ON_CBN_SELCHANGE( IDC_GIRDER, OnGirderChanged )
   ON_CBN_SELCHANGE( IDC_POI, OnLocationChanged )
   ON_BN_CLICKED(IDC_TIME_LOG,OnXAxis)
   ON_BN_CLICKED(IDC_TIME_LINEAR,OnXAxis)
   ON_BN_CLICKED(IDC_INTERVALS,OnXAxis)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CStressHistoryGraphController::OnInitDialog()
{
   CEAFGraphControlWindow::OnInitDialog();

   EAFGetBroker(&m_pBroker);

   FillGroupCtrl();
   FillGirderCtrl();
   FillLocationCtrl();

   CheckRadioButton(IDC_TIME,IDC_INTERVALS,IDC_TIME);

   return TRUE;
}

CGirderKey CStressHistoryGraphController::GetGirderKey()
{
   return m_GirderKey;
}

pgsPointOfInterest CStressHistoryGraphController::GetLocation()
{
   return m_Poi;
}

void CStressHistoryGraphController::OnGroupChanged()
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

void CStressHistoryGraphController::OnGirderChanged()
{
   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   int curSel = pcbGirder->GetCurSel();
   GirderIndexType gdrIdx = (GirderIndexType)(pcbGirder->GetItemData(curSel));
   if ( gdrIdx != m_GirderKey.girderIndex )
   {
#pragma Reminder("UPDATE: need to worry about the case when the girder index is greater than the number of girders in the current group")
      m_GirderKey.girderIndex = gdrIdx;
      FillLocationCtrl();
      UpdateGraph();
   }
}

void CStressHistoryGraphController::OnLocationChanged()
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

void CStressHistoryGraphController::OnXAxis()
{
   UpdateGraph();
}

void CStressHistoryGraphController::FillGroupCtrl()
{
   CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
   int curSel = pcbGroup->GetCurSel();

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

void CStressHistoryGraphController::FillGirderCtrl()
{
   CComboBox* pcbGirder = (CComboBox*)GetDlgItem(IDC_GIRDER);
   int curSel = pcbGirder->GetCurSel();

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

void CStressHistoryGraphController::FillLocationCtrl()
{
   CComboBox* pcbLocation = (CComboBox*)GetDlgItem(IDC_POI);
   int curSel = pcbLocation->GetCurSel();
   
   GET_IFACE(IPointOfInterest,pPoi);
   std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(CSegmentKey(m_GirderKey,ALL_SEGMENTS)));

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest& poi(*iter);
      
      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);

      CString strItem;
      std::_tstring strAttributes = poi.GetAttributes(POI_ERECTED_SEGMENT,false);
      if ( strAttributes.size() == 0 )
      {
         strItem.Format(_T("Segment %d, %s (X=%s)"),
            LABEL_SEGMENT(poi.GetSegmentKey().segmentIndex),
            FormatDimension(poi.GetDistFromStart(),pDisplayUnits->GetSpanLengthUnit()),
            FormatDimension(Xg,pDisplayUnits->GetSpanLengthUnit())
            );
      }
      else
      {
         strItem.Format(_T("Segment %d, %s (%s) (X=%s)"),
            LABEL_SEGMENT(poi.GetSegmentKey().segmentIndex),
            FormatDimension(poi.GetDistFromStart(),pDisplayUnits->GetSpanLengthUnit()),
            poi.GetAttributes(POI_ERECTED_SEGMENT,false).c_str(),
            FormatDimension(Xg,pDisplayUnits->GetSpanLengthUnit())
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
   else if (m_Poi.GetSegmentKey().groupIndex != m_GirderKey.groupIndex || m_Poi.GetSegmentKey().girderIndex != m_GirderKey.girderIndex)
   {
      m_Poi = vPoi[curSel];
   }
}

void CStressHistoryGraphController::UpdateGraph()
{
   ((CStressHistoryGraphBuilder*)GetGraphBuilder())->InvalidateGraph();
   ((CStressHistoryGraphBuilder*)GetGraphBuilder())->Update();
}

int CStressHistoryGraphController::GetXAxisType()
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
void CStressHistoryGraphController::AssertValid() const
{
	CEAFGraphControlWindow::AssertValid();
}

void CStressHistoryGraphController::Dump(CDumpContext& dc) const
{
	CEAFGraphControlWindow::Dump(dc);
}
#endif //_DEBUG
