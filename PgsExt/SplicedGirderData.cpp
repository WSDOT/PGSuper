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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\SpanData2.h>
#include <PgsExt\ClosureJointData.h>
#include <PgsExt\GirderGroupData.h>
#include <PgsExt\BridgeDescription2.h>

#include <PsgLib\GirderLibraryEntry.h>

#include <IFace\BeamFactory.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// NOTE: One of the original ideas for this class was to have adjacent segments be merged together
// when a temporary support or pier was removed from the model. The hope was to retain the girder
// profile geometry after editing. The geometry is kept intact when a closure joint is added
// to the girder, but it cannot be maintained when a closure is removed.
// 
// Some of the issues that were encountered were:
// 1) We don't have support for a general girder profile (this can be added in the future).
//    The general profile would be basically distance from left end and height parameters
//    that define the profile of the bottom of the girder (could use linear segments or curve fitting
//    to connect the points). An example of a case when a general layout is needed is merging
//    adjacent segments that are both double variation (double linear/double parabolic). The merged
//    segment would have four transition regions and the basic segment layout doesn't support this.
//
// 2) Merging of segments with a single variation with segments with a double variation can result
//    in segments that need a general segment profile. In special cases the variations would merge
//    together nicely, however the overall length of the segment must be known to compute the length 
//    of the merged prismatic or tapered length sections. The overall segment length is not known
//    at this level of the model.
//
// My suspicion is that users will generally layout the overall framing with prismatic girders and then
// modify the depth of the section as needed to accommodate the stress and strength requirements. The
// weirdness that will come from not merging adjacent segments should be minimal.

CSplicedGirderData::CSplicedGirderData()
{
   Init();

   Resize(1); 

   m_bCreatingNewGirder = false;
}

CSplicedGirderData::CSplicedGirderData(const CSplicedGirderData& rOther)
{
   Init();

   m_bCreatingNewGirder = false;
   MakeCopy(rOther,true);
}

CSplicedGirderData::CSplicedGirderData(CGirderGroupData* pGirderGroup,GirderIndexType gdrIdx,GirderIDType gdrID,const CSplicedGirderData& rOther)
{
   Init();

   m_GirderIndex                   = gdrIdx;
   m_GirderID                      = gdrID;
   m_pGirderGroup = pGirderGroup;

   m_bCreatingNewGirder = true;
   MakeCopy(rOther,true);
   m_bCreatingNewGirder = false;
}

CSplicedGirderData::CSplicedGirderData(CGirderGroupData* pGirderGroup)
{
   Init();

   m_pGirderGroup = pGirderGroup;

   Resize(1);
   m_bCreatingNewGirder = false;
}  

CSplicedGirderData::~CSplicedGirderData()
{
   DeleteSegments();
   DeleteClosures();
}

void CSplicedGirderData::Init()
{
   m_GirderIndex = INVALID_INDEX;
   m_GirderID = INVALID_ID;

   m_GirderGroupIndex = INVALID_INDEX;
   m_PierIndex[pgsTypes::metStart] = INVALID_INDEX;
   m_PierIndex[pgsTypes::metEnd] = INVALID_INDEX;

   m_PTData.SetGirder(this);

   m_pGirderGroup = nullptr;

   m_pGirderLibraryEntry = nullptr;

   m_ConditionFactor = 1.0;
   m_ConditionFactorType = pgsTypes::cfGood;

   m_TopWidthType = pgsTypes::twtSymmetric;
   m_LeftTopWidth[pgsTypes::metStart] = 0;
   m_RightTopWidth[pgsTypes::metStart] = 0;
   m_LeftTopWidth[pgsTypes::metEnd] = 0;
   m_RightTopWidth[pgsTypes::metEnd] = 0;
}

void CSplicedGirderData::Clear()
{
   RemovePTFromTimelineManager();
   RemoveSegmentsFromTimelineManager();
   RemoveClosureJointsFromTimelineManager();
}

void CSplicedGirderData::DeleteSegments()
{
   std::for_each(std::begin(m_Segments), std::end(m_Segments), [](auto* pSegment) {delete pSegment; pSegment = nullptr; });
   m_Segments.clear();
}

void CSplicedGirderData::DeleteClosures()
{
   std::for_each(std::begin(m_Closures), std::end(m_Closures), [](auto* pClosure) {delete pClosure; pClosure = nullptr; });
   m_Closures.clear();
}

void CSplicedGirderData::Resize(SegmentIndexType nSegments)
{
   SegmentIndexType nOldSegments = m_Segments.size();
   if ( nSegments < nOldSegments )
   {
      // Removing segments
      SegmentIndexType nToRemove = nOldSegments - nSegments;
      for ( SegmentIndexType i = 0; i < nToRemove; i++ )
      {
         CPrecastSegmentData* pSegment = m_Segments.back();
         delete pSegment;
         m_Segments.pop_back();

         if ( i != 0 )
         {
            CClosureJointData* pClosure = m_Closures.back();
            delete pClosure;
            m_Closures.pop_back();
         }
      }
   }
   else
   {
      // Adding segments
      SegmentIndexType nToAdd = nSegments - nOldSegments;
      for ( SegmentIndexType i = 0; i < nToAdd; i++ )
      {
         if ( m_Segments.size() != 0 )
         {
            // don't add the first closure if there aren't any segments
            CClosureJointData* pNewClosure = new CClosureJointData;
            pNewClosure->SetGirder(this);
            m_Closures.push_back(pNewClosure);
         }

         CPrecastSegmentData* pNewSegment = new CPrecastSegmentData(this);
         CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
         if ( pBridgeDesc )
         {
            pNewSegment->SetID( pBridgeDesc->GetNextSegmentID() );
         }
         m_Segments.push_back(pNewSegment);
      }
   }

#if defined _DEBUG
   if ( 1 < m_Segments.size() )
   {
      ATLASSERT(m_Segments.size()-1 == m_Closures.size());
   }
#endif
}

void CSplicedGirderData::CopyHaunchData(const CSplicedGirderData& rOther)
{
   ATLASSERT(m_Segments.size() == rOther.m_Segments.size()); // this won't end well

   SegmentIndexType nSegs = m_Segments.size();
   for (SegmentIndexType iSeg=0; iSeg<nSegs; iSeg++)
   {
      std::vector<Float64> haunches = rOther.m_Segments[iSeg]->GetDirectHaunchDepths(true);
      m_Segments[iSeg]->SetDirectHaunchDepths(haunches);
   }
}

CSplicedGirderData& CSplicedGirderData::operator= (const CSplicedGirderData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void CSplicedGirderData::CopySplicedGirderData(const CSplicedGirderData* pGirder)
{
   MakeCopy(*pGirder,true/*copy only data*/);
}

bool CSplicedGirderData::operator==(const CSplicedGirderData& rOther) const
{
   const CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
   if ( !pBridgeDesc->UseSameGirderForEntireBridge() && m_GirderType != rOther.m_GirderType )
   {
      return false;
   }

   if ( m_PTData != rOther.m_PTData )
   {
      return false;
   }

   if (pBridgeDesc->GetGirderSpacingType() == pgsTypes::sbsGeneralAdjacentWithTopWidth)
   {
      if (m_TopWidthType != rOther.m_TopWidthType)
      {
         return false;
      }

      for (int i = 0; i < 2; i++)
      {
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
         if (!IsEqual(m_LeftTopWidth[endType], rOther.m_LeftTopWidth[endType]))
         {
            return false;
         }

         if (m_TopWidthType == pgsTypes::twtAsymmetric && !IsEqual(m_RightTopWidth[endType], rOther.m_RightTopWidth[endType]))
         {
            return false;
         }
      }
   }

   CollectionIndexType nSegments = m_Segments.size();
   for ( CollectionIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      if ( *m_Segments[segIdx] != *rOther.m_Segments[segIdx] )
      {
         return false;
      }

      if ( segIdx < nSegments-1 && (*m_Closures[segIdx] != *rOther.m_Closures[segIdx]) )
      {
         return false;
      }
   }

   if ( m_ConditionFactor != rOther.m_ConditionFactor )
   {
      return false;
   }

   if ( m_ConditionFactorType != rOther.m_ConditionFactorType )
   {
      return false;
   }

   return true;
}

bool CSplicedGirderData::operator!=(const CSplicedGirderData& rOther) const
{
   return !operator==(rOther);
}

void CSplicedGirderData::MakeCopy(const CSplicedGirderData& rOther,bool bCopyDataOnly)
{
   Resize(rOther.GetSegmentCount());

   // must have the same number of segments and closure joints
   ATLASSERT(rOther.m_Segments.size() == m_Segments.size());
   ATLASSERT(rOther.m_Closures.size() == m_Closures.size());

   if ( !bCopyDataOnly )
   {
      m_GirderIndex = rOther.m_GirderIndex;
      m_GirderID    = rOther.m_GirderID;
   }

   m_GirderGroupIndex = rOther.GetGirderGroupIndex();
   m_PierIndex[pgsTypes::metStart] = rOther.GetPierIndex(pgsTypes::metStart);
   m_PierIndex[pgsTypes::metEnd]   = rOther.GetPierIndex(pgsTypes::metEnd);

   m_TopWidthType = rOther.m_TopWidthType;
   for (int i = 0; i < 2; i++)
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)(i);
      m_LeftTopWidth[endType] = rOther.m_LeftTopWidth[endType];
      m_RightTopWidth[endType] = rOther.m_RightTopWidth[endType];
   }

   m_PTData = rOther.m_PTData;

   m_GirderType          = rOther.m_GirderType;
   m_pGirderLibraryEntry = rOther.m_pGirderLibraryEntry;

   CTimelineManager* pMyTimelineMgr = GetTimelineManager();
   const CTimelineManager* pOtherTimelineMgr = GetTimelineManager();

   // create new segments
   SegmentIndexType nSegments = rOther.m_Segments.size();
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const CPrecastSegmentData* pOtherSegment = rOther.m_Segments[segIdx];
      if ( bCopyDataOnly )
      {
         m_Segments[segIdx]->CopySegmentData(pOtherSegment,true);
      }
      else
      {
         *m_Segments[segIdx] = *pOtherSegment;
      }

      if ( pMyTimelineMgr && pOtherTimelineMgr // make sure time line managers are not null
         && pMyTimelineMgr != pOtherTimelineMgr // make sure they aren't the same time line manager (it's a waste of effort if the pointers reference the same object)
         )
      {
         SegmentIDType otherSegID = pOtherSegment->GetID();
         SegmentIDType segID = m_Segments[segIdx]->GetID();
         ATLASSERT(segIdx == pOtherSegment->GetIndex());
         EventIndexType constructionEventIdx, erectionEventIdx;
         pOtherTimelineMgr->GetSegmentEvents(otherSegID,&constructionEventIdx,&erectionEventIdx);
         pMyTimelineMgr->SetSegmentEvents(segID,constructionEventIdx,erectionEventIdx);
      }

      if ( segIdx < nSegments-1 )
      {
         const CClosureJointData* pOtherClosure = rOther.m_Closures[segIdx];

         if ( m_bCreatingNewGirder )
         {
            // we are creating a new girder that is a copy of an existing girder
            // in this case, we ignore the bCopyDataOnly flag for closure joints
            // and do a full copy. we want to copy the data and the connection pointers
            // at the closure.
            *m_Closures[segIdx] = *pOtherClosure;
         }
         else
         {
            if ( bCopyDataOnly )
            {
               m_Closures[segIdx]->CopyClosureJointData(pOtherClosure);
            }
            else
            {
               *m_Closures[segIdx] = *pOtherClosure;
            }
         }

         if (!bCopyDataOnly && (pMyTimelineMgr && pOtherTimelineMgr) /*both not null*/ && (pMyTimelineMgr != pOtherTimelineMgr)/*pointers don't reference same object*/ )
         {
            ClosureIDType otherClosureID = pOtherClosure->GetID();
            ClosureIDType closureID = m_Closures[segIdx]->GetID();
            EventIndexType castClosureEventIdx = pOtherTimelineMgr->GetCastClosureJointEventIndex(otherClosureID);
            if ( castClosureEventIdx != INVALID_INDEX )
            {
               pMyTimelineMgr->SetCastClosureJointEventByIndex(closureID,castClosureEventIdx);
            }
         }
      }
   }
   UpdateLinks();    // links segments and closures
   UpdateSegments(); // sets the span pointers on the segments


   m_ConditionFactor     = rOther.m_ConditionFactor;
   m_ConditionFactorType = rOther.m_ConditionFactorType;

   PGS_ASSERT_VALID;
}


