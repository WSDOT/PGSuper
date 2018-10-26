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
#include <PgsExt\SplicedGirderData.h>

class CBridgeDescription2;
class CPierData2;

/*****************************************************************************
CLASS 
   CGirderGroupData

DESCRIPTION
   Defines a group of related girders. The girders in the group
   all span between a start and end pier.


COPYRIGHT
   Copyright © 1997-2011
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

class PGSEXTCLASS CGirderGroupData
{
public:
   CGirderGroupData();
   CGirderGroupData(CBridgeDescription2* pBridge);
   CGirderGroupData(CPierData2* pStartPier,CPierData2* pEndPier);
   CGirderGroupData(const CGirderGroupData& rOther);
   ~CGirderGroupData();

   CGirderGroupData& operator=(const CGirderGroupData& rOther);
   void CopyGirderGroupData(const CGirderGroupData* pGroup);
   bool operator==(const CGirderGroupData& rOther) const;
   bool operator!=(const CGirderGroupData& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   // =================================================================================
   // Methods called by the framework. Don't call these methods directly
   // =================================================================================
   void SetIndex(GroupIndexType grpIdx);
   void SetID(GroupIDType grpID);
   void SetBridgeDescription(CBridgeDescription2* pBridge);
   void Clear();
   void SetPier(pgsTypes::MemberEndType end,CPierData2* pPier);
   void SetPiers(CPierData2* pStartPier,CPierData2* pEndPier);
   void AddSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType face);
   void RemoveSpan(SpanIndexType spanIdx,pgsTypes::RemovePierType rmPierType);


   // =================================================================================
   // Configuration information
   // =================================================================================
   GroupIndexType GetIndex() const;
   GroupIDType GetID() const;
   
   CBridgeDescription2* GetBridgeDescription();
   const CBridgeDescription2* GetBridgeDescription() const;

   CPierData2* GetPier(pgsTypes::MemberEndType end);
   const CPierData2* GetPier(pgsTypes::MemberEndType end) const;
   CPierData2* GetPier(PierIndexType pierIdx);
   const CPierData2* GetPier(PierIndexType pierIdx) const;
   PierIndexType GetPierIndex(pgsTypes::MemberEndType end) const;

   CGirderGroupData* GetPrevGirderGroup();
   const CGirderGroupData* GetPrevGirderGroup() const;
   CGirderGroupData* GetNextGirderGroup();
   const CGirderGroupData* GetNextGirderGroup() const;


   // =================================================================================
   // Girders in the group
   // =================================================================================

   // when adding new girders to the group, either by SetGirderCount or AddGirders, the
   // new girders have the same segment layout and properties as the right exterior girder
   // in the group

   // Creates new girders with one segment each. Removes all girders previously defined 
   // in this group
   void Initialize(GirderIndexType nGirders);

   // Set/Get number of girders
   void SetGirderCount(GirderIndexType nGirders);
   GirderIndexType GetGirderCount() const;

   // Add/Remove nGirders from the group
   void AddGirders(GirderIndexType nGirders);
   void RemoveGirders(GirderIndexType nGirders);

   // Set/Get the girder data for a specific girder
   void SetGirder(GirderIndexType gdrIdx,CSplicedGirderData* pGirderData); // does not change ID or Index
   CSplicedGirderData* GetGirder(GirderIndexType gdrIdx);
   const CSplicedGirderData* GetGirder(GirderIndexType gdrIdx) const;


   // =================================================================================
   // Slab Offset (Haunch) Used only when the parent bridge's SlabOffsetType is sotPier or sotGirder
   // =================================================================================

   // Set the slab offset at a pier within this girder group (same offset for all segments)
   // Use when slab offset type is pgsTypes::sotPier
   // The slab offset is applied to the ahead/back side of the first/pier of the group
   // or both sides if the pier is interior to the group
   void SetSlabOffset(PierIndexType pierIdx,Float64 offset);

   // Set/Get the slab offset at a pier for a specific girder within the group
   // Use when slab offset type is pgsTypes::sotGirder
   void SetSlabOffset(PierIndexType pierIdx,GirderIndexType gdrIdx,Float64 offset);
   Float64 GetSlabOffset(PierIndexType pierIdx,GirderIndexType gdrIdx,bool bGetRawValue = false) const;

   // Copies girder-by-girder slab offset data from one girder to another
   void CopySlabOffset(GirderIndexType sourceGdrIdx,GirderIndexType targetGdrIdx);


   // =================================================================================
   // Grouping of Girder Types (3@WF42G 1@WF42G_Modified 3@WF42G)
   // =================================================================================

   // creates a group that spans the range of girder indices provided
   // the other groups are re-factored
   // returns the index of the new group
   GroupIndexType CreateGirderTypeGroup(GirderIndexType firstGdrIdx,GirderIndexType lastGdrIdx);

   // returns the number of girder type groups
   GroupIndexType GetGirderTypeGroupCount() const;

   // Gets the first/last girder index for the girders in a group
   void GetGirderTypeGroup(GroupIndexType girderTypeGroupIdx,GirderIndexType* pFirstGdrIdx,GirderIndexType* pLastGdrIdx,std::_tstring* strName) const;

   // returns the group index that the girder belongs to
   GroupIndexType FindGroup(GirderIndexType gdrIdx) const;

   // expands girder type so every girder is treated independently
   // (un-joins girder groups)
   void ExpandAll();
   void Expand(GroupIndexType girderTypeGroupIdx);

   // joins girders together into a group. the last parameter is the index
   // of the girder who's type will be used for the group
   void JoinAll(GirderIndexType gdrIdx);
   void Join(GirderIndexType firstGdrIdx,GirderIndexType lastGdrIdx,GirderIndexType gdrIdx);

   // sets the name of the girder section used in the specified girder type group
   void SetGirderName(GroupIndexType girderTypeGroupIdx,LPCTSTR strName);
   void RenameGirder(GroupIndexType girderTypeGroupIdx,LPCTSTR strName);

   // returns the name of a girder
   LPCTSTR GetGirderName(GirderIndexType gdrIdx) const;

   void SetGirderLibraryEntry(GirderIndexType gdrIdx,const GirderLibraryEntry* pEntry);
   const GirderLibraryEntry* GetGirderLibraryEntry(GirderIndexType gdrIdx) const;


   // =================================================================================
   // Miscellanous
   // =================================================================================
   bool IsExteriorGirder(GirderIndexType gdrIdx) const;
   bool IsInteriorGirder(GirderIndexType gdrIdx) const;

   // returns the number of piers in this group
   PierIndexType GetPierCount() const;

   // returns the number of spans in this group
   SpanIndexType GetSpanCount() const;
   
   // returns the length of the girder measured between the end pier stations
   Float64 GetLength() const;

#if defined _DEBUG
   void AssertValid();
#endif

protected:
   void MakeCopy(const CGirderGroupData& rOther,bool bCopyOnlyData);
   virtual void MakeAssignment(const CGirderGroupData& rOther);

private:
   GroupIndexType m_GroupIdx;
   GroupIDType m_GroupID;

   CBridgeDescription2* m_pBridge;
   CPierData2* m_pPier[2]; // pier at the start of the group
   PierIndexType m_PierIndex[2]; // pier index at the start of the group (INVALID_INDEX if m_pPier points to an actual pier)

   std::vector<std::vector<Float64>> m_SlabOffsets; // outer vector, one entry per pier... inner vector, one entry per girder

   std::vector<CSplicedGirderData*> m_Girders;

   typedef std::pair<GirderIndexType,GirderIndexType> GirderTypeGroup; // index of first and last girder in the group
   std::vector<GirderTypeGroup> m_GirderTypeGroups; // defines how girder lines are grouped

   void RemoveGirder(GirderIndexType gdrIdx);

   void UpdatePiers();
   void UpdateSlabOffsets(PierIndexType newPierIdx);

   GirderIndexType GetPrivateGirderCount() const;
   friend CBridgeDescription2;

#if defined _DEBUG
   // Giving other classes access to our internal data members for debugging purposes only
   friend CGirderSpacing2;
#endif
};
