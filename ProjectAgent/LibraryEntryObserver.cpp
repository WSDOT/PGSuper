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

#include "StdAfx.h"
#include "LibraryEntryObserver.h"
#include "ProjectAgent.h"
#include "ProjectAgent_i.h"
#include "ProjectAgentImp.h"
#include <IFace\StatusCenter.h>
#include <EAF\EAFUIIntegration.h>
#include <algorithm>

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
void pgsLibraryEntryObserver::Update(ConcreteLibraryEntry* pSubject, Int32 hint)
{
   // no action required
   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(GirderLibraryEntry* pSubject, Int32 hint)
{
   m_pAgent->HoldEvents();
   if (hint & LibraryHints::EntryRenamed)
   {
      if ( m_pAgent->m_BridgeDescription.GetGirderLibraryEntry() == pSubject )
         m_pAgent->m_BridgeDescription.RenameGirder( pSubject->GetName().c_str() );

      GroupIndexType nGroups = m_pAgent->m_BridgeDescription.GetGirderGroupCount();
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         CGirderGroupData* pGroup = m_pAgent->m_BridgeDescription.GetGirderGroup(grpIdx);
         GroupIndexType nGirderTypeGroups = pGroup->GetGirderTypeGroupCount();
         for ( GroupIndexType gdrGroupIdx = 0; gdrGroupIdx < nGirderTypeGroups; gdrGroupIdx++ )
         {
            GirderIndexType firstGdrIdx,lastGdrIdx;
            std::_tstring strGirderName;
            pGroup->GetGirderTypeGroup(grpIdx,&firstGdrIdx,&lastGdrIdx,&strGirderName);

            if ( pGroup->GetGirderLibraryEntry(firstGdrIdx) == pSubject )
            {
               pGroup->RenameGirder( grpIdx, pSubject->GetName().c_str() );
            }
         }
      }

      m_pAgent->Fire_BridgeChanged(); // if we had a lessor event, we should fire that
                                      // need to fire something so that the bridge view tooltips
                                      // are updated to display the correct name
   }

   if (hint & LibraryHints::EntryEdited)
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

void pgsLibraryEntryObserver::Update(SpecLibraryEntry* pSubject, Int32 hint)
{
   m_pAgent->HoldEvents();
   if (hint & LibraryHints::EntryRenamed)
      m_pAgent->m_Spec = pSubject->GetName();

   if (hint & LibraryHints::EntryEdited)
   {
      ClearStatusItems();
      m_pAgent->SpecificationChanged(true);
   }
   m_pAgent->FirePendingEvents();

   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(RatingLibraryEntry* pSubject, Int32 hint)
{
   m_pAgent->HoldEvents();
   if (hint & LibraryHints::EntryRenamed)
      m_pAgent->m_RatingSpec = pSubject->GetName();

   if (hint & LibraryHints::EntryEdited)
   {
      ClearStatusItems();
      m_pAgent->RatingSpecificationChanged(true);
   }
   m_pAgent->FirePendingEvents();

   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(TrafficBarrierEntry* pSubject, Int32 hint)
{
   m_pAgent->HoldEvents();
   if (hint & LibraryHints::EntryRenamed)
   {
      if ( m_pAgent->m_BridgeDescription.GetLeftRailingSystem()->GetExteriorRailing() == pSubject )
         m_pAgent->m_BridgeDescription.GetLeftRailingSystem()->strExteriorRailing = pSubject->GetName();

      if ( m_pAgent->m_BridgeDescription.GetLeftRailingSystem()->GetInteriorRailing() == pSubject )
         m_pAgent->m_BridgeDescription.GetLeftRailingSystem()->strInteriorRailing = pSubject->GetName();

      if ( m_pAgent->m_BridgeDescription.GetRightRailingSystem()->GetExteriorRailing() == pSubject )
         m_pAgent->m_BridgeDescription.GetRightRailingSystem()->strExteriorRailing = pSubject->GetName();

      if ( m_pAgent->m_BridgeDescription.GetRightRailingSystem()->GetInteriorRailing() == pSubject )
         m_pAgent->m_BridgeDescription.GetRightRailingSystem()->strInteriorRailing = pSubject->GetName();
   }

   if (hint & LibraryHints::EntryEdited)
   {
      ClearStatusItems();
      m_pAgent->Fire_BridgeChanged();
   }
   m_pAgent->FirePendingEvents();

   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(LiveLoadLibraryEntry* pSubject, Int32 hint)
{
   m_pAgent->HoldEvents();
   if ( hint & LibraryHints::EntryRenamed )
   {
      for ( int i = 0; i < 8; i++ )
      {

         CProjectAgentImp::LiveLoadSelectionIterator begin = m_pAgent->m_SelectedLiveLoads[i].begin();
         CProjectAgentImp::LiveLoadSelectionIterator end   = m_pAgent->m_SelectedLiveLoads[i].end();

         CProjectAgentImp::LiveLoadSelection key;
         key.pEntry = pSubject;
         CProjectAgentImp::LiveLoadSelectionIterator found = std::find(begin,end,key);

         if ( found != end )
         {
            CProjectAgentImp::LiveLoadSelection& ll = *found;

            std::_tstring strOldName = ll.EntryName;
            
            ll.EntryName = pSubject->GetName();
            ll.pEntry = pSubject;

            m_pAgent->Fire_LiveLoadNameChanged(strOldName.c_str(),ll.EntryName.c_str());
         }
      }

   }

   if ( hint & LibraryHints::EntryEdited )
   {
      ClearStatusItems();
      m_pAgent->Fire_LiveLoadChanged();
   }
   m_pAgent->FirePendingEvents();

   GET_IFACE2(m_pAgent->m_pBroker,IEAFDocument,pDoc);
   pDoc->SetModified();
}

void pgsLibraryEntryObserver::Update(DuctLibraryEntry* pSubject,Int32 hint)
{
   m_pAgent->HoldEvents();
   if ( hint & LibraryHints::EntryRenamed )
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
               if ( pDuct->pDuctLibEntry == pSubject )
               {
                  pDuct->Name = pSubject->GetName();
               }
            }
         }
      }
   }

   if ( hint & LibraryHints::EntryEdited )
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

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsLibraryEntryObserver::AssertValid() const
{
   return true;
}

void pgsLibraryEntryObserver::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsLibraryEntryObserver" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsLibraryEntryObserver::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsLibraryEntryObserver");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsLibraryEntryObserver");

   TESTME_EPILOG("LibraryEntryObserver");
}
#endif // _UNITTEST