void CSplicedGirderData::MakeAssignment(const CSplicedGirderData& rOther)
{
   MakeCopy( rOther, false /*copy everything*/ );
}

HRESULT CSplicedGirderData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("Girder"),3.0);

   pStrSave->put_Property(_T("ID"),CComVariant(m_GirderID));

   CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
   if ( !pBridgeDesc->UseSameGirderForEntireBridge() )
   {
      pStrSave->put_Property(_T("GirderType"),CComVariant(m_GirderType.c_str()));
   }

   CollectionIndexType nSegments = m_Segments.size();
   pStrSave->put_Property(_T("SegmentCount"),CComVariant(nSegments));

   m_PTData.Save(pStrSave,pProgress);

   for ( CollectionIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      m_Segments[segIdx]->Save(pStrSave,pProgress);
      if ( segIdx < nSegments-1 )
      {
         m_Closures[segIdx]->Save(pStrSave,pProgress);
      }
   }

   if (pBridgeDesc->GetGirderSpacingType() == pgsTypes::sbsGeneralAdjacentWithTopWidth)
   {
      // added in version 2... changed to start/end in version 3
      pStrSave->put_Property(_T("TopWidthType"), CComVariant(m_TopWidthType));

      pStrSave->BeginUnit(_T("TopWidthStartOfSpan"), 1.0);
      pStrSave->put_Property(_T("LeftTopWidth"), CComVariant(m_LeftTopWidth[pgsTypes::metStart]));
      pStrSave->put_Property(_T("RightTopWidth"), CComVariant(m_RightTopWidth[pgsTypes::metStart]));
      pStrSave->EndUnit(); // TopWidthStartOfSpan

      pStrSave->BeginUnit(_T("TopWidthEndOfSpan"), 1.0);
      pStrSave->put_Property(_T("LeftTopWidth"), CComVariant(m_LeftTopWidth[pgsTypes::metEnd]));
      pStrSave->put_Property(_T("RightTopWidth"), CComVariant(m_RightTopWidth[pgsTypes::metEnd]));
      pStrSave->EndUnit(); // TopWidthEndOfSpan
   }

   pStrSave->put_Property(_T("ConditionFactorType"),CComVariant(m_ConditionFactorType));
   pStrSave->put_Property(_T("ConditionFactor"),CComVariant(m_ConditionFactor));

   pStrSave->EndUnit();
   return S_OK;
}

HRESULT CSplicedGirderData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;

   CHRException hr;

   DeleteSegments();
   DeleteClosures();

   try
   {
      hr = pStrLoad->BeginUnit(_T("Girder"));

      Float64 version;
      pStrLoad->get_Version(&version);

      CBridgeDescription2* pBridgeDesc = GetBridgeDescription();

      CComVariant var;
      var.vt = VT_ID;
      hr = pStrLoad->get_Property(_T("ID"),&var);
      m_GirderID = VARIANT2ID(var);

      if ( !pBridgeDesc->UseSameGirderForEntireBridge() )
      {
         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("GirderType"),&var);
         m_GirderType = OLE2T(var.bstrVal);
      }

      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("SegmentCount"),&var);
      SegmentIndexType nSegments = VARIANT2INDEX(var);

      hr = m_PTData.Load(pStrLoad,pProgress);

      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CPrecastSegmentData* pSegment = new CPrecastSegmentData;
         pSegment->SetGirder(this);
         pSegment->SetIndex(segIdx);

         if ( segIdx != 0 )
         {
            pSegment->SetClosureJoint(pgsTypes::metStart,m_Closures.back());
            m_Closures.back()->SetRightSegment(pSegment);
         }

         hr = pSegment->Load(pStrLoad,pProgress);

         ATLASSERT(pSegment->GetID() != INVALID_ID);

         m_Segments.push_back(pSegment);
         if ( segIdx < nSegments-1 )
         {
            CClosureJointData* pClosure = new CClosureJointData(this);

            pSegment->SetClosureJoint(pgsTypes::metEnd,pClosure);
            pClosure->SetLeftSegment(pSegment);

            hr = pClosure->Load(pStrLoad,pProgress);
            m_Closures.push_back(pClosure);
         }
      }

      if (1 < version && pBridgeDesc->GetGirderSpacingType() == pgsTypes::sbsGeneralAdjacentWithTopWidth)
      {
         // added in version 2.. changed to Start/End in version 3
         var.vt = VT_I8;
         hr = pStrLoad->get_Property(_T("TopWidthType"), &var);
         m_TopWidthType = (pgsTypes::TopWidthType)(var.lVal);

         if (version < 3)
         {
            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("LeftTopWidth"), &var);
            m_LeftTopWidth[pgsTypes::metStart] = var.dblVal;

            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("RightTopWidth"), &var);
            m_RightTopWidth[pgsTypes::metStart] = var.dblVal;

            m_LeftTopWidth[pgsTypes::metEnd]  = m_LeftTopWidth[pgsTypes::metStart];
            m_RightTopWidth[pgsTypes::metEnd] = m_RightTopWidth[pgsTypes::metStart];
         }
         else
         {
            hr = pStrLoad->BeginUnit(_T("TopWidthStartOfSpan"));
            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("LeftTopWidth"), &var);
            m_LeftTopWidth[pgsTypes::metStart] = var.dblVal;

            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("RightTopWidth"), &var);
            m_RightTopWidth[pgsTypes::metStart] = var.dblVal;
            hr = pStrLoad->EndUnit(); // TopWidthStartOfSpan


            hr = pStrLoad->BeginUnit(_T("TopWidthEndOfSpan"));
            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("LeftTopWidth"), &var);
            m_LeftTopWidth[pgsTypes::metEnd] = var.dblVal;

            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("RightTopWidth"), &var);
            m_RightTopWidth[pgsTypes::metEnd] = var.dblVal;
            hr = pStrLoad->EndUnit(); // TopWidthEndOfSpan
         }
      }

      var.vt = VT_I8;
      hr = pStrLoad->get_Property(_T("ConditionFactorType"),&var);
      m_ConditionFactorType = pgsTypes::ConditionFactorType(var.lVal);

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("ConditionFactor"),&var);
      m_ConditionFactor = var.dblVal;

      hr = pStrLoad->EndUnit();
   }
   catch (HRESULT)
   {
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   UpdateLinks();    // links segments and closures
   UpdateSegments(); // sets the span pointers on the segments

   return S_OK;
}

void CSplicedGirderData::SetIndex(GirderIndexType gdrIdx)
{
   m_GirderIndex = gdrIdx;
}

GirderIndexType CSplicedGirderData::GetIndex() const
{
   return m_GirderIndex;
}

void CSplicedGirderData::SetID(GirderIDType gdrID)
{
   m_GirderID = gdrID;
}

GirderIDType CSplicedGirderData::GetID() const
{
   return m_GirderID;
}

void CSplicedGirderData::SetGirderGroup(CGirderGroupData* pGirderGroup)
{
   m_pGirderGroup = pGirderGroup;

   if ( m_pGirderGroup )
   {
      UpdateSegments(); // update the span pointers for the segments
   }
   else
   {
      m_GirderGroupIndex              = INVALID_INDEX;
      m_PierIndex[pgsTypes::metStart] = INVALID_INDEX;
      m_PierIndex[pgsTypes::metEnd]   = INVALID_INDEX;
   }
}

CGirderGroupData* CSplicedGirderData::GetGirderGroup()
{
   return m_pGirderGroup;
}

const CGirderGroupData* CSplicedGirderData::GetGirderGroup() const
{
   return m_pGirderGroup;
}

GroupIndexType CSplicedGirderData::GetGirderGroupIndex() const
{
   return m_pGirderGroup ? m_pGirderGroup->GetIndex() : m_GirderGroupIndex;
}

const CPierData2* CSplicedGirderData::GetPier(pgsTypes::MemberEndType end) const
{
   return m_pGirderGroup ? m_pGirderGroup->GetPier(end) : nullptr;
}

CPierData2* CSplicedGirderData::GetPier(pgsTypes::MemberEndType end)
{
   return m_pGirderGroup ? m_pGirderGroup->GetPier(end) : nullptr;
}

PierIndexType CSplicedGirderData::GetPierIndex(pgsTypes::MemberEndType end) const
{
   return m_pGirderGroup ? m_pGirderGroup->GetPierIndex(end) : m_PierIndex[end];
}

const CPTData* CSplicedGirderData::GetPostTensioning() const
{
   return &m_PTData;
}

