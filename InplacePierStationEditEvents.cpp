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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "InplacePierStationEditEvents.h"
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <PgsExt\BridgeDescription.h>
#include "MovePierDlg.h"
#include "EditPierStation.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CInplacePierStationEditEvents::CInplacePierStationEditEvents(IBroker* pBroker,PierIndexType pierIdx) :
CInplaceEditDisplayObjectEvents(pBroker), m_PierIdx(pierIdx)
{
}

void CInplacePierStationEditEvents::Handle_OnChanged(iDisplayObject* pDO)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComQIPtr<iEditableUnitValueTextBlock> pTextBlock(pDO);
   ATLASSERT(pTextBlock);

   Float64 old_station = pTextBlock->GetValue();
   Float64 new_station = pTextBlock->GetEditedValue();

   if ( IsEqual(old_station,new_station) )
      return;

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   const unitStationFormat& station_format = pDisplayUnits->GetStationFormat();

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();

   Float64 prevPierStation = 0;
   Float64 nextPierStation = DBL_MAX;

   if ( m_PierIdx != 0 )
   {
      const CPierData* pPrevPier = pBridgeDesc->GetPier(m_PierIdx - 1);
      prevPierStation = pPrevPier->GetStation();
   }

   if ( m_PierIdx != nSpans )
   {
      const CPierData* pNextPier = pBridgeDesc->GetPier(m_PierIdx + 1);
      nextPierStation = pNextPier->GetStation();
   }


   CMovePierDlg dlg(m_PierIdx,old_station,new_station,prevPierStation,nextPierStation,nSpans,station_format);
   if ( dlg.DoModal() == IDOK )
   {
      if ( !IsEqual(old_station,new_station) )
      {
         txnEditPierStation* pTxn = new txnEditPierStation(m_PierIdx,old_station,new_station,dlg.m_Option);
         txnTxnManager::GetInstance()->Execute(pTxn);
      }
   }
}
