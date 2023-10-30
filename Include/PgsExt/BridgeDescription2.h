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
#include <PgsExt\DeckDescription2.h>
#include <PgsExt\RailingSystem.h>
#include <PgsExt\TemporarySupportData.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\SpanData2.h>
#include <PgsExt\GirderGroupData.h>
#include <PgsExt\TimelineManager.h>
#include <PgsExt\PointLoadData.h>
#include <PgsExt\DistributedLoadData.h>
#include <PgsExt\MomentLoadData.h>
#include <PgsExt\ConcreteMaterial.h>

#include <functional>

inline void UpdateSlabOffset(CPrecastSegmentData* pSegment, void* pData)
{
   std::array < Float64, 2>* pOffsets = (std::array<Float64, 2>*)(pData);
   pSegment->SetSlabOffset((*pOffsets)[pgsTypes::metStart], (*pOffsets)[pgsTypes::metEnd]);
}


#define LOADED_OLD_BRIDGE_TYPE MAKE_HRESULT(SEVERITY_SUCCESS,FACILITY_ITF,512)

/*****************************************************************************
CLASS 
   CBridgeDescription2

   Utility class for managing the bridge description data

DESCRIPTION
   Utility class for managing the bridge description data.

   Piers, Temporary Supports, Spliced Girders, Precast Segments, and Closures Joints
   can be identified by Index or by ID. The Index of an object changes as their parent
   collections change in size, however, their IDs remain constant. This objects are
   always identified in the Timeline Manager by ID.

   Care must be taken when copying objects. In some cases it is desirable to copy an
   object's ID and Index and others it is not. Copy constructors copy only the object's
   data and leaves the ID and Index as default values. Assignment operators copy the
   entire object including ID and Index. All objects have a Copy<Object>Data method.
   These methods copy the object's data but does not alter the ID or Index.

LOG
   rab : 04.29.2008 : Created file
*****************************************************************************/

class PGSEXTCLASS CBridgeDescription2
{
public:
   CBridgeDescription2();
   CBridgeDescription2(const CBridgeDescription2& rOther);
   ~CBridgeDescription2();