CPTData* CSplicedGirderData::GetPostTensioning()
{
   return &m_PTData;
}

void CSplicedGirderData::SetPostTensioning(const CPTData& ptData)
{
   m_PTData = ptData;
}

void CSplicedGirderData::UpdateLinks()
{
   // Updates the segment<->closure<->segment pointers

   SegmentIndexType nSegments = m_Segments.size();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      m_Segments[segIdx]->SetIndex(segIdx);
      m_Segments[segIdx]->SetGirder(this);

      if ( segIdx == 0 )
      {
         m_Segments[segIdx]->SetClosureJoint(pgsTypes::metStart,nullptr);
      }
      else
      {
         m_Segments[segIdx]->SetClosureJoint(pgsTypes::metStart,m_Closures[segIdx-1]);
      }

      if ( segIdx == nSegments-1 )
      {
         m_Segments[segIdx]->SetClosureJoint(pgsTypes::metEnd,nullptr);
      }
      else
      {
         m_Segments[segIdx]->SetClosureJoint(pgsTypes::metEnd,m_Closures[segIdx]);
      }

      if ( segIdx < nSegments-1 )
      {
         m_Closures[segIdx]->SetGirder(this);
         m_Closures[segIdx]->SetLeftSegment(m_Segments[segIdx]);
         m_Closures[segIdx]->SetRightSegment(m_Segments[segIdx+1]);
      }
   }

   PGS_ASSERT_VALID;
}

void CSplicedGirderData::UpdateSegments()
{
   // Assigns the span where each segment starts/ends to the segment
   CBridgeDescription2* pBridgeDesc = GetBridgeDescription();

   // If girder is not part of a group, or group is not part of a bridge, then this can't be done
   if ( pBridgeDesc == nullptr )
   {
      return;
   }

   CPierData2* pStartPier = m_pGirderGroup->GetPier(pgsTypes::metStart);
   CPierData2* pEndPier   = m_pGirderGroup->GetPier(pgsTypes::metEnd);

   CSpanData2* pStartSpan = pStartPier->GetNextSpan();
   Float64 prevSpanStart = pStartSpan->GetPrevPier()->GetStation();
   Float64 prevSpanEnd   = pStartSpan->GetNextPier()->GetStation();

   CSpanData2* pEndSpan = pStartSpan;
   Float64 nextSpanStart = prevSpanStart;
   Float64 nextSpanEnd   = prevSpanEnd;

   SegmentIndexType nSegments = m_Segments.size();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CPrecastSegmentData* pSegment = m_Segments[segIdx];
      pSegment->SetIndex(segIdx);

      CClosureJointData* pStartClosure  = pSegment->GetClosureJoint(pgsTypes::metStart);
      CClosureJointData* pEndClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);

      pSegment->ResolveReferences();
      if ( pStartClosure )
      {
         pStartClosure->ResolveReferences();
      }

      if ( pEndClosure )
      {
         pEndClosure->ResolveReferences();
      }

      Float64 segmentStartStation, segmentEndStation;
      pSegment->GetStations(&segmentStartStation,&segmentEndStation);

      // increment start span until the segment begins in the start span
      while ( !(IsLE(prevSpanStart,segmentStartStation) && (segmentStartStation < prevSpanEnd)) )
      {
         pStartSpan    = pStartSpan->GetNextPier()->GetNextSpan();
         prevSpanStart = pStartSpan->GetPrevPier()->GetStation();
         prevSpanEnd   = pStartSpan->GetNextPier()->GetStation();
      }

      // increment end span until the segment ends in the end span
      while ( !((nextSpanStart < segmentEndStation) && IsLE(segmentEndStation,nextSpanEnd)) )
      {
         pEndSpan      = pEndSpan->GetNextPier()->GetNextSpan();
         nextSpanStart = pEndSpan->GetPrevPier()->GetStation();
         nextSpanEnd   = pEndSpan->GetNextPier()->GetStation();
      }

      pSegment->SetSpans(pStartSpan,pEndSpan);

      if ( pSegment->GetID() == INVALID_ID )
      {
         pSegment->SetID(pBridgeDesc->GetNextSegmentID());
      }
   }

   PGS_ASSERT_VALID;
}

void CSplicedGirderData::SetClosureJoint(CollectionIndexType idx,const CClosureJointData& closure)
{
   *m_Closures[idx] = closure;
}

SegmentIndexType CSplicedGirderData::GetSegmentCount() const
{
   return m_Segments.size();
}

CPrecastSegmentData* CSplicedGirderData::GetSegment(SegmentIndexType idx)
{
   return m_Segments[idx];
}

const CPrecastSegmentData* CSplicedGirderData::GetSegment(SegmentIndexType idx) const
{
   return m_Segments[idx];
}

void CSplicedGirderData::SetSegment(SegmentIndexType idx,const CPrecastSegmentData& segment)
{
   m_Segments[idx]->CopySegmentData(&segment,false);
}

std::vector<pgsTypes::SegmentVariationType> CSplicedGirderData::GetSupportedSegmentVariations() const
{
   return GetSupportedSegmentVariations(m_pGirderLibraryEntry);
}

std::vector<pgsTypes::SegmentVariationType> CSplicedGirderData::GetSupportedSegmentVariations(const GirderLibraryEntry* pGirderLibEntry) const
{
   std::vector<pgsTypes::SegmentVariationType> variations;
   CComPtr<IBeamFactory> factory;
   pGirderLibEntry->GetBeamFactory(&factory);
   CComQIPtr<ISplicedBeamFactory,&IID_ISplicedBeamFactory> splicedFactory(factory);
   if ( splicedFactory )
   {
      variations = splicedFactory->GetSupportedSegmentVariations(pGirderLibEntry->IsVariableDepthSectionEnabled());
   }
   else
   {
      variations.push_back(pgsTypes::svtNone);
   }
   return variations;
}

std::vector<const CPrecastSegmentData*> CSplicedGirderData::GetSegmentsForSpan(SpanIndexType spanIdx) const
{
   std::vector<const CPrecastSegmentData*> vSegments;
   for (const auto pSegment : m_Segments)
   {
      SpanIndexType startSpanIdx = pSegment->GetSpanIndex(pgsTypes::metStart);
      SpanIndexType endSpanIdx   = pSegment->GetSpanIndex(pgsTypes::metEnd);
      if (startSpanIdx == spanIdx || endSpanIdx == spanIdx)
      {
         vSegments.push_back(pSegment);
      }
   }
   return vSegments;
}

CollectionIndexType CSplicedGirderData::GetClosureJointCount() const
{
   return m_Closures.size();
}

CClosureJointData* CSplicedGirderData::GetClosureJoint(CollectionIndexType idx)
{
   return m_Closures.size() <= idx ? nullptr : m_Closures[idx];
}

const CClosureJointData* CSplicedGirderData::GetClosureJoint(CollectionIndexType idx) const
{
   return m_Closures.size() <= idx ? nullptr : m_Closures[idx];
}

LPCTSTR CSplicedGirderData::GetGirderName() const
{
   const CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
   if ( pBridgeDesc && pBridgeDesc->UseSameGirderForEntireBridge() )
   {
      return pBridgeDesc->GetGirderName();
   }
   else
   {
      return m_GirderType.c_str();
   }
}

void CSplicedGirderData::SetGirderName(LPCTSTR strName)
{
   if ( m_GirderType != strName )
   {
      m_GirderType = strName;
      m_pGirderLibraryEntry = nullptr;
   }
}

const GirderLibraryEntry* CSplicedGirderData::GetGirderLibraryEntry() const
{
   const GirderLibraryEntry* pLibEntry = m_pGirderLibraryEntry;
   const CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
   if ( pBridgeDesc && pBridgeDesc->UseSameGirderForEntireBridge() )
   {
      pLibEntry = pBridgeDesc->GetGirderLibraryEntry();
   }

   return pLibEntry;
}

void CSplicedGirderData::SetGirderLibraryEntry(const GirderLibraryEntry* pEntry)
{
   if ( m_pGirderLibraryEntry != pEntry )
   {
      m_pGirderLibraryEntry = pEntry;

      if ( m_pGirderLibraryEntry != nullptr )
      {
         CComPtr<IBeamFactory> beamFactory;
         m_pGirderLibraryEntry->GetBeamFactory(&beamFactory);

         CComQIPtr<ISplicedBeamFactory,&IID_ISplicedBeamFactory> splicedBeamFactory(beamFactory);
         if ( splicedBeamFactory )
         {
            std::vector<pgsTypes::SegmentVariationType> variations = splicedBeamFactory->GetSupportedSegmentVariations(m_pGirderLibraryEntry->IsVariableDepthSectionEnabled());

            // need to make sure the segment variation type is consistent with the
            // types available for this girder library entry
            SegmentIndexType nSegments = GetSegmentCount();
            for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
            {
               CPrecastSegmentData* pSegment = GetSegment(segIdx);
               std::vector<pgsTypes::SegmentVariationType>::iterator found = std::find(variations.begin(),variations.end(),pSegment->GetVariationType());
               if ( found == variations.end() )
               {
                  // the current setting for the segment variation is no longer a valid
                  // value, so change it to the first available value
                  pSegment->SetVariationType(variations.front());

                  // the girder is a new type and it's height can vary. set the default height of the segment
                  // to match the height in the library entry.
                  if ( variations.front() != pgsTypes::svtNone )
                  {
                     Float64 startHeight = m_pGirderLibraryEntry->GetBeamHeight(pgsTypes::metStart);
                     Float64 endHeight   = m_pGirderLibraryEntry->GetBeamHeight(pgsTypes::metEnd);
                     pSegment->SetVariationParameters(pgsTypes::sztLeftPrismatic, 0.0,startHeight,pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztLeftPrismatic));
                     pSegment->SetVariationParameters(pgsTypes::sztRightPrismatic,0.0,endHeight,  pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztRightPrismatic));
                  }
               } // end if
            } // next segment
         } // if spliced girder
      } // if not null
   } // if lib entry different
}

void CSplicedGirderData::SetTopWidth(pgsTypes::TopWidthType type,Float64 leftStart,Float64 rightStart,Float64 leftEnd,Float64 rightEnd)
{
   m_TopWidthType = type;
   m_LeftTopWidth[pgsTypes::metStart] = leftStart;
   m_RightTopWidth[pgsTypes::metStart] = rightStart;

   m_LeftTopWidth[pgsTypes::metEnd] = leftEnd;
   m_RightTopWidth[pgsTypes::metEnd] = rightEnd;
}

