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
#include "StressHistoryGraphController.h"
#include <Graphing\StressHistoryGraphBuilder.h>

#include <EAF\EAFUtilities.h>
#include <IFace\DocumentType.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Selection.h>
#include <IFace\PointOfInterest.h>
#include <EAF\EAFDisplayUnits.h>

#include <EAF\EAFGraphBuilderBase.h>
#include <EAF\EAFGraphView.h>

#include <PgsExt\BridgeDescription2.h>

#include <PGSuperUnits.h>

IMPLEMENT_DYNCREATE(CStressHistoryGraphController,CEAFGraphControlWindow)

CStressHistoryGraphController::CStressHistoryGraphController()
{
}

BEGIN_MESSAGE_MAP(CStressHistoryGraphController, CEAFGraphControlWindow)
	//{{AFX_MSG_MAP(CStressHistoryGraphController)
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

   FillLocationCtrl();

   CheckRadioButton(IDC_TIME,IDC_INTERVALS,IDC_TIME);

   return TRUE;
}

pgsPointOfInterest CStressHistoryGraphController::GetLocation()
{
   return m_Poi;
}

void CStressHistoryGraphController::SetIntervalText(const CString& strText)
{
   CWnd* pWnd = GetDlgItem(IDC_INTERVAL_LIST);
   pWnd->SetWindowText(strText);
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

void CStressHistoryGraphController::FillLocationCtrl()
{
   CComboBox* pcbLocation = (CComboBox*)GetDlgItem(IDC_POI);
   int curSel = pcbLocation->GetCurSel();
   
   GET_IFACE(IPointOfInterest,pPoi);
   std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(CSegmentKey(0,0,ALL_SEGMENTS)));

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest& poi(*iter);
      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);
      CString strItem;
      strItem.Format(_T("%s"),FormatDimension(Xg,pDisplayUnits->GetSpanLengthUnit()));

      int idx = pcbLocation->AddString(strItem);
      pcbLocation->SetItemData(idx,poi.GetID());
   }


   if ( curSel == CB_ERR || pcbLocation->GetCount() <= curSel )
   {
      pcbLocation->SetCurSel(0);
      m_Poi = vPoi.front();
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
