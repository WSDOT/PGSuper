///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#include <PgsExt\LoadManager.h>
#include <PgsExt\GirderLabel.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CLoadManager::CLoadManager()
{
   m_pTimelineManager = nullptr;
}

CLoadManager::CLoadManager(const CLoadManager& rOther)
{
   m_pTimelineManager = nullptr;
   MakeCopy(rOther);
}

CLoadManager::~CLoadManager()
{
   Clear();
}

CLoadManager& CLoadManager::operator= (const CLoadManager& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CLoadManager::operator==(const CLoadManager& rOther) const
{
   if ( m_PointLoads != rOther.m_PointLoads )
      return false;

   if ( m_DistributedLoads != rOther.m_DistributedLoads )
      return false;

   if ( m_MomentLoads != rOther.m_MomentLoads )
      return false;

   return true;
}

bool CLoadManager::operator!=(const CLoadManager& rOther) const
{
   return !operator==(rOther);
}

HRESULT CLoadManager::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;

   Clear();

   CHRException hr;

   try
   {
      HRESULT hrLoadManager = pStrLoad->BeginUnit(_T("LoadManager")); // it is OK if this fails... it will for the older loading data

      // Point Loads
      hr = pStrLoad->BeginUnit(_T("UserDefinedPointLoads"));
      CComVariant var;
      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("Count"),&var);
      IndexType cnt = VARIANT2INDEX(var);

      for (IndexType i = 0; i < cnt; i++)
      {
         CPointLoadData data;

         hr = data.Load(pStrLoad);

         m_PointLoads.push_back(data);

         UserLoads::ms_NextPointLoadID = Max(UserLoads::ms_NextPointLoadID,data.m_ID+1);
      }
      hr = pStrLoad->EndUnit();

      // Distributed Loads
      hr = pStrLoad->BeginUnit(_T("UserDefinedDistributedLoads"));

      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("Count"),&var);

      cnt = VARIANT2INDEX(var);

      for (IndexType i = 0; i < cnt; i++)
      {
         CDistributedLoadData data;

         hr = data.Load(pStrLoad);

         m_DistributedLoads.push_back(data);
         UserLoads::ms_NextDistributedLoadID = Max(UserLoads::ms_NextDistributedLoadID,data.m_ID+1);
      }
       
      hr = pStrLoad->EndUnit();

      // Moment Loads
      HRESULT hrMoment = pStrLoad->BeginUnit(_T("UserDefinedMomentLoads"));
      // there was a time when we didn't have moment loads... this unit could fail if
      // we are loading a really old file. just skip instead of trying to figure out
      // if the file version is old enought
      if ( SUCCEEDED(hrMoment) )
      {
         var.vt = VT_INDEX;
         hr = pStrLoad->get_Property(_T("Count"),&var);

         cnt = VARIANT2INDEX(var);

         for (IndexType i = 0; i < cnt; i++)
         {
            CMomentLoadData data;

            hr = data.Load(pStrLoad);

            m_MomentLoads.push_back(data);
            UserLoads::ms_NextMomentLoadID = Max(UserLoads::ms_NextMomentLoadID,data.m_ID+1);
         }
            
         hr = pStrLoad->EndUnit(); // UserDefinedMomentLoad
      }


      if ( SUCCEEDED(hrLoadManager) )
      {
         hr = pStrLoad->EndUnit(); // LoadManager
      }
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   PGS_ASSERT_VALID;
   return hr;
}