   CBridgeDescription2& operator = (const CBridgeDescription2& rOther);
   bool operator==(const CBridgeDescription2& rOther) const;
   bool operator!=(const CBridgeDescription2& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   // =================================================================================
   // Girder Family
   // =================================================================================

   void SetGirderFamilyName(LPCTSTR strName);
   LPCTSTR GetGirderFamilyName() const;

   // =================================================================================
   // Timeline
   // =================================================================================

   void SetTimelineManager(const CTimelineManager* timelineMgr);
   const CTimelineManager* GetTimelineManager() const;
   CTimelineManager* GetTimelineManager();

   // =================================================================================
   // Bridge Location
   // =================================================================================

   // Set/Get the alignment offset. The alignment offset is the distance from the alignment
   // to the bridge line. The alignment offset is measured normal to the alignment. 
   // Positive values locates the bridge line to the right of the alignment
   void SetAlignmentOffset(Float64 alignmentOffset);
   Float64 GetAlignmentOffset() const;

   // =================================================================================
   // Bridge Deck and Railing System
   // =================================================================================

   CDeckDescription2* GetDeckDescription();
   const CDeckDescription2* GetDeckDescription() const;

   CRailingSystem* GetLeftRailingSystem();
   const CRailingSystem* GetLeftRailingSystem() const;

   CRailingSystem* GetRightRailingSystem();
   const CRailingSystem* GetRightRailingSystem() const;

   // Define how haunch is input. This can be older "A" dimension method or newer hidHaunchDirectly and hidHaunchPlusSlabDirectly methods
   void SetHaunchInputDepthType(pgsTypes::HaunchInputDepthType type);
   pgsTypes::HaunchInputDepthType GetHaunchInputDepthType() const;

   //// Older "A" dimension method for defining the haunch ///// - Data structures remain for this method, which is PGSuper's default method
   // set/get the slab offset type. This parameter indicates where the slab offset (aka haunch) is measured
   void SetSlabOffsetType(pgsTypes::SlabOffsetType slabOffsetType);
   pgsTypes::SlabOffsetType GetSlabOffsetType() const;

   // Set/get the slab offset. Has no net effect if slab offset type is not sotBridge
   // Get method returns invalid data if slab offset type is not sotBridge
   void SetSlabOffset(Float64 slabOffset);
   Float64 GetSlabOffset(bool bGetRawValue = false) const;

   // returns the least slab offset defined for the bridge
   Float64 GetLeastSlabOffset() const;

   // returns the minimum permissible value for slab offset
   Float64 GetMinSlabOffset() const;

   // set/get the Assumed Excess Camber type. This parameter indicates where the AssumedExcessCamber is measured
   void SetAssumedExcessCamberType(pgsTypes::AssumedExcessCamberType AssumedExcessCamberType);
   pgsTypes::AssumedExcessCamberType GetAssumedExcessCamberType() const;

   // Set/get the Assumed Excess Camber. Has no net effect if AssumedExcessCamber type is not sotBridge
   // Get method returns invalid data if AssumedExcessCamber type is not sotBridge
   void SetAssumedExcessCamber(Float64 assumedExcessCamber);
   Float64 GetAssumedExcessCamber(bool bGetRawValue = false) const;

   // Set/get the fillet. 
   void SetFillet(Float64 Fillet);
   Float64 GetFillet(bool bGetRawValue = false) const;

   // Data for multiple fillets was read and dealt with (version 3.1 of PGSuper had this)
   bool WasVersion3_1FilletRead() const;

   //// Direct input of Haunch depths (newer) /// - This method differs from PGSuper's older method using "A" dimension and assumed excess camber
   void SetHaunchInputLocationType(pgsTypes::HaunchInputLocationType type);
   pgsTypes::HaunchInputLocationType GetHaunchInputLocationType() const;

   void SetHaunchLayoutType(pgsTypes::HaunchLayoutType type);
   pgsTypes::HaunchLayoutType GetHaunchLayoutType() const;

   void SetHaunchInputDistributionType(pgsTypes::HaunchInputDistributionType type);
   pgsTypes::HaunchInputDistributionType GetHaunchInputDistributionType() const;

   // Set haunch depth as same for all segments or spans on entire bridge. Has no effect unless SetHaunchInputLocationType==hilSame4Bridge
   // Size of vector must match HaunchInputDistributionType enum val.
   void SetDirectHaunchDepths(std::vector<Float64> depths);
   std::vector<Float64> GetDirectHaunchDepths() const;

   // Get minimum allowable value for direct haunch depth input only. This accounts for deck thickness and fillet depending on HaunchInputDepthType
   Float64 GetMinimumAllowableHaunchDepth(pgsTypes::HaunchInputDepthType inputType) const;

   // =================================================================================
   // Spans and Girder Groups
   // =================================================================================

   // Use this method to create the first span in the bridge, also creates the first girder group.
   void CreateFirstSpan(const CPierData2* pFirstPier,const CSpanData2* pFirstSpan,const CPierData2* pNextPier,EventIndexType pierErectionEventIdx);

   // Appends a span to the end of the bridge.
   void AppendSpan(const CSpanData2* pSpanData,const CPierData2* pPierData,bool bCreateNewGroup,EventIndexType pierErectionEventIdx);

   // Inserts a new span at the location defined by refPierIdx and pierFace (e.g. Pier 2, Back Face)
   void InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace,Float64 spanLength,const CSpanData2* pSpanData,const CPierData2* pPierData,bool bCreateNewGroup,EventIndexType pierErectionEventIdx);

   // Removes a span and pier from the bridge.
   void RemoveSpan(SpanIndexType spanIdx,pgsTypes::RemovePierType rmPierType);

