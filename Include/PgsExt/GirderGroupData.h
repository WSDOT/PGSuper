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

#pragma once
#include <PgsExt\PgsExtExp.h>
#include <PgsExt\SplicedGirderData.h>

class CBridgeDescription2;
class CPierData2;

typedef struct CGirderTypeGroup
{
   GirderIndexType firstGdrIdx, lastGdrIdx;
   std::_tstring strName;
   const GirderLibraryEntry* pGdrEntry;
} CGirderTypeGroup;

typedef struct CGirderTopWidthGroup
{
   GirderIndexType firstGdrIdx, lastGdrIdx;
   pgsTypes::TopWidthType type;
   std::array<Float64, 2> left, right; // array index is pgsTypes::MemberEndType
} CGirderTopWidthGroup;

/*****************************************************************************
CLASS 
   CGirderGroupData

DESCRIPTION
   Defines a group of related girders. The girders in the group
   all span between a start and end pier.
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
   void CopyGirderGroupData(const CGirderGroupData* pGroup,bool bCopyDataOnly);
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
   CGirderTypeGroup GetGirderTypeGroup(GroupIndexType girderTypeGroupIdx) const;

   std::vector<CGirderTypeGroup> GetGirderTypeGroups() const;
   void SetGirderTypeGroups(const std::vector<CGirderTypeGroup>& girderTypeGroups);

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

   // =================================================================================
   // Grouping of Girder Top Widths (3@6' 1@7'-3" 3@6'-6")
   // Top width groups are only valid if IsTopWidthSpacing() returns true
   // =================================================================================

   // creates a group that spans the range of girder indices provided
   // the other groups are re-factored
   // returns the index of the new group
   GroupIndexType CreateGirderTopWidthGroup(GirderIndexType firstGdrIdx, GirderIndexType lastGdrIdx);

   // returns the number of girder top flange groups
   GroupIndexType GetGirderTopWidthGroupCount() const;

   // Gets the first/last girder index for the girders in a group
   void GetGirderTopWidthGroup(GroupIndexType groupIdx, GirderIndexType* pFirstGdrIdx, GirderIndexType* pLastGdrIdx, pgsTypes::TopWidthType* pType,Float64* pLeftStart,Float64* pRightStart,Float64* pLeftEnd,Float64* pRightEnd) const;
   CGirderTopWidthGroup GetGirderTopWidthGroup(GroupIndexType groupIdx) const;

   std::vector<CGirderTopWidthGroup> GetGirderTopWidthGroups() const;
   void SetGirderTopWidthGroups(const std::vector<CGirderTopWidthGroup>& girderTopWidthGroups);

   // returns the group index that the girder belongs to
   GroupIndexType FindGirderTopWidthGroup(GirderIndexType gdrIdx) const;

   // expands girder top flange group so every girder is treated independently
   // (un-joins girder groups)
   void ExpandAllGirderTopWidthGroups();
   void ExpandGirderTopWidthGroup(GroupIndexType girderTopWidthGroupIdx);

   // joins girders together into a group. the last parameter is the index
   // of the girder who's top flange width will be used for the group
   void JoinAllGirderTopWidthGroups(GirderIndexType gdrIdx);
   void JoinGirderTopWidthGroup(GirderIndexType firstGdrIdx, GirderIndexType lastGdrIdx, GirderIndexType gdrIdx);

   // sets the top flange width of the girder used in the specified girder type group
   void SetGirderTopWidth(GroupIndexType girderTypeGroupIdx,pgsTypes::TopWidthType type,Float64 leftStart,Float64 rightStart,Float64 leftEnd,Float64 rightEnd);
   void GetGirderTopWidth(GirderIndexType gdrIdx, pgsTypes::TopWidthType* pType, Float64* pLeftStart, Float64* pRightStart,Float64* pLeftEnd,Float64* pRightEnd) const;


   // =================================================================================
   void SetGirderLibraryEntry(GirderIndexType gdrIdx,const GirderLibraryEntry* pEntry);
   const GirderLibraryEntry* GetGirderLibraryEntry(GirderIndexType gdrIdx) const;


   // =================================================================================
   // Miscellaneous
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
   void MakeAssignment(const CGirderGroupData& rOther);

private:
   GroupIndexType m_GroupIdx;
   GroupIDType m_GroupID;

   CBridgeDescription2* m_pBridgeDesc;
   std::array<CPierData2*, 2> m_pPier; // pier at the start of the group
   std::array<PierIndexType, 2> m_PierIndex; // pier index at the start of the group (INVALID_INDEX if m_pPier points to an actual pier)

   std::vector<CSplicedGirderData*> m_Girders;

   typedef std::pair<GirderIndexType,GirderIndexType> GirderGroup; // index of first and last girder in the group
   std::vector<GirderGroup> m_GirderTypeGroups; // defines how girder types are grouped
   std::vector<GirderGroup> m_GirderTopWidthGroups; // defines how top flange widths are grouped

   void CreateGirderGroup(GirderIndexType firstGdrIdx, GirderIndexType lastGdrIdx, std::vector<GirderGroup>* pGroups, GroupIndexType* pNewGroupIdx);
   void ExpandAll(std::vector<GirderGroup>* pGroups);
   void Expand(GroupIndexType groupIdx, std::vector<GirderGroup>* pGroups);
   void Join(GirderIndexType firstGdrIdx, GirderIndexType lastGdrIdx, GirderIndexType gdrIdx, std::vector<GirderGroup>* pGroups);
   GroupIndexType FindGroup(GirderIndexType gdrIdx, const std::vector<GirderGroup>& groups) const;


   void RemoveGirder(GirderIndexType gdrIdx);

   void RepairGirderTypeGroups();

   void UpdatePiers();

   GirderIndexType GetPrivateGirderCount() const;
   friend CBridgeDescription2;

#if defined _DEBUG
   // Giving other classes access to our internal data members for debugging purposes only
   friend CGirderSpacing2;
#endif
};
