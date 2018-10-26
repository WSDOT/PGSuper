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
#pragma once

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\PrecastSegmentData.h>
#include <PgsExt\PTData.h>
#include <WBFLCore.h>
#include <StrData.h>

class CGirderGroupData;
class CPierData2;
class CClosureJointData;
class CTimelineManager;

/*****************************************************************************
CLASS 
   CSplicedGirderData

   Utility class for spliced girder input parameters.

DESCRIPTION
   Utility class for spliced girder input parameters.
*****************************************************************************/

class PGSEXTCLASS CSplicedGirderData
{
public:
   CSplicedGirderData();
   CSplicedGirderData(const CSplicedGirderData& rOther); // makes an exact copy
   CSplicedGirderData(CGirderGroupData* pGirderGroup,GirderIndexType gdrIdx,GirderIDType gdrID,const CSplicedGirderData& rOther); // copies only data, not ID or Index
   CSplicedGirderData(CGirderGroupData* pGirderGroup);
   ~CSplicedGirderData();

   void Clear();
   
   CSplicedGirderData& operator = (const CSplicedGirderData& rOther);
   void CopySplicedGirderData(const CSplicedGirderData* pGirder);
   bool operator==(const CSplicedGirderData& rOther) const;
   bool operator!=(const CSplicedGirderData& rOther) const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   // =================================================================================
   // Methods called by the framework. Don't call these methods directly
   // =================================================================================
   void SetIndex(GirderIndexType gdrIdx);
   void SetID(GirderIDType gdrID);
   void SetGirderGroup(CGirderGroupData* pGroup);


   // =================================================================================
   // Configuration information
   // =================================================================================

   // returns the girder index
   GirderIndexType GetIndex() const;

   // returns the girder's unique identifier
   GirderIDType GetID() const;

   // returns the girder group object this girder is a part of
   CGirderGroupData* GetGirderGroup();
   const CGirderGroupData* GetGirderGroup() const;
   GroupIndexType GetGirderGroupIndex() const;

   // returns the pier object at one end of this girder
   const CPierData2* GetPier(pgsTypes::MemberEndType end) const;
   CPierData2* GetPier(pgsTypes::MemberEndType end);

   // returns the index of the pier at one end of this girder
   PierIndexType GetPierIndex(pgsTypes::MemberEndType end) const;

   // Girder name
   LPCTSTR GetGirderName() const;
   void SetGirderName(LPCTSTR strName);

   // Girder library entry
   const GirderLibraryEntry* GetGirderLibraryEntry() const;
   void SetGirderLibraryEntry(const GirderLibraryEntry* pEntry);

   // =================================================================================
   // Precast Segments
   // =================================================================================
   SegmentIndexType GetSegmentCount() const;
   CPrecastSegmentData* GetSegment(SegmentIndexType idx);
   const CPrecastSegmentData* GetSegment(SegmentIndexType idx) const;
   void SetSegment(SegmentIndexType idx,const CPrecastSegmentData& segment);
   std::vector<pgsTypes::SegmentVariationType> GetSupportedSegmentVariations() const;
   std::vector<pgsTypes::SegmentVariationType> GetSupportedSegmentVariations(const GirderLibraryEntry* pGirderLibEntry) const;


   // =================================================================================
   // Closure Joints (occuring between segments)
   // =================================================================================
   CollectionIndexType GetClosureJointCount() const;
   CClosureJointData* GetClosureJoint(CollectionIndexType idx);
   const CClosureJointData* GetClosureJoint(CollectionIndexType idx) const;
   void SetClosureJoint(CollectionIndexType idx,const CClosureJointData& closure);


   // =================================================================================
   // Post-Tensioning Data
   // =================================================================================
   const CPTData* GetPostTensioning() const;
   CPTData* GetPostTensioning();
   void SetPostTensioning(const CPTData& ptData);


   // =================================================================================
   // Load Rating Condition Factors
   // =================================================================================
   pgsTypes::ConditionFactorType GetConditionFactorType() const;
   void SetConditionFactorType(pgsTypes::ConditionFactorType conditionFactorType);

   Float64 GetConditionFactor() const;
   void SetConditionFactor(Float64 conditionFactor);

   CGirderKey GetGirderKey() const;

#if defined _DEBUG
   void AssertValid();
#endif

protected:
   void MakeCopy(const CSplicedGirderData& rOther,bool bCopyDataOnly);
   void MakeAssignment(const CSplicedGirderData& rOther);

   // called by CBridgeDescription
   void InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType face);
   void RemoveSpan(SpanIndexType spanIdx,pgsTypes::RemovePierType rmPierType);
   void JoinSegmentsAtTemporarySupport(SupportIndexType tsIdx);
   void SplitSegmentsAtTemporarySupport(SupportIndexType tsIdx);
   void JoinSegmentsAtPier(PierIndexType pierIdx);
   void SplitSegmentsAtPier(PierIndexType pierIdx);
   void SplitSegmentRight(CPrecastSegmentData* pLeftSegment,CPrecastSegmentData* pRightSegment,Float64 splitStation);

   void UpdateLinks();
   void UpdateSegments();
   void DeleteSegments();
   void DeleteClosures();
   void Resize(SegmentIndexType nSegments);

   // Call this method when a segment is being removed from a girder
   // It removes references to this segment from the timeline manager
   void RemoveSegmentFromTimelineManager(const CPrecastSegmentData* pSegment);
   void RemoveSegmentsFromTimelineManager();

   void AddSegmentToTimelineManager(const CPrecastSegmentData* pSegment,const CPrecastSegmentData* pNewSegment);

   // Call this method when a closure is being removed from a girder
   // It removes references to this closure from the timeline manager
   void RemoveClosureJointFromTimelineManager(const CClosureJointData* pClosure);
   void RemoveClosureJointsFromTimelineManager();

   // Call this method when a closure joint is created.
   void AddClosureToTimelineManager(const CClosureJointData* pClosure,EventIndexType castClosureEventIdx);

   // Initializes the girder by creating a single segment. 
   // Called by CGirderGroupData when new girders are created. 
   // In this context "new" means a 100% new girder, not a new girder that is a copy
   // of another girder.
   void Initialize();

   CTimelineManager* GetTimelineManager();

   // Girder Group Reference

   // Index of the parent group if m_pGirderGroup is undefined, otherwise INVALID_INDEX
   GroupIndexType m_GirderGroupIndex;
   // weak reference to parent group
   CGirderGroupData* m_pGirderGroup;

   std::vector<CPrecastSegmentData*> m_Segments; // owned by this object
   std::vector<CClosureJointData*> m_Closures;    // owned by this object

   std::_tstring m_GirderType;
   const GirderLibraryEntry* m_pGirderLibraryEntry;

   CPTData m_PTData;

   // for load rating
   Float64 m_ConditionFactor;
   pgsTypes::ConditionFactorType m_ConditionFactorType;

   GirderIndexType m_GirderIndex;
   GirderIDType m_GirderID;

   PierIndexType m_PierIndex[2];

   // this is a special flag that is set to true when we are creating a new girder
   // that is a copy of an existing girder. it tells MakeCopy to ignore the
   // bCopyOnlyData flag and always copy all data for closures
   bool m_bCreatingNewGirder;

   friend CBridgeDescription2;
   friend CGirderGroupData;
   friend CPierData2;
   friend CTemporarySupportData;
};