   // Creates a new girder group
   GroupIndexType CreateGirderGroup(GroupIndexType refGroupIdx,pgsTypes::MemberEndType end,std::vector<Float64> spanLengths,GirderIndexType nGirders);

   // Returns a girder group
   // to avoid conflict with the CSpanData2 version, use L after the index: GetGirderGroup(0L);
   CGirderGroupData* GetGirderGroup(GroupIndexType grpIdx);
   const CGirderGroupData* GetGirderGroup(GroupIndexType grpIdx) const;

   // Returns the girder group that contains pSpan. 
   // don't use nullptr for the span object, otherwise you will get group 0
   CGirderGroupData* GetGirderGroup(const CSpanData2* pSpan);
   const CGirderGroupData* GetGirderGroup(const CSpanData2* pSpan) const;

   CGirderGroupData* FindGirderGroup(GroupIDType grpID);
   const CGirderGroupData* FindGirderGroup(GroupIDType grpID) const;

   // Returns the number of groups
   GroupIndexType GetGirderGroupCount() const;

   // Removes a girder group and all spans, piers, and girders within the group
   void RemoveGirderGroup(GroupIndexType grpIdx,pgsTypes::RemovePierType rmPierType);

   // =================================================================================
   // Spans and Piers
   // =================================================================================
   PierIndexType GetPierCount() const;
   SpanIndexType GetSpanCount() const;

   CPierData2* GetPier(PierIndexType pierIdx);
   const CPierData2* GetPier(PierIndexType pierIdx) const;

   CPierData2* FindPier(PierIDType pierID);
   const CPierData2* FindPier(PierIDType pierID) const;
   
   CSpanData2* GetSpan(SpanIndexType spanIdx);
   const CSpanData2* GetSpan(SpanIndexType spanIdx) const;

   // Sets the length of a span, moves all piers, spans, and temporary support after this span.
   bool SetSpanLength(SpanIndexType spanIdx,Float64 newLength);

   // Moves a pier to a new location. Span lengths are adjusted according to moveOption
   bool MovePier(PierIndexType pierIdx,Float64 newStation,pgsTypes::MovePierOption moveOption);

   // Internally all bridges start at abutment(0) and end at an abutment(nspans). However, the UI allows a bridge
   // to start with abitrary pier numbering. The functions below support this capability
   void SetPierDisplaySettings(pgsTypes::DisplayEndSupportType startPierType, pgsTypes::DisplayEndSupportType endPierType, PierIndexType startPierNumber);
   void GetPierDisplaySettings(pgsTypes::DisplayEndSupportType* pStartPierType, pgsTypes::DisplayEndSupportType* pEndPierType, PierIndexType* pStartPierNumber) const;
   PierIndexType GetDisplayStartingPierNumber() const;  // pier number at start of bridge for display purposes
   void SetDisplayStartingPierNumber(PierIndexType);
   pgsTypes::DisplayEndSupportType GetDisplayStartSupportType() const; // type of support and start of bridge
   void SetDisplayStartSupportType(pgsTypes::DisplayEndSupportType);
   pgsTypes::DisplayEndSupportType GetDisplayEndSupportType() const; // type of support and end of bridge
   void SetDisplayEndSupportType(pgsTypes::DisplayEndSupportType);

   // =================================================================================
   // Temporary Supports
   // =================================================================================

   // adds a temporary support to the bridge model. returns the temporary support index based on location along the bridge
   SupportIndexType AddTemporarySupport(CTemporarySupportData* pTempSupport,EventIndexType erectionEventIdx,EventIndexType removalEventIdx,EventIndexType castClosureEventIdx);

   // returns the number of temporary supports
   SupportIndexType GetTemporarySupportCount() const;

   // Returns a temporary support
   CTemporarySupportData* GetTemporarySupport(SupportIndexType tsIdx);
   const CTemporarySupportData* GetTemporarySupport(SupportIndexType tsIdx) const;