void CSplicedGirderData::GetTopWidth(pgsTypes::TopWidthType* pType,Float64* pLeftStart,Float64* pRightStart,Float64* pLeftEnd,Float64* pRightEnd) const
{
   const CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
   if (pBridgeDesc && pBridgeDesc->GetGirderSpacingType() == pgsTypes::sbsUniformAdjacentWithTopWidth)
   {
      pBridgeDesc->GetGirderTopWidth(pType,pLeftStart,pRightStart);
      *pLeftEnd = *pLeftStart;
      *pRightEnd = *pRightStart;
   }
   else
   {
      *pType = m_TopWidthType;
      *pLeftStart = m_LeftTopWidth[pgsTypes::metStart];
      *pRightStart = m_RightTopWidth[pgsTypes::metStart];

      if (m_pGirderLibraryEntry)
      {
         CComPtr<IBeamFactory> beamFactory;
         m_pGirderLibraryEntry->GetBeamFactory(&beamFactory);

         if (beamFactory->CanTopWidthVary())
         {
            *pLeftEnd = m_LeftTopWidth[pgsTypes::metEnd];
            *pRightEnd = m_RightTopWidth[pgsTypes::metEnd];
         }
         else
         {
            *pLeftEnd = *pLeftStart;
            *pRightEnd = *pRightStart;
         }
      }
      else
      {
         *pLeftEnd = *pLeftStart;
         *pRightEnd = *pRightStart;
      }
   }
}

Float64 CSplicedGirderData::GetTopWidth(pgsTypes::MemberEndType endType,Float64* pLeft,Float64* pRight) const
{
   pgsTypes::TopWidthType type;
   std::array<Float64,2> left, right;
   GetTopWidth(&type, &left[pgsTypes::metStart], &right[pgsTypes::metStart], &left[pgsTypes::metEnd], &right[pgsTypes::metEnd]);

   Float64 width;
   switch (type)
   {
   case pgsTypes::twtSymmetric:
   case pgsTypes::twtCenteredCG:
      width = left[endType];
      *pLeft = width / 2;
      *pRight = *pLeft;
      break;

   case pgsTypes::twtAsymmetric:
      width = left[endType] + right[endType];
      *pLeft = left[endType];
      *pRight = right[endType];
      break;
      
   default:
      ATLASSERT(false); // should never get here... assume full
      width = left[endType];
      *pLeft = width / 2;
      *pRight = *pLeft;
      break;
   }

   return width;
}

Float64 CSplicedGirderData::GetConditionFactor() const
{
   return m_ConditionFactor;
}

void CSplicedGirderData::SetConditionFactor(Float64 conditionFactor)
{
   m_ConditionFactor = conditionFactor;
}

pgsTypes::ConditionFactorType CSplicedGirderData::GetConditionFactorType() const
{
   return m_ConditionFactorType;
}

void CSplicedGirderData::SetConditionFactorType(pgsTypes::ConditionFactorType conditionFactorType)
{
   m_ConditionFactorType = conditionFactorType;
}

void CSplicedGirderData::SetDirectHaunchDepths(const std::vector<Float64>& haunchDepths)
{
   for (auto& segment: m_Segments)
   {
      segment->SetDirectHaunchDepths(haunchDepths);
   }
}

void CSplicedGirderData::SetDirectHaunchDepths(SegmentIndexType segIdx, const std::vector<Float64>& rHaunchDepths)
{
   ATLASSERT(segIdx < m_Segments.size());
   return m_Segments[segIdx]->SetDirectHaunchDepths(rHaunchDepths);
}

std::vector<Float64> CSplicedGirderData::GetDirectHaunchDepths(SegmentIndexType segIdx,bool bGetRawValue) const
{
   ATLASSERT(segIdx < m_Segments.size());
   if (bGetRawValue)
   {
      return m_Segments[segIdx]->GetDirectHaunchDepths(bGetRawValue);
   }
   else
   {
      const CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
      pgsTypes::HaunchInputLocationType type = pBridgeDesc->GetHaunchInputLocationType();
      if (type == pgsTypes::hilSame4Bridge)
      {
         return pBridgeDesc->GetDirectHaunchDepths();
      }
      else
      {
         return m_Segments[segIdx]->GetDirectHaunchDepths(true); // Get raw value - segment is our storage
      }
   }
}

CGirderKey CSplicedGirderData::GetGirderKey() const
{
   CGirderKey girderKey(INVALID_INDEX,GetIndex());
   
   if ( m_pGirderGroup )
   {
      girderKey.groupIndex = m_pGirderGroup->GetIndex();
   }

   return girderKey;
}

void CSplicedGirderData::InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType face)
{
   UpdateSegments();
   m_PTData.InsertSpan(refPierIdx,face);
}

void CSplicedGirderData::RemoveSpan(SpanIndexType spanIdx,pgsTypes::RemovePierType rmPierType)
{
   ATLASSERT(m_pGirderGroup != nullptr); // must be part of a group to remove a span

   auto segIter = std::begin(m_Segments);
   auto segIterEnd = std::end(m_Segments);
   for (; segIter != segIterEnd; segIter++)
   {
      CPrecastSegmentData* pSegment = *segIter;
      SpanIndexType startSpanIdx = pSegment->GetSpanIndex(pgsTypes::metStart);
      SpanIndexType endSpanIdx   = pSegment->GetSpanIndex(pgsTypes::metEnd);

      if ( startSpanIdx == spanIdx && endSpanIdx == spanIdx )
      {
         // Segment starts and ends in the span that is being removed
         // Remove the segment and closures
         SegmentIndexType segIdx = pSegment->GetIndex();
         CClosureJointData* pStartClosure = pSegment->GetClosureJoint(pgsTypes::metStart);
         CClosureJointData* pEndClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);

         RemoveSegmentFromTimelineManager(pSegment);

         if ( pStartClosure )
         {
            pStartClosure->GetLeftSegment()->SetClosureJoint(pgsTypes::metEnd,nullptr);
            m_Closures[pStartClosure->GetIndex()] = nullptr;
            RemoveClosureJointFromTimelineManager(pStartClosure);
            delete pStartClosure;
         }

         if ( pEndClosure )
         {
            pEndClosure->GetRightSegment()->SetClosureJoint(pgsTypes::metStart,nullptr);
            m_Closures[pEndClosure->GetIndex()] = nullptr;
            RemoveClosureJointFromTimelineManager(pEndClosure);
            delete pEndClosure;
         }

         // Delete the segment, mark it's position in the vector with nullptr
         delete pSegment;
         *segIter = nullptr;
      }
      else if (startSpanIdx == spanIdx )
      {
         // segment starts in the span that is being removed but does not end in it
         // Make the segment start in the next span
         pSegment->SetSpan(pgsTypes::metStart,m_pGirderGroup->GetBridgeDescription()->GetSpan(spanIdx+1));
      }
      else if ( endSpanIdx == spanIdx )
      {
         // segment ends in the span that is being removed but does not start in it
         // Make the segment end in the previous span
         pSegment->SetSpan(pgsTypes::metEnd,m_pGirderGroup->GetBridgeDescription()->GetSpan(spanIdx-1));
      }
      else if (spanIdx < startSpanIdx)
      {
         // segment doesn't touch this span at all and the segments are in spans after this span
         // end the loop
         break;
      }
      //else
      //{
      //   // segment doesn't touch this span at all... do nothing
      //}
   }

   // Remove all deleted segments and resize the vector
   m_Segments.erase(std::remove(std::begin(m_Segments),std::end(m_Segments),(CPrecastSegmentData*)nullptr),std::end(m_Segments));

   // Remove all references to deleted closure joints
   m_Closures.erase(std::remove(std::begin(m_Closures),std::end(m_Closures),(CClosureJointData*)nullptr),std::end(m_Closures));

   UpdateSegments(); // causes the segment index and other things to be updated after a change

   // Remove the span from the post-tensioning definition
   m_PTData.RemoveSpan(spanIdx,rmPierType);
}

