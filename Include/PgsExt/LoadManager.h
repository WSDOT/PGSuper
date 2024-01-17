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

#pragma once
#include <PgsExt\PgsExtExp.h>
#include <PgsExt\TimelineManager.h>
#include <PgsExt\PointLoadData.h>
#include <PgsExt\DistributedLoadData.h>
#include <PgsExt\MomentLoadData.h>


/*****************************************************************************
CLASS 
   CLoadManager

   Utility class for managing the user defined load data
*****************************************************************************/

class PGSEXTCLASS CLoadManager
{
public:
   CLoadManager();
   CLoadManager(const CLoadManager& rOther);
   ~CLoadManager();

   CLoadManager& operator = (const CLoadManager& rOther);
   bool operator==(const CLoadManager& rOther) const;
   bool operator!=(const CLoadManager& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   void SetTimelineManager(CTimelineManager* timelineMgr);
   const CTimelineManager* GetTimelineManager() const;
   CTimelineManager* GetTimelineManager();

   // =================================================================================
   // Loads
   // =================================================================================
   IndexType GetPointLoadCount() const;
   IndexType AddPointLoad(EventIDType eventID,const CPointLoadData& pld);
   const CPointLoadData* GetPointLoad(IndexType idx) const;
   const CPointLoadData* FindPointLoad(LoadIDType loadID) const;
   EventIndexType GetPointLoadEventIndex(LoadIDType loadID) const;
   EventIDType GetPointLoadEventID(LoadIDType loadID) const;
   bool UpdatePointLoad(IndexType idx, EventIDType eventID, const CPointLoadData& pld, bool* pbMovedGirders, CSpanKey* pPrevKey);
   bool UpdatePointLoadByID(LoadIDType loadID, EventIDType eventID, const CPointLoadData& pld, bool* pbMovedGirders, CSpanKey* pPrevKey);
   void DeletePointLoad(IndexType idx, CSpanKey* pKey);
   void DeletePointLoadByID(LoadIDType loadID, CSpanKey* pKey);
   std::vector<CPointLoadData> GetPointLoads(const CSpanKey& spanKey) const;

   IndexType GetDistributedLoadCount() const;
   IndexType AddDistributedLoad(EventIDType eventID,const CDistributedLoadData& pld);
   const CDistributedLoadData* GetDistributedLoad(IndexType idx) const;
   const CDistributedLoadData* FindDistributedLoad(LoadIDType loadID) const;
   EventIndexType GetDistributedLoadEventIndex(LoadIDType loadID) const;
   EventIDType GetDistributedLoadEventID(LoadIDType loadID) const;
   bool UpdateDistributedLoad(IndexType idx, EventIDType eventID, const CDistributedLoadData& pld, bool* pbMovedGirders, CSpanKey* pPrevKey);
   bool UpdateDistributedLoadByID(LoadIDType loadID, EventIDType eventID, const CDistributedLoadData& pld, bool* pbMovedGirders, CSpanKey* pPrevKey);
   void DeleteDistributedLoad(IndexType idx,CSpanKey* pKey);
   void DeleteDistributedLoadByID(LoadIDType loadID, CSpanKey* pKey);
   std::vector<CDistributedLoadData> GetDistributedLoads(const CSpanKey& spanKey) const;

   IndexType GetMomentLoadCount() const;
   IndexType AddMomentLoad(EventIDType eventID,const CMomentLoadData& pld);
   const CMomentLoadData* GetMomentLoad(IndexType idx) const;
   const CMomentLoadData* FindMomentLoad(LoadIDType loadID) const;
   EventIndexType GetMomentLoadEventIndex(LoadIDType loadID) const;
   EventIDType GetMomentLoadEventID(LoadIDType loadID) const;
   bool UpdateMomentLoad(IndexType idx, EventIDType eventID, const CMomentLoadData& pld, bool* pbMovedGirders, CSpanKey* pPrevKey);
   bool UpdateMomentLoadByID(LoadIDType loadID, EventIDType eventID, const CMomentLoadData& pld, bool* pbMovedGirders, CSpanKey* pPrevKey);
   void DeleteMomentLoad(IndexType idx,CSpanKey* pKey);
   void DeleteMomentLoadByID(LoadIDType loadID, CSpanKey* pKey);
   std::vector<CMomentLoadData> GetMomentLoads(const CSpanKey& spanKey) const;

   // =================================================================================
   // Miscellaneous
   // =================================================================================

   bool HasUserLoad(const CGirderKey& girderKey,UserLoads::LoadCase lcType) const;

   // Removes everything
   void Clear();

   // Old beta versions created some bad loading data
   // Call this to fix the problem.
   CString FixBadLoads();

protected:
   void MakeCopy(const CLoadManager& rOther);
   void MakeAssignment(const CLoadManager& rOther);

private:
   CTimelineManager* m_pTimelineManager;

   CPointLoadData* FindPointLoadByID(LoadIDType loadID);
   CDistributedLoadData* FindDistributedLoadByID(LoadIDType loadID);
   CMomentLoadData* FindMomentLoadByID(LoadIDType loadID);

   // user defined loads
   typedef std::vector<CPointLoadData> PointLoadList;
   typedef PointLoadList::iterator PointLoadListIterator;
   PointLoadList m_PointLoads;

   typedef std::vector<CDistributedLoadData> DistributedLoadList;
   typedef DistributedLoadList::iterator DistributedLoadListIterator;
   DistributedLoadList m_DistributedLoads;

   typedef std::vector<CMomentLoadData> MomentLoadList;
   typedef MomentLoadList::iterator MomentLoadListIterator;
   MomentLoadList m_MomentLoads;

   friend CTimelineManager;

#if defined _DEBUG
   void AssertValid();
#endif
};
