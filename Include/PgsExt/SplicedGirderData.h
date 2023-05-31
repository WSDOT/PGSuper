///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

   // Set/Get the top width of the girder
   // Bridge framing for some girder types are defined by their top width and joint width
   // For these types of girders, when the top width can be different for each girder,
   // this parameter applies, otherwise it is invalid.
   void SetTopWidth(pgsTypes::TopWidthType type, Float64 leftStart, Float64 rightStart,Float64 leftEnd,Float64 rightEnd);
   void GetTopWidth(pgsTypes::TopWidthType* pType,Float64* pLeftStart,Float64* pRightStart,Float64* pLeftEnd,Float64* pRightEnd) const;
   Float64 GetTopWidth(pgsTypes::MemberEndType endType,Float64* pLeft,Float64* pRight) const; // using the top width type, returns the actual top width

   // =================================================================================
   // Precast Segments
   // =================================================================================
   SegmentIndexType GetSegmentCount() const;
   CPrecastSegmentData* GetSegment(SegmentIndexType idx);
   const CPrecastSegmentData* GetSegment(SegmentIndexType idx) const;
   void SetSegment(SegmentIndexType idx,const CPrecastSegmentData& segment);
   std::vector<pgsTypes::SegmentVariationType> GetSupportedSegmentVariations() const;
   std::vector<pgsTypes::SegmentVariationType> GetSupportedSegmentVariations(const GirderLibraryEntry* pGirderLibEntry) const;
   std::vector<const CPrecastSegmentData*> GetSegmentsForSpan(SpanIndexType spanIdx) const;


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

   // =================================================================================
   // Direct Haunch Depth Used only when the parent bridge's HaunchInputDepthType==hidHaunchDirectly or hidHaunchPlusSlabDirectly,
   // and  HaunchLayoutType==hltAlongSegments, and HaunchInputLocationType==hilSame4AllGirders or hilPerEach
   // Note that data from girder 0 is used for all segments when hilSame4AllGirders
   // =================================================================================
   // Set the Haunch at a girder the same for all segments.
   void SetDirectHaunchDepths(const std::vector<Float64>& haunchDepth);

   // Set/Get the Haunch Depth at a girder for a specific segment. i.e.; 
   // Use when hilPerEach
   void SetDirectHaunchDepths(SegmentIndexType segIdx, const std::vector<Float64>& HaunchDepth);
   std::vector<Float64> GetDirectHaunchDepths(SegmentIndexType segIdx,bool bGetRawValue = false) const;

   // Haunch input data is tricky to copy - use a function to do this.
   void CopyHaunchData(const CSplicedGirderData& rOther);

   CGirderKey GetGirderKey() const;

#if defined _DEBUG
   void AssertValid();
#endif

protected:
   void Init();
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

   void RemovePTFromTimelineManager();

   // Initializes the girder by creating a single segment. 
   // Called by CGirderGroupData when new girders are created. 
   // In this context "new" means a 100% new girder, not a new girder that is a copy
   // of another girder.
   void Initialize();

   CTimelineManager* GetTimelineManager();
   CBridgeDescription2* GetBridgeDescription();
   const CBridgeDescription2* GetBridgeDescription() const;

   // Girder Group Reference

   // Index of the parent group if m_pGirderGroup is undefined, otherwise INVALID_INDEX
   GroupIndexType m_GirderGroupIndex;
   // weak reference to parent group
   CGirderGroupData* m_pGirderGroup;

   // Girder top width (array index is pgsTypes::MemberEndType)
   pgsTypes::TopWidthType m_TopWidthType;
   Float64 m_LeftTopWidth[2];
   Float64 m_RightTopWidth[2];
   
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