void CSplicedGirderData::JoinSegmentsAtTemporarySupport(SupportIndexType tsIdx)
{
   // Before
   //
   //                      tsIdx
   // ======================||========================||==================
   //      Segment                   Segment                 Segment
   //
   // After
   //
   // ================================================||==================
   //                     Segment                              Segment

   std::vector<CPrecastSegmentData*>::iterator segIter(m_Segments.begin());
   std::vector<CClosureJointData*>::iterator closureIter(m_Closures.begin());
   std::vector<CClosureJointData*>::iterator closureIterEnd(m_Closures.end());
   for ( ; closureIter != closureIterEnd; segIter++, closureIter++ )
   {
      CClosureJointData* pClosure = *closureIter;
      const CTemporarySupportData* pTS = pClosure->GetTemporarySupport();
      if ( pTS && pTS->GetIndex() == tsIdx )
      {
         // we found the closure that is going away
         // take the right hand segment with it
         RemoveClosureJointFromTimelineManager(pClosure);

         CPrecastSegmentData* pLeftSegment  = pClosure->GetLeftSegment();
         CPrecastSegmentData* pRightSegment = pClosure->GetRightSegment();

         // the right hand segment is going away (the segments cannot be merged)

         pLeftSegment->SetClosureJoint(pgsTypes::metEnd, pRightSegment->GetClosureJoint(pgsTypes::metEnd) );
         pLeftSegment->SetSpan(pgsTypes::metEnd,pRightSegment->GetSpan(pgsTypes::metEnd));
         
         if ( pRightSegment->GetClosureJoint(pgsTypes::metEnd) )
         {
            pRightSegment->GetClosureJoint(pgsTypes::metEnd)->SetLeftSegment(pLeftSegment);
         }

         delete pClosure;
         pClosure = nullptr;
         m_Closures.erase(closureIter);

         segIter++;
         ATLASSERT(*segIter == pRightSegment);

         // Copy the right segment's right end end block to the right end of the left segment.
         // Any end blocks at the right end of the left segment and the left end of the right segment
         // are lost (though, there should not be any)
         ATLASSERT(IsZero(pLeftSegment->EndBlockLength[pgsTypes::metEnd]));
         ATLASSERT(IsZero(pLeftSegment->EndBlockTransitionLength[pgsTypes::metEnd]));
         ATLASSERT(IsZero(pLeftSegment->EndBlockWidth[pgsTypes::metEnd]));
         ATLASSERT(IsZero(pRightSegment->EndBlockLength[pgsTypes::metStart]));
         ATLASSERT(IsZero(pRightSegment->EndBlockTransitionLength[pgsTypes::metStart]));
         ATLASSERT(IsZero(pRightSegment->EndBlockWidth[pgsTypes::metStart]));
         pLeftSegment->EndBlockLength[pgsTypes::metEnd]           = pRightSegment->EndBlockLength[pgsTypes::metEnd];
         pLeftSegment->EndBlockTransitionLength[pgsTypes::metEnd] = pRightSegment->EndBlockTransitionLength[pgsTypes::metEnd];
         pLeftSegment->EndBlockWidth[pgsTypes::metEnd]            = pRightSegment->EndBlockWidth[pgsTypes::metEnd];

         // the left and right segments have been merged into a single segment
         // now, look at the construction and erection events.... use the earliest
         // event for construction and erection. if the left segment is supported by a
         // temporary support, the combined segment cannot be erected before the temporary support
         CTimelineManager* pTimelineMgr = GetTimelineManager();
         if (pTimelineMgr)
         {
            SegmentIDType leftSegID = pLeftSegment->GetID();
            SegmentIDType rightSegID = pRightSegment->GetID();

            // construction events
            EventIndexType leftSegConstructEventIdx = pTimelineMgr->GetSegmentConstructionEventIndex(leftSegID);
            EventIndexType rightSegConstructEventIdx = pTimelineMgr->GetSegmentConstructionEventIndex(rightSegID);
            if (rightSegConstructEventIdx < leftSegConstructEventIdx)
            {
               // the right segment is constructed before the left segment, so remove the left segment from
               // its construction event and put it into the construction event for the right segment
               CTimelineEvent* pLeftSegConstructionEvent = pTimelineMgr->GetEventByIndex(leftSegConstructEventIdx);
               CTimelineEvent* pRightSegConstructionEvent = pTimelineMgr->GetEventByIndex(rightSegConstructEventIdx);
               pLeftSegConstructionEvent->GetConstructSegmentsActivity().RemoveSegment(leftSegID);
               pRightSegConstructionEvent->GetConstructSegmentsActivity().AddSegment(leftSegID);
            }


            EventIndexType leftSegErectionEventIdx = pTimelineMgr->GetSegmentErectionEventIndex(leftSegID);
            EventIndexType rightSegErectionEventIdx = pTimelineMgr->GetSegmentErectionEventIndex(rightSegID);

            EventIndexType tempSupportErectionIdx = INVALID_INDEX;
            for (int i = 0; i < 2; i++)
            {
               pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
               if (pLeftSegment->GetClosureJoint(endType))
               {
                  // the left segment is supported by a temporary support so find out when the temporary support
                  // is erected... the combined segment cannot be erected before the temporary support with the
                  // latest erection interval is erected
                  auto tsID = pLeftSegment->GetClosureJoint(endType)->GetTemporarySupportID();
                  EventIndexType erectionEventIdx, removalEventIdx;
                  pTimelineMgr->GetTempSupportEvents(tsID, &erectionEventIdx, &removalEventIdx);
                  tempSupportErectionIdx = (tempSupportErectionIdx == INVALID_INDEX ? erectionEventIdx : Max(tempSupportErectionIdx, erectionEventIdx));
               }
            }


            if (rightSegErectionEventIdx < leftSegErectionEventIdx // the right segment is erected before the left segment
               &&
               tempSupportErectionIdx <= rightSegErectionEventIdx // the right segment is erected at or after the latest erected temporary support of the left segment is erected
               )
            {
               // remove the left segment from its erection event and put it into the erection event for the right segment
               CTimelineEvent* pLeftSegErectionEvent = pTimelineMgr->GetEventByIndex(leftSegErectionEventIdx);
               CTimelineEvent* pRightSegErectionEvent = pTimelineMgr->GetEventByIndex(rightSegErectionEventIdx);
               pLeftSegErectionEvent->GetErectSegmentsActivity().RemoveSegment(leftSegID);
               pRightSegErectionEvent->GetErectSegmentsActivity().AddSegment(leftSegID);
            }
         }

         // remove the construction and erection events for the right segment from the time line
         RemoveSegmentFromTimelineManager(pRightSegment);

         delete pRightSegment; // we are done with the right segment... it no longer exists
         pRightSegment = nullptr;
         m_Segments.erase(segIter);

         break;
      }
   }

   UpdateLinks();
}

void CSplicedGirderData::SplitSegmentsAtTemporarySupport(SupportIndexType tsIdx)
{
   // Before
   //
   // ================================================||==================
   //                     Segment                              Segment
   //
   // After
   //
   // ======================||========================||==================
   //      Segment        tsIdx      Segment                   Segment


   CTemporarySupportData* pTS = m_pGirderGroup->GetBridgeDescription()->GetTemporarySupport(tsIdx);
   Float64 tsStation = pTS->GetStation();

   std::vector<CPrecastSegmentData*>::iterator segIter(m_Segments.begin());
   std::vector<CPrecastSegmentData*>::iterator segIterEnd(m_Segments.end());
   std::vector<CClosureJointData*>::iterator closureIter(m_Closures.begin());
   for ( ; segIter != segIterEnd; segIter++, closureIter++ )
   {
      CPrecastSegmentData* pSegment = *segIter;
      CClosureJointData* pStartClosure = pSegment->GetClosureJoint(pgsTypes::metStart);
      CClosureJointData* pEndClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);

      Float64 startStation, endStation;
      pSegment->GetStations(&startStation,&endStation);

      if ( startStation <= tsStation && tsStation < endStation )
      {
         // we found the segment that needs to be split
         // create a new closure and a new segment for the right hand side of the closure

         CClosureJointData* pNewClosure = new CClosureJointData(this,pTS);
         CPrecastSegmentData* pNewSegment = new CPrecastSegmentData(this);

         pNewSegment->SetID( m_pGirderGroup->GetBridgeDescription()->GetNextSegmentID() );

         pNewSegment->SetSpan(pgsTypes::metStart,pTS->GetSpan());
         pNewSegment->SetSpan(pgsTypes::metEnd,pSegment->GetSpan(pgsTypes::metEnd));

         SplitSegmentRight(pSegment,pNewSegment,tsStation);

         pNewSegment->SetClosureJoint(pgsTypes::metEnd,pSegment->GetClosureJoint(pgsTypes::metEnd));

         if ( pSegment->GetClosureJoint(pgsTypes::metEnd) )
         {
            pSegment->GetClosureJoint(pgsTypes::metEnd)->SetLeftSegment(pNewSegment);
         }

         pSegment->SetSpan(pgsTypes::metEnd,pTS->GetSpan());
         pSegment->SetClosureJoint(pgsTypes::metEnd,pNewClosure);
         pNewClosure->SetLeftSegment(pSegment);

         pNewSegment->SetClosureJoint(pgsTypes::metStart,pNewClosure);
         pNewClosure->SetRightSegment(pNewSegment);

         pNewClosure->SetTemporarySupport(pTS);

         if ( pStartClosure )
         {
            pNewClosure->SetConcrete(pStartClosure->GetConcrete());
         }
         else if ( pEndClosure )
         {
            pNewClosure->SetConcrete(pEndClosure->GetConcrete());
         }

         // use the same material for the new segment
         pNewSegment->Material = pSegment->Material;
         
         // use the same handling data for the new segment
         pNewSegment->HandlingData = pSegment->HandlingData;

         // use the same slab offsets for the new segment
         pNewSegment->m_SlabOffset[pgsTypes::metStart] = pSegment->m_SlabOffset[pgsTypes::metEnd];
         pNewSegment->m_SlabOffset[pgsTypes::metEnd] = pNewSegment->m_SlabOffset[pgsTypes::metStart];

         // don't copy strands, shear data, or longitudinal rebar data to
         // the new segment. It may not be compatible with its geometry.

         m_Segments.insert(segIter+1,pNewSegment);
         m_Closures.insert(closureIter,pNewClosure);

         UpdateLinks();

         AddSegmentToTimelineManager(pSegment,pNewSegment);

         // Find the time line event where the closure joint activity includes the temporary support where the segments are split
         CTimelineManager* pTimelineMgr = GetTimelineManager();
         ATLASSERT(pTimelineMgr);
         EventIndexType nEvents = pTimelineMgr->GetEventCount();
         EventIndexType eventIdx;
         for ( eventIdx = 0; eventIdx < nEvents; eventIdx++ )
         {
            CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);
            if ( pTimelineEvent->GetCastClosureJointActivity().IsEnabled() || pTimelineEvent->GetCastClosureJointActivity().HasTempSupport(pTS->GetID()) )
            {
               // set the closure joint casting event for the new closure joint
               pTimelineMgr->SetCastClosureJointEventByIndex(pNewClosure->GetID(),eventIdx);
               break;
            }
         }

         if ( nEvents <= eventIdx )
         {
            // event wasn't found so just use the segment erection event
            EventIndexType erectionEventIdx = pTimelineMgr->GetSegmentErectionEventIndex(pSegment->GetID());
            pTimelineMgr->SetCastClosureJointEventByIndex(pNewClosure->GetID(),erectionEventIdx);
         }

         break;
      }
   }

   PGS_ASSERT_VALID;
}

