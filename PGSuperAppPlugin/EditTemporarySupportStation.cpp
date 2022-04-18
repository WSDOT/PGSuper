///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include "PGSpliceDoc.h"
#include "EditTemporarySupportStation.h"
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditTemporarySupportStation::txnEditTemporarySupportStation(SupportIndexType tsIdx,Float64 oldStation,Float64 newStation)
{
   m_TsIdx = tsIdx;
   m_Station[0] = oldStation;
   m_Station[1] = newStation;
}

txnEditTemporarySupportStation::~txnEditTemporarySupportStation()
{
}

bool txnEditTemporarySupportStation::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditTemporarySupportStation::Undo()
{
   DoExecute(0);
}

txnTransaction* txnEditTemporarySupportStation::CreateClone() const
{
   return new txnEditTemporarySupportStation(m_TsIdx,m_Station[0],m_Station[1]);
}

std::_tstring txnEditTemporarySupportStation::Name() const
{
   std::_tostringstream os;
   os << "Edit Station of Temporary Support " << LABEL_TEMPORARY_SUPPORT(m_TsIdx);
   return os.str();
}

bool txnEditTemporarySupportStation::IsUndoable()
{
   return true;
}

bool txnEditTemporarySupportStation::IsRepeatable()
{
   return false;
}

void txnEditTemporarySupportStation::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);
   m_TsIdx = pBridgeDesc->MoveTemporarySupport(m_TsIdx,m_Station[i]);
}
