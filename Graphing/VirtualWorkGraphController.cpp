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
#include "VirtualWorkGraphController.h"
#include <Graphing\VirtualWorkGraphBuilder.h>

#include <EAF\EAFUtilities.h>
#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\Selection.h>

#include <Hints.h>

#include <EAF\EAFGraphBuilderBase.h>
#include <EAF\EAFGraphView.h>

#include <PgsExt\BridgeDescription2.h>

IMPLEMENT_DYNCREATE(CVirtualWorkGraphController,CIntervalGirderGraphControllerBase)

CVirtualWorkGraphController::CVirtualWorkGraphController():
CIntervalGirderGraphControllerBase(false/*don't use ALL_GROUPS*/)
{
   m_GroupIdx = 0;
}

BEGIN_MESSAGE_MAP(CVirtualWorkGraphController, CIntervalGirderGraphControllerBase)
	//{{AFX_MSG_MAP(CVirtualWorkGraphController)
   ON_CBN_SELCHANGE( IDC_POI, OnLocationChanged )
   ON_CONTROL_RANGE( BN_CLICKED, IDC_FORCE, IDC_MOMENT, OnGraphTypeChanged)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CVirtualWorkGraphController::OnInitDialog()
{
   CIntervalGirderGraphControllerBase::OnInitDialog();

   CheckRadioButton(IDC_FORCE,IDC_MOMENT,IDC_FORCE);
   FillLocationCtrl();

   return TRUE;
}

IndexType CVirtualWorkGraphController::GetGraphCount()
{
   return 1;
}

void CVirtualWorkGraphController::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   if ( lHint == HINT_BRIDGECHANGED )
   {
      // The bridge changed, so reset the controls
      FillGirderCtrl();
   }
}

void CVirtualWorkGraphController::OnGraphTypeChanged(UINT nIDC)
{
   UpdateGraph();
}

CVirtualWorkGraphBuilder::GraphType CVirtualWorkGraphController::GetGraphType()
{
   if ( GetCheckedRadioButton(IDC_FORCE,IDC_MOMENT) == IDC_FORCE )
   {
      return CVirtualWorkGraphBuilder::UnitForce;
   }
   else
   {
      return CVirtualWorkGraphBuilder::UnitMoment;
   }
}

pgsPointOfInterest CVirtualWorkGraphController::GetLocation()
{
   return m_Poi;
}

void CVirtualWorkGraphController::OnLocationChanged()
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

void CVirtualWorkGraphController::FillLocationCtrl()
{
   CComboBox* pcbLocation = (CComboBox*)GetDlgItem(IDC_POI);
   int curSel = pcbLocation->GetCurSel();

   CGirderKey girderKey = GetGirderKey();
   
   GET_IFACE(IPointOfInterest,pPoi);
   std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)));

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
   else if (m_Poi.GetSegmentKey().groupIndex != girderKey.groupIndex || m_Poi.GetSegmentKey().girderIndex != girderKey.girderIndex)
   {
      m_Poi = vPoi[curSel];
   }
}

#ifdef _DEBUG
void CVirtualWorkGraphController::AssertValid() const
{
	CIntervalGirderGraphControllerBase::AssertValid();
}

void CVirtualWorkGraphController::Dump(CDumpContext& dc) const
{
	CIntervalGirderGraphControllerBase::Dump(dc);
}
#endif //_DEBUG