void CSplicedGirderData::JoinSegmentsAtPier(PierIndexType pierIdx)
{
   // Before
   //
   //                     pierIdx
   // ======================||========================||==================
   //      Segment                   Segment                 Segment
   //
   // After
   //
   // ================================================||==================
   //                     Segment                              Segment

   // Search for the closure joint that is being removed from the model.
   // The segments on each side of this closure need to be joined.
   std::vector<CPrecastSegmentData*>::iterator segIter(m_Segments.begin());
   std::vector<CClosureJointData*>::iterator closureIter(m_Closures.begin());
   std::vector<CClosureJointData*>::iterator closureIterEnd(m_Closures.end());
   for ( ; closureIter != closureIterEnd; segIter++, closureIter++ )
   {
      CClosureJointData* pClosure = *closureIter;
      const CPierData2* pPier = pClosure->GetPier();
      if ( pPier && pPier->GetIndex() == pierIdx )
      {
         // we found the closure that is going away
         // take the right hand segment with it (remove the right hand segment from the model)

         // this should be an interior pier and its connection should be one of the closure joint types (because it is changing from a closure joint type to a continuous type)
         ATLASSERT(pPier->IsInteriorPier());
         ATLASSERT(pPier->GetSegmentConnectionType() == pgsTypes::psctContinousClosureJoint || pPier->GetSegmentConnectionType() == pgsTypes::psctIntegralClosureJoint);

         CPrecastSegmentData* pLeftSegment  = pClosure->GetLeftSegment();
         CPrecastSegmentData* pRightSegment = pClosure->GetRightSegment();

         RemoveClosureJointFromTimelineManager(pClosure);

         // the right hand segment is going away (the segments cannot be merged)

         pLeftSegment->SetClosureJoint(pgsTypes::metEnd, pRightSegment->GetClosureJoint(pgsTypes::metEnd) );
         
         if ( pRightSegment->GetClosureJoint(pgsTypes::metEnd) )
         {
            pRightSegment->GetClosureJoint(pgsTypes::metEnd)->SetLeftSegment(pLeftSegment);
         }

         // the left segment now ends in the span where the right segment used to end
         pLeftSegment->SetSpan(pgsTypes::metEnd,pRightSegment->GetSpan(pgsTypes::metEnd));

         delete pClosure;
         m_Closures.erase(closureIter);

         segIter++;
         ATLASSERT(*segIter == pRightSegment);

         // Copy the right segment's right end end block to the right end of the left segment.
         // Any end blocks at the right end of the left segment and the left end of the right segment
         // are lost (though, there should not be any)
         ATLASSERT(IsZero(pLeftSegment->EndBlockLength[pgsTypes::metEnd]));
         ATLASSERT(IsZero(pLeftSegment->EndBlockTransitionLength[pgsTypes::metEnd]));
         ATLASSERT(IsZero(pLeftSegment->EndBlockWidth[pgsTypes::metEnd]));
         ATLASSERT(IsZero(pRightSegment->EndBlockLength[pgsTypes::metStart]));
         ATLASSERT(IsZero(pRightSegment->EndBlockTransitionLength[pgsTypes::metStart]));
         ATLASSERT(IsZero(pRightSegment->EndBlockWidth[pgsTypes::metStart]));
         pLeftSegment->EndBlockLength[pgsTypes::metEnd]           = pRightSegment->EndBlockLength[pgsTypes::metEnd];
         pLeftSegment->EndBlockTransitionLength[pgsTypes::metEnd] = pRightSegment->EndBlockTransitionLength[pgsTypes::metEnd];
         pLeftSegment->EndBlockWidth[pgsTypes::metEnd]            = pRightSegment->EndBlockWidth[pgsTypes::metEnd];

         RemoveSegmentFromTimelineManager(pRightSegment);

         delete pRightSegment;
         m_Segments.erase(segIter);

         break;
      }
   }

   UpdateLinks();
}

void CSplicedGirderData::SplitSegmentsAtPier(PierIndexType pierIdx)
{
   // Before
   //
   // ================================================||==================
   //                     Segment                              Segment
   //
   // After
   //
   // ======================||========================||==================
   //      Segment      pierIdx      Segment                   Segment


   CPierData2* pPier = m_pGirderGroup->GetBridgeDescription()->GetPier(pierIdx);
   Float64 pierStation = pPier->GetStation();

   // Search for segment that needs to be split
   std::vector<CPrecastSegmentData*>::iterator segIter(m_Segments.begin());
   std::vector<CPrecastSegmentData*>::iterator segIterEnd(m_Segments.end());
   std::vector<CClosureJointData*>::iterator closureIter(m_Closures.begin());
   for ( ; segIter != segIterEnd; segIter++, closureIter++ )
   {
      CPrecastSegmentData* pSegment = *segIter;
      CClosureJointData* pStartClosure = pSegment->GetClosureJoint(pgsTypes::metStart);
      CClosureJointData* pEndClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);

      Float64 startStation, endStation;
      pSegment->GetStations(&startStation,&endStation);

      if ( startStation <= pierStation && pierStation < endStation )
      {
         // we found the segment that needs to be split
         // create a new closure and a new segment for the right hand side of the closure

         CClosureJointData* pNewClosure = new CClosureJointData(this,pPier);
         CPrecastSegmentData* pNewSegment = new CPrecastSegmentData(this);
         pNewSegment->SetID( m_pGirderGroup->GetBridgeDescription()->GetNextSegmentID() );
         pNewSegment->SetSpan(pgsTypes::metStart,pPier->GetNextSpan());
         pNewSegment->SetSpan(pgsTypes::metEnd,pSegment->GetSpan(pgsTypes::metEnd));

         SplitSegmentRight(pSegment,pNewSegment,pierStation);

         pNewSegment->SetClosureJoint(pgsTypes::metEnd,pSegment->GetClosureJoint(pgsTypes::metEnd));

         if ( pSegment->GetClosureJoint(pgsTypes::metEnd) )
         {
            pSegment->GetClosureJoint(pgsTypes::metEnd)->SetLeftSegment(pNewSegment);
         }

         pSegment->SetSpan(pgsTypes::metEnd,pPier->GetPrevSpan());
         pSegment->SetClosureJoint(pgsTypes::metEnd,pNewClosure);
         pNewClosure->SetLeftSegment(pSegment);

         pNewSegment->SetClosureJoint(pgsTypes::metStart,pNewClosure);
         pNewClosure->SetRightSegment(pNewSegment);

         pNewClosure->SetPier(pPier);

         // use the same material for the new segment
         pNewSegment->Material = pSegment->Material;

         // use the same handling data for the new segment
         pNewSegment->HandlingData = pSegment->HandlingData;

         m_Segments.insert(segIter+1,pNewSegment);
         m_Closures.insert(closureIter,pNewClosure);


         AddSegmentToTimelineManager(pSegment,pNewSegment);

         CTimelineManager* pTimelineMgr = GetTimelineManager();
         ATLASSERT(pTimelineMgr);

         // Find the evnet where the closure joint activity includes the temporary support where the segments are split
         EventIndexType nEvents = pTimelineMgr->GetEventCount();
         EventIndexType eventIdx;
         for ( eventIdx = 0; eventIdx < nEvents; eventIdx++ )
         {
            CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);
            if ( pTimelineEvent->GetCastClosureJointActivity().IsEnabled() || pTimelineEvent->GetCastClosureJointActivity().HasPier(pPier->GetID()) )
            {
               // set the closure joint casting event for the new closure joint
               pTimelineMgr->SetCastClosureJointEventByIndex(pNewClosure->GetID(),eventIdx);
               break;
            }
         }

         if ( nEvents <= eventIdx )
         {
            // event wasn't found so just use the segment erection event
            EventIndexType erectionEventIdx = pTimelineMgr->GetSegmentErectionEventIndex(pSegment->GetID());
            pTimelineMgr->SetCastClosureJointEventByIndex(pNewSegment->GetID(),erectionEventIdx);
         }

         break;
      }
   }

   UpdateLinks();
}