   CTemporarySupportData* FindTemporarySupport(SupportIDType tsID);
   const CTemporarySupportData* FindTemporarySupport(SupportIDType tsID) const;

   // sets the data for a temporary support. returns the index of the temporary support 
   // Index may change if temporary support is moved (temp supports are located by station)
   SupportIndexType SetTemporarySupportByIndex(SupportIndexType tsIdx,const CTemporarySupportData& ts);

   // sets the data for a temporary support. does not change the ID, returns the index of the temporary support
   // Index may change if temporary support is moved (temp supports are located by station)
   SupportIndexType SetTemporarySupportByID(SupportIDType tsID,const CTemporarySupportData& ts);

   // removes temporary support from the bridge model
   void RemoveTemporarySupportByIndex(SupportIndexType tsIdx);
   void RemoveTemporarySupportByID(SupportIDType tsID);

   // Moves a temporary support. returns the index of the temporary support after it is moved
   // (temp supports are sorted by timeline event so its index can change)
   SupportIndexType MoveTemporarySupport(SupportIndexType tsIdx,Float64 newStation);

   // =================================================================================
   // Number of Girders
   // =================================================================================

   // Indicates if the same number of girders are used in all groups
   void UseSameNumberOfGirdersInAllGroups(bool bSame);
   bool UseSameNumberOfGirdersInAllGroups() const;

   // Sets the number of girders in all groups. Does nothing if UseSameNumberOfGirdersInAllGroups is false
   void SetGirderCount(GirderIndexType nGirders);

   // Returns the number of girders in all groups. The result is meaningless if UseSameNumberOfGirdersInAllGroups is false
   GirderIndexType GetGirderCount() const;

   // Returns the number of girders in the group with the least number of girders
   GirderIndexType GetMinGirderCount() const;

   // Returns the number of girders in the group with the most girders
   GirderIndexType GetMaxGirderCount() const;

   // =================================================================================
   // Girder Type
   // =================================================================================

   // Indicates if the same girder is used in all spans
   void UseSameGirderForEntireBridge(bool bSame);
   bool UseSameGirderForEntireBridge() const;

   // These methods are only meaningful if UseSameGirderForEntireBridge is true

   // Returns the girder name (the name from the girder library entry)
   LPCTSTR GetGirderName() const;

   // Changes the name of the girder without changing the associated library entry
   void RenameGirder(LPCTSTR strName);

   // Sets the name of the girder and updates the library entry
   void SetGirderName(LPCTSTR strName);

   // Returns the library entry for the girder in use
   const GirderLibraryEntry* GetGirderLibraryEntry() const;

   // changes the girder by changing the library entry
   void SetGirderLibraryEntry(const GirderLibraryEntry* pEntry);

   // Set/Get the girder orientation (plumb, normal to deck, etc)
   void SetGirderOrientation(pgsTypes::GirderOrientationType gdrOrientation);
   pgsTypes::GirderOrientationType GetGirderOrientation() const;

   // =================================================================================
   // Girder Spacing
   // =================================================================================

   // NOTE: for adjacent girders with fixed or user defined top widths and joints, the "spacing"
   // parameters are actually the joint widths

   // get/set type of girder spacing used
   void SetGirderSpacingType(pgsTypes::SupportedBeamSpacing sbs);
   pgsTypes::SupportedBeamSpacing GetGirderSpacingType() const;

   // get/set the girder spacing (or joint width) if the spacing is constant
   // for the entire bridge
   Float64 GetGirderSpacing() const;
   void SetGirderSpacing(Float64 spacing);

   // Defines how girder spacing (or joint width) is measured
   void SetMeasurementType(pgsTypes::MeasurementType mt);
   pgsTypes::MeasurementType GetMeasurementType() const;

   // Defines where girder spacing (or joint width) is measured
   void SetMeasurementLocation(pgsTypes::MeasurementLocation ml);
   pgsTypes::MeasurementLocation GetMeasurementLocation() const;