HRESULT CLoadManager::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;
   pStrSave->BeginUnit(_T("LoadManager"),1.0);
   
   // Point Loads
   pStrSave->BeginUnit(_T("UserDefinedPointLoads"),1.0);

   hr = pStrSave->put_Property(_T("Count"),CComVariant((long)m_PointLoads.size()));
   if ( FAILED(hr) )
   {
      return hr;
   }

   PointLoadListIterator iter(m_PointLoads.begin());
   PointLoadListIterator iterEnd(m_PointLoads.end());
   for ( ; iter != iterEnd; iter++)
   {
      hr = iter->Save(pStrSave);
      if ( FAILED(hr) )
      {
         return hr;
      }
   }
   
   pStrSave->EndUnit(); // UserDefinedPoint Loads

   // Distributed Loads
   pStrSave->BeginUnit(_T("UserDefinedDistributedLoads"),1.0);

   hr = pStrSave->put_Property(_T("Count"),CComVariant((long)m_DistributedLoads.size()));
   if ( FAILED(hr) )
   {
      return hr;
   }

   for (DistributedLoadListIterator it=m_DistributedLoads.begin(); it!=m_DistributedLoads.end(); it++)
   {
      hr = it->Save(pStrSave);
      if ( FAILED(hr) )
      {
         return hr;
      }
   }
   
   pStrSave->EndUnit(); // UserDefinedDistributedLoads

   // Moment Loads
   pStrSave->BeginUnit(_T("UserDefinedMomentLoads"),1.0);

   hr = pStrSave->put_Property(_T("Count"),CComVariant((long)m_MomentLoads.size()));
   if ( FAILED(hr) )
   {
      return hr;
   }

   for (MomentLoadListIterator it=m_MomentLoads.begin(); it!=m_MomentLoads.end(); it++)
   {
      hr = it->Save(pStrSave);
      if ( FAILED(hr) )
      {
         return hr;
      }
   }
   
   pStrSave->EndUnit(); // UseDefinedMomentLoads

   pStrSave->EndUnit(); // LoadManager
   return hr;
}

void CLoadManager::SetTimelineManager(CTimelineManager* pTimelineMgr)
{
   m_pTimelineManager = pTimelineMgr;
   m_pTimelineManager->SetLoadManager(this);
}

const CTimelineManager* CLoadManager::GetTimelineManager() const
{
   return m_pTimelineManager;
}

CTimelineManager* CLoadManager::GetTimelineManager()
{
   return m_pTimelineManager;
}

void CLoadManager::Clear()
{
   m_PointLoads.clear();
   m_DistributedLoads.clear();
   m_MomentLoads.clear();
}

CollectionIndexType CLoadManager::GetPointLoadCount() const
{
   return m_PointLoads.size();
}

CollectionIndexType CLoadManager::AddPointLoad(EventIDType eventID,const CPointLoadData& pld)
{
   ATLASSERT(eventID != INVALID_ID);

   m_PointLoads.push_back(pld);
   m_PointLoads.back().m_ID = UserLoads::ms_NextPointLoadID++;

   CTimelineEvent* pEvent = m_pTimelineManager->GetEventByID(eventID);
   ATLASSERT(pEvent);
   pEvent->GetApplyLoadActivity().AddUserLoad(m_PointLoads.back().m_ID);

   return m_PointLoads.size()-1;
}

const CPointLoadData* CLoadManager::GetPointLoad(CollectionIndexType idx) const
{
   ATLASSERT(0 <= idx && idx < GetPointLoadCount() );

   return &m_PointLoads[idx];
}

const CPointLoadData* CLoadManager::FindPointLoad(LoadIDType loadID) const
{
   PointLoadList::const_iterator iter(m_PointLoads.begin());
   PointLoadList::const_iterator end(m_PointLoads.end());
   for ( ; iter != end; iter++ )
   {
      const CPointLoadData& loadData = *iter;
      if ( loadData.m_ID == loadID )
      {
         return &loadData;
      }
   }

   return nullptr;
}