void CSplicedGirderData::SplitSegmentRight(CPrecastSegmentData* pLeftSegment,CPrecastSegmentData* pRightSegment,Float64 splitStation)
{
   // split off the right side of pLeftSegment and apply it to pRightSegment
   const CSplicedGirderData* pGirder = pLeftSegment->GetGirder();
   const CGirderGroupData* pGroup = pGirder->GetGirderGroup();

   Float64 startStation,endStation;
   pLeftSegment->GetStations(&startStation,&endStation);
   Float64 leftSegmentLength = endStation - startStation;

   Float64 newLeftSegmentLength = (splitStation-startStation);

   pgsTypes::SegmentVariationType leftVariation  = pLeftSegment->GetVariationType();
   if ( leftVariation == pgsTypes::svtLinear || leftVariation == pgsTypes::svtParabolic )
   {
      Float64 leftPrismaticLength, rightPrismaticLength;
      Float64 leftHeight,rightHeight;
      Float64 leftBottomFlangeDepth,rightBottomFlangeDepth;
      pLeftSegment->GetVariationParameters(pgsTypes::sztLeftPrismatic, true,&leftPrismaticLength, &leftHeight,&leftBottomFlangeDepth);
      pLeftSegment->GetVariationParameters(pgsTypes::sztRightPrismatic,true,&rightPrismaticLength,&rightHeight,&rightBottomFlangeDepth);

      Float64 splitFraction = (splitStation - startStation)/leftSegmentLength;
      Float64 leftPrismaticFraction  = leftPrismaticLength/leftSegmentLength;
      Float64 rightPrismaticFraction = (leftSegmentLength-rightPrismaticLength)/leftSegmentLength;

      if ( ::InRange(0.0,splitFraction,leftPrismaticFraction) )
      {
         // split in left prismatic section
         
         // left segment is prismatic with the properties of the left prismatic segment
         // right segment is linear
         pRightSegment->SetVariationType(leftVariation);
         for ( int i = 0; i < 4; i++ )
         {
            pgsTypes::SegmentZoneType zone = pgsTypes::SegmentZoneType(i);
            if ( i == 0 )
            {
               // the entire left segment is reduced to just the left prismatic length
               // make the left prismatic zone of the left segment equal to the new left segment length
               pLeftSegment->SetVariationParameters(zone,newLeftSegmentLength,leftHeight,leftBottomFlangeDepth);

               // the left prismatic zone of the right segment has the length of the old left segment less the new left segment
               pRightSegment->SetVariationParameters(zone,leftSegmentLength-newLeftSegmentLength,leftHeight,leftBottomFlangeDepth);
            }
            else
            {
               // for all other zones, copy the left segment parameters to the right segment
               // and make the left zones zero length
               Float64 length,height,bottomFlangeDepth;
               pLeftSegment->GetVariationParameters(zone,true,&length,&height,&bottomFlangeDepth);
               pLeftSegment->SetVariationParameters(zone,0.0,leftHeight,leftBottomFlangeDepth);
               pRightSegment->SetVariationParameters(zone,length,height,bottomFlangeDepth);
            }
         }
      }
      else if ( ::InRange(rightPrismaticFraction,splitFraction,1.0) )
      {
         // split in right prismatic section

         // shorten the length of the right prismatic zone of the left segment and make the right segment
         // have the shortened length
         Float64 length,height,bottomFlangeDepth;
         pLeftSegment->GetVariationParameters(pgsTypes::sztRightPrismatic,true,&length,&height,&bottomFlangeDepth);
         Float64 delta_length = leftSegmentLength - newLeftSegmentLength;
         pLeftSegment->SetVariationParameters(pgsTypes::sztRightPrismatic,length - delta_length,height,bottomFlangeDepth);
         pRightSegment->SetVariationParameters(pgsTypes::sztLeftPrismatic,delta_length,height,bottomFlangeDepth);
         pRightSegment->SetVariationParameters(pgsTypes::sztLeftTapered,0,height,bottomFlangeDepth);
         pRightSegment->SetVariationParameters(pgsTypes::sztRightTapered,0,height,bottomFlangeDepth);
         pRightSegment->SetVariationParameters(pgsTypes::sztRightPrismatic,0,height,bottomFlangeDepth);
      }
      else
      {
         // split in tapered section

         // find height and bottom flange depth where the segment is split
         Float64 commonHeight = ::LinInterp(newLeftSegmentLength,leftHeight,rightHeight,leftSegmentLength - leftPrismaticLength - rightPrismaticLength);
         Float64 commonBottomFlangeDepth = ::LinInterp(newLeftSegmentLength,leftBottomFlangeDepth,rightBottomFlangeDepth,leftSegmentLength - leftPrismaticLength - rightPrismaticLength);

         // left segment right prismatic section has no length and uses the common values
         pLeftSegment->SetVariationParameters(pgsTypes::sztRightPrismatic,0,commonHeight,commonBottomFlangeDepth);

         // right segment left prismatic has not length and uses common values
         pRightSegment->SetVariationType(leftVariation);
         pRightSegment->SetVariationParameters(pgsTypes::sztLeftPrismatic,0,commonHeight,commonBottomFlangeDepth);

         // right segment right prismatic takes on the values of the old left segment right prismatic zone
         pRightSegment->SetVariationParameters(pgsTypes::sztRightPrismatic,rightPrismaticLength,rightHeight,rightBottomFlangeDepth);
      }
   }
   else if ( leftVariation == pgsTypes::svtDoubleLinear || leftVariation == pgsTypes::svtDoubleParabolic )
   {
      Float64 leftPrismaticLength,leftTaperedLength,rightTaperedLength,rightPrismaticLength;
      Float64 leftPrismaticHeight,leftTaperedHeight,rightTaperedHeight,rightPrismaticHeight;
      Float64 leftPrismaticBottomFlangeDepth,leftTaperedBottomFlangeDepth,rightTaperedBottomFlangeDepth,rightPrismaticBottomFlangeDepth;
      pLeftSegment->GetVariationParameters(pgsTypes::sztLeftPrismatic, true,&leftPrismaticLength, &leftPrismaticHeight, &leftPrismaticBottomFlangeDepth);
      pLeftSegment->GetVariationParameters(pgsTypes::sztLeftTapered,   true,&leftTaperedLength,   &leftTaperedHeight,   &leftTaperedBottomFlangeDepth);
      pLeftSegment->GetVariationParameters(pgsTypes::sztRightTapered,  true,&rightTaperedLength,  &rightTaperedHeight,  &rightTaperedBottomFlangeDepth);
      pLeftSegment->GetVariationParameters(pgsTypes::sztRightPrismatic,true,&rightPrismaticLength,&rightPrismaticHeight,&rightPrismaticBottomFlangeDepth);

      Float64 splitFraction = (splitStation - startStation)/leftSegmentLength;
      Float64 leftPrismaticFraction  = leftPrismaticLength/leftSegmentLength;
      Float64 leftTaperedFraction    = (leftPrismaticLength+leftTaperedLength)/leftSegmentLength;
      Float64 rightTaperedFraction   = (leftSegmentLength-rightPrismaticLength-rightTaperedLength)/leftSegmentLength;
      Float64 rightPrismaticFraction = (leftSegmentLength-rightPrismaticLength)/leftSegmentLength;
      if ( ::InRange(0.0,splitFraction,leftPrismaticFraction) )
      {
         // split in left prismatic zone

         // left segment is constant depth linear segment
         pLeftSegment->SetVariationType(pgsTypes::svtLinear);
         pLeftSegment->SetVariationParameters(pgsTypes::sztLeftPrismatic,newLeftSegmentLength,leftPrismaticHeight,leftPrismaticBottomFlangeDepth);
         pLeftSegment->SetVariationParameters(pgsTypes::sztRightPrismatic,0,leftPrismaticHeight,leftPrismaticBottomFlangeDepth);

         // right is Float64 linear/parabolic
         pRightSegment->SetVariationType(leftVariation);
         pRightSegment->SetVariationParameters(pgsTypes::sztLeftPrismatic,leftPrismaticLength-newLeftSegmentLength,leftPrismaticHeight,leftPrismaticBottomFlangeDepth);
         pRightSegment->SetVariationParameters(pgsTypes::sztLeftTapered,leftTaperedLength,leftTaperedHeight,leftTaperedBottomFlangeDepth);
         pRightSegment->SetVariationParameters(pgsTypes::sztRightTapered,rightTaperedLength,rightTaperedHeight,rightTaperedBottomFlangeDepth);
         pRightSegment->SetVariationParameters(pgsTypes::sztRightPrismatic,rightPrismaticLength,rightPrismaticHeight,rightPrismaticBottomFlangeDepth);
      }
      else if ( ::InRange(leftPrismaticFraction,splitFraction,leftTaperedFraction) )
      {
         // split in left tapered zone

         // find height and bottom flange depth where the segment is split
         Float64 commonHeight = ::LinInterp(newLeftSegmentLength-leftPrismaticLength,leftPrismaticHeight,leftTaperedHeight,leftTaperedLength);
         Float64 commonBottomFlangeDepth = ::LinInterp(newLeftSegmentLength-leftPrismaticLength,leftPrismaticBottomFlangeDepth,leftTaperedBottomFlangeDepth,leftTaperedLength);

         // left segment becomes single linear/parabolic
         pLeftSegment->SetVariationType(leftVariation == pgsTypes::svtDoubleLinear ? pgsTypes::svtLinear : pgsTypes::svtParabolic);
         pLeftSegment->SetVariationParameters(pgsTypes::sztRightPrismatic,0.0,commonHeight,commonBottomFlangeDepth);

         // right segment becomes Float64 linear/parabolic
         pRightSegment->SetVariationType(leftVariation);
         pRightSegment->SetVariationParameters(pgsTypes::sztLeftPrismatic,0.0,commonHeight,commonBottomFlangeDepth);
         pRightSegment->SetVariationParameters(pgsTypes::sztLeftTapered,leftPrismaticLength + leftTaperedLength - newLeftSegmentLength,leftTaperedHeight,leftTaperedBottomFlangeDepth);
         pRightSegment->SetVariationParameters(pgsTypes::sztRightTapered,rightTaperedLength,rightTaperedHeight,rightTaperedBottomFlangeDepth);
         pRightSegment->SetVariationParameters(pgsTypes::sztRightPrismatic,rightPrismaticLength,rightPrismaticHeight,rightPrismaticBottomFlangeDepth);
      }
      else if ( ::InRange(rightTaperedFraction,splitFraction,rightPrismaticFraction) )
      {
         // split in right tapered zone

         // find height and bottom flange depth where the segment is split
         Float64 centralLength = leftSegmentLength - leftPrismaticLength - leftTaperedLength - rightTaperedLength - rightPrismaticLength;
         Float64 commonHeight = ::LinInterp(newLeftSegmentLength-leftPrismaticLength-leftTaperedLength-centralLength,rightTaperedHeight,rightPrismaticHeight,rightTaperedLength);
         Float64 commonBottomFlangeDepth = ::LinInterp(newLeftSegmentLength-leftPrismaticLength-leftTaperedLength-centralLength,rightTaperedBottomFlangeDepth,rightPrismaticBottomFlangeDepth,rightTaperedLength);

         pLeftSegment->SetVariationParameters(pgsTypes::sztRightTapered,newLeftSegmentLength-leftPrismaticLength-leftTaperedLength-centralLength,rightTaperedHeight,rightTaperedBottomFlangeDepth);
         pLeftSegment->SetVariationParameters(pgsTypes::sztRightPrismatic,0.0,commonHeight,commonBottomFlangeDepth);

         pRightSegment->SetVariationType(leftVariation == pgsTypes::svtDoubleLinear ? pgsTypes::svtLinear : pgsTypes::svtParabolic);
         pRightSegment->SetVariationParameters(pgsTypes::sztLeftPrismatic,0.0,commonHeight,commonBottomFlangeDepth);
         pRightSegment->SetVariationParameters(pgsTypes::sztRightPrismatic,rightPrismaticLength,rightPrismaticHeight,rightPrismaticBottomFlangeDepth);
      }
      else if ( ::InRange(rightPrismaticFraction,splitFraction,1.0) )
      {
         // split in right prismatic zone

         // left segment remains the same except the right prismatic length is shortened
         pLeftSegment->SetVariationParameters(pgsTypes::sztRightPrismatic,rightPrismaticLength - (leftSegmentLength-newLeftSegmentLength),rightPrismaticHeight,rightPrismaticBottomFlangeDepth);
         
         // right segment is constant depth linear segment
         pRightSegment->SetVariationType(pgsTypes::svtLinear);
         pRightSegment->SetVariationParameters(pgsTypes::sztLeftPrismatic,leftSegmentLength-newLeftSegmentLength,rightPrismaticHeight,rightPrismaticBottomFlangeDepth);
         pRightSegment->SetVariationParameters(pgsTypes::sztRightPrismatic,0,rightPrismaticHeight,rightPrismaticBottomFlangeDepth);
      }
      else
      {
         // split in central linear zone (between left and right tapers)

         // find height and bottom flange depth where the segment is split
         Float64 commonHeight = ::LinInterp(newLeftSegmentLength,leftTaperedHeight,rightTaperedHeight,leftSegmentLength - leftPrismaticLength - leftTaperedLength - rightTaperedLength - rightPrismaticLength);
         Float64 commonBottomFlangeDepth = ::LinInterp(newLeftSegmentLength,leftTaperedBottomFlangeDepth,rightTaperedBottomFlangeDepth,leftSegmentLength - leftPrismaticLength - leftTaperedLength - rightTaperedLength - rightPrismaticLength);

         pLeftSegment->SetVariationType(leftVariation == pgsTypes::svtDoubleLinear ? pgsTypes::svtLinear : pgsTypes::svtParabolic);
         pLeftSegment->SetVariationParameters(pgsTypes::sztRightPrismatic,newLeftSegmentLength-leftPrismaticLength-leftTaperedLength,commonHeight,commonBottomFlangeDepth);

         pRightSegment->SetVariationType(leftVariation == pgsTypes::svtDoubleLinear ? pgsTypes::svtLinear : pgsTypes::svtParabolic);
         pRightSegment->SetVariationParameters(pgsTypes::sztLeftPrismatic,leftSegmentLength - newLeftSegmentLength-rightPrismaticLength-rightTaperedLength,commonHeight,commonBottomFlangeDepth);
         pRightSegment->SetVariationParameters(pgsTypes::sztRightPrismatic,rightPrismaticLength,rightPrismaticHeight,rightPrismaticBottomFlangeDepth);
      }
   }
   else if ( leftVariation == pgsTypes::svtNone )
   {
      // No variation so nothing to do
      pRightSegment->SetVariationType(pgsTypes::svtNone);
   }
   else
   {
#pragma Reminder("UPDATE: need to complete this")
      // General
      ATLASSERT(false); // general variation isn't supported yet
   }

#pragma Reminder("UPDATE: need to split end block data")
   // Case 1: Split is in middle of segment (not in an end block)
   //   Copy right end block of left segment to right end of right segment
   // 
   // Case 2: Split is in left end block
   //   ???
   //   Portion of end block stays in left segment and portion is at left end of right segment
   //   Right end end block is copied to right end of right segment
   //
   // Case 3: Split is in left end block transition
   //   ??? Same as case 2 except for the shape of the end block
   // 
   // Case 4: Split is in right end block transition
   //   ??? Similar to case 3
   //
   // Case 5: Split is in the right end block
   //   ??? Similar to case 4

   PGS_ASSERT_VALID;
}