   // Defines where girder spacing (or joint width) is measured
   void SetWorkPointLocation(pgsTypes::WorkPointLocation ml);
   pgsTypes::WorkPointLocation GetWorkPointLocation() const;

   // Set/Get the girder top width if top width is the same
   // for the entire bridge. This parameter is invalid if the girder spacing type 
   // does not support top width (e.g. top width is only used for certain types of
   // adjacent girders where the bridge layout is defined by girder top width and
   // joint width). Top width is always measured normal to the CL of the girder.
   // Only valid if GetGirderSpacingType() returns pgsTypes::sbsUniformAdjacentWithTopWidth
   void SetGirderTopWidth(pgsTypes::TopWidthType type,Float64 left,Float64 right);
   void GetGirderTopWidth(pgsTypes::TopWidthType* pType,Float64* pLeft,Float64* pRight) const;
   Float64 GetGirderTopWidth() const;

   // set/get the reference girder index. if index is INVALID_INDEX then
   // the geometric center of the girder group is the reference point
   void SetRefGirder(GirderIndexType refGdrIdx);
   GirderIndexType GetRefGirder() const;

   // offset to the reference girder, measured from either the CL bridge or alignment
   // as indicated by RefGirderOffsetType, MeasurementType, and LocationType
   void SetRefGirderOffset(Float64 offset);
   Float64 GetRefGirderOffset() const;

   // indicated what the reference girder offset is measured relative to
   void SetRefGirderOffsetType(pgsTypes::OffsetMeasurementType offsetDatum);
   pgsTypes::OffsetMeasurementType GetRefGirderOffsetType() const;


   // =================================================================================
   // Miscellaneous
   // =================================================================================

   // Removes everything from the bridge model
   void Clear();

   // Returns the overall length of the bridge measured between the stations of the first and last pier
   Float64 GetLength() const;

   // Returns the station range for the bridge
   void GetStationRange(Float64* pStartStation,Float64* pEndStation) const;

   // Returns true if a station is between the stations of the first and last piers
   bool IsOnBridge(Float64 station) const;

   // If the specified station is at the location of a pier, the pier index is returned
   // otherwise INVALID_INDEX is returned
   PierIndexType IsPierLocation(Float64 station,Float64 tolerance = 0.001) const;

   // If the specified station is at the location of a temporary support, the support index is returned
   // otherwise INVALID_INDEX is returned
   SupportIndexType IsTemporarySupportLocation(Float64 station,Float64 tolerance = 0.001) const;

   // sets/gets how distribution factors are defined. if DirectlyInput, then
   // this bridge model has the distribution factor values (see CSpanData2)
   void SetDistributionFactorMethod(pgsTypes::DistributionFactorMethod method);
   pgsTypes::DistributionFactorMethod GetDistributionFactorMethod() const;

   CSplicedGirderData* FindGirder(GirderIDType gdrID);
   const CSplicedGirderData* FindGirder(GirderIDType gdrID) const;

   CPrecastSegmentData* FindSegment(SegmentIDType segID);
   const CPrecastSegmentData* FindSegment(SegmentIDType segID) const;

   CClosureJointData* FindClosureJoint(ClosureIDType closureID);
   const CClosureJointData* FindClosureJoint(ClosureIDType closureID) const;

   void CopyDown(bool bGirderCount,bool bGirderType,bool bSpacing,bool bSlabOffset,bool bAssumedExcessCamber, bool bDirectHaunchDepths, bool bBearingData); 
                    // takes all the data defined at the bridge level and copies
                    // it down to the spans and girders (only for this parameters set to true)

   // This function is meant to copy haunch data between COMPATIBLE copies of this data structure. 
   // Haunch data is complex and rooted throughout the bridge heirarchy. Results are unpredictable if copy is between highly different data (e.g., rOther has different number of spans).
   void CopyHaunchSettings(const CBridgeDescription2& rOther, bool bMakeDeepCopy = true);

