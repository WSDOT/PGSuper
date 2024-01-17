///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\EditBridge.h>
#include <EAF\EAFUtilities.h>
#include <PgsExt\GirderLabel.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditBridge::txnEditBridge(const CBridgeDescription2& oldBridgeDesc,const CBridgeDescription2& newBridgeDesc,
   pgsTypes::ExposureCondition oldExposureCondition, pgsTypes::ExposureCondition newExposureCondition,
                             Float64 oldRelHumidity, Float64 newRelHumidity)
{
   m_bBridgeDescOnly = false;
   m_BridgeDesc[0] = oldBridgeDesc;
   m_BridgeDesc[1] = newBridgeDesc;

   m_ExposureCondition[0] = oldExposureCondition;
   m_ExposureCondition[1] = newExposureCondition;

   m_RelHumidity[0] = oldRelHumidity;
   m_RelHumidity[1] = newRelHumidity;
}

txnEditBridge::txnEditBridge(const CBridgeDescription2& oldBridgeDesc,const CBridgeDescription2& newBridgeDesc)
{
   m_bBridgeDescOnly = true;
   m_BridgeDesc[0] = oldBridgeDesc;
   m_BridgeDesc[1] = newBridgeDesc;
}

txnEditBridge::~txnEditBridge()
{
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

   if ( !m_bBridgeDescOnly )
   {
      pEnvironment->SetExposureCondition( m_ExposureCondition[i] );
      pEnvironment->SetRelHumidity( m_RelHumidity[i] );
   }

   pBridgeDesc->SetBridgeDescription( m_BridgeDesc[i] );

   // Set pier labelling. This is also in the BridgeAgent, but we use static members in the pgsPierLabel class for performance
   pgsPierLabel::SetPierLabelSettings(m_BridgeDesc[i].GetDisplayStartSupportType(), m_BridgeDesc[i].GetDisplayEndSupportType(), m_BridgeDesc[i].GetDisplayStartingPierNumber());

   pEvents->FirePendingEvents();
}

std::unique_ptr<CEAFTransaction> txnEditBridge::CreateClone() const
{
   if ( m_bBridgeDescOnly )
   {
      return std::make_unique<txnEditBridge>(m_BridgeDesc[0],m_BridgeDesc[1]);
   }
   else
   {
      return std::make_unique<txnEditBridge>(m_BridgeDesc[0],        m_BridgeDesc[1],
                               m_ExposureCondition[0], m_ExposureCondition[1],
                               m_RelHumidity[0],       m_RelHumidity[1] );
   }
}

std::_tstring txnEditBridge::Name() const
{
   return _T("Edit Bridge");
}

bool txnEditBridge::IsUndoable() const
{
   return true;
}

bool txnEditBridge::IsRepeatable() const
{
   return false;
}