void CSplicedGirderData::AddSegmentToTimelineManager(const CPrecastSegmentData* pSegment,const CPrecastSegmentData* pNewSegment)
{
   CTimelineManager* pTimelineMgr = GetTimelineManager();
   ATLASSERT(pTimelineMgr);

   EventIndexType constructionEventIdx = pTimelineMgr->GetSegmentConstructionEventIndex(pSegment->GetID());
   pTimelineMgr->SetSegmentConstructionEventByIndex(pNewSegment->GetID(),constructionEventIdx);

   EventIndexType erectionEventIdx = pTimelineMgr->GetSegmentErectionEventIndex(pSegment->GetID());
   pTimelineMgr->SetSegmentErectionEventByIndex(pNewSegment->GetID(),erectionEventIdx);
}

void CSplicedGirderData::RemoveSegmentFromTimelineManager(const CPrecastSegmentData* pSegment)
{
   CTimelineManager* pTimelineMgr = GetTimelineManager();
   if ( pTimelineMgr )
   {
      SegmentIDType segID = pSegment->GetID();

      EventIndexType constructEventIdx = pTimelineMgr->GetSegmentConstructionEventIndex( segID );
      if ( constructEventIdx != INVALID_INDEX )
      {
         CTimelineEvent* pConstructionEvent = pTimelineMgr->GetEventByIndex(constructEventIdx);
         pConstructionEvent->GetConstructSegmentsActivity().RemoveSegment( segID );
      }
      
      EventIndexType erectionEventIdx = pTimelineMgr->GetSegmentErectionEventIndex( segID );
      if ( erectionEventIdx != INVALID_INDEX )
      {
         CTimelineEvent* pErectionEvent = pTimelineMgr->GetEventByIndex(erectionEventIdx);
         pErectionEvent->GetErectSegmentsActivity().RemoveSegment( segID );
      }
   }
}

void CSplicedGirderData::RemoveSegmentsFromTimelineManager()
{
   std::for_each(std::begin(m_Segments), std::end(m_Segments), [&](auto* pSegment) {RemoveSegmentFromTimelineManager(pSegment); });
}

void CSplicedGirderData::RemoveClosureJointFromTimelineManager(const CClosureJointData* pClosure)
{
   if ( m_pGirderGroup && m_pGirderGroup->GetGirderCount() == 0 )
   {
      // Any particular closure joint is cast for all girders at the same time. Even though we
      // model every closure joint individually in the physical model, they are cast at the same time.
      // For example, the closure between segment 2 and 3 for all girders is cast at the same time.
      // The closure between segment 3 and 4 for all girders is also cast at the same time, but not
      // necessarily at the same time as the closure between segments 2 and 3.
      //
      // For this reason, we only remove the closure casting activity from the time line when 
      // the girder count is zero

      CTimelineManager* pTimelineMgr = GetTimelineManager();
      if ( pTimelineMgr )
      {
         EventIndexType eventIdx = pTimelineMgr->GetCastClosureJointEventIndex(pClosure);
         if ( eventIdx != INVALID_INDEX )
         {
            CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);

            if ( pClosure->GetPier() )
            {
               pTimelineEvent->GetCastClosureJointActivity().RemovePier(pClosure->GetPier()->GetID());
            }

            if ( pClosure->GetTemporarySupport() )
            {
               pTimelineEvent->GetCastClosureJointActivity().RemoveTempSupport(pClosure->GetTemporarySupport()->GetID());
            }
         }
      }
   }
}

void CSplicedGirderData::RemoveClosureJointsFromTimelineManager()
{
   std::for_each(std::begin(m_Closures), std::end(m_Closures), [&](auto* pClosure) {RemoveClosureJointFromTimelineManager(pClosure); });
}

void CSplicedGirderData::AddClosureToTimelineManager(const CClosureJointData* pClosure,EventIndexType castClosureEventIdx)
{
   CTimelineManager* pTimelineMgr = GetTimelineManager();
   ATLASSERT(pTimelineMgr);

   IDType closureID = pClosure->GetID();

   pTimelineMgr->SetCastClosureJointEventByIndex(closureID,castClosureEventIdx);
}

void CSplicedGirderData::RemovePTFromTimelineManager()
{
   m_PTData.RemoveDucts();
}

void CSplicedGirderData::Initialize()
{
   DeleteClosures();
   DeleteSegments();

   // this should be a new girder with no segments (a default girder, not one created with the copy constructor)
   ATLASSERT(m_Segments.size() == 0);
   ATLASSERT(m_pGirderGroup->GetBridgeDescription() != nullptr);// add girder to its group before initializing

   CPrecastSegmentData* pSegment = new CPrecastSegmentData;
   pSegment->SetGirder(this);
   pSegment->SetIndex(0);
   pSegment->SetSpans(GetPier(pgsTypes::metStart)->GetNextSpan(),GetPier(pgsTypes::metEnd)->GetPrevSpan());
   pSegment->SetID(m_pGirderGroup->GetBridgeDescription()->GetNextSegmentID());

   m_Segments.push_back(pSegment);
}

CBridgeDescription2* CSplicedGirderData::GetBridgeDescription()
{
   return m_pGirderGroup ? m_pGirderGroup->GetBridgeDescription() : nullptr;
}

const CBridgeDescription2* CSplicedGirderData::GetBridgeDescription() const
{
   return m_pGirderGroup ? m_pGirderGroup->GetBridgeDescription() : nullptr;
}

CTimelineManager* CSplicedGirderData::GetTimelineManager()
{
   CBridgeDescription2* pBridgeDesc = GetBridgeDescription();
   return pBridgeDesc ? pBridgeDesc->GetTimelineManager() : nullptr;
}

#if defined _DEBUG
void CSplicedGirderData::AssertValid()
{
   if ( m_pGirderGroup == nullptr )
   {
      return;
   }

   for(auto* pSegment : m_Segments)
   {
      _ASSERT(pSegment->GetGirder() == this);
      pSegment->AssertValid();

      _ASSERT(pSegment->GetIndex() != INVALID_INDEX);

      const CSpanData2* pStartSpan = pSegment->GetSpan(pgsTypes::metStart);
      _ASSERT( pStartSpan ? pStartSpan->GetBridgeDescription() == m_pGirderGroup->GetBridgeDescription() : true );

      const CSpanData2* pEndSpan = pSegment->GetSpan(pgsTypes::metEnd);
      _ASSERT( pEndSpan ? pEndSpan->GetBridgeDescription() == m_pGirderGroup->GetBridgeDescription() : true );

      // segment starts/end in same span or ends in a later span
      _ASSERT( pSegment->GetSpanIndex(pgsTypes::metStart) <= pSegment->GetSpanIndex(pgsTypes::metEnd) );

      // start location of segment must be same as or after the start location of the segment's start span
      // and end location of segment must be same as or before the end location of the segment's end span
      if ( m_pGirderGroup->GetBridgeDescription() != nullptr )
      {
         _ASSERT(pSegment->GetID() != INVALID_ID); // must have a valid ID if we are ultimately part of a bridge

         Float64 segStartLoc, segEndLoc;
         pSegment->GetStations(&segStartLoc,&segEndLoc);
         if ( pStartSpan )
         {
            _ASSERT( ::IsLE(pStartSpan->GetPrevPier()->GetStation(),segStartLoc) );
         }

         if ( pEndSpan )
         {
            _ASSERT( ::IsLE(segEndLoc,pEndSpan->GetNextPier()->GetStation()) );
         }
      }

      CClosureJointData* pStartClosure  = pSegment->GetClosureJoint(pgsTypes::metStart);
      CClosureJointData* pEndClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);

      if ( pStartClosure )
      {
         _ASSERT( pStartClosure->GetGirder() == this );
         _ASSERT( pStartClosure->GetRightSegment() == pSegment );
         pStartClosure->AssertValid();
      }

      if ( pEndClosure )
      {
         _ASSERT( pEndClosure->GetGirder() == this );
         _ASSERT( pEndClosure->GetLeftSegment() == pSegment );
         pEndClosure->AssertValid();
      }
   }

   std::vector<CClosureJointData*>::iterator closureIter(m_Closures.begin());
   std::vector<CClosureJointData*>::iterator closureIterEnd(m_Closures.end());
   for ( ; closureIter != closureIterEnd; closureIter++ )
   {
       CClosureJointData* pClosure = *closureIter;
      _ASSERT(pClosure->GetGirder() == this);

      _ASSERT(pClosure->GetID() != INVALID_ID);
      _ASSERT(pClosure->GetIndex() != INVALID_INDEX);

      CPrecastSegmentData* pLeftSegment  = pClosure->GetLeftSegment();
      CPrecastSegmentData* pRightSegment = pClosure->GetRightSegment();

      if ( pLeftSegment )
      {
         _ASSERT( pLeftSegment->GetGirder() == this );
         _ASSERT( pLeftSegment->GetClosureJoint(pgsTypes::metEnd) == pClosure );
      }

      if ( pRightSegment )
      {
         _ASSERT( pRightSegment->GetGirder() == this );
         _ASSERT( pRightSegment->GetClosureJoint(pgsTypes::metStart) == pClosure );
      }
   }
}
#endif // _DEBUG