   // Returns a vector of the valid connection types for a pier. 
   std::vector<pgsTypes::BoundaryConditionType> GetBoundaryConditionTypes(PierIndexType pierIdx) const;
   std::vector<pgsTypes::PierSegmentConnectionType> GetPierSegmentConnectionTypes(PierIndexType pierIdx) const;

   // Returns the number of closure joints on a girder line. The number of closures
   // in each girder within a group are equal. 
   IndexType GetClosureJointCount() const;

   // Returns the closure joint data
   CClosureJointData* GetClosureJoint(const CClosureKey& closureKey);
   const CClosureJointData* GetClosureJoint(const CClosureKey& closureKey) const;

   const CPrecastSegmentData* GetSegment(const CSegmentKey& segmentKey) const;
   CPrecastSegmentData* GetSegment(const CSegmentKey& segmentKey);

   // Gets the segments on either side of a temporary support... left and right segment key will be the same if segment is continuous over TS
   void GetSegmentsAtTemporarySupport(SupportIndexType tsIdx,CSegmentKey* pLeftSegmentKey,CSegmentKey* pRightSegmentKey) const;

   // Returns the approximate maximum width of the bridge. The width is computed as:
   // for bridges with decks - the maximum sum of the left right deck overhangs
   // for bridges without decks - the maximum spacing width (this width is based on the raw input and is not adjusted for skews)
   Float64 GetBridgeWidth() const;

   // set/get the Bearing type. This parameter indicates where the Bearing is measured
   void SetBearingType(pgsTypes::BearingType BearingType);
   pgsTypes::BearingType GetBearingType() const;

   // Set/get the Bearing. Has no net effect if Bearing type is not sotBridge
   // Get method returns invalid data if Bearing type is not sotBridge
   void SetBearingData(const CBearingData2& Bearing);
   const CBearingData2* GetBearingData() const;
   CBearingData2* GetBearingData();


   // Longitudinal joints (note, joint spacing is in the GirderSpacing data above)
   bool HasLongitudinalJoints() const; // has a joint with a shape
   bool HasStructuralLongitudinalJoints() const; // the joint is also structural
   const CConcreteMaterial& GetLongitudinalJointMaterial() const;
   void SetLongitudinalJointMaterial(const CConcreteMaterial& material);

   void ForEachSegment(std::function<void(CPrecastSegmentData*,void*)>& fn,void* pData);

   // Returns true if the overall bridge model is stable
   bool IsStable() const;

   bool IsValidSpan(SpanIndexType spanIdx) const;
   bool IsValidLayout() const;
   bool IsValidBridge() const;

protected:
   void MakeCopy(const CBridgeDescription2& rOther);
   void MakeAssignment(const CBridgeDescription2& rOther);

private:
   SupportIDType m_TempSupportID; // generates unique ID's for temporary supports
   void UpdateNextTemporarySupportID(SupportIDType tsID);
   SupportIDType GetNextTemporaryID(bool bIncrement = true);

   PierIDType m_PierID; // generates unique ID's for piers
   void UpdateNextPierID(PierIDType pierID);
   PierIDType GetNextPierID(bool bIncrement = true);

   GroupIDType m_GirderGroupID; // generates unique ID's for girder groups
   void UpdateNextGirderGroupID(GroupIDType grpID);
   GroupIDType GetNextGirderGroupID(bool bIncrement = true);

   SegmentIDType m_SegmentID; // generates unique ID's for precast girder segments
   void UpdateNextSegmentID(SegmentIDType segID);
   SegmentIDType GetNextSegmentID(bool bIncrement = true);

   GirderIDType m_GirderID; // generates unique ID's for spliced girders
   void UpdateNextGirderID(GirderIDType gdrID);
   GirderIDType GetNextGirderID(bool bIncrement = true);

