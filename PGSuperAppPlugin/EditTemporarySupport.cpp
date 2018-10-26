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
#include "EditTemporarySupport.h"
#include "PGSpliceDoc.h"

#include <PgsExt\ClosurePourData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


txnEditTemporarySupport::txnEditTemporarySupport(SupportIDType tsID,const txnEditTemporarySupportData& oldData,const txnEditTemporarySupportData& newData)
{
   m_tsID = tsID;
   m_TSData[0] = oldData;
   m_TSData[1] = newData;
}

txnEditTemporarySupport::~txnEditTemporarySupport()
{
}

bool txnEditTemporarySupport::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditTemporarySupport::Undo()
{
   DoExecute(0);
}

txnTransaction* txnEditTemporarySupport::CreateClone() const
{
   return new txnEditTemporarySupport(m_tsID,m_TSData[0],m_TSData[1]);
}

std::_tstring txnEditTemporarySupport::Name() const
{
   std::_tostringstream os;
   os << "Edit Temporary Support " << LABEL_TEMPORARY_SUPPORT(m_TSData[0].m_TS.GetIndex());
   return os.str();
}

bool txnEditTemporarySupport::IsUndoable()
{
   return true;
}

bool txnEditTemporarySupport::IsRepeatable()
{
   return false;
}

void txnEditTemporarySupport::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IEvents, pEvents);
   pEvents->HoldEvents();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   pIBridgeDesc->SetTemporarySupportByID(m_tsID,m_TSData[i].m_TS);
   pIBridgeDesc->SetTempSupportEventsByID(m_tsID,m_TSData[i].m_ErectionEvent,m_TSData[i].m_RemovalEvent);

   pIBridgeDesc->SetGirderSpacingType(m_TSData[i].m_GirderSpacingType);
   pIBridgeDesc->SetMeasurementLocation(m_TSData[i].m_GirderMeasurementLocation);

   if ( m_TSData[i].m_TS.GetConnectionType() == pgsTypes::sctClosurePour )
   {
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      const CTemporarySupportData* pTS = pBridgeDesc->FindTemporarySupport(m_tsID);
      const CSpanData2* pSpan = pTS->GetSpan();
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      GroupIndexType grpIdx = pGroup->GetIndex();
#pragma Reminder("REVIEW: possible bug")
      // using girder index 0 to get the closure pour. I think this is ok because all closures
      // at a TS or Pier are cast at the same time.
      const CClosurePourData* pClosure = pTS->GetClosurePour(0);
      ATLASSERT(pClosure);
      CollectionIndexType closureIdx = pClosure->GetIndex();
      pIBridgeDesc->SetCastClosurePourEventByIndex(grpIdx,closureIdx,m_TSData[i].m_ClosureEvent);
   }

   pEvents->FirePendingEvents();
}
