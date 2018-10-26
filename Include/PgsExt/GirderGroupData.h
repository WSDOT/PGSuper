///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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


   // =================================================================================
   // Configuration information
   // =================================================================================
   GroupIndexType GetIndex() const;
   GroupIDType GetID() const;
   
   CBridgeDescription2* GetBridgeDescription();
   const CBridgeDescription2* GetBridgeDescription() const;

   CPierData2* GetPier(pgsTypes::MemberEndType end);
   const CPierData2* GetPier(pgsTypes::MemberEndType end) const;
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
   // Slab Offset (Haunch) Used only when the parent bridge's SlabOffsetType is sotGroup or sotsegment
   // =================================================================================

   // Set the slab offset at the ends of the girder group (same offset for all segments)
   // Use when slab offset type is pgsTypes::sotGroup
   void SetSlabOffset(pgsTypes::MemberEndType end,Float64 offset);

   // Set/Get the slab offset at the ends of the segments in a girder
   // Use when slab offset type is pgsTypes::sotSegment
   void SetSlabOffset(GirderIndexType gdrIdx,pgsTypes::MemberEndType end,Float64 offset);
   Float64 GetSlabOffset(GirderIndexType gdrIdx,pgsTypes::MemberEndType end) const;


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

   // returns the girder type name for a given girder index
   void SetGirderName(GroupIndexType grpIdx,LPCTSTR strName);
   void RenameGirder(GroupIndexType grpIdx,LPCTSTR strName);
   LPCTSTR GetGirderName(GirderIndexType gdrIdx) const;

   void SetGirderLibraryEntry(GirderIndexType gdrIdx,const GirderLibraryEntry* pEntry);
   const GirderLibraryEntry* GetGirderLibraryEntry(GirderIndexType gdrIdx) const;


   // =================================================================================
   // Miscellanous
   // =================================================================================
   bool IsExteriorGirder(GirderIndexType gdrIdx) const;
   bool IsInteriorGirder(GirderIndexType gdrIdx) const;
   
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
   CPierData2* m_pPier[2];
   PierIndexType m_PierIndex[2];

   std::vector<Float64> m_SlabOffset[2]; // slab offset is defined by segment

   std::vector<CSplicedGirderData*> m_Girders;

   typedef std::pair<GirderIndexType,GirderIndexType> GirderTypeGroup; // index of first and last girder in the group
   std::vector<GirderTypeGroup> m_GirderTypeGroups; // defines how girder lines are grouped

   void RemoveGirder(GirderIndexType gdrIdx);

   void UpdatePiers();

#if defined _DEBUG
   // Giving other classes access to our internal data members for debugging purposes only
   friend CGirderSpacing2;
#endif
};