bool CLoadManager::UpdatePointLoad(CollectionIndexType idx, EventIDType eventID,const CPointLoadData& pld,bool* pbMovedGirders,CSpanKey* pPrevKey)
{
   ATLASSERT(0 <= idx && idx < GetPointLoadCount() );
   ATLASSERT(eventID != INVALID_ID);

   EventIDType oldEventID = m_pTimelineManager->FindUserLoadEventID(m_PointLoads[idx].m_ID);

   *pbMovedGirders = false;
   if ( m_PointLoads[idx] != pld || eventID != oldEventID )
   {
      // must fire a delete event if load is moved to another girder
      const CSpanKey& prevKey = m_PointLoads[idx].m_SpanKey;

      if (prevKey != pld.m_SpanKey)
      {
         *pbMovedGirders = true;
         *pPrevKey = prevKey;
      }

      CTimelineEvent* pEvent = m_pTimelineManager->GetEventByID(oldEventID);
      pEvent->GetApplyLoadActivity().RemoveUserLoad(m_PointLoads[idx].m_ID);

      m_PointLoads[idx] = pld;

      pEvent = m_pTimelineManager->GetEventByID(eventID);
      ATLASSERT(pEvent);
      pEvent->GetApplyLoadActivity().AddUserLoad(m_PointLoads[idx].m_ID);

      return true;
   }

   return false;
}

void CLoadManager::DeletePointLoad(CollectionIndexType idx,CSpanKey* pKey)
{
   ATLASSERT(0 <= idx && idx < GetPointLoadCount() );

   PointLoadListIterator it( m_PointLoads.begin() );
   it += idx;

   EventIndexType eventIdx = m_pTimelineManager->FindUserLoadEventIndex(it->m_ID);
   ATLASSERT(eventIdx != INVALID_INDEX);
   CTimelineEvent* pEvent = m_pTimelineManager->GetEventByIndex(eventIdx);
   pEvent->GetApplyLoadActivity().RemoveUserLoad(it->m_ID);

   *pKey = it->m_SpanKey;

   m_PointLoads.erase(it);
}


CollectionIndexType CLoadManager::GetDistributedLoadCount() const
{
   return m_DistributedLoads.size();
}

CollectionIndexType CLoadManager::AddDistributedLoad(EventIDType eventID,const CDistributedLoadData& pld)
{
   ATLASSERT(eventID != INVALID_ID);

   m_DistributedLoads.push_back(pld);
   m_DistributedLoads.back().m_ID = UserLoads::ms_NextDistributedLoadID++;

   CTimelineEvent* pEvent = m_pTimelineManager->GetEventByID(eventID);
   ATLASSERT(pEvent);
   pEvent->GetApplyLoadActivity().AddUserLoad(m_DistributedLoads.back().m_ID);

   return m_DistributedLoads.size()-1;
}

const CDistributedLoadData* CLoadManager::GetDistributedLoad(CollectionIndexType idx) const
{
   ATLASSERT(0 <= idx && idx < GetDistributedLoadCount() );

   return &m_DistributedLoads[idx];
}

const CDistributedLoadData* CLoadManager::FindDistributedLoad(LoadIDType loadID) const
{
   DistributedLoadList::const_iterator iter(m_DistributedLoads.begin());
   DistributedLoadList::const_iterator end(m_DistributedLoads.end());
   for ( ; iter != end; iter++ )
   {
      const CDistributedLoadData& loadData = *iter;
      if ( loadData.m_ID == loadID )
      {
         return &loadData;
      }
   }

   return nullptr;
}

