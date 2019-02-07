///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSEXT_GIRDERTYPES_H_
#define INCLUDED_PGSEXT_GIRDERTYPES_H_


#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <WBFLCore.h>

#include <PsgLib\GirderLibraryEntry.h>

class CGirderSpacing;
class CSpanData;
class CGirderData;

class PGSEXTCLASS CGirderTypes
{
public:
   CGirderTypes();
   CGirderTypes(const CGirderTypes& rOther);
   ~CGirderTypes();

   CGirderTypes& operator = (const CGirderTypes& rOther);
   bool operator == (const CGirderTypes& rOther) const;
   bool operator != (const CGirderTypes& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   void SetGirderData(GirderIndexType gdrIdx,const CGirderData& gdrData);
   const CGirderData& GetGirderData(GirderIndexType gdrIdx) const;
   CGirderData& GetGirderData(GirderIndexType gdrIdx);

   void SetSlabOffset(GirderIndexType gdrIdx,pgsTypes::MemberEndType end,Float64 offset);
   Float64 GetSlabOffset(GirderIndexType gdrIdx,pgsTypes::MemberEndType end);
   Float64 GetSlabOffset(GirderIndexType gdrIdx,pgsTypes::MemberEndType end) const;

   GirderIndexType GetGirderCount() const;
   void SetGirderCount(GirderIndexType nGirders);
   void AddGirders(GirderIndexType nGirders);
   void RemoveGirders(GirderIndexType nGirders);

   void GetGirderGroup(GroupIndexType groupIdx,GirderIndexType* pFirstGdrIdx,GirderIndexType* pLastGdrIdx,std::_tstring& strName) const;

   // returns the group index that the girder belongs to
   GroupIndexType FindGroup(GirderIndexType gdrIdx) const;

   // creates a group that spans the range of girder indices provided
   // the other groups are re-factored
   // returns the index of the new group
   GroupIndexType CreateGroup(GirderIndexType firstGdrIdx,GirderIndexType lastGdrIdx);

   // expends girder type definitions so every girder is treated independently
   // (un-joins girder groups)
   void ExpandAll();
   void Expand(GroupIndexType groupIdx);

   // joins girders together into a group. the last parameter is the index
   // of the girder who's type will be used for the group
   void JoinAll(GirderIndexType gdrIdx);
   void Join(GirderIndexType firstGdrIdx,GirderIndexType lastGdrIdx,GirderIndexType gdrIdx);

   // returns the number of girder type groups
   GroupIndexType GetGirderGroupCount() const;

   // returns the girder type name for a given girder index
   void SetGirderName(GroupIndexType grpIdx,LPCTSTR strName);
   void RenameGirder(GroupIndexType grpIdx,LPCTSTR strName);
   LPCTSTR GetGirderName(GirderIndexType gdrIdx) const;

   void SetGirderLibraryEntry(GroupIndexType grpIdx,const GirderLibraryEntry* pEntry);
   const GirderLibraryEntry* GetGirderLibraryEntry(GirderIndexType gdrIdx) const;

   void SetSpan(const CSpanData* pSpan);
   const CSpanData* GetSpan() const;

protected:
   void MakeCopy(const CGirderTypes& rOther);
   void MakeAssignment(const CGirderTypes& rOther);

private:
   std::vector<CGirderData> m_GirderData;
   std::vector<Float64> m_SlabOffset[2]; // slab offset at each end of the girder for each girder line
                                         // index is pgsTypes::metStart and pgsTypes::metEnd

   typedef std::pair<GirderIndexType,GirderIndexType> GirderGroup; // index of first and last girder in the group
   std::vector<GirderGroup> m_GirderGroups; // defines how girder lines are grouped

#if defined _DEBUG
   void AssertValid() const;
#endif

   friend CSpanData;
   const CSpanData* m_pSpan;
};

#endif // INCLUDED_PGSEXT_GIRDERTYPES_H_
