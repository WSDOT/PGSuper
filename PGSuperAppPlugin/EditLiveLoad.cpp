///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include "EditLiveLoad.h"
#include "PGSuperDoc.h"

#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditLiveLoad::txnEditLiveLoad(const txnEditLiveLoadData& oldDesign,const txnEditLiveLoadData& newDesign,
                                 const txnEditLiveLoadData& oldFatigue,const txnEditLiveLoadData& newFatigue,
                                 const txnEditLiveLoadData& oldPermit,const txnEditLiveLoadData& newPermit,
                                 EventIndexType oldEventIdx,EventIndexType newEventIdx)
{
   m_Design[0]   = oldDesign;
   m_Design[1]   = newDesign;
   m_Fatigue[0]  = oldFatigue;
   m_Fatigue[1]  = newFatigue;
   m_Permit[0]   = oldPermit;
   m_Permit[1]   = newPermit;
   m_EventIndex[0] = oldEventIdx;
   m_EventIndex[1] = newEventIdx;
}

txnEditLiveLoad::~txnEditLiveLoad()
{
}

bool txnEditLiveLoad::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditLiveLoad::Undo()
{
   DoExecute(0);
}

std::unique_ptr<CEAFTransaction> txnEditLiveLoad::CreateClone() const
{
   return std::make_unique<txnEditLiveLoad>(m_Design[0],m_Design[1],m_Fatigue[0],m_Fatigue[1],m_Permit[0],m_Permit[1],m_EventIndex[0],m_EventIndex[1]);
}

std::_tstring txnEditLiveLoad::Name() const
{
   return _T("Edit Live Load");
}

bool txnEditLiveLoad::IsUndoable() const
{
   return true;
}

bool txnEditLiveLoad::IsRepeatable() const
{
   return false;
}

void txnEditLiveLoad::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2( pBroker, ILiveLoads, pLiveLoad );
   GET_IFACE2( pBroker, IEvents,    pEvents   );
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   pLiveLoad->SetLiveLoadNames(            pgsTypes::lltDesign,m_Design[i].m_VehicleNames);
   pLiveLoad->SetTruckImpact(              pgsTypes::lltDesign,m_Design[i].m_TruckImpact);
   pLiveLoad->SetLaneImpact(               pgsTypes::lltDesign,m_Design[i].m_LaneImpact);
   pLiveLoad->SetPedestrianLoadApplication(pgsTypes::lltDesign,m_Design[i].m_PedestrianLoadApplicationType);

   pLiveLoad->SetLiveLoadNames(            pgsTypes::lltFatigue,m_Fatigue[i].m_VehicleNames);
   pLiveLoad->SetTruckImpact(              pgsTypes::lltFatigue,m_Fatigue[i].m_TruckImpact);
   pLiveLoad->SetLaneImpact(               pgsTypes::lltFatigue,m_Fatigue[i].m_LaneImpact);
   pLiveLoad->SetPedestrianLoadApplication(pgsTypes::lltFatigue,m_Fatigue[i].m_PedestrianLoadApplicationType);

   pLiveLoad->SetLiveLoadNames(            pgsTypes::lltPermit,m_Permit[i].m_VehicleNames);
   pLiveLoad->SetTruckImpact(              pgsTypes::lltPermit,m_Permit[i].m_TruckImpact);
   pLiveLoad->SetLaneImpact(               pgsTypes::lltPermit,m_Permit[i].m_LaneImpact);
   pLiveLoad->SetPedestrianLoadApplication(pgsTypes::lltPermit,m_Permit[i].m_PedestrianLoadApplicationType);

   GET_IFACE2( pBroker, IBridgeDescription, pIBridgeDesc );
   pIBridgeDesc->SetLiveLoadEventByIndex(m_EventIndex[i]);
}