bool CLoadManager::UpdateDistributedLoad(CollectionIndexType idx, EventIDType eventID,const CDistributedLoadData& pld,bool* pbMovedGirder,CSpanKey* pPrevKey)
{
   ATLASSERT(0 <= idx && idx < GetDistributedLoadCount() );
   ATLASSERT(eventID != INVALID_ID);

   EventIDType oldEventID = m_pTimelineManager->FindUserLoadEventID(m_DistributedLoads[idx].m_ID);

   *pbMovedGirder = false;
   if ( m_DistributedLoads[idx] != pld || eventID != oldEventID )
   {
      // must fire a delete event if load is moved to another girder
      const CSpanKey& prevKey = m_DistributedLoads[idx].m_SpanKey;

      if (prevKey != pld.m_SpanKey)
      {
         *pbMovedGirder = true;
         *pPrevKey = prevKey;
      }

      CTimelineEvent* pEvent = m_pTimelineManager->GetEventByID(oldEventID);
      pEvent->GetApplyLoadActivity().RemoveUserLoad(m_DistributedLoads[idx].m_ID);

      m_DistributedLoads[idx] = pld;

      pEvent = m_pTimelineManager->GetEventByID(eventID);
      ATLASSERT(pEvent);
      pEvent->GetApplyLoadActivity().AddUserLoad(m_DistributedLoads[idx].m_ID);
      return true;
   }

   return false;
}

void CLoadManager::DeleteDistributedLoad(CollectionIndexType idx,CSpanKey* pKey)
{
   ATLASSERT(0 <= idx && idx < GetDistributedLoadCount() );

   DistributedLoadListIterator it( m_DistributedLoads.begin() );
   it += idx;

   EventIndexType eventIdx = m_pTimelineManager->FindUserLoadEventIndex(it->m_ID);
   ATLASSERT(eventIdx != INVALID_INDEX);
   CTimelineEvent* pEvent = m_pTimelineManager->GetEventByIndex(eventIdx);
   pEvent->GetApplyLoadActivity().RemoveUserLoad(it->m_ID);

   *pKey = it->m_SpanKey;

   m_DistributedLoads.erase(it);
}


CollectionIndexType CLoadManager::GetMomentLoadCount() const
{
   return m_MomentLoads.size();
}

CollectionIndexType CLoadManager::AddMomentLoad(EventIDType eventID,const CMomentLoadData& pld)
{
   ATLASSERT(eventID != INVALID_ID);

   m_MomentLoads.push_back(pld);
   m_MomentLoads.back().m_ID = UserLoads::ms_NextMomentLoadID++;

   CTimelineEvent* pEvent = m_pTimelineManager->GetEventByID(eventID);
   ATLASSERT(pEvent);
   pEvent->GetApplyLoadActivity().AddUserLoad(m_MomentLoads.back().m_ID);

   return m_MomentLoads.size()-1;
}

const CMomentLoadData* CLoadManager::GetMomentLoad(CollectionIndexType idx) const
{
   ATLASSERT(0 <= idx && idx < GetMomentLoadCount() );

   return &m_MomentLoads[idx];
}

const CMomentLoadData* CLoadManager::FindMomentLoad(LoadIDType loadID) const
{
   MomentLoadList::const_iterator iter(m_MomentLoads.begin());
   MomentLoadList::const_iterator end(m_MomentLoads.end());
   for ( ; iter != end; iter++ )
   {
      const CMomentLoadData& loadData = *iter;
      if ( loadData.m_ID == loadID )
      {
         return &loadData;
      }
   }

   return nullptr;
}

bool CLoadManager::UpdateMomentLoad(CollectionIndexType idx, EventIDType eventID,const CMomentLoadData& pld,bool* pbMovedGirder,CSpanKey* pPrevKey)
{
   ATLASSERT(0 <= idx && idx < GetMomentLoadCount() );
   ATLASSERT(eventID != INVALID_ID);

   EventIDType oldEventID = m_pTimelineManager->FindUserLoadEventID(m_DistributedLoads[idx].m_ID);

   *pbMovedGirder = false;
   if ( m_MomentLoads[idx] != pld || oldEventID != eventID )
   {
      // must fire a delete event if load is moved to another girder
      const CSpanKey& prevKey = m_MomentLoads[idx].m_SpanKey;

      if (prevKey != pld.m_SpanKey)
      {
         *pbMovedGirder = true;
         *pPrevKey = prevKey;
      }

      CTimelineEvent* pEvent = m_pTimelineManager->GetEventByID(oldEventID);
      pEvent->GetApplyLoadActivity().RemoveUserLoad(m_MomentLoads[idx].m_ID);

      m_MomentLoads[idx] = pld;

      pEvent = m_pTimelineManager->GetEventByID(eventID);
      ATLASSERT(pEvent);
      pEvent->GetApplyLoadActivity().AddUserLoad(pld.m_ID);

      return true;
   }
   return false;
}

