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
   CollectionIndexType GetPointLoadCount() const;
   CollectionIndexType AddPointLoad(EventIDType eventID,const CPointLoadData& pld);
   const CPointLoadData* GetPointLoad(CollectionIndexType idx) const;
   const CPointLoadData* FindPointLoad(LoadIDType loadID) const;
   bool UpdatePointLoad(CollectionIndexType idx, EventIDType eventID,const CPointLoadData& pld,bool* pbMovedGirders,CSpanKey* pPrevKey);
   void DeletePointLoad(CollectionIndexType idx,CSpanKey* pKey);


   CollectionIndexType GetDistributedLoadCount() const;
   CollectionIndexType AddDistributedLoad(EventIDType eventID,const CDistributedLoadData& pld);
   const CDistributedLoadData* GetDistributedLoad(CollectionIndexType idx) const;
   const CDistributedLoadData* FindDistributedLoad(LoadIDType loadID) const;
   bool UpdateDistributedLoad(CollectionIndexType idx, EventIDType eventID,const CDistributedLoadData& pld,bool* pbMovedGirders,CSpanKey* pPrevKey);
   void DeleteDistributedLoad(CollectionIndexType idx,CSpanKey* pKey);

   CollectionIndexType GetMomentLoadCount() const;
   CollectionIndexType AddMomentLoad(EventIDType eventID,const CMomentLoadData& pld);
   const CMomentLoadData* GetMomentLoad(CollectionIndexType idx) const;
   const CMomentLoadData* FindMomentLoad(LoadIDType loadID) const;
   bool UpdateMomentLoad(CollectionIndexType idx, EventIDType eventID,const CMomentLoadData& pld,bool* pbMovedGirders,CSpanKey* pPrevKey);
   void DeleteMomentLoad(CollectionIndexType idx,CSpanKey* pKey);

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
   virtual void MakeAssignment(const CLoadManager& rOther);

private:
   CTimelineManager* m_pTimelineManager;

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
