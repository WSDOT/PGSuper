///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include "EditBridge.h"
#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditBridge::txnEditBridge(const CBridgeDescription& oldBridgeDesc,const CBridgeDescription& newBridgeDesc,
                             enumExposureCondition oldExposureCondition, enumExposureCondition newExposureCondition,
                             Float64 oldRelHumidity, Float64 newRelHumidity)
{
   m_pBridgeDesc[0] = new CBridgeDescription(oldBridgeDesc);
   m_pBridgeDesc[1] = new CBridgeDescription(newBridgeDesc);

   m_ExposureCondition[0] = oldExposureCondition;
   m_ExposureCondition[1] = newExposureCondition;

   m_RelHumidity[0] = oldRelHumidity;
   m_RelHumidity[1] = newRelHumidity;
}

txnEditBridge::~txnEditBridge()
{
   delete m_pBridgeDesc[0];
   delete m_pBridgeDesc[1];
}

bool txnEditBridge::Execute()
{
   Execute(1);
   return true;
}

void txnEditBridge::Undo()
{
   Execute(0);
}

void txnEditBridge::Execute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);
   GET_IFACE2(pBroker,IEnvironment, pEnvironment );
   GET_IFACE2(pBroker,IEvents, pEvents);

   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   pEnvironment->SetExposureCondition( m_ExposureCondition[i] );
   pEnvironment->SetRelHumidity( m_RelHumidity[i] );

   // Reconcile edit before commiting - not on undo
   if (i==1)
   {
      m_pBridgeDesc[1]->ReconcileEdits( pBroker, m_pBridgeDesc[0] );
   }

   pBridgeDesc->SetBridgeDescription( *m_pBridgeDesc[i] );

   pEvents->FirePendingEvents();

#if defined _DEBUG
   ATLASSERT( *pBridgeDesc->GetBridgeDescription() == *m_pBridgeDesc[i] );
#endif // _DEBUG
}

txnTransaction* txnEditBridge::CreateClone() const
{
   return new txnEditBridge(*m_pBridgeDesc[0],       *m_pBridgeDesc[1],
                            m_ExposureCondition[0], m_ExposureCondition[1],
                            m_RelHumidity[0],       m_RelHumidity[1]);
}

std::_tstring txnEditBridge::Name() const
{
   return _T("Edit Bridge");
}

bool txnEditBridge::IsUndoable()
{
   return true;
}

bool txnEditBridge::IsRepeatable()
{
   return false;
}