void CLoadManager::DeleteMomentLoad(CollectionIndexType idx,CSpanKey* pKey)
{
   ATLASSERT(0 <= idx && idx < GetMomentLoadCount() );

   MomentLoadListIterator it( m_MomentLoads.begin() );
   it += idx;

   EventIndexType eventIdx = m_pTimelineManager->FindUserLoadEventIndex(it->m_ID);
   ATLASSERT(eventIdx != INVALID_INDEX);
   CTimelineEvent* pEvent = m_pTimelineManager->GetEventByIndex(eventIdx);
   pEvent->GetApplyLoadActivity().RemoveUserLoad(it->m_ID);

   *pKey = it->m_SpanKey;

   m_MomentLoads.erase(it);
}

bool CLoadManager::HasUserLoad(const CGirderKey& girderKey,UserLoads::LoadCase lcType) const
{
   for (const auto& pntLd : m_PointLoads)
   {
      if ( pntLd.m_LoadCase == lcType )
      {
         return true;
      }
   }

   for (const auto& distLd : m_DistributedLoads)
   {
      if ( distLd.m_LoadCase == lcType )
      {
         return true;
      }
   }

   for (const auto& momLd : m_MomentLoads)
   {
      if ( momLd.m_LoadCase == lcType )
      {
         return true;
      }
   }

   return false;
}

void CLoadManager::MakeCopy(const CLoadManager& rOther)
{
#if defined _DEBUG
   const_cast<CLoadManager*>(&rOther)->AssertValid();
#endif

   // clear out the deck, spans and piers... then rebuild
   Clear();

   m_PointLoads = rOther.m_PointLoads;
   m_DistributedLoads = rOther.m_DistributedLoads;
   m_MomentLoads = rOther.m_MomentLoads;

   m_pTimelineManager = rOther.m_pTimelineManager;
   m_pTimelineManager->SetLoadManager(this);
   PGS_ASSERT_VALID;
}

void CLoadManager::MakeAssignment(const CLoadManager& rOther)
{
   MakeCopy( rOther );
}

#if defined _DEBUG
void CLoadManager::AssertValid()
{
}
#endif



CTimelineManager* g_pTimelineMgr;
template <class T>
bool IsBadLoad(const T& load)
{
   EventIDType eventID = g_pTimelineMgr->FindUserLoadEventID(load.m_ID);
   return eventID == INVALID_ID ? true : false;
}

