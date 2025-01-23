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

#include "StdAfx.h"
#include "LibraryEntryObserver.h"
#include "ProjectAgent.h"
#include "CLSID.h"
#include "ProjectAgentImp.h"
#include <IFace\StatusCenter.h>
#include <EAF\EAFUIIntegration.h>
#include <algorithm>

#if defined _USE_MULTITHREADING
#include <future>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsLibraryEntryObserver
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsLibraryEntryObserver::pgsLibraryEntryObserver(CProjectAgentImp* pAgent)
{
   m_pAgent = pAgent;
}

pgsLibraryEntryObserver::~pgsLibraryEntryObserver()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsLibraryEntryObserver::Update(ConcreteLibraryEntry& subject, Int32 hint)
{
   // no action required
   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(GirderLibraryEntry& subject, Int32 hint)
{
   m_pAgent->HoldEvents();
   if (hint & WBFL::Library::Hint_EntryRenamed)
   {
      if (m_pAgent->m_BridgeDescription.GetGirderLibraryEntry() == &subject)
      {
         m_pAgent->m_BridgeDescription.RenameGirder(subject.GetName().c_str());
      }

#if defined _USE_MULTITHREADING
      std::vector<std::future<void>> vFutures;
#endif
      GroupIndexType nGroups = m_pAgent->m_BridgeDescription.GetGirderGroupCount();
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         CGirderGroupData* pGirderGroup = m_pAgent->m_BridgeDescription.GetGirderGroup(grpIdx);
         GroupIndexType nGirderTypeGroups = pGirderGroup->GetGirderTypeGroupCount();
         for ( GroupIndexType gdrGroupIdx = 0; gdrGroupIdx < nGirderTypeGroups; gdrGroupIdx++ )
         {
            GirderIndexType firstGdrIdx,lastGdrIdx;
            std::_tstring strGirderName;
            pGirderGroup->GetGirderTypeGroup(gdrGroupIdx,&firstGdrIdx,&lastGdrIdx,&strGirderName);

            if (pGirderGroup->GetGirderLibraryEntry(firstGdrIdx) == &subject )
            {
#if defined _USE_MULTITHREADING
               // must capture by value into the lambda expression... as the loop progresses
               // pGirderGroup and gdrGroupIdx change values. If we capture by reference the values
               // will get changed when they should be constant during each async call.
               std::future<void> f(std::async([=]{pGirderGroup->RenameGirder(gdrGroupIdx, subject.GetName().c_str());}));
               vFutures.push_back(std::move(f));
#else
               pGirderGroup->RenameGirder(gdrGroupIdx, subject.GetName().c_str());
#endif
            }
         }
      }

#if defined _USE_MULTITHREADING
      for (auto& f : vFutures)
      {
         f.wait(); // wait until the threads are done running (use f.get() if there is a return value)
      }
#endif

      m_pAgent->Fire_BridgeChanged(); // if we had a lessor event, we should fire that
                                      // need to fire something so that the bridge view tooltips
                                      // are updated to display the correct name
   }

   if (hint & WBFL::Library::Hint_EntryEdited)
   {
      ClearStatusItems();

      // make sure changed library data is compatible with current project data
      m_pAgent->DealWithGirderLibraryChanges(true);

      m_pAgent->Fire_BridgeChanged();
   }
   m_pAgent->FirePendingEvents();

   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(SpecLibraryEntry& subject, Int32 hint)
{
   m_pAgent->HoldEvents();
   if (hint & WBFL::Library::Hint_EntryRenamed)
   {
      m_pAgent->m_Spec = subject.GetName();
      m_pAgent->SpecificationRenamed();
   }

   if (hint & WBFL::Library::Hint_EntryEdited)
   {
      ClearStatusItems();
      m_pAgent->SpecificationChanged(true);
   }
   m_pAgent->FirePendingEvents();

   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(RatingLibraryEntry& subject, Int32 hint)
{
   m_pAgent->HoldEvents();
   if (hint & WBFL::Library::Hint_EntryRenamed)
      m_pAgent->m_RatingSpec = subject.GetName();

   if (hint & WBFL::Library::Hint_EntryEdited)
   {
      ClearStatusItems();
      m_pAgent->RatingSpecificationChanged(true);
   }
   m_pAgent->FirePendingEvents();

   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(TrafficBarrierEntry& subject, Int32 hint)
{
   m_pAgent->HoldEvents();
   if (hint & WBFL::Library::Hint_EntryRenamed)
   {
      if ( m_pAgent->m_BridgeDescription.GetLeftRailingSystem()->GetExteriorRailing() == &subject )
         m_pAgent->m_BridgeDescription.GetLeftRailingSystem()->strExteriorRailing = subject.GetName();

      if ( m_pAgent->m_BridgeDescription.GetLeftRailingSystem()->GetInteriorRailing() == &subject )
         m_pAgent->m_BridgeDescription.GetLeftRailingSystem()->strInteriorRailing = subject.GetName();

      if ( m_pAgent->m_BridgeDescription.GetRightRailingSystem()->GetExteriorRailing() == &subject )
         m_pAgent->m_BridgeDescription.GetRightRailingSystem()->strExteriorRailing = subject.GetName();

      if ( m_pAgent->m_BridgeDescription.GetRightRailingSystem()->GetInteriorRailing() == &subject )
         m_pAgent->m_BridgeDescription.GetRightRailingSystem()->strInteriorRailing = subject.GetName();
   }

   if (hint & WBFL::Library::Hint_EntryEdited)
   {
      ClearStatusItems();
      m_pAgent->Fire_BridgeChanged();
   }
   m_pAgent->FirePendingEvents();

   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(LiveLoadLibraryEntry& subject, Int32 hint)
{
   m_pAgent->HoldEvents();
   if ( hint & WBFL::Library::Hint_EntryRenamed )
   {
      bool bWasEntryUsed = false;
      std::_tstring strOldName;

      for ( int i = 0; i < (int)pgsTypes::lltLiveLoadTypeCount; i++ )
      {
         pgsTypes::LiveLoadType llType = (pgsTypes::LiveLoadType)i;

         CProjectAgentImp::LiveLoadSelectionIterator begin = m_pAgent->m_SelectedLiveLoads[llType].begin();
         CProjectAgentImp::LiveLoadSelectionIterator end   = m_pAgent->m_SelectedLiveLoads[llType].end();

         CProjectAgentImp::LiveLoadSelection key;
         key.pEntry = &subject;
         CProjectAgentImp::LiveLoadSelectionIterator found = std::find(begin,end,key);

         if ( found != end )
         {
            CProjectAgentImp::LiveLoadSelection ll = *found;

            bWasEntryUsed = true;
            std::_tstring strOldName = ll.EntryName;
            
            ll.EntryName = subject.GetName(); // this changes the value of the key that the collection is sorted on... we have to remove and re-insert to ensure proper sorting
            ll.pEntry = &subject;

            m_pAgent->m_SelectedLiveLoads[llType].erase(found);
            m_pAgent->m_SelectedLiveLoads[llType].insert(ll);

         }
      }

      if (bWasEntryUsed)
      {
         m_pAgent->Fire_LiveLoadNameChanged(strOldName.c_str(), subject.GetName().c_str());
      }
   }

   if ( hint & WBFL::Library::Hint_EntryEdited )
   {
      ClearStatusItems();
      m_pAgent->Fire_LiveLoadChanged();
   }
   m_pAgent->FirePendingEvents();

   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(DuctLibraryEntry& subject,Int32 hint)
{
   m_pAgent->HoldEvents();
   if ( hint & WBFL::Library::Hint_EntryRenamed )
   {
      GroupIndexType nGroups = m_pAgent->m_BridgeDescription.GetGirderGroupCount();
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         CGirderGroupData* pGroup = m_pAgent->m_BridgeDescription.GetGirderGroup(grpIdx);
         GirderIndexType nGirders = pGroup->GetGirderCount();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
            CPTData* pPTData = pGirder->GetPostTensioning();
            DuctIndexType nDucts = pPTData->GetDuctCount();
            for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
            {
               CDuctData* pDuct = pPTData->GetDuct(ductIdx);
               if ( pDuct->pDuctLibEntry == &subject )
               {
                  pDuct->Name = subject.GetName();
               }
            }
         }
      }
   }

   if ( hint & WBFL::Library::Hint_EntryEdited )
   {
      ClearStatusItems();
      m_pAgent->Fire_BridgeChanged();
   }
   m_pAgent->FirePendingEvents();

   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(HaulTruckLibraryEntry& subject,Int32 hint)
{
   m_pAgent->HoldEvents();
   if ( hint & WBFL::Library::Hint_EntryRenamed )
   {
#if defined _USE_MULTITHREADING
      std::vector<std::future<void>> vFutures;
#endif

      GroupIndexType nGroups = m_pAgent->m_BridgeDescription.GetGirderGroupCount();
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         CGirderGroupData* pGroup = m_pAgent->m_BridgeDescription.GetGirderGroup(grpIdx);
#if defined _USE_MULTITHREADING
         // do the renaming for each group/span in its own asyncronous operation
         std::future<void> f(std::async([=] {
            GirderIndexType nGirders = pGroup->GetGirderCount();
            for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
            {
               CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
               SegmentIndexType nSegments = pGirder->GetSegmentCount();
               for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
               {
                  CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
                  if (pSegment->HandlingData.pHaulTruckLibraryEntry == &subject)
                  {
                     pSegment->HandlingData.HaulTruckName = subject.GetName();
                  }
               }
            }
         }));
         vFutures.push_back(std::move(f));
#else
         GirderIndexType nGirders = pGroup->GetGirderCount();
         for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
         {
            CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
            SegmentIndexType nSegments = pGirder->GetSegmentCount();
            for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
            {
               CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
               if (pSegment->HandlingData.pHaulTruckLibraryEntry == pSubject)
               {
                  pSegment->HandlingData.HaulTruckName = subject.GetName();
               }
            }
         }
#endif
      }

#if defined _USE_MULTITHREADING
      for (auto& f : vFutures)
      {
         f.wait(); // wait until the threads are done running (use f.get() if there is a return value)
      }
#endif
   }

   if ( hint & WBFL::Library::Hint_EntryEdited )
   {
      ClearStatusItems();
      m_pAgent->Fire_BridgeChanged();
   }
   m_pAgent->FirePendingEvents();

   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

//======================== ACCESS     =======================================
void pgsLibraryEntryObserver::SetAgent(CProjectAgentImp* pAgent)
{
   m_pAgent = pAgent;
}

void pgsLibraryEntryObserver::ClearStatusItems()
{
   GET_IFACE2(m_pAgent->m_pBroker,IEAFStatusCenter,pStatusCenter);
   pStatusCenter->RemoveByStatusGroupID(m_pAgent->m_StatusGroupID);
}

//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
