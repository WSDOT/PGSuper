///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include "EditClosureJoint.h"
#include "PGSpliceDoc.h"

#include <IFACE\Project.h>
#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


txnEditClosureJoint::txnEditClosureJoint(const CSegmentKey& closureKey,const txnEditClosureJointData& newData)
{
   m_ClosureKey = closureKey;
   m_NewData = newData;
}

txnEditClosureJoint::~txnEditClosureJoint()
{
}

bool txnEditClosureJoint::Execute()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   m_OldData.clear();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(m_ClosureKey.groupIndex);

   GirderIndexType nGirders    = pGroup->GetGirderCount();
   GirderIndexType firstGdrIdx = (m_ClosureKey.girderIndex == ALL_GIRDERS ? 0 : m_ClosureKey.girderIndex);
   GirderIndexType lastGdrIdx  = (m_ClosureKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGdrIdx);

   for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
   {
      // collect up the old girder data (we will need it for Undo)
      txnEditClosureJointData oldData;
      oldData.m_ClosureKey = m_ClosureKey;
      oldData.m_ClosureKey.girderIndex = gdrIdx;

      oldData.m_ClosureJoint = *pBridgeDesc->GetClosureJoint(oldData.m_ClosureKey);
      oldData.m_TimelineMgr = (*pIBridgeDesc->GetTimelineManager());

      m_OldData.insert(oldData);

      // Copy the new closure joint data onto this closure joint
      SetClosureJointData(oldData.m_ClosureKey,m_NewData);
   }

   return true;
}

void txnEditClosureJoint::Undo()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   std::set<txnEditClosureJointData>::iterator iter(m_OldData.begin());
   std::set<txnEditClosureJointData>::iterator end(m_OldData.end());
   for ( ; iter != end; iter++ )
   {
      const txnEditClosureJointData& oldData = *iter;
      SetClosureJointData(oldData.m_ClosureKey,oldData);
   }
}

std::unique_ptr<CEAFTransaction>txnEditClosureJoint::CreateClone() const
{
   return std::make_unique<txnEditClosureJoint>(m_ClosureKey,m_NewData);
}

std::_tstring txnEditClosureJoint::Name() const
{
   std::_tostringstream os;
   if ( m_NewData.m_PierIdx != INVALID_INDEX )
   {
      os << "Edit Closure Joint for Girder " << LABEL_GIRDER(m_ClosureKey.girderIndex) << " at Pier " << LABEL_PIER(m_NewData.m_PierIdx);
   }
   else
   {
      os << "Edit Closure Joint for Girder " << LABEL_GIRDER(m_ClosureKey.girderIndex) << " at Temporary Support " << LABEL_TEMPORARY_SUPPORT(m_NewData.m_TSIdx);
   }

   return os.str();
}

bool txnEditClosureJoint::IsUndoable() const
{
   return true;
}

bool txnEditClosureJoint::IsRepeatable() const
{
   return false;
}

void txnEditClosureJoint::SetClosureJointData(const CSegmentKey& closureKey,const txnEditClosureJointData& data)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IEvents, pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   pIBridgeDesc->SetClosureJointData(closureKey,data.m_ClosureJoint);
   pIBridgeDesc->SetTimelineManager(data.m_TimelineMgr);
}