   std::_tstring m_strGirderFamilyName;

   bool m_bSameNumberOfGirders;
   GirderIndexType m_nGirders;

   bool m_bSameGirderName;
   std::_tstring m_strGirderName;
   const GirderLibraryEntry* m_pGirderLibraryEntry;
   pgsTypes::GirderOrientationType m_GirderOrientation;

   pgsTypes::SupportedBeamSpacing m_GirderSpacingType;
   Float64 m_GirderSpacing;
   
   pgsTypes::TopWidthType m_TopWidthType; // defines how top width is defined
   Float64 m_LeftTopWidth; // full or left top flange width
   Float64 m_RightTopWidth; // right top flange width (not used for full or balanced types)

   GirderIndexType m_RefGirderIdx;
   Float64 m_RefGirderOffset;
   pgsTypes::OffsetMeasurementType m_RefGirderOffsetType;
   pgsTypes::MeasurementType m_MeasurementType;
   pgsTypes::MeasurementLocation m_MeasurementLocation;
   pgsTypes::WorkPointLocation m_WorkPointLocation;

   pgsTypes::HaunchInputDepthType m_HaunchInputDepthType;

   Float64 m_SlabOffset;
   pgsTypes::SlabOffsetType m_SlabOffsetType;

   Float64 m_Fillet; // The fillet for entire bridge
   bool m_bWasVersion3_1FilletRead; // If this is true, we need to post a status center item stating that 
                                   // data for multiple fillets was read and dealt with (version 3.1 of PGSuper had this)

   Float64 m_AssumedExcessCamber; // assummed excess camber for entire bridge
   pgsTypes::AssumedExcessCamberType m_AssumedExcessCamberType;

   pgsTypes::HaunchInputLocationType m_HaunchInputLocationType;
   pgsTypes::HaunchLayoutType m_HaunchLayoutType;
   pgsTypes::HaunchInputDistributionType m_HaunchInputDistributionType;
   std::vector<Float64> m_HaunchDepths;

   CBearingData2 m_BearingData;
   pgsTypes::BearingType m_BearingType;

   CTimelineManager m_TimelineManager;

   CDeckDescription2 m_Deck;
   CRailingSystem m_LeftRailingSystem;
   CRailingSystem m_RightRailingSystem;

   std::vector<CPierData2*> m_Piers;
   std::vector<CSpanData2*> m_Spans;
   std::vector<CTemporarySupportData*> m_TemporarySupports;
   std::vector<CGirderGroupData*> m_GirderGroups;

   pgsTypes::DisplayEndSupportType m_DisplayStartSupportType;
   pgsTypes::DisplayEndSupportType m_DisplayEndSupportType;
   PierIndexType m_StartingPierNumber;

   Float64 m_AlignmentOffset; // offset from Alignment to CL Bridge (< 0 = right of alignment)

   pgsTypes::DistributionFactorMethod m_LLDFMethod;

   CConcreteMaterial m_LongitudinalJointConcrete;

   bool MoveBridge(PierIndexType pierIdx,Float64 newStation);
   bool MoveBridgeAdjustPrevSpan(PierIndexType pierIdx,Float64 newStation);
   bool MoveBridgeAdjustNextSpan(PierIndexType pierIdx,Float64 newStation);
   bool MoveBridgeAdjustAdjacentSpans(PierIndexType pierIdx,Float64 newStation);

   void RenumberGroups();
   void RenumberSpans();
   void UpdateTemporarySupports();

   void ClearGirderGroups();

   void RemoveNegMomentRebar(PierIndexType removeRebarPierIdx);

   HRESULT LoadOldBridgeDescription(Float64 version,IStructuredLoad* pStrLoad,IProgress* pProgress);

#if defined _DEBUG
   void AssertValid();
#endif

   friend CGirderGroupData;
   friend CSplicedGirderData;
   friend CPrecastSegmentData;
   friend CSpanData2;
};