CString CLoadManager::FixBadLoads()
{
   CString strBadLoads;

   // Prior to Version 3, Beta 52, there were cases where we would lose track of when a load was
   // applied to the timeline. There is was not a fix-able error. Search the loads and delete
   // any load that we just model anymore.
   g_pTimelineMgr = m_pTimelineManager;
   std::vector<CPointLoadData>::iterator newPointLoadEnd = std::remove_if(m_PointLoads.begin(),m_PointLoads.end(),IsBadLoad<CPointLoadData>);
   std::vector<CDistributedLoadData>::iterator newDistributedLoadEnd = std::remove_if(m_DistributedLoads.begin(),m_DistributedLoads.end(),IsBadLoad<CDistributedLoadData>);
   std::vector<CMomentLoadData>::iterator newMomentLoadEnd = std::remove_if(m_MomentLoads.begin(),m_MomentLoads.end(),IsBadLoad<CMomentLoadData>);
   g_pTimelineMgr = nullptr;

   std::vector<CString> vDeletedLoads;
   if ( newPointLoadEnd != m_PointLoads.end() )
   {
      std::vector<CPointLoadData>::iterator iter(newPointLoadEnd);
      for ( ; iter != m_PointLoads.end(); iter++ )
      {
         CString strMessage;
         CString strSpanLabel;
         if ( iter->m_SpanKey.spanIndex == ALL_SPANS )
         {
            strSpanLabel = _T("All Spans");
         }
         else
         {
            strSpanLabel.Format(_T("Span %d"),LABEL_SPAN(iter->m_SpanKey.spanIndex));
         }

         CString strGirderLabel;
         if ( iter->m_SpanKey.girderIndex == ALL_GIRDERS )
         {
            strGirderLabel = _T("All Girders");
         }
         else
         {
            strGirderLabel.Format(_T("Girder %s"),LABEL_GIRDER(iter->m_SpanKey.girderIndex));
         }

         strMessage.Format(_T("Point Load: %s, %s, %s"),strSpanLabel,strGirderLabel,iter->m_Description.c_str());
         vDeletedLoads.push_back(strMessage);
      }

      m_PointLoads.erase(newPointLoadEnd,m_PointLoads.end());
   }

   if ( newDistributedLoadEnd != m_DistributedLoads.end() )
   {
      std::vector<CDistributedLoadData>::iterator iter(newDistributedLoadEnd);
      for ( ; iter != m_DistributedLoads.end(); iter++ )
      {
         CString strMessage;
         CString strSpanLabel;
         if ( iter->m_SpanKey.spanIndex == ALL_SPANS )
         {
            strSpanLabel = _T("All Spans");
         }
         else
         {
            strSpanLabel.Format(_T("Span %d"),LABEL_SPAN(iter->m_SpanKey.spanIndex));
         }

         CString strGirderLabel;
         if ( iter->m_SpanKey.girderIndex == ALL_GIRDERS )
         {
            strGirderLabel = _T("All Girders");
         }
         else
         {
            strGirderLabel.Format(_T("Girder %s"),LABEL_GIRDER(iter->m_SpanKey.girderIndex));
         }

         strMessage.Format(_T("Distributed Load: %s, %s, %s"),strSpanLabel,strGirderLabel,iter->m_Description.c_str());
         vDeletedLoads.push_back(strMessage);
      }

      m_DistributedLoads.erase(newDistributedLoadEnd,m_DistributedLoads.end());
   }
 
   if ( newMomentLoadEnd != m_MomentLoads.end() )
   {
      std::vector<CMomentLoadData>::iterator iter(newMomentLoadEnd);
      for ( ; iter != m_MomentLoads.end(); iter++ )
      {
         CString strMessage;
         CString strSpanLabel;
         if ( iter->m_SpanKey.spanIndex == ALL_SPANS )
         {
            strSpanLabel = _T("All Spans");
         }
         else
         {
            strSpanLabel.Format(_T("Span %d"),LABEL_SPAN(iter->m_SpanKey.spanIndex));
         }

         CString strGirderLabel;
         if ( iter->m_SpanKey.girderIndex == ALL_GIRDERS )
         {
            strGirderLabel = _T("All Girders");
         }
         else
         {
            strGirderLabel.Format(_T("Girder %s"),LABEL_GIRDER(iter->m_SpanKey.girderIndex));
         }

         strMessage.Format(_T("Moment Load: %s, %s, %s"),strSpanLabel,strGirderLabel,iter->m_Description.c_str());
         vDeletedLoads.push_back(strMessage);
      }

      m_MomentLoads.erase(newMomentLoadEnd,m_MomentLoads.end());
   }

   if ( 0 < vDeletedLoads.size() )
   {
      strBadLoads = (_T("The following loads could no longer be modeled and have been deleted:"));
      for (const auto& str : vDeletedLoads)
      {
         strBadLoads += _T("\r\n") + str;
      }
   }

   return strBadLoads;
}
