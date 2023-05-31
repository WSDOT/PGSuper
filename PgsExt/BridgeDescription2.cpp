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
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\ClosureJointData.h>
#include <WbflAtlExt.h>
#include <PGSuperException.h>

#include <IFace\Project.h>
#include <IFace\BeamFactory.h>
#include <WBFLGenericBridge.h>

#include "BridgeDescription.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define FILE_VERSION 14
// Version 14 - added new Direct haunch depth input options
// Version 13 - added starting pier renumbering
// Version 12 - added work point location
// Version 11 - Added BearingData, and Top Width Spacing for adjacent deck beams
// Version 10 - added assumed excess camber data
// No new data was added in version 9.0. The version number was changed so we can tell the difference between files
// that have an explicity construction event (9.0 <= version) for diaphragms and those that do not (version <= 8.0)

/****************************************************************************
CLASS
   CBridgeDescription2
****************************************************************************/
CBridgeDescription2::CBridgeDescription2()
{
   m_AlignmentOffset = 0;

   m_LLDFMethod = pgsTypes::Calculated;

   m_Deck.SetBridgeDescription(this);
   m_TimelineManager.SetBridgeDescription(this);

   m_HaunchInputDepthType = pgsTypes::hidACamber; // default to old method

   m_SlabOffset     = WBFL::Units::ConvertToSysUnits( 10.0, WBFL::Units::Measure::Inch );
   m_SlabOffsetType = pgsTypes::sotBridge;

   m_Fillet     = WBFL::Units::ConvertToSysUnits( 0.75, WBFL::Units::Measure::Inch );

   m_AssumedExcessCamber = 0.0;
   m_AssumedExcessCamberType = pgsTypes::aecBridge;

   m_HaunchInputLocationType = pgsTypes::hilSame4Bridge;
   m_HaunchLayoutType = pgsTypes::hltAlongSpans;
   m_HaunchInputDistributionType = pgsTypes::hidUniform; // seems reasonable
   m_HaunchDepths.assign((int)m_HaunchInputDistributionType, m_Fillet);

   m_BearingType = pgsTypes::brtPier;

   m_pGirderLibraryEntry = nullptr;

   // Set some reasonable defaults
   m_nGirders = 0;
   m_bSameNumberOfGirders = true;
   m_GirderSpacingType = pgsTypes::sbsUniform;
   m_GirderSpacing = 0.0;
   m_GirderOrientation = pgsTypes::Plumb;

   m_TempSupportID = 0;
   m_SegmentID     = 0;
   m_PierID        = 0;
   m_GirderID      = 0;
   m_GirderGroupID = 0;

   m_MeasurementLocation = pgsTypes::AtPierLine;
   m_MeasurementType = pgsTypes::NormalToItem;

   m_WorkPointLocation = pgsTypes::wplTopGirder; // long standing program default

   m_RefGirderIdx = INVALID_INDEX;
   m_RefGirderOffset = 0;
   m_RefGirderOffsetType = pgsTypes::omtBridge;

   m_LongitudinalJointConcrete.bHasInitial = false;

   m_TopWidthType = pgsTypes::twtSymmetric;
   m_LeftTopWidth = 0;
   m_RightTopWidth = 0;
   
   m_bWasVersion3_1FilletRead = false;


   m_StartingPierNumber = 1;
   m_DisplayStartSupportType = pgsTypes::desAbutment;
   m_DisplayEndSupportType = pgsTypes::desAbutment;
}

CBridgeDescription2::CBridgeDescription2(const CBridgeDescription2& rOther)
{
   m_AlignmentOffset = 0;

   m_LLDFMethod = pgsTypes::Calculated;

   m_Deck.SetBridgeDescription(this);
   m_TimelineManager.SetBridgeDescription(this);

   m_SlabOffset     = WBFL::Units::ConvertToSysUnits( 10.0, WBFL::Units::Measure::Inch );
   m_SlabOffsetType = pgsTypes::sotBridge;

   m_Fillet     = WBFL::Units::ConvertToSysUnits( 0.75, WBFL::Units::Measure::Inch );

   m_pGirderLibraryEntry = nullptr;

   m_nGirders = 0;

   m_TempSupportID = 0;
   m_SegmentID     = 0;
   m_PierID        = 0;
   m_GirderID      = 0;
   m_GirderGroupID = 0;

   m_MeasurementLocation = pgsTypes::AtPierLine;
   m_MeasurementType = pgsTypes::NormalToItem;

   m_RefGirderIdx = INVALID_INDEX;
   m_RefGirderOffset = 0;
   m_RefGirderOffsetType = pgsTypes::omtBridge;

   MakeCopy(rOther);
}

CBridgeDescription2::~CBridgeDescription2()
{
   Clear();
}

CBridgeDescription2& CBridgeDescription2::operator= (const CBridgeDescription2& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   ATLASSERT(*this == rOther); // should be equal after assignment
   return *this;
}

bool CBridgeDescription2::operator==(const CBridgeDescription2& rOther) const
{
   if ( m_strGirderFamilyName != rOther.m_strGirderFamilyName )
   {
      return false;
   }

   if ( m_GirderOrientation != rOther.m_GirderOrientation )
   {
      return false;
   }

   if ( m_bSameGirderName != rOther.m_bSameGirderName )
   {
      return false;
   }

   if ( m_bSameGirderName &&  m_strGirderName != rOther.m_strGirderName )
   {
      return false;
   }
   
   if ( m_bSameNumberOfGirders != rOther.m_bSameNumberOfGirders )
   {
      return false;
   }

   if ( m_bSameNumberOfGirders && m_nGirders != rOther.m_nGirders )
   {
      return false;
   }

   if ( m_GirderSpacingType != rOther.m_GirderSpacingType )
   {
      return false;
   }

   if ( m_TimelineManager != rOther.m_TimelineManager )
   {
      return false;
   }

   if ( m_AlignmentOffset != rOther.m_AlignmentOffset )
   {
      return false;
   }

   if (m_HaunchInputDepthType != rOther.m_HaunchInputDepthType)
   {
      return false;
   }

   if (pgsTypes::hidACamber == m_HaunchInputDepthType)
   {
      // "A" dimension haunch input method variables
      if (m_SlabOffsetType != rOther.m_SlabOffsetType)
      {
         return false;
      }

      if (m_SlabOffsetType == pgsTypes::sotBridge)
   {
         if (!IsEqual(m_SlabOffset,rOther.m_SlabOffset))
      {
         return false;
      }
   }

      if (m_AssumedExcessCamberType != rOther.m_AssumedExcessCamberType)
   {
      return false;
   }
   
      if (m_AssumedExcessCamberType == pgsTypes::aecBridge)
      {
         if (!IsEqual(m_AssumedExcessCamber,rOther.m_AssumedExcessCamber))
         {
            return false;
         }
      }
   }
   else
   {
      // Haunch input directly
      if (m_HaunchInputLocationType != rOther.m_HaunchInputLocationType)
   {
      return false;
   }

      if (m_HaunchLayoutType != rOther.m_HaunchLayoutType)
   {
         return false;
      }

      if (m_HaunchInputDistributionType != rOther.m_HaunchInputDistributionType)
      {
         return false;
      }

      if (!std::equal(m_HaunchDepths.cbegin(),m_HaunchDepths.cend(),rOther.m_HaunchDepths.cbegin(),[](const auto& a,const auto& b) {return IsEqual(a,b); }))
      {
         return false;
      }
   }

   if (!IsEqual(m_Fillet,rOther.m_Fillet))
   {
      return false;
   }

   if ( m_DisplayStartSupportType != rOther.m_DisplayStartSupportType )
   {
      return false;
   }

   if ( m_DisplayEndSupportType != rOther.m_DisplayEndSupportType )
   {
      return false;
   }

   if ( m_StartingPierNumber != rOther.m_StartingPierNumber )
   {
      return false;
   }
      
   if ( m_BearingType != rOther.m_BearingType )
   {
      return false;
   }

   if ( m_BearingType == pgsTypes::brtBridge )
   {
      if ( m_BearingData != rOther.m_BearingData )
      {
         return false;
      }
   }

   if (HasStructuralLongitudinalJoints() != rOther.HasStructuralLongitudinalJoints())
   {
      return false;
   }

   if (HasStructuralLongitudinalJoints() && m_LongitudinalJointConcrete != rOther.m_LongitudinalJointConcrete)
   {
      return false;
   }

   if ( m_WorkPointLocation != rOther.m_WorkPointLocation )
   {
      return false;
   }

   // the bridge-wide value of girder spacing only applies
   // if spacing is _T("sbsUniform")
   if ( IsBridgeSpacing(m_GirderSpacingType) )
   {
      if ( !IsEqual(m_GirderSpacing,rOther.m_GirderSpacing) )
      {
         return false;
      }

      if ( m_MeasurementLocation != rOther.m_MeasurementLocation )
      {
         return false;
      }

      if ( m_MeasurementType != rOther.m_MeasurementType )
      {
         return false;
      }

      if ( m_RefGirderIdx != rOther.m_RefGirderIdx )
      {
         return false;
      }

      if ( !IsEqual(m_RefGirderOffset,rOther.m_RefGirderOffset) )
      {
         return false;
      }

      if ( m_RefGirderOffsetType != rOther.m_RefGirderOffsetType )
      {
         return false;
      }

      if (IsTopWidthSpacing(m_GirderSpacingType))
      {
         if (m_TopWidthType != rOther.m_TopWidthType || !IsEqual(m_LeftTopWidth, rOther.m_LeftTopWidth))
         {
            return false;
         }

         if (m_TopWidthType == pgsTypes::twtAsymmetric && !IsEqual(m_RightTopWidth, rOther.m_RightTopWidth))
         {
            return false;
         }
      }
   }

   if ( m_GirderGroups.size() != rOther.m_GirderGroups.size() )
   {
      return false;
   }

   GroupIndexType nGroups = m_GirderGroups.size();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      if ( *m_GirderGroups[grpIdx] != *rOther.m_GirderGroups[grpIdx] )
      {
         return false;
      }
   }

   if ( m_Piers.size() != rOther.m_Piers.size() )
   {
      return false;
   }

   PierIndexType nPiers = m_Piers.size();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      if (*m_Piers[pierIdx] != *rOther.m_Piers[pierIdx] )
      {
         return false;
      }
   }

   if ( m_Spans.size() != rOther.m_Spans.size() )
   {
      return false;
   }

   SpanIndexType nSpans = m_Spans.size();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      if (*m_Spans[spanIdx] != *rOther.m_Spans[spanIdx] )
      {
         return false;
      }
   }

   if ( m_TemporarySupports.size() != rOther.m_TemporarySupports.size() )
   {
      return false;
   }

   SupportIndexType nTS = m_TemporarySupports.size();
   for (SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      if ( *m_TemporarySupports[tsIdx] != *rOther.m_TemporarySupports[tsIdx] )
      {
         return false;
      }
   }

   if ( m_Deck != rOther.m_Deck )
   {
      return false;
   }

   if ( m_LeftRailingSystem != rOther.m_LeftRailingSystem )
   {
      return false;
   }

   if ( m_RightRailingSystem != rOther.m_RightRailingSystem )
   {
      return false;
   }

   if ( m_LLDFMethod != rOther.m_LLDFMethod )
   {
      return false;
   }

   return true;
}

bool CBridgeDescription2::operator!=(const CBridgeDescription2& rOther) const
{
   return !operator==(rOther);
}

HRESULT CBridgeDescription2::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;

   Clear();

   CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit(_T("BridgeDescription"));

      Float64 version;
      pStrLoad->get_Version(&version);

      if ( version < 7 )
      {
         hr = LoadOldBridgeDescription(version,pStrLoad,pProgress);
         pStrLoad->EndUnit(); // BridgeDescription

         return LOADED_OLD_BRIDGE_TYPE;
      }
      
      CComVariant var;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("GirderFamilyName"),&var);
      m_strGirderFamilyName = OLE2T(var.bstrVal);

      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("GirderOrientation"),&var);
      m_GirderOrientation = (pgsTypes::GirderOrientationType)(var.lVal);

      var.vt = VT_BOOL;
      hr = pStrLoad->get_Property(_T("UseSameGirderForEntireBridge"),&var);
      m_bSameGirderName = (var.boolVal == VARIANT_TRUE ? true : false);

      if ( m_bSameGirderName )
      {
         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("Girder"),&var);
         m_strGirderName = OLE2T(var.bstrVal);
      }

      var.vt = VT_BOOL;
      hr = pStrLoad->get_Property(_T("UseSameNumberOfGirdersInAllGroups"),&var);
      m_bSameNumberOfGirders = (var.boolVal == VARIANT_TRUE ? true : false);

      if ( m_bSameNumberOfGirders )
      {
         var.vt = VT_INDEX;
         hr = pStrLoad->get_Property(_T("GirderCount"),&var);
         m_nGirders = VARIANT2INDEX(var);
      }

      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("GirderSpacingType"),&var);
      m_GirderSpacingType = (pgsTypes::SupportedBeamSpacing)(var.lVal);

      if ( IsBridgeSpacing(m_GirderSpacingType) )
      {
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("GirderSpacing"),&var);
         m_GirderSpacing = var.dblVal;

         if (9 < version && IsTopWidthSpacing(m_GirderSpacingType))
         {
            // added in version 10
            //var.vt = VT_R8;
            //hr = pStrLoad->get_Property(_T("GirderTopWidth"), &var);
            //m_LeftTopWidth = var.dblVal;

            // NOTE: if your file balks when trying to open, and the property is "GirderTopWidth"
            // comment out the code below and uncomment the code above, load, and save your file
            // and then put the code back the way it was.
            // Files with "GirderTopWidth" are from development versions that were never released.
            // We aren't providing compatibility for these files.
            var.vt = VT_I8;
            hr = pStrLoad->get_Property(_T("TopWidthType"), &var);
            m_TopWidthType = (pgsTypes::TopWidthType)(var.lVal);

            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("LeftTopWidth"), &var);
            m_LeftTopWidth = var.dblVal;

            var.vt = VT_R8;
            hr = pStrLoad->get_Property(_T("RightTopWidth"), &var);
            m_RightTopWidth = var.dblVal;
         }

         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("MeasurementLocation"),&var);
         m_MeasurementLocation = (pgsTypes::MeasurementLocation)(var.lVal);

         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("MeasurementType"),&var);
         m_MeasurementType = (pgsTypes::MeasurementType)(var.lVal);      

         var.vt = VT_INDEX;
         hr = pStrLoad->get_Property(_T("RefGirder"),&var);
         m_RefGirderIdx = VARIANT2INDEX(var);

         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("RefGirderOffsetType"),&var);
         m_RefGirderOffsetType = (pgsTypes::OffsetMeasurementType)(var.lVal);

         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("RefGirderOffset"),&var);
         m_RefGirderOffset = var.dblVal;
      }

      if (11 < version)
      {
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("WorkPointLocation"),&var);
         m_WorkPointLocation = (pgsTypes::WorkPointLocation)(var.lVal);
      }

      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("LLDFMethod"),&var);
      m_LLDFMethod = (pgsTypes::DistributionFactorMethod)(var.lVal);

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("AlignmentOffset"),&var);
      m_AlignmentOffset = var.dblVal;

      m_HaunchInputDepthType = pgsTypes::hidACamber; // the only type before version 14
      if (13 < version)
      {
         var.vt = VT_UI4;
         hr = pStrLoad->get_Property(_T("HaunchInputDepthType"),&var);
         m_HaunchInputDepthType = (pgsTypes::HaunchInputDepthType)(var.lVal);
      }

      if (m_HaunchInputDepthType == pgsTypes::hidACamber)
      {
      var.vt = VT_UI4;
      hr = pStrLoad->get_Property(_T("SlabOffsetType"),&var);
      m_SlabOffsetType = (pgsTypes::SlabOffsetType)(var.lVal);
         if (m_SlabOffsetType == pgsTypes::sotBridge)
      {
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("SlabOffset"),&var);
         m_SlabOffset = var.dblVal;
      }

      m_bWasVersion3_1FilletRead = false;

      if (version > 7)
      {
         if (version == 8 || version == 9)
         {
            // In version 8, we made a mistake and allowed fillets to be all over the bridge. This was fixed
            // in vers 10, when we went back to a single fillet for the entire bridge. See mantis 209 and 752
         var.vt = VT_UI4;
               hr = pStrLoad->get_Property(_T("FilletType"),&var);
            LONG ft = (var.lVal); // if value was for entire bridge, we can just store
            if (ft == 0 /* pgsTypes::sotBridge */)
         {
            var.vt = VT_R8;
                  hr = pStrLoad->get_Property(_T("Fillet"),&var);
               m_Fillet = var.dblVal;
            }
            else
            {
               // Fillet was defined for multiple spans or girders. We will need get max value of all defined
               // fillets and assign it to our bridge after we are finished loading
               m_bWasVersion3_1FilletRead = true;
               m_Fillet = 0.0;
            }
         }
         else
         {
            var.vt = VT_R8;
               hr = pStrLoad->get_Property(_T("Fillet"),&var);
            m_Fillet = var.dblVal;
         }
      }
      
      if (9 < version)
      {
         var.vt = VT_UI4;
            hr = pStrLoad->get_Property(_T("AssExcessCamberType"),&var);
            pgsTypes::AssumedExcessCamberType ct = (pgsTypes::AssumedExcessCamberType)(var.lVal);
         m_AssumedExcessCamberType = ct;

         if (ct == pgsTypes::aecBridge)
         {
            var.vt = VT_R8;
               hr = pStrLoad->get_Property(_T("AssExcessCamber"),&var);
            m_AssumedExcessCamber = var.dblVal;
         }
      }
      }
      else
      {
         // Haunch depth input directly
         var.vt = VT_UI4;
         hr = pStrLoad->get_Property(_T("HaunchInputLocationType"),&var);
         pgsTypes::HaunchInputLocationType hit = (pgsTypes::HaunchInputLocationType)(var.lVal);
         m_HaunchInputLocationType = hit;

         var.vt = VT_UI4;
         hr = pStrLoad->get_Property(_T("HaunchLayoutType"),&var);
         pgsTypes::HaunchLayoutType hlt = (pgsTypes::HaunchLayoutType)(var.lVal);
         m_HaunchLayoutType = hlt;

         var.vt = VT_UI4;
         hr = pStrLoad->get_Property(_T("HaunchInputDistributionType"),&var);
         pgsTypes::HaunchInputDistributionType hdt = (pgsTypes::HaunchInputDistributionType)(var.lVal);
         m_HaunchInputDistributionType = hdt;

         if (pgsTypes::hilSame4Bridge == m_HaunchInputLocationType)
         {
            hr = pStrLoad->BeginUnit(_T("HaunchDepths"));

            m_HaunchDepths.clear();
            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("nValues"),&var);
            IndexType nvals = VARIANT2INDEX(var);

            var.vt = VT_R8;
            for (IndexType iv = 0; iv < nvals; iv++)
            {
               hr = pStrLoad->get_Property(_T("HaunchVal"),&var);
               m_HaunchDepths.push_back(var.dblVal);
            }

            pStrLoad->EndUnit();
         }

         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("Fillet"),&var);
         m_Fillet = var.dblVal;
      }

      if (13 <= version)
      {
         var.vt = VT_UI8;
         hr = pStrLoad->get_Property(_T("StartingPierNumber"), &var);
         m_StartingPierNumber = var.llVal;

         var.vt = VT_UI4;
         hr = pStrLoad->get_Property(_T("DisplayStartSupportType"), &var);
         pgsTypes::DisplayEndSupportType dt = (pgsTypes::DisplayEndSupportType)(var.lVal);
         m_DisplayStartSupportType = dt;

         var.vt = VT_UI4;
         hr = pStrLoad->get_Property(_T("DisplayEndSupportType"), &var);
         dt = (pgsTypes::DisplayEndSupportType)(var.lVal);
         m_DisplayEndSupportType = dt;
      }

      if ( 10 < version )
      {
         var.vt = VT_UI4;
         hr = pStrLoad->get_Property(_T("BearingType"),&var);
         m_BearingType = (pgsTypes::BearingType)(var.lVal);
         if ( m_BearingType == pgsTypes::brtBridge )
         {
            hr = m_BearingData.Load(pStrLoad,pProgress);
         }
      }

      // Events
      hr = m_TimelineManager.Load(pStrLoad,pProgress);

      hr = pStrLoad->BeginUnit(_T("Piers"));
      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("PierCount"),&var);
      PierIndexType nPiers = VARIANT2INDEX(var);
      PierIndexType pierIdx = 0;
      // allocate, number, and associated all piers with bridge before loading
      // this has to be done so that links can be restored property
      for ( pierIdx = 0; pierIdx < nPiers; pierIdx++ )
      {
         CPierData2* pPier = new CPierData2;
         pPier->SetBridgeDescription(this);
         pPier->SetIndex(pierIdx);
         m_Piers.push_back(pPier);
      }
      // load each pier
      for ( pierIdx = 0; pierIdx < nPiers; pierIdx++ )
      {
         CPierData2* pPier = m_Piers[pierIdx];
         hr = pPier->Load(pStrLoad,pProgress);
         ATLASSERT(pPier->GetID() != INVALID_ID);
         UpdateNextPierID(pPier->GetID());
      }
      pStrLoad->EndUnit();

      pStrLoad->BeginUnit(_T("TemporarySupports"));
      var.vt = VT_INDEX;
      pStrLoad->get_Property(_T("TemporarySupportCount"),&var);
      SupportIndexType nTS = VARIANT2INDEX(var);
      for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
      {
         CTemporarySupportData* pTS = new CTemporarySupportData();
         m_TemporarySupports.push_back(pTS);

         pTS->SetIndex(tsIdx);

         hr = pTS->Load(pStrLoad,pProgress,this);
         ATLASSERT(pTS->GetID() != INVALID_ID);
         UpdateNextTemporarySupportID(pTS->GetID());
      }
      pStrLoad->EndUnit(); // Temporary Supports

      pStrLoad->BeginUnit(_T("Spans"));
      SpanIndexType nSpans = nPiers-1;
      for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
      {
         CSpanData2* pSpan = new CSpanData2;
         pSpan->SetBridgeDescription(this);
         pSpan->SetIndex(spanIdx);
         hr = pSpan->Load(pStrLoad,pProgress);
         m_Spans.push_back(pSpan);
      }
      pStrLoad->EndUnit();

      RenumberSpans(); // numbers the spans, piers, and hooks up the pointers
      UpdateTemporarySupports();

      // Load the deck
      hr = m_Deck.Load(pStrLoad,pProgress);

      if (version < 8)
      {
         // In version 8 we moved fillet from deck into bridge
         m_Fillet = m_Deck.m_LegacyFillet;
      }

      if (version < 10)
      {
         // prior versions had bearing data (support width) in connection data
         m_BearingType = pgsTypes::brtPier;
      }

      Float64 railing_version;
      hr = pStrLoad->BeginUnit(_T("LeftRailingSystem"));
      pStrLoad->get_Version(&railing_version);
      hr = m_LeftRailingSystem.Load(pStrLoad,pProgress);
      hr = pStrLoad->EndUnit();

      hr = pStrLoad->BeginUnit(_T("RightRailingSystem"));
      pStrLoad->get_Version(&railing_version);
      hr = m_RightRailingSystem.Load(pStrLoad,pProgress);
      hr = pStrLoad->EndUnit();

      // load girder groups
      hr = pStrLoad->BeginUnit(_T("GirderGroups"));
      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("GirderGroupCount"),&var);
      GroupIndexType nGroups = VARIANT2INDEX(var);
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         CGirderGroupData* pGroup = new CGirderGroupData(this);
         pGroup->SetIndex(grpIdx);
         hr = pGroup->Load(pStrLoad,pProgress);
         ATLASSERT(pGroup->GetID() != INVALID_ID);
         UpdateNextGirderGroupID(pGroup->GetID());
         m_GirderGroups.push_back(pGroup);
      }
      hr = pStrLoad->EndUnit(); // GirderGroups

      // added in version 11
      if (10 < version)
      {
         var.vt = VT_BOOL;
         hr = pStrLoad->get_Property(_T("StructuralLongitudinalJoints"), &var);
         if (var.boolVal == VARIANT_TRUE)
         {
            hr = m_LongitudinalJointConcrete.Load(pStrLoad,pProgress);
         }
      }


      hr = pStrLoad->EndUnit(); // BridgeDescription

      /////////////////////////////////////////////////////////////
      // END OF LOADING BRIDGE DESCRIPTION
      /////////////////////////////////////////////////////////////

      // Sometime in the past, the bearing offset and end distance measurement types were inadvertantly allowed to be
      // unequal values between the back and ahead faces of the pier. The UI forces them to be the same.
      // Here we will check the values and make corrections as needed
      for (const auto& pPier : m_Piers)
      {
         if ( pPier->GetPrevSpan() == nullptr )
         {
            // this is the first pier so the back side needs to be set equal to the ahead side
            pPier->m_EndDistanceMeasurementType[pgsTypes::Back] = pPier->m_EndDistanceMeasurementType[pgsTypes::Ahead];
            pPier->m_BearingOffsetMeasurementType[pgsTypes::Back] = pPier->m_BearingOffsetMeasurementType[pgsTypes::Ahead];
         }
         else if ( pPier->GetNextSpan() == nullptr )
         {
            // this is the last pier so the ahead side needs to be set equal to the back side
            pPier->m_EndDistanceMeasurementType[pgsTypes::Ahead] = pPier->m_EndDistanceMeasurementType[pgsTypes::Back];
            pPier->m_BearingOffsetMeasurementType[pgsTypes::Ahead] = pPier->m_BearingOffsetMeasurementType[pgsTypes::Back];
         }

         ATLASSERT(pPier->m_EndDistanceMeasurementType[pgsTypes::Ahead] == pPier->m_EndDistanceMeasurementType[pgsTypes::Back]);
         ATLASSERT(pPier->m_BearingOffsetMeasurementType[pgsTypes::Ahead] == pPier->m_BearingOffsetMeasurementType[pgsTypes::Back]);
      }

      if ( ::IsBridgeSpacing(m_GirderSpacingType) )
      {
         // If the spacing type is for the entire bridge, the temporary support objects
         // do not load their girder spacing objects. At load time, the temporary support
         // objects aren't part of the bridge model so they have no way to learn the
         // number of girders that rest upon them and thus cannot initialize the spacing
         // object for the currect number of girders.... so we do that here.
         //
         // This is similar to the pier spacing initialization done at the end of
         // CGirderGroupData::Load()
         std::vector<CTemporarySupportData*>::iterator tsIter(m_TemporarySupports.begin());
         std::vector<CTemporarySupportData*>::iterator tsIterEnd(m_TemporarySupports.end());
         for ( ; tsIter != tsIterEnd; tsIter++ )
         {
            CTemporarySupportData* pTS = *tsIter;
            CGirderSpacing2* pSpacing = pTS->GetSegmentSpacing();
            CSpanData2* pSpan = pTS->GetSpan();
            CGirderGroupData* pGroup = GetGirderGroup(pSpan);
            pSpacing->InitGirderCount(pGroup->GetGirderCount());
            pSpacing->SetGirderSpacing(0,m_GirderSpacing);
         }
      }

      if (version < 9)
      {
         // this applies to spliced girder because in the project agent, the timeline gets blasted and re-built for prestressed girder bridges
         // prior to version 9, we didn't explicitly have an event for diapraghs. we assumed diaphragms were installed when segments were erected.
         // set the diaphragm load event for the event when the last segment is erected
         EventIndexType erectSegmentEventIdx = m_TimelineManager.GetLastSegmentErectionEventIndex();
         m_TimelineManager.GetEventByIndex(erectSegmentEventIdx)->GetApplyLoadActivity().ApplyIntermediateDiaphragmLoad();
      }

      bool doSlabOffset =    m_HaunchInputDepthType == pgsTypes::hidACamber && m_SlabOffsetType == pgsTypes::sotBridge;
      bool doAssumedCamber = m_HaunchInputDepthType == pgsTypes::hidACamber && m_AssumedExcessCamberType == pgsTypes::aecBridge;
      bool doDirectHaunch =  m_HaunchInputDepthType != pgsTypes::hidACamber && m_HaunchInputLocationType == pgsTypes::hilSame4Bridge;

      // Copy values down to the individual element level for parameters defined at the bridge level
      CopyDown(m_bSameNumberOfGirders, 
               m_bSameGirderName, 
               IsBridgeSpacing(m_GirderSpacingType),
               doSlabOffset, 
               doAssumedCamber,
               doDirectHaunch,
               m_BearingType==pgsTypes::brtBridge);
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   PGS_ASSERT_VALID;
   return hr;
}

HRESULT CBridgeDescription2::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;
   pStrSave->BeginUnit(_T("BridgeDescription"),FILE_VERSION);

   pStrSave->put_Property(_T("GirderFamilyName"),CComVariant(CComBSTR(m_strGirderFamilyName.c_str())));
   pStrSave->put_Property(_T("GirderOrientation"),CComVariant(m_GirderOrientation));
   pStrSave->put_Property(_T("UseSameGirderForEntireBridge"),CComVariant(m_bSameGirderName));
   if ( m_bSameGirderName )
   {
      pStrSave->put_Property(_T("Girder"),CComVariant(CComBSTR(m_strGirderName.c_str())));
   }

   pStrSave->put_Property(_T("UseSameNumberOfGirdersInAllGroups"),CComVariant(m_bSameNumberOfGirders) );
   if ( m_bSameNumberOfGirders )
   {
      pStrSave->put_Property(_T("GirderCount"),CComVariant(m_nGirders));
   }

   pStrSave->put_Property(_T("GirderSpacingType"),CComVariant(m_GirderSpacingType) );
   if ( IsBridgeSpacing(m_GirderSpacingType) )
   {
      pStrSave->put_Property(_T("GirderSpacing"),       CComVariant(m_GirderSpacing));

      // added in version 11
      if (IsTopWidthSpacing(m_GirderSpacingType))
      {
         pStrSave->put_Property(_T("TopWidthType"), CComVariant(m_TopWidthType));
         pStrSave->put_Property(_T("LeftTopWidth"), CComVariant(m_LeftTopWidth));
         pStrSave->put_Property(_T("RightTopWidth"), CComVariant(m_RightTopWidth));
      }

      pStrSave->put_Property(_T("MeasurementLocation"), CComVariant(m_MeasurementLocation));
      pStrSave->put_Property(_T("MeasurementType"),     CComVariant(m_MeasurementType));
      pStrSave->put_Property(_T("RefGirder"),CComVariant(m_RefGirderIdx));
      pStrSave->put_Property(_T("RefGirderOffsetType"),CComVariant(m_RefGirderOffsetType));
      pStrSave->put_Property(_T("RefGirderOffset"),CComVariant(m_RefGirderOffset));
   }

   pStrSave->put_Property(_T("WorkPointLocation"), CComVariant(m_WorkPointLocation));

   hr = pStrSave->put_Property(_T("LLDFMethod"),CComVariant(m_LLDFMethod));

   hr = pStrSave->put_Property(_T("AlignmentOffset"),CComVariant(m_AlignmentOffset)); // added in version 4

   hr = pStrSave->put_Property(_T("HaunchInputDepthType"),CComVariant(m_HaunchInputDepthType)); // added in version 14

   if (m_HaunchInputDepthType == pgsTypes::hidACamber)
   {
   hr = pStrSave->put_Property(_T("SlabOffsetType"),CComVariant(m_SlabOffsetType));
      if (m_SlabOffsetType == pgsTypes::sotBridge)
   {
      hr = pStrSave->put_Property(_T("SlabOffset"),CComVariant(m_SlabOffset));
   }

   hr = pStrSave->put_Property(_T("Fillet"),CComVariant(m_Fillet));
   
   hr = pStrSave->put_Property(_T("AssExcessCamberType"),CComVariant(m_AssumedExcessCamberType)); // Added in version 8
      if (m_AssumedExcessCamberType == pgsTypes::aecBridge)
   {
      hr = pStrSave->put_Property(_T("AssExcessCamber"),CComVariant(m_AssumedExcessCamber));
   }
   }
   else
   {
      // Direct input of haunch depths
      hr = pStrSave->put_Property(_T("HaunchInputLocationType"),CComVariant(m_HaunchInputLocationType)); //         Added in version 14
      hr = pStrSave->put_Property(_T("HaunchLayoutType"),CComVariant(m_HaunchLayoutType)); //                       Added in version 14
      hr = pStrSave->put_Property(_T("HaunchInputDistributionType"),CComVariant(m_HaunchInputDistributionType)); // Added in version 14

      if (pgsTypes::hilSame4Bridge == m_HaunchInputLocationType)
      {
         hr = pStrSave->BeginUnit(_T("HaunchDepths"),1.0);
         pStrSave->put_Property(_T("nValues"),CComVariant(m_HaunchDepths.size()));
         for (const auto haunchVal : m_HaunchDepths)
         {
            hr = pStrSave->put_Property(_T("HaunchVal"),CComVariant(haunchVal));
         }
         hr = pStrSave->EndUnit();
      }

      hr = pStrSave->put_Property(_T("Fillet"),CComVariant(m_Fillet));
   }

   // Pier renumbering. Added in version 13
   hr = pStrSave->put_Property(_T("StartingPierNumber"),CComVariant(m_StartingPierNumber));
   hr = pStrSave->put_Property(_T("DisplayStartSupportType"),CComVariant(m_DisplayStartSupportType));
   hr = pStrSave->put_Property(_T("DisplayEndSupportType"),CComVariant(m_DisplayEndSupportType));

   hr = pStrSave->put_Property(_T("BearingType"),CComVariant(m_BearingType)); // Added in version 9
   if ( m_BearingType == pgsTypes::brtBridge )
   {
      hr = m_BearingData.Save(pStrSave, pProgress);
   }
   
   m_TimelineManager.Save(pStrSave,pProgress);

   pStrSave->BeginUnit(_T("Piers"),1.0);
   pStrSave->put_Property(_T("PierCount"),CComVariant(m_Piers.size()));
   std::for_each(std::begin(m_Piers), std::end(m_Piers), [pStrSave,pProgress](auto* pPier) {pPier->Save(pStrSave, pProgress); });
   pStrSave->EndUnit();

   pStrSave->BeginUnit(_T("TemporarySupports"),1.0);
   pStrSave->put_Property(_T("TemporarySupportCount"),CComVariant(m_TemporarySupports.size()));
   std::for_each(std::begin(m_TemporarySupports), std::end(m_TemporarySupports), [pStrSave, pProgress](auto* pTS) {pTS->Save(pStrSave, pProgress); });
   pStrSave->EndUnit();

   pStrSave->BeginUnit(_T("Spans"),1.0);
   std::for_each(std::begin(m_Spans), std::end(m_Spans), [pStrSave, pProgress](auto* pSpan) {pSpan->Save(pStrSave, pProgress); });
   pStrSave->EndUnit();


   m_Deck.Save(pStrSave,pProgress);

   pStrSave->BeginUnit(_T("LeftRailingSystem"),2.0);
   m_LeftRailingSystem.Save(pStrSave,pProgress);
   pStrSave->EndUnit();

   pStrSave->BeginUnit(_T("RightRailingSystem"),2.0);
   m_RightRailingSystem.Save(pStrSave,pProgress);
   pStrSave->EndUnit();

   pStrSave->BeginUnit(_T("GirderGroups"),1.0);
   pStrSave->put_Property(_T("GirderGroupCount"),CComVariant(m_GirderGroups.size()));
   std::for_each(std::begin(m_GirderGroups), std::end(m_GirderGroups), [pStrSave, pProgress](auto* pGroup) {pGroup->Save(pStrSave, pProgress); });
   pStrSave->EndUnit();

   // added in version 11
   pStrSave->put_Property(_T("StructuralLongitudinalJoints"), CComVariant(HasStructuralLongitudinalJoints()));
   if (HasStructuralLongitudinalJoints())
   {
      m_LongitudinalJointConcrete.Save(pStrSave,pProgress);
   }
   
   pStrSave->EndUnit(); // BridgeDescription
   return hr;
}

void CBridgeDescription2::SetGirderFamilyName(LPCTSTR strName)
{
   m_strGirderFamilyName = strName;
}

LPCTSTR CBridgeDescription2::GetGirderFamilyName() const
{
   return m_strGirderFamilyName.c_str();
}

void CBridgeDescription2::SetTimelineManager(const CTimelineManager* pTimelineMgr)
{
   m_TimelineManager = *pTimelineMgr;
   m_TimelineManager.SetBridgeDescription(this);
}

const CTimelineManager* CBridgeDescription2::GetTimelineManager() const
{
   return &m_TimelineManager;
}

CTimelineManager* CBridgeDescription2::GetTimelineManager()
{
   return &m_TimelineManager;
}


void CBridgeDescription2::SetAlignmentOffset(Float64 alignmentOffset)
{
   m_AlignmentOffset = alignmentOffset;
}

Float64 CBridgeDescription2::GetAlignmentOffset() const
{
   return m_AlignmentOffset;
}


CDeckDescription2* CBridgeDescription2::GetDeckDescription()
{
   return &m_Deck;
}

const CDeckDescription2* CBridgeDescription2::GetDeckDescription() const
{
   return &m_Deck;
}

CRailingSystem* CBridgeDescription2::GetLeftRailingSystem()
{
   return &m_LeftRailingSystem;
}

const CRailingSystem* CBridgeDescription2::GetLeftRailingSystem() const
{
   return &m_LeftRailingSystem;
}

CRailingSystem* CBridgeDescription2::GetRightRailingSystem()
{
   return &m_RightRailingSystem;
}

const CRailingSystem* CBridgeDescription2::GetRightRailingSystem() const
{
   return &m_RightRailingSystem;
}

void CBridgeDescription2::SetHaunchInputDepthType(pgsTypes::HaunchInputDepthType type)
{
   m_HaunchInputDepthType = type;
}

pgsTypes::HaunchInputDepthType CBridgeDescription2::GetHaunchInputDepthType() const
{
   return m_HaunchInputDepthType;
}

void CBridgeDescription2::SetSlabOffsetType(pgsTypes::SlabOffsetType slabOffsetType)
{
   m_SlabOffsetType = slabOffsetType;
}

pgsTypes::SlabOffsetType CBridgeDescription2::GetSlabOffsetType() const
{
   return m_SlabOffsetType;
}

void CBridgeDescription2::SetSlabOffset(Float64 slabOffset)
{
   m_SlabOffset = slabOffset;
}

Float64 CBridgeDescription2::GetSlabOffset(bool bGetRawValue) const
{
   if ( bGetRawValue )
   {
      return m_SlabOffset;
   }

   if ( m_Deck.DeckType == pgsTypes::sdtNone )
   {
      return 0;
   }

   return m_SlabOffset;
}

Float64 CBridgeDescription2::GetLeastSlabOffset() const
{
   if ( m_SlabOffsetType == pgsTypes::sotBridge )
   {
      return GetSlabOffset();
   }

   // this will cover the case of by pier and by segment
   Float64 minSlabOffset = DBL_MAX;
   for(const auto* pGroup : m_GirderGroups)
   {
      // getting slab offset by segment will cover piers and temporary supports
      // if slab offset is defined by pier, then we just need to use one girder
      // because all of the other girders will be the same
      GirderIndexType nGirders = (m_SlabOffsetType == pgsTypes::sotBearingLine ? 1 : pGroup->GetGirderCount());
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            Float64 start, end;
            pSegment->GetSlabOffset(&start, &end);
            minSlabOffset = Min(minSlabOffset, start, end);
         }
      }
   }

   return minSlabOffset;
}

Float64 CBridgeDescription2::GetMinSlabOffset() const
{
   Float64 minSlabOffset = m_Deck.GrossDepth;
   if (m_Deck.GetDeckType() == pgsTypes::sdtCompositeCIP || IsOverlayDeck(m_Deck.GetDeckType())) // CIP deck or overlay
   {
      minSlabOffset += m_Fillet;
   }
   else if (m_Deck.GetDeckType() == pgsTypes::sdtCompositeSIP) // SIP
   {
      minSlabOffset += m_Deck.PanelDepth + m_Fillet;
   }

   return minSlabOffset;
}

void CBridgeDescription2::SetAssumedExcessCamberType(pgsTypes::AssumedExcessCamberType camberType)
{
   m_AssumedExcessCamberType = camberType;
}

pgsTypes::AssumedExcessCamberType CBridgeDescription2::GetAssumedExcessCamberType() const
{
   return m_AssumedExcessCamberType;
}

void CBridgeDescription2::SetAssumedExcessCamber(Float64 assumedExcessCamber)
{
   m_AssumedExcessCamber = assumedExcessCamber;
}

Float64 CBridgeDescription2::GetAssumedExcessCamber(bool bGetRawValue) const
{
   if ( bGetRawValue )
   {
      return m_AssumedExcessCamber;
   }

   if ( m_Deck.DeckType == pgsTypes::sdtNone )
   {
      return 0;
   }

   return m_AssumedExcessCamber;
}

void CBridgeDescription2::SetFillet(Float64 Fillet)
{
   m_Fillet = Fillet;
}

Float64 CBridgeDescription2::GetFillet(bool bGetRawValue) const
{
   if ( bGetRawValue )
   {
      return m_Fillet;
   }

   if ( m_Deck.DeckType == pgsTypes::sdtNone )
   {
      return 0;
   }

   return m_Fillet;
}

bool CBridgeDescription2::WasVersion3_1FilletRead() const
{
   return m_bWasVersion3_1FilletRead;
}

void CBridgeDescription2::SetHaunchInputLocationType(pgsTypes::HaunchInputLocationType type)
{
   m_HaunchInputLocationType = type;
}

pgsTypes::HaunchInputLocationType CBridgeDescription2::GetHaunchInputLocationType() const
{
   return m_HaunchInputLocationType;
}

void CBridgeDescription2::SetHaunchLayoutType(pgsTypes::HaunchLayoutType type)
{
   m_HaunchLayoutType = type;
}

pgsTypes::HaunchLayoutType CBridgeDescription2::GetHaunchLayoutType() const
{
   return m_HaunchLayoutType;
}

void CBridgeDescription2::SetHaunchInputDistributionType(pgsTypes::HaunchInputDistributionType type)
{
   m_HaunchInputDistributionType = type;
}

pgsTypes::HaunchInputDistributionType CBridgeDescription2::GetHaunchInputDistributionType() const
{
   return m_HaunchInputDistributionType;
}

void CBridgeDescription2::SetDirectHaunchDepths(std::vector<Float64> depths)
{
   m_HaunchDepths = depths;
}

std::vector<Float64> CBridgeDescription2::GetDirectHaunchDepths() const
{
   return m_HaunchDepths;
}

Float64 CBridgeDescription2::GetMinimumAllowableHaunchDepth(pgsTypes::HaunchInputDepthType inputType) const
{
   // This function is for checking haunch input
   ATLASSERT(inputType == pgsTypes::hidHaunchDirectly || inputType == pgsTypes::hidHaunchPlusSlabDirectly);

   Float64 fillet = GetFillet();
   if (inputType == pgsTypes::hidHaunchDirectly)
   {
      return fillet;
   }
   else
   {
      const CDeckDescription2* pDeck = GetDeckDescription();
      Float64 Tdeck;
      if (pDeck->GetDeckType() == pgsTypes::sdtCompositeSIP)
      {
         Tdeck = pDeck->GrossDepth + pDeck->PanelDepth;
      }
      else
      {
         Tdeck = pDeck->GrossDepth;
      }

      return fillet + Tdeck;
   }
}

void CBridgeDescription2::CreateFirstSpan(const CPierData2* pFirstPier,const CSpanData2* pFirstSpan,const CPierData2* pNextPier,EventIndexType pierErectionEventIdx)
{
   _ASSERT( 0 == m_Piers.size() && 0 == m_Spans.size() ); // this call should only be made once

   CPierData2* firstPier;
   CSpanData2* firstSpan;
   CPierData2* nextPier;

   if ( pFirstPier )
   {
      firstPier = new CPierData2(*pFirstPier);
   }
   else
   {
      firstPier = new CPierData2();
   }

   if ( pFirstSpan )
   {
      firstSpan = new CSpanData2(*pFirstSpan);
   }
   else
   {
      firstSpan = new CSpanData2();
   }

   if ( pNextPier )
   {
      nextPier = new CPierData2(*pNextPier);
   }
   else
   {
      nextPier = new CPierData2();
      nextPier->SetStation( firstPier->GetStation() + WBFL::Units::ConvertToSysUnits(100.0,WBFL::Units::Measure::Feet) );
   }

   firstPier->SetBridgeDescription(this);
   firstPier->SetID( GetNextPierID() );
   firstSpan->SetBridgeDescription(this);
   nextPier->SetBridgeDescription(this);
   nextPier->SetID( GetNextPierID() );

   m_Piers.push_back(firstPier);
   m_Spans.push_back(firstSpan);
   m_Piers.push_back(nextPier);

   // Hooks up span/pier pointers
   RenumberSpans();

   // Create first group
   CGirderGroupData* pGroup = new CGirderGroupData(this);
   pGroup->SetPiers(m_Piers.front(),m_Piers.back());
   pGroup->SetIndex(m_GirderGroups.size());
   pGroup->SetID( GetNextGirderGroupID() );
   m_nGirders = Max((GirderIndexType)1,m_nGirders);
   pGroup->Initialize(m_nGirders);
   m_GirderGroups.push_back(pGroup);

   // Set the erection event for the new pier
   if ( pierErectionEventIdx != INVALID_INDEX )
   {
      CTimelineEvent* pTimelineEvent = m_TimelineManager.GetEventByIndex(pierErectionEventIdx);
      ATLASSERT(pTimelineEvent != nullptr); // bad pierErectionEventIdx ???
      pTimelineEvent->GetErectPiersActivity().Enable(true);
      pTimelineEvent->GetErectPiersActivity().AddPier(firstPier->GetID());
      pTimelineEvent->GetErectPiersActivity().AddPier(nextPier->GetID());
   }

   PGS_ASSERT_VALID;
}

void CBridgeDescription2::AppendSpan(const CSpanData2* pSpanData,const CPierData2* pPierData,bool bCreateNewGroup,EventIndexType pierErectionEventIdx)
{
   // Appends a new span to the end of the bridge. If pSpanData and/or pPierData is nullptr, the
   // new span/pier will be a copy of the last span/pier in the bridge. If bCreateNewGroup is true
   // a new girder group is created. This group will have the same number of girders as the last group
   // in the bridge. If the last group has more than one span, single segment girders will be created,
   // otherwise the girders from the last group will be copied. If bCreateNewGroup is false, the
   // last group in the bridge will be expanded and so will the girders in that group

   // INVALID_INDEX -> insert new span after the last pier
   // -1.0 -> Use pier station stationing to determine span length
   InsertSpan(INVALID_INDEX,pgsTypes::Ahead,-1.0,pSpanData,pPierData,bCreateNewGroup,pierErectionEventIdx);
}

void CBridgeDescription2::InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace,Float64 newSpanLength, const CSpanData2* pSpanData,const CPierData2* pPierData,bool bCreateNewGroup,EventIndexType pierErectionEventIdx)
{
   // Inserts a new span into the bridge. The new span is inserted at the pier defined by refPierIdx on 
   // the side of the pier defined by pierFace. newSpanLength defines the length of the new span.
   // If pSpanData and/or pPierData is nullptr, the new span/pier will be a copy of the last span/pier in 
   // the bridge. If bCreateNewGroup is true a new girder group is created. A new group can only
   // be created if refPierIdx is the first/last pier in a group and the pierFace is not inside the group.
   // The new group will have the same number of girders as the adjacent group
   // in the bridge. If the adjacent group has more than one span, single segment girders will be created,
   // otherwise the girders from the adjacent group will be copied. If bCreateNewGrouop is false, the
   // adjacent or containing group will be expanded and so will the girders in that group

   // If this fires, then the call to CreateFirstSpan hasn't been made yet
   ASSERT( 2 <= m_Piers.size() && 1 <= m_Spans.size() && 1 <= m_GirderGroups.size() ); 

   // get the refernece pier index
   if ( refPierIdx == INVALID_INDEX )
   {
      // append at end of bridge
      refPierIdx = m_Piers.size()-1;
   }

   CPierData2* pRefPier = m_Piers[refPierIdx];

   // Negative span length means that we take stationing from piers - better have pier data
   if ( newSpanLength <= 0 )
   {
      if ( pPierData == nullptr )
      {
         //ATLASSERT(false); // this should not happen... pPierData must reference a pier if newSpanLength is < 0.
         // RAB - 9/12/2013: I commented out the assert because it often fires when loading a bridge description from
         // an older version of PGSuper. The older files load correctly so this really isn't a problem.

         // Just so we don't have problems in release builds, make the new span length 100 ft and keep going
         newSpanLength = WBFL::Units::ConvertToSysUnits(100.0,WBFL::Units::Measure::Feet);
      }
      else
      {
         // compute the length of the new span
         newSpanLength = pPierData->GetStation() - pRefPier->GetStation();
      }
   }

   // Index for our new span
   SpanIndexType newSpanIdx = (SpanIndexType)refPierIdx;

   // Copy properties from the span on the side of the pier in question (if it exists)
   SpanIndexType refSpanIdx;
   if (refPierIdx == 0)
   {
      // appending span at start of bridge
      refSpanIdx = 0;
   }
   else if ((PierIndexType)m_Piers.size()-1 <= refPierIdx)
   {
      // appending span at end of bridge
      refSpanIdx = m_Spans.size()-1;
   }
   else if (pierFace == pgsTypes::Back)
   {
      // inserting span somewhere in the middle of the bridge
      // on the back side of the reference pier so the span is the
      // span that is previous to this pier
      refSpanIdx = (SpanIndexType)(refPierIdx-1);
   }
   else
   {
      ASSERT(pierFace == pgsTypes::Ahead);
      refSpanIdx = (SpanIndexType)refPierIdx;
   }

   // If creating a new group, make sure the new span is at the boundary of a group
   // and not inside of a group. Can't add a group within a group
   GroupIndexType newGroupIdx = INVALID_INDEX;
   GroupIndexType refGroupIdx = INVALID_INDEX;
   if ( bCreateNewGroup )
   {
      CGirderGroupData* pRefGroup = GetGirderGroup(m_Spans[refSpanIdx]);
      ATLASSERT(pRefGroup != nullptr);
      if ( pRefGroup->GetPierIndex(pgsTypes::metStart) != refPierIdx &&
           pRefGroup->GetPierIndex(pgsTypes::metEnd)   != refPierIdx )
      {
         // cannot create the span because the new group would be
         // inside of an existing group
         ATLASSERT(false);
         return;
      }
      else
      {
         refGroupIdx = pRefGroup->GetIndex();

         if ( pRefGroup->GetPierIndex(pgsTypes::metStart) == refPierIdx )
         {
            newGroupIdx = refGroupIdx; // insert new group before ref group
         }
         else
         {
            newGroupIdx = refGroupIdx + 1; // insert new group after ref group
         }
      }
   }

   // Create the new span and pier objects
   CSpanData2* pNewSpan;
   CPierData2* pNewPier;

   if ( pSpanData )
   {
      pNewSpan = new CSpanData2(*pSpanData);
   }
   else
   {
      pNewSpan = new CSpanData2(*m_Spans[refSpanIdx]);
   }

   if ( pPierData )
   {
      pNewPier = new CPierData2(*pPierData);

      // put this pier at the location of the reference pier... then
      // later, it will be moved by "span length" putting it back where it belongs
      pNewPier->SetStation(pRefPier->GetStation());
   }
   else
   {
      pNewPier = new CPierData2(*pRefPier); // this pier is created at the location of the reference pier

      // if the reference pier is at the start or end of the bridge (no next or previous span)
      // then the spacing, connection geometry, and diaphragm information on one side of the 
      // pier is not defined... the reference pier is going to become an intermediate pier so 
      // copy the spacing from one side to another

      if ( pierFace == pgsTypes::Ahead && m_Piers[refPierIdx]->GetNextSpan() == nullptr )
      {
         pRefPier->SetGirderSpacing(pgsTypes::Ahead,*(pRefPier->GetGirderSpacing(pgsTypes::Back)));

         Float64 endDist;
         ConnectionLibraryEntry::EndDistanceMeasurementType endDistMeasure;
         pRefPier->GetGirderEndDistance(pgsTypes::Back,&endDist,&endDistMeasure);

         Float64 brgOffset;
         ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetMeasure;
         pRefPier->GetBearingOffset(pgsTypes::Back,&brgOffset,&brgOffsetMeasure);

         if ( brgOffset < endDist )
         {
            // the ends of the girders are going to overlap... adjust the end
            // distance on both sides of the pier

            // NOTE: this isn't exactly the perfect way to do it. The measurement type
            // of brgOffset and endDist are not taking into account. The goal is to
            // ensure that the ends of girders at a common pier don't occupy the same space
            endDist = brgOffset;
            pRefPier->SetGirderEndDistance(pgsTypes::Back,endDist,endDistMeasure);
            pRefPier->SetBearingOffset(pgsTypes::Back,brgOffset,brgOffsetMeasure);

            pRefPier->SetGirderEndDistance(pgsTypes::Ahead,endDist,endDistMeasure);
            pRefPier->SetBearingOffset(pgsTypes::Ahead,brgOffset,brgOffsetMeasure);
         }
         else
         {
            pRefPier->SetGirderEndDistance(pgsTypes::Ahead,endDist,endDistMeasure);
            pRefPier->SetBearingOffset(pgsTypes::Ahead,brgOffset,brgOffsetMeasure);
         }

         pRefPier->MirrorBearingData(pgsTypes::Back); // make ahead bearings same as back

         pRefPier->SetDiaphragmHeight(pgsTypes::Ahead,pRefPier->GetDiaphragmHeight(pgsTypes::Back));
         pRefPier->SetDiaphragmWidth(pgsTypes::Ahead,pRefPier->GetDiaphragmWidth(pgsTypes::Back));

         pRefPier->SetDiaphragmLoadType(pgsTypes::Ahead,pRefPier->GetDiaphragmLoadType(pgsTypes::Back));
         pRefPier->SetDiaphragmLoadLocation(pgsTypes::Ahead,pRefPier->GetDiaphragmLoadLocation(pgsTypes::Back));

         pRefPier->SetSlabOffset(pgsTypes::Ahead, pRefPier->GetSlabOffset(pgsTypes::Back, true));
      }
      else if ( pierFace == pgsTypes::Back && m_Piers[refPierIdx]->GetPrevSpan() == nullptr )
      {
         pRefPier->SetGirderSpacing(pgsTypes::Back,*(pRefPier->GetGirderSpacing(pgsTypes::Ahead)));

         Float64 endDist;
         ConnectionLibraryEntry::EndDistanceMeasurementType endDistMeasure;
         pRefPier->GetGirderEndDistance(pgsTypes::Ahead,&endDist,&endDistMeasure);

         Float64 brgOffset;
         ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetMeasure;
         pRefPier->GetBearingOffset(pgsTypes::Ahead,&brgOffset,&brgOffsetMeasure);

         if ( brgOffset < endDist )
         {
            // the ends of the girders are going to overlap... adjust the end
            // distance on both sides of the pier

            // NOTE: this isn't exactly the perfect way to do it. The measurement type
            // of brgOffset and endDist are not taking into account. The goal is to
            // ensure that the ends of girders at a common pier don't occupy the same space
            endDist = brgOffset;
            pRefPier->SetGirderEndDistance(pgsTypes::Ahead,endDist,endDistMeasure);
            pRefPier->SetBearingOffset(pgsTypes::Ahead,brgOffset,brgOffsetMeasure);
            pRefPier->SetGirderEndDistance(pgsTypes::Back,endDist,endDistMeasure);
            pRefPier->SetBearingOffset(pgsTypes::Back,brgOffset,brgOffsetMeasure);
         }
         else
         {
            pRefPier->SetGirderEndDistance(pgsTypes::Back,endDist,endDistMeasure);
            pRefPier->SetBearingOffset(pgsTypes::Back,brgOffset,brgOffsetMeasure);
         }

         pRefPier->MirrorBearingData(pgsTypes::Ahead); // make back bearings same as ahead

         pRefPier->SetDiaphragmHeight(pgsTypes::Back,pRefPier->GetDiaphragmHeight(pgsTypes::Ahead));
         pRefPier->SetDiaphragmWidth(pgsTypes::Back,pRefPier->GetDiaphragmWidth(pgsTypes::Ahead));

         pRefPier->SetDiaphragmLoadType(pgsTypes::Back,pRefPier->GetDiaphragmLoadType(pgsTypes::Ahead));
         pRefPier->SetDiaphragmLoadLocation(pgsTypes::Back,pRefPier->GetDiaphragmLoadLocation(pgsTypes::Ahead));

         pRefPier->SetSlabOffset(pgsTypes::Back, pRefPier->GetSlabOffset(pgsTypes::Ahead, true));
      }
      else
      {
         // the new pier is an interior pier so make sure the connection geometry is ok
         for ( int i = 0; i < 2; i++ )
         {
            pgsTypes::PierFaceType face = (pgsTypes::PierFaceType)i;
            Float64 endDist;
            ConnectionLibraryEntry::EndDistanceMeasurementType endDistMeasure;
            pNewPier->GetGirderEndDistance(face,&endDist,&endDistMeasure);

            Float64 brgOffset;
            ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetMeasure;
            pNewPier->GetBearingOffset(face,&brgOffset,&brgOffsetMeasure);

            // NOTE: this isn't exactly the perfect way to do it. The measurement type
            // of brgOffset and endDist are not taking into account. The goal is to
            // ensure that the ends of girders at a common pier don't occupy the same space

            if ( brgOffset < endDist )
            { 
               endDist = brgOffset;
               pNewPier->SetGirderEndDistance(face,endDist,endDistMeasure);
            }
         }

         if ( pierFace == pgsTypes::Back && m_Piers[refPierIdx]->GetNextSpan() == nullptr )
         {
            // the new pier is an interior pier, but it got its information from the last pier
            // the spacing was only defined on the back side... copy the spacing to the ahead side
            pNewPier->SetGirderSpacing(pgsTypes::Ahead,*pNewPier->GetGirderSpacing(pgsTypes::Back));
         }
         else if ( pierFace == pgsTypes::Ahead && m_Piers[refPierIdx]->GetPrevSpan() == nullptr )
         {
            // the new pier is an interior pier, but it got its information from the first pier
            // the spacing was only defined on the ahead side... copy the spacing to the back side
            pNewPier->SetGirderSpacing(pgsTypes::Back,*pNewPier->GetGirderSpacing(pgsTypes::Ahead));
         }
      }
   }

   pNewPier->SetID( GetNextPierID() );

   // assign bridge model to new span and pier
   pNewSpan->SetBridgeDescription(this);
   pNewPier->SetBridgeDescription(this);

   // Update the girder group pier references if needed and renumber spans and piers
   // If we are creating a new group, modify the reference group otherwise get the group
   // associated with the reference span. Capture the index of the start and end piers before
   // updating the span/pier collections and numbering
   CGirderGroupData* pGroup = (bCreateNewGroup ? m_GirderGroups[refGroupIdx] : GetGirderGroup(m_Spans[refSpanIdx]));
   PierIndexType startPierIdx = pGroup->GetPier(pgsTypes::metStart)->GetIndex();
   PierIndexType endPierIdx   = pGroup->GetPier(pgsTypes::metEnd)->GetIndex();

   // Put the new span in the spans collection
   m_Spans.insert(m_Spans.begin()+newSpanIdx,pNewSpan); 

   // Put the new pier in the piers collection. While doing this, capture
   // the iterator that points to the first pier that needs to be moved
   // to account for the length of the new span (remember that above we set the
   // location of the new pier equal to that of the reference pier)
   std::vector<CPierData2*>::iterator movePierIter; // iterator to the first pier that needs to be moved in order to maintain span lengths
   if ( pierFace == pgsTypes::Back )
   {
      // insert new pier before the reference pier
      //pNewPier->SetConnectionType(pgsTypes::bctContinuousAfterDeck);
      movePierIter = m_Piers.insert(m_Piers.begin() + refPierIdx, pNewPier );
      movePierIter++; //increment to get to the ref pier... move the ref pier and all the follow
   }
   else
   {
      // insert new pier after the reference pier
      //m_Piers[refPierIdx]->SetConnectionType(pgsTypes::bctContinuousAfterDeck);
      movePierIter = m_Piers.insert(m_Piers.begin() + refPierIdx + 1, pNewPier);
      // the new pier, and all that follow get moved
   }

   RenumberSpans(); // updates indicies as well as Pier<-->Span<-->Pier pointers

   // now, update the group pier referneces
   // notice that the comparison to refPierIdx is based on the pier indices prior to renumbering
   // so that is why we had to capture the start/end pier indicies before renumbering.
   // Updating the piers must be done after renumbering otherwise it messes up the girder objects
   // stored within the group objects
   bool bUpdateStartPier = (startPierIdx == refPierIdx && (bCreateNewGroup ? (pierFace == pgsTypes::Ahead) : (pierFace == pgsTypes::Back)));
   bool bUpdateEndPier   = (endPierIdx   == refPierIdx && (bCreateNewGroup ? (pierFace == pgsTypes::Back)  : (pierFace == pgsTypes::Ahead)));
   if ( bUpdateStartPier )
   {
      // the new group is being inserted at the start of the group. this group will start at the new pier
      pGroup->SetPier(pgsTypes::metStart,pNewPier);
   }

   if ( bUpdateEndPier )
   {
      // the new group is being inserted at the end of the group. this group will end at the new pier
      pGroup->SetPier(pgsTypes::metEnd,pNewPier);
   }


   // Adjust location of down-station piers
   if ( refPierIdx == 0 && refSpanIdx == 0 && pierFace == pgsTypes::Back )
   {
      // If the new span is inserted before the first span, don't adjust anything
      pNewPier->SetStation( pNewPier->GetStation() - newSpanLength );
   }
   else
   {
      if (0.0 < newSpanLength)
      {
         // offset all piers after the new pier by the length of the new span
         std::vector<CPierData2*>::iterator pierIter(movePierIter);
         std::vector<CPierData2*>::iterator pierIterEnd(m_Piers.end());
         for ( ; pierIter != pierIterEnd; pierIter++ )
         {
            CPierData2* pPier = *pierIter;
            pPier->SetStation( pPier->GetStation() + newSpanLength);
         }

         // offset all temporary supports after the new pier by the length of the new span
         movePierIter--; // go back to the stationary pier and move all temporary supports that come after it
         std::vector<CTemporarySupportData*>::iterator tsIter(m_TemporarySupports.begin());
         std::vector<CTemporarySupportData*>::iterator tsIterEnd(m_TemporarySupports.end());
         for ( ; tsIter != tsIterEnd; tsIter++ )
         {
            CTemporarySupportData* pTS = *tsIter;
            if ( (*movePierIter)->GetStation() < pTS->GetStation() )
            {
               pTS->SetStation(pTS->GetStation() + newSpanLength);
            }
         }
         UpdateTemporarySupports(); // update the span references for the temporary supports (since some of them moved)
      }
   }

   if ( bCreateNewGroup )
   {
      // creating a new group.... 
      // create the same number of girders in this group as there is in the reference group
      // the new girders will have one segment

      // get the reference group. this group will be used to get default data for the new group
      ASSERT(refGroupIdx != INVALID_INDEX);
      CGirderGroupData* pRefGroup = m_GirderGroups[refGroupIdx];

      if (pierFace == pgsTypes::Back)
      {
#pragma Reminder("REVIEW: This should probably be contained within the group and girder objects")
         // the new group is being created before the reference group
         // the reference group, and all groups downstream will be shifted by one span
         auto iter = m_GirderGroups.begin() + refGroupIdx;
         auto end = m_GirderGroups.end();
         for (; iter != end; iter++)
         {
            auto* pGroup(*iter);
            for (auto* pGirder : pGroup->m_Girders)
            {
               DuctIndexType nDucts = pGirder->m_PTData.GetDuctCount();
               for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
               {
                  if (pGirder->m_PTData.GetDuct(ductIdx)->DuctGeometryType == CDuctGeometry::Parabolic)
                  {
                     // shift all the span data in the ParabolicDuctGeometry by one span
                     // because we've added a group to the start of the bridge that is one span in length
                     pGirder->m_PTData.GetDuct(ductIdx)->ParabolicDuctGeometry.Shift(1);
                  }
               }
            }
         }
      }

      // Get the boundary piers for the new group
      CPierData2* pPrevPier = pNewSpan->GetPrevPier();
      CPierData2* pNextPier = pNewSpan->GetNextPier();

      // create the new group
      CGirderGroupData* pNewGroup = new CGirderGroupData(pPrevPier,pNextPier);
      pNewGroup->SetID( GetNextGirderGroupID() );

      // save the new group
      m_GirderGroups.insert(m_GirderGroups.begin()+newGroupIdx,pNewGroup);

      // update group numbering
      RenumberGroups();
      
      // Make the new group have the same number of girders as the reference group
      GirderIndexType nGirders = pRefGroup->GetGirderCount();
      pNewGroup->Initialize(nGirders); // creates girders and initializes the slab offsets

      // Copy the girder type grouping information
      std::vector<CGirderTypeGroup> vTypeGroups = pRefGroup->GetGirderTypeGroups();
      pNewGroup->SetGirderTypeGroups(vTypeGroups);

      // Copy the girder top width grouping information
      std::vector<CGirderTopWidthGroup> vTopWidthGroups = pRefGroup->GetGirderTopWidthGroups();
      pNewGroup->SetGirderTopWidthGroups(vTopWidthGroups);

      // Make the staging match the reference group
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pRefGirder = pRefGroup->GetGirder(gdrIdx);
         CSplicedGirderData* pNewGirder = pNewGroup->GetGirder(gdrIdx);
         pNewGirder->SetGirderName( pRefGirder->GetGirderName() ); // library references will be resolved by the owner of this bridge

         // raw copy of haunch data
         pNewGirder->CopyHaunchData(*pRefGirder);

         const CPrecastSegmentData* pRefSegment = pRefGirder->GetSegment(0);
         SegmentIDType refSegID = pRefSegment->GetID();

         EventIndexType constructionEventIdx = m_TimelineManager.GetSegmentConstructionEventIndex(refSegID);
         EventIndexType erectionEventIdx = m_TimelineManager.GetSegmentErectionEventIndex(refSegID);

         SegmentIndexType nSegments = pNewGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CPrecastSegmentData* pNewSegment = pNewGirder->GetSegment(segIdx);
            SegmentIDType newSegID = pNewSegment->GetID();

            m_TimelineManager.SetSegmentConstructionEventByIndex( newSegID, constructionEventIdx );
            m_TimelineManager.SetSegmentErectionEventByIndex( newSegID, erectionEventIdx );

            const CPrecastSegmentData* pRefSegment = pRefGirder->GetSegment(segIdx);
            pNewSegment->CopySegmentData(pRefSegment,false);
         }
      }
   }
   else
   {
      // Adjust girder groups for new span/pier

      // A new group was not created... therefore the new span was insert into an existing
      // group. The girders in that group must be adjusted for the new span
      CGirderGroupData* pGroup = GetGirderGroup(pNewSpan);
      pGroup->AddSpan(refPierIdx,pierFace); // updates the slab offsets and the girders in the group

      // Adjust the connection type at the new pier.
      if ( refPierIdx == 0 && pierFace == pgsTypes::Back )
      {
         // new span/pier are before the first pier
         CPierData2* pRefPier = m_Piers[refPierIdx+1]; // +1 because we added the new pier to index 0 of the array
         pRefPier->SetSegmentConnectionType(pgsTypes::psctContinuousSegment,INVALID_INDEX/*parameter not used*/);
      }
      else if ( refPierIdx == m_Piers.size()-2 && pierFace == pgsTypes::Ahead ) // -2 because we added a pier (otherwise it would be -1)
      {
         // new span/pier are after the last pier
         CPierData2* pRefPier = m_Piers[refPierIdx];
         pRefPier->SetSegmentConnectionType(pgsTypes::psctContinuousSegment,INVALID_INDEX/*parameter not used*/);
      }
      else
      {
         // new span/pier are inside of a group. the end piers of the group don't change
         // ie. do nothing
         pNewPier->SetSegmentConnectionType(pgsTypes::psctContinuousSegment,INVALID_INDEX/*parameter not used*/);
      }
   }

   // Set the erection event for the new pier
   if ( pierErectionEventIdx != INVALID_INDEX )
   {
      PierIndexType newPierID = pNewPier->GetID();
      CTimelineEvent* pTimelineEvent = m_TimelineManager.GetEventByIndex(pierErectionEventIdx);
      ATLASSERT(pTimelineEvent != nullptr); // if it is nullptr, pierErectionEventIdx was bad, event not defined
      pTimelineEvent->GetErectPiersActivity().Enable(true);
      pTimelineEvent->GetErectPiersActivity().AddPier(newPierID);
   }

   IndexType castDeckEventIdx = m_TimelineManager.GetCastDeckEventIndex();
   if (castDeckEventIdx != INVALID_INDEX)
   {
      auto* pEvent = m_TimelineManager.GetEventByIndex(castDeckEventIdx);
      pEvent->GetCastDeckActivity().InsertSpan(this, newSpanIdx, pNewPier->GetIndex());
   }

   PGS_ASSERT_VALID;
}

void CBridgeDescription2::RemoveSpan(SpanIndexType spanIdx,pgsTypes::RemovePierType rmPierType)
{
   // Removes a span and associated pier
   // If this is the last span in a group, the group will be removed as well

   CSpanData2* pPrevSpan = GetSpan(spanIdx-1);
   CSpanData2* pSpan     = GetSpan(spanIdx);
   CSpanData2* pNextSpan = GetSpan(spanIdx+1);

   CPierData2* pPrevPier = pSpan->GetPrevPier();
   CPierData2* pNextPier = pSpan->GetNextPier();

   bool bDeletePrevPier = false;
   bool bDeleteNextPier = false;

   PierIndexType removePierIdx = (rmPierType == pgsTypes::PrevPier ? pPrevPier->GetIndex() : pNextPier->GetIndex());
   PierIDType    removePierID  = (rmPierType == pgsTypes::PrevPier ? pPrevPier->GetID()    : pNextPier->GetID());
   PierIndexType nPiers = m_Piers.size(); // number of piers before removal

   // remove the span from deck casting (this matters if we are doing staged casting)
   IndexType castDeckEventIdx = m_TimelineManager.GetCastDeckEventIndex();
   if (castDeckEventIdx != INVALID_INDEX)
   {
      auto* pEvent = m_TimelineManager.GetEventByIndex(castDeckEventIdx);
      pEvent->GetCastDeckActivity().RemoveSpan(this, spanIdx, rmPierType);
   }

   //
   // remove all temporary supports that occur in this span
   //

   // collect the IDs of the temporary supports to remove
   // (we can't remove when the TS is found because it will alter the container and 
   // mess up the iterator)
   std::vector<SupportIndexType> tsIndices;
   std::for_each(std::begin(m_TemporarySupports), std::end(m_TemporarySupports), [&tsIndices, &pSpan](auto* pTS) {if (pTS->GetSpan() == pSpan) tsIndices.push_back(pTS->GetIndex()); });

   // remove temporary supports by index.. work in reverse order. the spliced girder closure joints and segments will be adjusted
   std::for_each(std::rbegin(tsIndices), std::rend(tsIndices), [&](auto tsIdx) {RemoveTemporarySupportByIndex(tsIdx); });

   // Adjust the girder group
   CGirderGroupData* pGroup = GetGirderGroup(pSpan);
   bool bDeleteGroup = false;
   if ( pGroup->GetPier(pgsTypes::metStart) == pPrevPier && pGroup->GetPier(pgsTypes::metEnd) == pNextPier )
   {
      // pSpan is the only span in pGroup... when pSpan goes away, so does the group
      bDeleteGroup = true; // delete the group at the end
      m_GirderGroups.erase(m_GirderGroups.begin()+pGroup->GetIndex());
   }

   // If the pier that is being removed is at the boundary of a group, capture the 
   // adjacent groups so they can be updated below
   // (this update needs to happen after RenumberSpans is called)
   CPierData2* pRemovePier = m_Piers[removePierIdx];
   CPierData2* pCommonPier = nullptr; // this is the pier that spans will join at when the span/pier are removed
   CGirderGroupData* pPrevGroup = nullptr;
   CGirderGroupData* pNextGroup = nullptr;
   if ( pRemovePier->IsBoundaryPier() )
   {
      pCommonPier = (rmPierType == pgsTypes::PrevPier ? m_Piers[removePierIdx+1] : m_Piers[removePierIdx-1]);
      pPrevGroup = pRemovePier->GetPrevGirderGroup();
      pNextGroup = pRemovePier->GetNextGirderGroup();
   }

   if (pRemovePier->IsInteriorPier() && !IsSegmentContinuousOverPier(pRemovePier->GetSegmentConnectionType()))
   {
      // the pier that is going to be removed is an interior pier and the segments are not continuous over
      // the pier, therefore there is a closure. Remove the closure from the timeline
      CClosureJointData* pClosure = pRemovePier->GetClosureJoint(0);
      EventIndexType eventIdx = m_TimelineManager.GetCastClosureJointEventIndex(pClosure);
      if (eventIdx != INVALID_INDEX)
      {
         CTimelineEvent* pTimelineEvent = m_TimelineManager.GetEventByIndex(eventIdx);

         if (pClosure->GetPier())
         {
            ATLASSERT(pClosure->GetPier()->GetID() == pRemovePier->GetID());
            pTimelineEvent->GetCastClosureJointActivity().RemovePier(pClosure->GetPier()->GetID());
         }
      }
   }

   if(pCommonPier && pCommonPier->IsInteriorPier())
   {
      // the common pier will become a boundary pier so, there is a closure joint
      // at the common pier, make sure the closure joint is removed from the timeline
      // QUESTION??? WHEN DOES THE CLOSURE JOINT OBJECT GET DELETED? THAT IS WHEN THE
      // CASTING EVENT SHOULD BE REMOVED
      if (!IsSegmentContinuousOverPier(pCommonPier->GetSegmentConnectionType()))
      {
         CClosureJointData* pClosure = pCommonPier->GetClosureJoint(0);
         EventIndexType eventIdx = m_TimelineManager.GetCastClosureJointEventIndex(pClosure);
         if (eventIdx != INVALID_INDEX)
         {
            CTimelineEvent* pTimelineEvent = m_TimelineManager.GetEventByIndex(eventIdx);

            if (pClosure->GetPier())
            {
               pTimelineEvent->GetCastClosureJointActivity().RemovePier(pClosure->GetPier()->GetID());
            }
         }
      }
   }

   // remove the span from the group
   pGroup->RemoveSpan(spanIdx, rmPierType); // this will update the slab offsets and remove the span from the girders in this group

#pragma Reminder("REVIEW: This should probably be contained within the group and girder objects")
  // span references in all groups after this group need to be updated
   CGirderGroupData* pDownStationGroup = pGroup->GetNextGirderGroup(); // pNextGroup is already used and we don't want to mess with it so using pDownStationGroup
   while (pDownStationGroup != nullptr)
   {
      for (CSplicedGirderData* pGirder : pDownStationGroup->m_Girders)
      {
         DuctIndexType nDucts = pGirder->m_PTData.GetDuctCount();
         for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
         {
            CDuctData* pDuct = pGirder->m_PTData.GetDuct(ductIdx);
            if (pDuct->DuctGeometryType == CDuctGeometry::Parabolic)
            {
               pDuct->ParabolicDuctGeometry.Shift(-1);
            }
         }
      }
     pDownStationGroup = pDownStationGroup->GetNextGirderGroup();
   }

   // Remove span and pier from the bridge
   Float64 removedPierStation;
   if ( rmPierType == pgsTypes::PrevPier )
   {
      m_Spans.erase(m_Spans.begin()+spanIdx);

      removedPierStation = m_Piers[removePierIdx]->GetStation();

      bDeletePrevPier = true;

      m_Piers.erase(m_Piers.begin()+removePierIdx);
   }
   else
   {
      m_Spans.erase(m_Spans.begin()+spanIdx);

      removedPierStation = m_Piers[removePierIdx]->GetStation();

      bDeleteNextPier = true;

      m_Piers.erase(m_Piers.begin()+removePierIdx);
   }

   // Remove negative rebar data at the pier that is being removed
   RemoveNegMomentRebar(removePierIdx);

   // for all neg moment rebar data occuring after the removed pier, decrement the pier index by one
   for (auto& nmRebar : m_Deck.DeckRebarData.NegMomentRebar)
   {
      if (removePierIdx < nmRebar.PierIdx)
      {
         nmRebar.PierIdx--;
      }
   }

   // Fix up the span/pier pointer and update the span/pier index values
   RenumberSpans();

   // if there is a common pier, update the groups at the common pier
   if ( pCommonPier )
   {
      if ( pPrevGroup )
      {
         pPrevGroup->SetPier(pgsTypes::metEnd,pCommonPier);
      }

      if ( pNextGroup )
      {
         pNextGroup->SetPier(pgsTypes::metStart,pCommonPier);
      }
   }

   if ( removePierIdx == 0 && spanIdx == 0 )
   {
      // Don't alter bridge if first pier and span are removed
   }
   else
   {
      Float64 span_length = pSpan->GetSpanLength();

      // offset all piers after the pier that was removed by the length of the span that was removed
      // ie. re-station all the piers after the pier that was removed so that span lengths are maintained
      std::for_each(std::begin(m_Piers) + removePierIdx, std::end(m_Piers), [span_length](auto* pPier) {pPier->SetStation(pPier->GetStation() - span_length);});

      // offset all temporary supports after the pier that was removed by the length of the span that was removed
      std::for_each(std::begin(m_TemporarySupports), std::end(m_TemporarySupports), [span_length, removedPierStation](auto* pTS) {if (removedPierStation < pTS->GetStation()) pTS->SetStation(pTS->GetStation() - span_length); });
   }


   if ( bDeletePrevPier )
   {
      delete pPrevPier;
      pPrevPier = nullptr;
   }

   delete pSpan;

   if ( bDeleteNextPier )
   {
      delete pNextPier;
      pNextPier = nullptr;
   }

   if ( bDeleteGroup )
   {
      delete pGroup;
      pGroup = nullptr;
      RenumberGroups();
   }

   PGS_ASSERT_VALID;
}

GroupIndexType CBridgeDescription2::CreateGirderGroup(GroupIndexType refGroupIdx,pgsTypes::MemberEndType end,std::vector<Float64> spanLengths,GirderIndexType nGirders)
{
   // Creates a new girder group. The location of the group is defined by the refGroupIdx and end parameters.
   // refGroupIdx specifies the group that this new group will be created adjacent to.
   // end specifies at which of of the reference group the new group will be located
   // e.g.  Group 2, Start... the new group will be created before group 2
   // e.g.  Group 6, End... the new group will be created after group 6
   // if refGroupIdx is INVALID_INDEX then the new group is created adjacent to the last group
   // the size of the spanLengths vector defines the number of spans that will be created within this new group
   // the values inside the spanLengths vector will be the span lengths of these new spans
   // nGirders specifies the number of girders within the group

   CGirderGroupData* pRefGroup;
   if ( refGroupIdx == INVALID_INDEX )
   {
      pRefGroup = m_GirderGroups.back();
   }
   else
   {
      pRefGroup = m_GirderGroups[refGroupIdx];
   }

   CPierData2* pRefPier = pRefGroup->GetPier(end);

   // indices into pier and span vectors that mark the base location for
   // inserting new piers and spans
   PierIndexType refPierIdx = pRefPier->GetIndex();
   SpanIndexType refSpanIdx = refPierIdx + (end == pgsTypes::metStart ? 1 : 0);

   // pointers to the piers at the start and end of the new group
   CPierData2* pStartPier = (end == pgsTypes::metStart ? nullptr : pRefGroup->GetPier(pgsTypes::metStart));
   CPierData2* pEndPier   = (end == pgsTypes::metStart ? pRefGroup->GetPier(pgsTypes::metEnd) : nullptr);
   
   Float64 offset = 0; // the amount the piers and temporary supports must be
                       // offset to accomodate the new spans

   // Create new spans and piers
   auto iter = std::begin(spanLengths);
   auto iterEnd = std::end(spanLengths);
   int i = 0;
   for ( ; iter != iterEnd; iter++, i++ )
   {
      Float64 spanLength = *iter;
      offset += spanLength;

      CSpanData2* pNewSpan = new CSpanData2;
      pNewSpan->SetBridgeDescription(this);
      m_Spans.insert(m_Spans.begin() + refSpanIdx + i, pNewSpan);

      CPierData2* pNewPier = new CPierData2;
      pNewPier->SetBridgeDescription(this);
      m_Piers.insert(m_Piers.begin() + refPierIdx + i,pNewPier);

      // new group starts with the first new pier created... capture it
      if ( end == pgsTypes::metStart && i == 0 )
      {
         pStartPier = pNewPier;
      }
      
      // new groups ends with the last new pier created... update this pointer
      // every time. when the loop completes, it will have the correct pointer
      if ( end == pgsTypes::metEnd )
      {
         pEndPier = pNewPier;
      }
   }

   CGirderGroupData* pGirderGroupData = new CGirderGroupData(pStartPier,pEndPier);
   pGirderGroupData->SetBridgeDescription(this);
   pGirderGroupData->SetID( GetNextGirderGroupID() );
   m_GirderGroups.push_back(pGirderGroupData);
   RenumberGroups();

   // Update the deck casting activity for the new spans/piers in the new group
   IndexType castDeckEventIdx = m_TimelineManager.GetCastDeckEventIndex();
   if (castDeckEventIdx != INVALID_INDEX)
   {
      auto& castDeckActivity = m_TimelineManager.GetEventByIndex(castDeckEventIdx)->GetCastDeckActivity();
      CPierData2* pPier = pStartPier;
      while (pPier != pEndPier)
      {
         PierIndexType pierIdx = pPier->GetIndex();
         SpanIndexType spanIdx = pPier->GetNextSpan()->GetIndex();
         castDeckActivity.InsertSpan(this, spanIdx, pierIdx);
         pPier = pPier->GetNextSpan()->GetNextPier();
      }
   }

   return pGirderGroupData->GetIndex();
}

CGirderGroupData* CBridgeDescription2::GetGirderGroup(GroupIndexType grpIdx)
{
   if ( grpIdx == INVALID_INDEX || m_GirderGroups.size() <= grpIdx )
   {
      return nullptr; 
   }

   return m_GirderGroups[grpIdx];
}

const CGirderGroupData* CBridgeDescription2::GetGirderGroup(GroupIndexType grpIdx) const
{
   if ( grpIdx == INVALID_INDEX || m_GirderGroups.size() <= grpIdx )
   {
      return nullptr; 
   }

   return m_GirderGroups[grpIdx];
}

CGirderGroupData* CBridgeDescription2::GetGirderGroup(const CSpanData2* pSpan)
{
   if ( pSpan == nullptr )
   {
      return nullptr;
   }

   PierIndexType prevPierIdx = pSpan->GetPrevPier()->GetIndex();
   PierIndexType nextPierIdx = pSpan->GetNextPier()->GetIndex();

   for(auto* pGroup : m_GirderGroups)
   {
      PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
      PierIndexType endPierIdx   = pGroup->GetPierIndex(pgsTypes::metEnd);

      if ( (startPierIdx <= prevPierIdx && prevPierIdx <= endPierIdx) &&
           (startPierIdx <= nextPierIdx && nextPierIdx <= endPierIdx) )
      {
         return pGroup;
      }
   }

   return nullptr;
}

const CGirderGroupData* CBridgeDescription2::GetGirderGroup(const CSpanData2* pSpan) const
{
   if ( pSpan == nullptr )
   {
      return nullptr;
   }

   PierIndexType prevPierIdx = pSpan->GetPrevPier()->GetIndex();
   PierIndexType nextPierIdx = pSpan->GetNextPier()->GetIndex();

   for(const auto* pGroup : m_GirderGroups)
   {
      PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
      PierIndexType endPierIdx   = pGroup->GetPierIndex(pgsTypes::metEnd);

      if ( startPierIdx <= prevPierIdx && nextPierIdx <= endPierIdx )
      {
         return pGroup;
      }
   }

   return nullptr;
}

GroupIndexType CBridgeDescription2::GetGirderGroupCount() const
{
   return m_GirderGroups.size();
}

void CBridgeDescription2::RemoveGirderGroup(GroupIndexType grpIdx,pgsTypes::RemovePierType rmPierType)
{
   ATLASSERT(grpIdx != INVALID_INDEX);
   ATLASSERT(grpIdx < m_GirderGroups.size());

   IndexType castDeckEventIdx = m_TimelineManager.GetCastDeckEventIndex();
   if (castDeckEventIdx != INVALID_INDEX)
   {
      auto* pEvent = m_TimelineManager.GetEventByIndex(castDeckEventIdx);
      pEvent->GetCastDeckActivity().RemoveGirderGroup(this, grpIdx, rmPierType);
   }

   // removes a girder group and removes the spans, piers, and girders within the group
   CGirderGroupData* pGroup = m_GirderGroups[grpIdx];

   Float64 group_length = pGroup->GetLength();

   // Get indices for the first and last pier in the group
   PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
   PierIndexType endPierIdx   = pGroup->GetPierIndex(pgsTypes::metEnd);

   // Get the indices for all the spans that will be removed
   SpanIndexType startSpanIdx = startPierIdx;
   SpanIndexType endSpanIdx   = endPierIdx-1;

   // Adjust the pier indies so that the pier at the start or end is not removed
   // Also adjust the piers at the ends of the adjacent groups
   if ( rmPierType == pgsTypes::PrevPier )
   {
      // the pier at the start of this group is going to be deleted, therefore the end pier of this
      // group will become the end pier of the previous group
      CGirderGroupData* pPrevGroup = pGroup->GetPrevGirderGroup();
      if ( pPrevGroup )
      {
         CPierData2* pPier = pGroup->GetPier(pgsTypes::metEnd);
         CSpanData2* pSpan = pPrevGroup->GetPier(pgsTypes::metEnd)->GetSpan(pgsTypes::Back);
         pPier->SetSpan(pgsTypes::Back,pSpan);
         pPrevGroup->SetPier(pgsTypes::metEnd,pPier);
      }

      // pier at the start of the group is removed, so save the last pier
      endPierIdx--;
   }
   else
   {
      // the pier at the end of this group is going to be deleted, therefore the start pier of the
      // next group will become the start pier of this group
      CGirderGroupData* pNextGroup = pGroup->GetNextGirderGroup();
      if ( pNextGroup )
      {
         CPierData2* pPier = pGroup->GetPier(pgsTypes::metStart);
         CSpanData2* pSpan = pNextGroup->GetPier(pgsTypes::metStart)->GetSpan(pgsTypes::Ahead);
         pPier->SetSpan(pgsTypes::Ahead,pSpan);
         pNextGroup->SetPier(pgsTypes::metStart,pPier);
      }

      // pier at the end of the group is removed, so save the first pier
      startPierIdx++;
   }

   // delete the piers
   for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
   {
      CPierData2* pPier = m_Piers[pierIdx];

      // remove deck negative moment rebar at this pier
      PierIndexType removeRebarPierIdx = pPier->GetIndex();
      RemoveNegMomentRebar(removeRebarPierIdx);

      delete pPier;
      pPier = nullptr;
   }

   // move all piers that occur after the last pier in the group to the left
   // by an amount equal to the length of the group
   if ( grpIdx == 0 && rmPierType == pgsTypes::PrevPier )
   {
      // if we are removing the first group along with its start pier, don't 
      // move the bridge.

      // DO NOTHING HERE
   }
   else
   {
      PierIndexType nPiers = GetPierCount();
      for ( PierIndexType pierIdx = endPierIdx+1; pierIdx < nPiers; pierIdx++ )
      {
         CPierData2* pPier = m_Piers[pierIdx];
         pPier->SetStation(pPier->GetStation() - group_length);
      }
   }

   // remove deleted slots from the pier vector
   m_Piers.erase(m_Piers.begin()+startPierIdx,m_Piers.begin()+endPierIdx+1);
   
   // move all the temporary supports that occur after the last pier in the group to the left
   // by an amount equal to the length of the group
   for(auto* pTS : m_TemporarySupports)
   {
      if ( endSpanIdx < pTS->GetSpan()->GetIndex() )
      {
         pTS->SetStation(pTS->GetStation() - group_length);
      }
   }

   // delete the spans
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      CSpanData2* pSpan = m_Spans[spanIdx];
      delete pSpan;
      pSpan = nullptr;
   }

   // remove deleted slots from span vector
   m_Spans.erase(m_Spans.begin()+startSpanIdx,m_Spans.begin()+endSpanIdx+1);

   // delete group
   delete pGroup;
   pGroup = nullptr;

   // remove slot from group vector
   m_GirderGroups.erase(m_GirderGroups.begin()+grpIdx);

   RenumberSpans();
   RenumberGroups();

   PGS_ASSERT_VALID;
}

PierIndexType CBridgeDescription2::GetPierCount() const
{
   return m_Piers.size();
}

SpanIndexType CBridgeDescription2::GetSpanCount() const
{
   return m_Spans.size();
}

CPierData2* CBridgeDescription2::GetPier(PierIndexType pierIdx)
{
   if (0 <= pierIdx && pierIdx < (PierIndexType)m_Piers.size() )
   {
     return m_Piers[pierIdx];
   }

   return nullptr;
}

const CPierData2* CBridgeDescription2::GetPier(PierIndexType pierIdx) const
{
   if (0 <= pierIdx && pierIdx < (PierIndexType)m_Piers.size() )
   {
      return m_Piers[pierIdx];
   }

   return nullptr;
}

CPierData2* CBridgeDescription2::FindPier(PierIDType pierID)
{
   for(auto* pPier : m_Piers)
   {
      if ( pPier->GetID() == pierID )
      {
         return pPier;
      }
   }

   ATLASSERT(false); // Pier not found
   return nullptr;
}

const CPierData2* CBridgeDescription2::FindPier(PierIDType pierID) const
{
   for (const auto* pPier : m_Piers)
   {
      if ( pPier->GetID() == pierID )
      {
         return pPier;
      }
   }

   ATLASSERT(false); // Pier not found
   return nullptr;
}

CSpanData2* CBridgeDescription2::GetSpan(SpanIndexType spanIdx)
{
   if ( 0 <= spanIdx && spanIdx < (SpanIndexType)m_Spans.size() )
   {
     return m_Spans[spanIdx];
   }

   return nullptr;
}

const CSpanData2* CBridgeDescription2::GetSpan(SpanIndexType spanIdx) const
{
   if ( 0 <= spanIdx && spanIdx < (SpanIndexType)m_Spans.size() )
   {
      return m_Spans[spanIdx];
   }

   return nullptr;
}


bool CBridgeDescription2::MovePier(PierIndexType pierIdx,Float64 newStation,pgsTypes::MovePierOption moveOption)
{
   bool bRetVal = false;
   switch( moveOption )
   {
   case pgsTypes::MoveBridge:
      bRetVal = MoveBridge(pierIdx,newStation);
      break;

   case pgsTypes::AdjustPrevSpan:
      bRetVal = MoveBridgeAdjustPrevSpan(pierIdx,newStation);
      break;

   case pgsTypes::AdjustNextSpan:
      bRetVal = MoveBridgeAdjustNextSpan(pierIdx,newStation);
      break;

   case pgsTypes::AdjustAdjacentSpans:
      bRetVal = MoveBridgeAdjustAdjacentSpans(pierIdx,newStation);
      break;
   }

   return bRetVal;
}

void CBridgeDescription2::SetPierDisplaySettings(pgsTypes::DisplayEndSupportType startPierType, pgsTypes::DisplayEndSupportType endPierType, PierIndexType startPierNumber)
{
   m_DisplayStartSupportType = startPierType;
   m_DisplayEndSupportType = endPierType;
   m_StartingPierNumber = startPierNumber;
}

void CBridgeDescription2::GetPierDisplaySettings(pgsTypes::DisplayEndSupportType* pStartPierType, pgsTypes::DisplayEndSupportType* pEndPierType, PierIndexType* pStartPierNumber) const
{
   *pStartPierType = m_DisplayStartSupportType;
   *pEndPierType = m_DisplayEndSupportType;
   *pStartPierNumber = m_StartingPierNumber;
}

PierIndexType CBridgeDescription2::GetDisplayStartingPierNumber() const
{
   return m_StartingPierNumber;
}

void CBridgeDescription2::SetDisplayStartingPierNumber(PierIndexType num)
{
   m_StartingPierNumber = num;
}

pgsTypes::DisplayEndSupportType CBridgeDescription2::GetDisplayStartSupportType() const
{
   return m_DisplayStartSupportType;
}

void CBridgeDescription2::SetDisplayStartSupportType(pgsTypes::DisplayEndSupportType dtype)
{
   m_DisplayStartSupportType = dtype;
}

pgsTypes::DisplayEndSupportType CBridgeDescription2::GetDisplayEndSupportType() const
{
   return m_DisplayEndSupportType;
}

void CBridgeDescription2::SetDisplayEndSupportType(pgsTypes::DisplayEndSupportType dtype)
{
   m_DisplayEndSupportType = dtype;
}

bool CBridgeDescription2::SetSpanLength(SpanIndexType spanIdx,Float64 newLength)
{
   _ASSERT( 0 < newLength );
   CSpanData2* pSpan = GetSpan(spanIdx);
   Float64 length = pSpan->GetSpanLength();
   Float64 deltaL = newLength - length;

   if ( IsZero(deltaL) )
   {
      return false;
   }

   Float64 startSpanStation = pSpan->GetPrevPier()->GetStation();
   Float64 endSpanStation = pSpan->GetNextPier()->GetStation();

   // move all the piers from the end of this span to the end of the bridge
   while ( pSpan )
   {
      CPierData2* pNextPier = pSpan->GetNextPier();
      pNextPier->SetStation( pNextPier->GetStation() + deltaL);

      pSpan = pNextPier->GetNextSpan();
   }

   pSpan = GetSpan(spanIdx);
   Float64 newEndSpanStation = pSpan->GetNextPier()->GetStation();

   for( auto* pTS : m_TemporarySupports)
   {
      Float64 tsStation = pTS->GetStation();

      if ( endSpanStation < tsStation )
      {
         // temporary support is in a span that follows the span whose length is changing
         pTS->SetStation( tsStation + deltaL );
      }
      else if ( newEndSpanStation < tsStation )
      {
         // the temporary support is in the span that is changing length and the end of the span will come before the temporary 
         // support making it invalid. move the temorary support keeping its relative location within the span

         ATLASSERT(::InRange(startSpanStation,tsStation,endSpanStation));
         ATLASSERT(deltaL < 0);

         // for shrinking spans, maintain the relative position of the temporary support within the span
         Float64 newTSStation = (tsStation - startSpanStation)*(endSpanStation + deltaL - startSpanStation)/(endSpanStation - startSpanStation) + startSpanStation;
         pTS->SetStation(newTSStation);
      }
   }

   return true;
}

SupportIndexType CBridgeDescription2::AddTemporarySupport(CTemporarySupportData* pTempSupport,EventIndexType erectionEventIdx,EventIndexType removalEventIdx,EventIndexType castClosureEventIdx)
{
   if ( pTempSupport->GetID() == INVALID_ID )
   {
      pTempSupport->SetID( GetNextTemporaryID() );
   }

   m_TemporarySupports.push_back(pTempSupport);
   UpdateTemporarySupports();

   m_TimelineManager.SetTempSupportEvents(pTempSupport->GetID(),erectionEventIdx,removalEventIdx);

   if ( pTempSupport->GetConnectionType() == pgsTypes::tsctClosureJoint )
   {
      CGirderGroupData* pGroup = GetGirderGroup(pTempSupport->GetSpan());
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         pGirder->SplitSegmentsAtTemporarySupport(pTempSupport->GetIndex());
      }

      pTempSupport->GetSegmentSpacing()->SetGirderCount(nGirders);

      CClosureJointData* pClosure = pTempSupport->GetClosureJoint(0);
      ClosureIDType closureID = pClosure->GetID();

      m_TimelineManager.SetCastClosureJointEventByIndex(closureID,castClosureEventIdx);
   }

   return pTempSupport->GetIndex();
}

SupportIndexType CBridgeDescription2::GetTemporarySupportCount() const
{
   return m_TemporarySupports.size();
}

CTemporarySupportData* CBridgeDescription2::GetTemporarySupport(SupportIndexType tsIdx)
{
   if ( 0 <= tsIdx && tsIdx < (SupportIndexType)m_TemporarySupports.size() )
   {
      return m_TemporarySupports[tsIdx];
   }
   
   return nullptr;
}

const CTemporarySupportData* CBridgeDescription2::GetTemporarySupport(SupportIndexType tsIdx) const
{
   if ( 0 <= tsIdx && tsIdx < (SupportIndexType)m_TemporarySupports.size() )
   {
      return m_TemporarySupports[tsIdx];
   }
   
   return nullptr;
}

CTemporarySupportData* CBridgeDescription2::FindTemporarySupport(SupportIDType tsID)
{
   for( auto* pTS : m_TemporarySupports)
   {
      if ( pTS->GetID() == tsID )
      {
         return pTS;
      }
   }
   
   ATLASSERT(false); // not found
   return nullptr;
}

const CTemporarySupportData* CBridgeDescription2::FindTemporarySupport(SupportIDType tsID) const
{
   for (const auto* pTS : m_TemporarySupports)
   {
      if ( pTS->GetID() == tsID )
      {
         return pTS;
      }
   }
   
   ATLASSERT(false); // temporary support not found
   return nullptr;
}

SupportIndexType CBridgeDescription2::SetTemporarySupportByIndex(SupportIndexType tsIdx,const CTemporarySupportData& tsData)
{
   ATLASSERT( 0 <= tsIdx && tsIdx < (SupportIndexType)m_TemporarySupports.size() );
   ATLASSERT( m_Piers.front()->GetStation() < tsData.GetStation() && tsData.GetStation() < m_Piers.back()->GetStation() );

   CTemporarySupportData* pTS = m_TemporarySupports[tsIdx];
   ATLASSERT(pTS->GetIndex() == tsIdx);

   if ( !IsEqual(pTS->GetStation(),tsData.GetStation()) )
   {
      CSpanData2* pSpan = pTS->GetSpan();
      if ( ::InRange(pSpan->GetPrevPier()->GetStation(),tsData.GetStation(),pSpan->GetNextPier()->GetStation()) )
      {
         // temporary support is not changing spans
         std::vector<CTemporarySupportData*> vTS = pSpan->GetTemporarySupports();
         ATLASSERT( 1 <= vTS.size() );
         SupportIndexType ltsIdx = tsIdx - vTS.front()->GetIndex(); // local temporary support index (index in vTS... tsIdx is index in m_TemporarySupports)

         Float64 thisTSStation = tsData.GetStation();
         Float64 prevTSStation = (ltsIdx == 0 ? -DBL_MAX : vTS[ltsIdx-1]->GetStation());
         Float64 nextTSStation = (ltsIdx == vTS.size()-1 ? DBL_MAX : vTS[ltsIdx+1]->GetStation());
         if ( prevTSStation < thisTSStation && thisTSStation < nextTSStation )
         {
            // this temporary support does not change relative position in the span with respect to the other temporary supports
            pTS->CopyTemporarySupportData(&tsData);
            return tsIdx;
         }
         else
         {
            // temporary support has moved... remove it and re-insert it at the new location
            CTemporarySupportData* pNewTS = new CTemporarySupportData(tsData);
            pNewTS->SetID(pTS->GetID()); // maintain its ID

            EventIndexType erectionEventIdx, removeEventIdx;
            m_TimelineManager.GetTempSupportEvents(pTS->GetID(),&erectionEventIdx,&removeEventIdx);

            EventIndexType castClosureJointEventIdx = INVALID_INDEX;
            CClosureJointData* pClosure = pTS->GetClosureJoint(0);
            if ( pClosure )
            {
               castClosureJointEventIdx = m_TimelineManager.GetCastClosureJointEventIndex(pClosure);
            }

            RemoveTemporarySupportByIndex(tsIdx);

            // LEAVE THE FUNCTION HERE
            AddTemporarySupport(pNewTS,erectionEventIdx,removeEventIdx,castClosureJointEventIdx);
            return pNewTS->GetIndex();
         }
      }
      else
      {
         // temporary support has moved... remove it and re-insert it at the new location
         CTemporarySupportData* pNewTS = new CTemporarySupportData(tsData);
         pNewTS->SetID(pTS->GetID()); // maintain its ID

         EventIndexType erectionEventIdx, removeEventIdx;
         m_TimelineManager.GetTempSupportEvents(pTS->GetID(),&erectionEventIdx,&removeEventIdx);

         EventIndexType castClosureJointEventIdx = INVALID_INDEX;
         CClosureJointData* pClosure = pTS->GetClosureJoint(0);
         if ( pClosure )
         {
            castClosureJointEventIdx = m_TimelineManager.GetCastClosureJointEventIndex(pClosure);
         }

         RemoveTemporarySupportByIndex(tsIdx);

         // LEAVE THE FUNCTION HERE
         AddTemporarySupport(pNewTS,erectionEventIdx,removeEventIdx,castClosureJointEventIdx);
         return pNewTS->GetIndex();
      }
   }

   // ONLY CONTINUE ON IF THE TEMPORARY SUPPORT DIDN'T MOVE

   // if the connection type changed, then we have to alter the spliced girder segments
   if ( pTS->GetConnectionType() != tsData.GetConnectionType() )
   {
      CGirderGroupData* pGroup = GetGirderGroup(pTS->GetSpan());
      ATLASSERT(pGroup);

      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         if ( tsData.GetConnectionType() == pgsTypes::tsctContinuousSegment )
         {
            // segment is becoming continuous over this temporary support. need to join segments into one
            pGirder->JoinSegmentsAtTemporarySupport( tsIdx );
         }
         else
         {
            // segment needs to be broken into two segments at this temporary support
            // because the connection type is now closure joint
            pGirder->SplitSegmentsAtTemporarySupport( tsIdx );
         }
      }

      if ( tsData.GetConnectionType() == pgsTypes::tsctClosureJoint )
      {
         pTS->GetSegmentSpacing()->SetGirderCount(nGirders);
      }
   }

   pTS->CopyTemporarySupportData(&tsData); // copies data, but does not change the ID

   UpdateTemporarySupports();

   PGS_ASSERT_VALID;

   return pTS->GetIndex();
}

SupportIndexType CBridgeDescription2::SetTemporarySupportByID(SupportIDType tsID,const CTemporarySupportData& tsData)
{
   for(auto* pTS : m_TemporarySupports)
   {
      if ( pTS->GetID() == tsID )
      {
         SupportIndexType tsIdx = pTS->GetIndex();
         return SetTemporarySupportByIndex(tsIdx,tsData);
      }
   }
   ATLASSERT(false); // should never get here if tsID is valid
   return INVALID_INDEX;
}

void CBridgeDescription2::RemoveTemporarySupportByIndex(SupportIndexType tsIdx)
{
   ATLASSERT( 0 <= tsIdx && tsIdx < (SupportIndexType)m_TemporarySupports.size() );

   CTemporarySupportData* pTS = m_TemporarySupports[tsIdx];
   ATLASSERT(pTS->GetIndex() == tsIdx);

   // Remove the temporary support from the spliced girders before it is actually gone
   if ( pTS->GetConnectionType() == pgsTypes::tsctClosureJoint )
   {
      // remove the closure joint from the timeline
      const CClosureJointData* pClosure = pTS->GetClosureJoint(0);
      CTimelineManager* pTimelineMgr = GetTimelineManager();
      if ( pTimelineMgr )
      {
         EventIndexType eventIdx = pTimelineMgr->GetCastClosureJointEventIndex(pClosure);
         if ( eventIdx != INVALID_INDEX )
         {
            ATLASSERT(pClosure->GetPier() == nullptr);
            ATLASSERT(pClosure->GetTemporarySupport() && pClosure->GetTemporarySupport() == pTS);
            CTimelineEvent* pTimelineEvent = m_TimelineManager.GetEventByIndex(eventIdx);
            pTimelineEvent->GetCastClosureJointActivity().RemoveTempSupport(pTS->GetID());
         }
      }

      const CSpanData2* pSpan = pTS->GetSpan();
      CGirderGroupData* pGroup = GetGirderGroup(pSpan);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         pGirder->JoinSegmentsAtTemporarySupport(tsIdx);
      }
   }

   // remove temporary support from the timeline
   // (note that the closure joint was removed from the timeline when the segments were joined)
   SupportIDType tsID = pTS->GetID();
   //const CClosureJointData* pClosure = pTS->GetClosureJoint(0);
   //if ( pClosure )
   //{
   //   EventIndexType castClosureEventIdx = m_TimelineManager.GetCastClosureJointEventIndex(pClosure);
   //   m_TimelineManager.GetEventByIndex(castClosureEventIdx)->GetCastClosureJointActivity().RemoveTempSupport(tsID);
   //}

   EventIndexType erectEventIdx, removeEventIdx;
   m_TimelineManager.GetTempSupportEvents(tsID,&erectEventIdx,&removeEventIdx);
   m_TimelineManager.GetEventByIndex(erectEventIdx)->GetErectPiersActivity().RemoveTempSupport(tsID);
   m_TimelineManager.GetEventByIndex(removeEventIdx)->GetRemoveTempSupportsActivity().RemoveTempSupport(tsID);

   delete pTS;
   m_TemporarySupports.erase(m_TemporarySupports.begin()+tsIdx);

   UpdateTemporarySupports();
   PGS_ASSERT_VALID;
}

void CBridgeDescription2::RemoveTemporarySupportByID(SupportIDType tsID)
{
   for (auto* pTS : m_TemporarySupports)
   {
      if ( pTS->GetID() == tsID )
      {
         SupportIndexType tsIdx = pTS->GetIndex();
         RemoveTemporarySupportByIndex(tsIdx);
         break;
      }
   }
   PGS_ASSERT_VALID;
}

SupportIndexType CBridgeDescription2::MoveTemporarySupport(SupportIndexType tsIdx,Float64 newStation)
{
   ATLASSERT(0 <= tsIdx && tsIdx < (SupportIndexType)m_TemporarySupports.size() );

   CTemporarySupportData tsData = *m_TemporarySupports[tsIdx];
   tsData.SetStation(newStation);
   SupportIndexType idx = SetTemporarySupportByIndex(tsIdx,tsData);
   PGS_ASSERT_VALID;
   return idx;
}

void CBridgeDescription2::Clear()
{
   m_Deck.DeckEdgePoints.clear();
   m_Deck.DeckRebarData.NegMomentRebar.clear();

   ClearGirderGroups();

   std::for_each(std::begin(m_TemporarySupports), std::end(m_TemporarySupports), [](auto* pTS) {delete pTS; });
   m_TemporarySupports.clear();

   std::for_each(std::begin(m_Piers), std::end(m_Piers), [](auto* pPier) {delete pPier; });
   m_Piers.clear();

   std::for_each(std::begin(m_Spans), std::end(m_Spans), [](auto* pSpan) {delete pSpan; });
   m_Spans.clear();

   m_TimelineManager.Clear();

   m_TempSupportID = 0;
   m_SegmentID     = 0;
   m_PierID        = 0;
   m_GirderID      = 0;
}

Float64 CBridgeDescription2::GetLength() const
{
   if ( m_Piers.size() == 0 )
   {
      return 0.0;
   }

   return m_Piers.back()->GetStation() - m_Piers.front()->GetStation();
}

void CBridgeDescription2::GetStationRange(Float64* pStartStation,Float64* pEndStation) const
{
   if ( m_Piers.size() == 0 )
   {
      *pStartStation = 0;
      *pEndStation   = 0;
      return;
   }

   *pStartStation = m_Piers.front()->GetStation();
   *pEndStation   = m_Piers.back()->GetStation();
}

bool CBridgeDescription2::IsOnBridge(Float64 station) const
{
   Float64 startStation, endStation;
   GetStationRange(&startStation,&endStation);
   return ::InRange(startStation,station,endStation);
}

PierIndexType CBridgeDescription2::IsPierLocation(Float64 station,Float64 tolerance) const
{
   for(const auto* pPier : m_Piers)
   {
      if ( IsEqual(station,pPier->GetStation(),tolerance) )
      {
         return pPier->GetIndex();
      }
   }

   return INVALID_INDEX;
}

SupportIndexType CBridgeDescription2::IsTemporarySupportLocation(Float64 station,Float64 tolerance) const
{
   for(const auto* pTS : m_TemporarySupports)
   {
      if ( IsEqual(station,pTS->GetStation(),tolerance) )
      {
         return pTS->GetIndex();
      }
   }

   return INVALID_INDEX;
}

void CBridgeDescription2::UseSameNumberOfGirdersInAllGroups(bool bSame) 
{
   if ( m_bSameNumberOfGirders != bSame )
   {
      m_bSameNumberOfGirders = bSame;

      // make sure the internal data structures for all groups are
      // set to the correct number of girders
      std::for_each(std::begin(m_GirderGroups), std::end(m_GirderGroups), [nGirders=m_nGirders](auto* pGroup) {pGroup->SetGirderCount(nGirders); });
   }
}

bool CBridgeDescription2::UseSameNumberOfGirdersInAllGroups() const
{
   return m_bSameNumberOfGirders;
}

void CBridgeDescription2::SetGirderCount(GirderIndexType nGirders)
{
   m_nGirders = nGirders;

   if ( m_bSameNumberOfGirders )
   {
      for(auto* pGroup : m_GirderGroups)
      {
         pGroup->SetGirderCount(nGirders);

         CSpanData2* pStartSpan = pGroup->GetPier(pgsTypes::metStart)->GetNextSpan();
         CSpanData2* pEndSpan   = pGroup->GetPier(pgsTypes::metEnd)->GetNextSpan();
         bool bDone = false;
         CSpanData2* pSpan = pStartSpan;
         while ( !bDone )
         {
            std::vector<CTemporarySupportData*> vTS = pSpan->GetTemporarySupports();
            std::for_each(std::begin(vTS), std::end(vTS), [&nGirders](auto* pTS) {pTS->GetSegmentSpacing()->SetGirderCount(nGirders); });

            pSpan = pSpan->GetNextPier()->GetNextSpan();
            if ( pSpan == pEndSpan )
            {
               bDone = true;
            }
         }
      }
   }
}

GirderIndexType CBridgeDescription2::GetGirderCount() const
{
   return m_nGirders;
}

GirderIndexType CBridgeDescription2::GetMinGirderCount() const
{
   if (m_bSameNumberOfGirders)
   {
      return m_nGirders;
   }
   else
   {
      GirderIndexType nGirders = MAX_INDEX;
      for (auto group : m_GirderGroups)
      {
         nGirders = min(nGirders, group->GetGirderCount());
      }
      return nGirders;
   }
}

GirderIndexType CBridgeDescription2::GetMaxGirderCount() const
{
   if (m_bSameNumberOfGirders)
   {
      return m_nGirders;
   }
   else
   {
      GirderIndexType nGirders = 0;
      for (auto group : m_GirderGroups)
      {
         nGirders = max(nGirders,group->GetGirderCount());
      }
      return nGirders;
   }
}

void CBridgeDescription2::UseSameGirderForEntireBridge(bool bSame)
{
   m_bSameGirderName = bSame;
}

bool CBridgeDescription2::UseSameGirderForEntireBridge() const
{
   return m_bSameGirderName;
}

LPCTSTR CBridgeDescription2::GetGirderName() const
{
   return m_strGirderName.c_str();
}

void CBridgeDescription2::RenameGirder(LPCTSTR strName)
{
   m_strGirderName = strName;
}

void CBridgeDescription2::SetGirderName(LPCTSTR strName)
{
   if ( m_strGirderName != strName )
   {
      // girder name changed...
      m_strGirderName = strName;

      // need to reset prestressing data
      // need to reset longitudinal reinforcement
      // need to reset transverse reinforcement
      for(auto* pGroup : m_GirderGroups)
      {
         GirderIndexType nGirders = pGroup->GetGirderCount();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
            SegmentIndexType nSegments = pGirder->GetSegmentCount();
            for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
            {
               CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
               pSegment->Strands.ResetPrestressData();

               if ( m_pGirderLibraryEntry )
               {
                  pSegment->ShearData = m_pGirderLibraryEntry->GetShearData();
                  pSegment->LongitudinalRebarData.CopyGirderEntryData(m_pGirderLibraryEntry);
               }
            }
         }
      }
   }
}

const GirderLibraryEntry* CBridgeDescription2::GetGirderLibraryEntry() const
{
   return m_pGirderLibraryEntry;
}

void CBridgeDescription2::SetGirderLibraryEntry(const GirderLibraryEntry* pEntry)
{
   if ( m_pGirderLibraryEntry != pEntry )
   {
      // girder entry changed...
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
            for(auto* pGroup : m_GirderGroups)
            {
               GirderIndexType nGirders = pGroup->GetGirderCount();
               for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
               {
                  CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
                  SegmentIndexType nSegments = pGirder->GetSegmentCount();
                  for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
                  {
                     CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
                     std::vector<pgsTypes::SegmentVariationType>::iterator found = std::find(variations.begin(),variations.end(),pSegment->GetVariationType());
                     if ( found == variations.end() )
                     {
                        // the current setting fot the segment variation is no longer a valid
                        // value, so change it to the first available value
                        pSegment->SetVariationType(variations.front());
                     } // end if
                  } // next segment
               } // next girder
            } // next group
         } // if spliced girder
      } // if not null
   } // if lib entry different
}

void CBridgeDescription2::SetGirderOrientation(pgsTypes::GirderOrientationType gdrOrientation)
{
   m_GirderOrientation = gdrOrientation;
}

pgsTypes::GirderOrientationType CBridgeDescription2::GetGirderOrientation() const
{
   return m_GirderOrientation;
}

void CBridgeDescription2::SetGirderSpacingType(pgsTypes::SupportedBeamSpacing sbs)
{
   bool bExpandGroups = IsBridgeSpacing(m_GirderSpacingType) && !IsBridgeSpacing(sbs) ? true : false;
   m_GirderSpacingType = sbs;
   if (bExpandGroups)
   {
      for (auto& group : m_GirderGroups)
      {
         if (group->GetGirderTypeGroupCount() <= 1)
         {
            group->ExpandAll();
            group->JoinAll(0);
         }

         if (group->GetGirderTopWidthGroupCount() <= 1)
         {
            group->ExpandAllGirderTopWidthGroups();
            group->JoinAllGirderTopWidthGroups(0);
         }
      }
   }
}

pgsTypes::SupportedBeamSpacing CBridgeDescription2::GetGirderSpacingType() const
{
   return m_GirderSpacingType;
}

void CBridgeDescription2::SetGirderSpacing(Float64 spacing)
{
   m_GirderSpacing = spacing;
}

Float64 CBridgeDescription2::GetGirderSpacing() const
{
   return m_GirderSpacing;
}

void CBridgeDescription2::SetGirderTopWidth(pgsTypes::TopWidthType type,Float64 left,Float64 right)
{
   m_TopWidthType = type;
   m_LeftTopWidth = left;
   m_RightTopWidth = right;
}

void CBridgeDescription2::GetGirderTopWidth(pgsTypes::TopWidthType* pType, Float64* pLeft, Float64* pRight) const
{
   *pType = m_TopWidthType;
   *pLeft = m_LeftTopWidth;
   *pRight = m_RightTopWidth;
}

Float64 CBridgeDescription2::GetGirderTopWidth() const
{
   pgsTypes::TopWidthType type;
   Float64 left, right;
   GetGirderTopWidth(&type, &left, &right);
   switch (type)
   {
   case pgsTypes::twtSymmetric:
   case pgsTypes::twtCenteredCG:
      return left;

   case pgsTypes::twtAsymmetric:
      return left + right;

   default:
      ATLASSERT(false); // should never get here... assume full
      return left;
   }
}

void CBridgeDescription2::SetMeasurementType(pgsTypes::MeasurementType mt)
{
   m_MeasurementType = mt;
}

pgsTypes::MeasurementType CBridgeDescription2::GetMeasurementType() const
{
   return m_MeasurementType;
}

void CBridgeDescription2::SetMeasurementLocation(pgsTypes::MeasurementLocation ml)
{
   m_MeasurementLocation = ml;
}

pgsTypes::MeasurementLocation CBridgeDescription2::GetMeasurementLocation() const
{
   return m_MeasurementLocation;
}

void CBridgeDescription2::SetWorkPointLocation(pgsTypes::WorkPointLocation wl)
{
   m_WorkPointLocation = wl;
}

pgsTypes::WorkPointLocation CBridgeDescription2::GetWorkPointLocation() const
{
   return m_WorkPointLocation;
}

void CBridgeDescription2::SetRefGirder(GirderIndexType refGdrIdx)
{
   m_RefGirderIdx = refGdrIdx;
}

GirderIndexType CBridgeDescription2::GetRefGirder() const
{
   return m_RefGirderIdx;
}

void CBridgeDescription2::SetRefGirderOffset(Float64 offset)
{
   m_RefGirderOffset = offset;
}

Float64 CBridgeDescription2::GetRefGirderOffset() const
{
   return m_RefGirderOffset;
}

void CBridgeDescription2::SetRefGirderOffsetType(pgsTypes::OffsetMeasurementType offsetType)
{
   m_RefGirderOffsetType = offsetType;
}

pgsTypes::OffsetMeasurementType CBridgeDescription2::GetRefGirderOffsetType() const
{
   return m_RefGirderOffsetType;
}

void CBridgeDescription2::MakeCopy(const CBridgeDescription2& rOther)
{
#if defined _DEBUG
   const_cast<CBridgeDescription2*>(&rOther)->AssertValid();
#endif

   // clear out the deck, spans and piers... then rebuild
   Clear();

   m_TempSupportID = rOther.m_TempSupportID;
   m_PierID        = rOther.m_PierID;
   m_SegmentID     = rOther.m_SegmentID;
   m_GirderID      = rOther.m_GirderID;
   m_GirderGroupID = rOther.m_GirderGroupID;


   m_strGirderFamilyName      = rOther.m_strGirderFamilyName;

   m_bSameNumberOfGirders     = rOther.m_bSameNumberOfGirders;
   m_nGirders                 = rOther.m_nGirders;

   m_HaunchInputDepthType     = rOther.m_HaunchInputDepthType;

   m_SlabOffset               = rOther.m_SlabOffset;
   m_SlabOffsetType           = rOther.m_SlabOffsetType;

   m_Fillet               = rOther.m_Fillet;

   m_AssumedExcessCamber = rOther.m_AssumedExcessCamber;
   m_AssumedExcessCamberType      = rOther.m_AssumedExcessCamberType;

   m_HaunchInputLocationType     = rOther.m_HaunchInputLocationType;
   m_HaunchLayoutType            = rOther.m_HaunchLayoutType;
   m_HaunchInputDistributionType = rOther.m_HaunchInputDistributionType;
   m_HaunchDepths                = rOther.m_HaunchDepths;

   m_BearingData          = rOther.m_BearingData;
   m_BearingType          = rOther.m_BearingType;

   m_bSameGirderName          = rOther.m_bSameGirderName;
   m_strGirderName            = rOther.m_strGirderName;
   m_pGirderLibraryEntry      = rOther.m_pGirderLibraryEntry;
   m_GirderOrientation        = rOther.m_GirderOrientation;

   m_GirderSpacingType        = rOther.m_GirderSpacingType;
   m_GirderSpacing            = rOther.m_GirderSpacing;
   m_TopWidthType             = rOther.m_TopWidthType;
   m_LeftTopWidth             = rOther.m_LeftTopWidth;
   m_RightTopWidth            = rOther.m_RightTopWidth;
   m_MeasurementType          = rOther.m_MeasurementType;
   m_MeasurementLocation      = rOther.m_MeasurementLocation;
   m_WorkPointLocation      = rOther.m_WorkPointLocation;
   m_RefGirderIdx             = rOther.m_RefGirderIdx;
   m_RefGirderOffset          = rOther.m_RefGirderOffset;
   m_RefGirderOffsetType      = rOther.m_RefGirderOffsetType;

   m_TimelineManager          = rOther.m_TimelineManager;
   m_TimelineManager.SetBridgeDescription(this);

   m_LeftRailingSystem        = rOther.m_LeftRailingSystem;
   m_RightRailingSystem       = rOther.m_RightRailingSystem;
   m_AlignmentOffset          = rOther.m_AlignmentOffset;

   m_StartingPierNumber      = rOther.m_StartingPierNumber;
   m_DisplayStartSupportType = rOther.m_DisplayStartSupportType;
   m_DisplayEndSupportType   = rOther.m_DisplayEndSupportType;
   
   m_Deck                     = rOther.m_Deck;
   m_Deck.SetBridgeDescription(this);

   m_LLDFMethod               = rOther.m_LLDFMethod;

   m_LongitudinalJointConcrete = rOther.m_LongitudinalJointConcrete;

   // Copy Piers
   for(const auto* pPier : rOther.m_Piers)
   {
      CPierData2* pNewPier = new CPierData2;
      *pNewPier = *pPier; // assign everything

      pNewPier->SetBridgeDescription(this);
     
      m_Piers.push_back(pNewPier);
   }

   // Copy Spans
   for(const auto* pSpan : rOther.m_Spans)
   {
      CSpanData2* pNewSpan = new CSpanData2;

      *pNewSpan = *pSpan; // assign everything

      pNewSpan->SetBridgeDescription(this);
      
      m_Spans.push_back(pNewSpan);
   }

   // Hookup the pier<->span<->pier pointers (also updates the indices)
   RenumberSpans();

   // Copy Temporary Supports
   for(const auto* pTS : rOther.m_TemporarySupports)
   {
      CTemporarySupportData* pNewTS = new CTemporarySupportData;
      *pNewTS = *pTS; // assign everything
      ATLASSERT(*pNewTS == *pTS);
      m_TemporarySupports.push_back(pNewTS);
   }

   // Updates the temporary supports pointer to its containing span
   UpdateTemporarySupports();


   // Copy Girder Groups
   // This logically comes first, but it must be last. When a group is copied, it copies
   // girders, segments, and closures.... it also resolves the references to temporary supports,
   // piers, and spans. In order to resolve these references, they must exist.
   for(const auto* pGroup : rOther.m_GirderGroups)
   {
      CGirderGroupData* pNewGroup = new CGirderGroupData(this);
      *pNewGroup = *pGroup;
      m_GirderGroups.push_back(pNewGroup);
   }

   bool doSlabOffset = m_HaunchInputDepthType == pgsTypes::hidACamber && m_SlabOffsetType == pgsTypes::sotBridge;
   bool doAssumedCamber = m_HaunchInputDepthType == pgsTypes::hidACamber && m_AssumedExcessCamberType == pgsTypes::aecBridge;
   bool doDirectHaunch = m_HaunchInputDepthType != pgsTypes::hidACamber && m_HaunchInputLocationType == pgsTypes::hilSame4Bridge;

   CopyDown(m_bSameNumberOfGirders,m_bSameGirderName,::IsBridgeSpacing(m_GirderSpacingType),
            doSlabOffset,doAssumedCamber,doDirectHaunch,
            m_BearingType==pgsTypes::brtBridge); 

   PGS_ASSERT_VALID;
}

void CBridgeDescription2::MakeAssignment(const CBridgeDescription2& rOther)
{
   MakeCopy( rOther );
}

void CBridgeDescription2::SetDistributionFactorMethod(pgsTypes::DistributionFactorMethod method)
{
   m_LLDFMethod = method;
}

pgsTypes::DistributionFactorMethod CBridgeDescription2::GetDistributionFactorMethod() const
{
   return m_LLDFMethod;
}

CGirderGroupData* CBridgeDescription2::FindGirderGroup(GroupIDType grpID)
{
   for(auto* pGroup : m_GirderGroups)
   {
      if ( pGroup->GetID() == grpID )
      {
         return pGroup;
      }
   }

   ATLASSERT(false); // not found
   return nullptr;
}

const CGirderGroupData* CBridgeDescription2::FindGirderGroup(GroupIDType grpID) const
{
   for (const auto* pGroup : m_GirderGroups)
   {
      if ( pGroup->GetID() == grpID )
      {
         return pGroup;
      }
   }

   ATLASSERT(false); // not found
   return nullptr;
}

CSplicedGirderData* CBridgeDescription2::FindGirder(GirderIDType gdrID)
{
   for (auto* pGroup : m_GirderGroups)
   {
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         if ( pGirder->GetID() == gdrID )
         {
            return pGirder;
         }
      }
   }

   ATLASSERT(false); // not found
   return nullptr;
}

const CSplicedGirderData* CBridgeDescription2::FindGirder(GirderIDType gdrID) const
{
   for (const auto* pGroup : m_GirderGroups)
   {
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         if ( pGirder->GetID() == gdrID )
         {
            return pGirder;
         }
      }
   }

   ATLASSERT(false); // not found
   return nullptr;
}

CPrecastSegmentData* CBridgeDescription2::FindSegment(SegmentIDType segID)
{
   for (auto* pGroup : m_GirderGroups)
   {
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            if ( pSegment->GetID() == segID )
            {
               return pSegment;
            }
         }
      }
   }

   ATLASSERT(false); // not found
   return nullptr;
}

const CPrecastSegmentData* CBridgeDescription2::FindSegment(SegmentIDType segID) const
{
   for (const auto* pGroup : m_GirderGroups)
   {
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            if ( pSegment->GetID() == segID )
            {
               return pSegment;
            }
         }
      }
   }

   ATLASSERT(false); // not found
   return nullptr;
}

CClosureJointData* CBridgeDescription2::FindClosureJoint(ClosureIDType closureID)
{
   for (auto* pGroup : m_GirderGroups)
   {
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for (SegmentIndexType segIdx = 0; segIdx < nSegments-1; segIdx++ )
         {
            CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            CClosureJointData* pClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);
            if ( pClosure->GetID() == closureID )
            {
               return pClosure;
            }
         }
      }
   }

   ATLASSERT(false); // not found
   return nullptr;
}

const CClosureJointData* CBridgeDescription2::FindClosureJoint(ClosureIDType closureID) const
{
   for (const auto* pGroup : m_GirderGroups)
   {
      GirderIndexType nGirders = pGroup->GetPrivateGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         if ( nSegments == 0 )
         {
            continue;
         }

         for (SegmentIndexType segIdx = 0; segIdx < nSegments-1; segIdx++ )
         {
            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            const CClosureJointData* pClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);
            if ( pClosure->GetID() == closureID )
            {
               return pClosure;
            }
         }
      }
   }

   return nullptr;
}

void CBridgeDescription2::CopyDown(bool bGirderCount,bool bGirderType,bool bSpacing,bool bSlabOffset,bool bAssumedExcessCamber,bool bDirectHaunchDepths,bool bBearingData)
{
   for (auto* pGroup : m_GirderGroups)
   {
      if ( bGirderCount )
      {
         pGroup->SetGirderCount( m_nGirders );
      }

      if ( bGirderType )
      {
         pGroup->JoinAll(0);
         pGroup->SetGirderName(0,m_strGirderName.c_str());
         pGroup->SetGirderLibraryEntry(0, m_pGirderLibraryEntry );
      }

      if ( bSlabOffset )
      {
         PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
         PierIndexType endPierIdx   = pGroup->GetPierIndex(pgsTypes::metEnd);
         GirderIndexType nGirders = pGroup->GetGirderCount();
         for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
         {
            CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
            SegmentIndexType nSegments = pGirder->GetSegmentCount();
            for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
            {
               CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
               pSegment->SetSlabOffset(m_SlabOffset, m_SlabOffset);
            }

            if (gdrIdx == 0)
            {
               for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
               {
                  CPierData2* pPier = GetPier(pierIdx);
                  if (pierIdx == startPierIdx)
                  {
                     pPier->SetSlabOffset(pgsTypes::Ahead, m_SlabOffset);
                  }
                  else if (pierIdx == endPierIdx)
                  {
                     pPier->SetSlabOffset(pgsTypes::Back, m_SlabOffset);
                  }
                  else
                  {
                     pPier->SetSlabOffset(m_SlabOffset, m_SlabOffset);
                  }

                  // Ability for temp supports to have slab offset was removed in version 7
                  //if (pierIdx != endPierIdx)
                  //{
                  //   SpanIndexType spanIdx = (SpanIndexType)pierIdx;
                  //   CSpanData2* pSpan = GetSpan(spanIdx);
                  //   std::vector<CTemporarySupportData*> vTS = pSpan->GetTemporarySupports();
                  //   for (auto* pTS : vTS)
                  //   {
                  //      pTS->SetSlabOffset(m_SlabOffset, m_SlabOffset);
                  //   }
                  //}
               }
            }
         }
      }

      if (bSpacing)
      {
         pGroup->JoinAllGirderTopWidthGroups(0);

         GirderIndexType nGirders = pGroup->GetGirderCount();
         for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
         {
            CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
            pGirder->SetTopWidth(m_TopWidthType, m_LeftTopWidth, m_RightTopWidth, m_LeftTopWidth, m_RightTopWidth);
         }
      }
   }// group loop

   if(bDirectHaunchDepths)
   {
      // Set for spans
      for(auto* pSpan : m_Spans)
      {
         GirderIndexType nGirders = pSpan->GetGirderCount();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            pSpan->SetDirectHaunchDepths(m_HaunchDepths);
         }
      }

      // Set for segments
      for (auto* pGroup : m_GirderGroups)
      {
         GirderIndexType nGdrs = pGroup->GetGirderCount();
         for (GirderIndexType iGdr = 0; iGdr < nGdrs; iGdr++)
         {
            auto pGdr = pGroup->GetGirder(iGdr);
            pGdr->SetDirectHaunchDepths(m_HaunchDepths);
         }
      }
   }

   if (bAssumedExcessCamber)
   {
      for (auto* pSpan : m_Spans)
      {
         GirderIndexType nGirders = pSpan->GetGirderCount();
         for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
         {
            pSpan->SetAssumedExcessCamber(gdrIdx,m_AssumedExcessCamber);
         }
      }
   }


   if ( bSpacing )
   {
      for(auto* pPier : m_Piers)
      {
         if ( pPier->IsInteriorPier() && 
            (pPier->GetSegmentConnectionType() == pgsTypes::psctContinuousSegment ||
             pPier->GetSegmentConnectionType() == pgsTypes::psctIntegralSegment)
            )
         {
            // spacing is not defined at this pier... skip it
            continue;
         }

         if ( pPier->GetPrevSpan() )
         {
            CGirderSpacing2* pBackSpacing = pPier->GetGirderSpacing(pgsTypes::Back);
            if ( bGirderCount )
            {
               pBackSpacing->InitGirderCount(m_nGirders);
            }

            pBackSpacing->JoinAll(0);
            pBackSpacing->SetGirderSpacing(0,m_GirderSpacing);
            pBackSpacing->SetMeasurementLocation(m_MeasurementLocation);
            pBackSpacing->SetMeasurementType(m_MeasurementType);
            pBackSpacing->SetRefGirder(m_RefGirderIdx);
            pBackSpacing->SetRefGirderOffset(m_RefGirderOffset);
            pBackSpacing->SetRefGirderOffsetType(m_RefGirderOffsetType);
         }

         if ( pPier->GetNextSpan() )
         {
            CGirderSpacing2* pAheadSpacing = pPier->GetGirderSpacing(pgsTypes::Ahead);
            if ( bGirderCount )
            {
               pAheadSpacing->InitGirderCount(m_nGirders);
            }

            pAheadSpacing->JoinAll(0);
            pAheadSpacing->SetGirderSpacing(0,m_GirderSpacing);
            pAheadSpacing->SetMeasurementLocation(m_MeasurementLocation);
            pAheadSpacing->SetMeasurementType(m_MeasurementType);
            pAheadSpacing->SetRefGirder(m_RefGirderIdx);
            pAheadSpacing->SetRefGirderOffset(m_RefGirderOffset);
            pAheadSpacing->SetRefGirderOffsetType(m_RefGirderOffsetType);
         }
      }

      for(auto* pTS : m_TemporarySupports)
      {
         CGirderSpacing2* pSpacing = pTS->GetSegmentSpacing();
         if ( bGirderCount )
         {
            pSpacing->InitGirderCount(m_nGirders);
         }
         pSpacing->JoinAll(0);
         pSpacing->SetGirderSpacing(0,m_GirderSpacing);
         pSpacing->SetMeasurementLocation(m_MeasurementLocation);
         pSpacing->SetMeasurementType(m_MeasurementType);
         pSpacing->SetRefGirder(m_RefGirderIdx);
         pSpacing->SetRefGirderOffset(m_RefGirderOffset);
         pSpacing->SetRefGirderOffsetType(m_RefGirderOffsetType);
      }
   }

   if (bBearingData)
   {
      if (this->m_BearingType == pgsTypes::brtBridge)
      {
         for(auto* pPier : m_Piers)
         {
            for (Uint32 i = 0; i < 2; i++)
            {
               pgsTypes::PierFaceType face = (pgsTypes::PierFaceType)i;

               const CSpanData2* pSpan = pPier->GetSpan(face);
               if (pSpan)
               {
                  pPier->SetBearingData(face, m_BearingData);
               }
            }
         }
      }
   }


   PGS_ASSERT_VALID;
}

void CBridgeDescription2::CopyHaunchSettings(const CBridgeDescription2& rOther, bool bMakeDeepCopy)
{
   // If told, make a deep copy of all haunch-related data here. Otherwise our == operator will cause an assert after the copy operation
   pgsTypes::HaunchInputDepthType inputType = rOther.GetHaunchInputDepthType();
   this->SetHaunchInputDepthType(inputType);

   // start with slab offset input
   pgsTypes::SlabOffsetType slabOffsetType = rOther.GetSlabOffsetType();
   this->SetSlabOffsetType(slabOffsetType);

   if (slabOffsetType == pgsTypes::sotBridge || bMakeDeepCopy)
   {
      Float64 slabOffset = rOther.GetSlabOffset(true);
      this->SetSlabOffset(slabOffset);
   }

   // Input data at piers
   if (slabOffsetType == pgsTypes::sotBearingLine || bMakeDeepCopy)
   {
      PierIndexType pierCnt = rOther.GetPierCount();
      if (this->GetPierCount() != pierCnt)
      {
         ATLASSERT(0); // incompatible data. we will do nothing
         return;
      }
      else
      {
         for (PierIndexType iPier = 0; iPier < pierCnt; iPier++)
         {
            const CPierData2* pOtherPier = rOther.GetPier(iPier);
            CPierData2* pThisPier = this->GetPier(iPier);
            Float64 back,ahead;
            pOtherPier->GetSlabOffset(&back,&ahead,true);
            pThisPier->SetSlabOffset(back,ahead);
         }
      }
   }

   GroupIndexType groupCnt = rOther.GetGirderGroupCount();

   // input data at segments
   // each segment has its own slab offsets
   if (slabOffsetType == pgsTypes::sotSegment || bMakeDeepCopy)
   {
      if (groupCnt != this->GetGirderGroupCount())
      {
         ATLASSERT(0);
         return;
      }
      else
      {
         for (GroupIndexType iGroup = 0; iGroup < groupCnt; iGroup++)
         {
            const CGirderGroupData* pOtherGroup = rOther.GetGirderGroup(iGroup);
            CGirderGroupData* pThisGroup = this->GetGirderGroup(iGroup);

            GirderIndexType girderCnt = pOtherGroup->GetGirderCount();
            if (girderCnt != pThisGroup->GetGirderCount())
            {
               ATLASSERT(0);
               return;
            }
            else
            {
               for (GirderIndexType iGirder = 0; iGirder < girderCnt; iGirder++)
               {
                  const CSplicedGirderData* pOtherGirder = pOtherGroup->GetGirder(iGirder);
                  CSplicedGirderData* pThisGirder = pThisGroup->GetGirder(iGirder);
                  SegmentIndexType segmentCnt = pOtherGirder->GetSegmentCount();
                  if (segmentCnt != pThisGirder->GetSegmentCount())
                  {
                     ATLASSERT(0);
                     return;
                  }
                  else
                  {
                     for (SegmentIndexType iSegment = 0; iSegment < segmentCnt; iSegment++)
                     {
                        const CPrecastSegmentData* pOtherSegment = pOtherGirder->GetSegment(iSegment);
                        CPrecastSegmentData* pThisSegment = pThisGirder->GetSegment(iSegment);
                        Float64 back,ahead;
                        pOtherSegment->GetSlabOffset(&back,&ahead,true);
                        pThisSegment->SetSlabOffset(back,ahead);
                     }
                  }
               }
            }
         }
      }
   }

   // Slab offset data is set. Now need assumed excess camber data. 
   pgsTypes::AssumedExcessCamberType camberInputType = rOther.GetAssumedExcessCamberType();
   this->SetAssumedExcessCamberType(camberInputType);

   Float64 camber = rOther.GetAssumedExcessCamber();
   this->SetAssumedExcessCamber(camber);

   SpanIndexType spanCnt = rOther.GetSpanCount();
   if (spanCnt != this->GetSpanCount())
   {
      ATLASSERT(0);
      return;
   }
   else
   {
      for (SpanIndexType iSpan = 0; iSpan < spanCnt; iSpan++)
      {
         const CSpanData2* pOtherSpan = rOther.GetSpan(iSpan);
         CSpanData2* pThisSpan = this->GetSpan(iSpan);
         GirderIndexType girderCnt = pOtherSpan->GetGirderCount();
         for (GirderIndexType iGirder = 0; iGirder < girderCnt; iGirder++)
         {
            Float64 camber = pOtherSpan->GetAssumedExcessCamber(iGirder,true);
            pThisSpan->SetAssumedExcessCamber(iGirder,camber);
         }
      }
   }

   // End of A Camber. Now Direct haunch input
   // We can treat hidHaunchDirectly and hidHaunchPlusSlabDirectly the same because the raw input data is the same 
   pgsTypes::HaunchInputLocationType haunchInputLocationType = rOther.GetHaunchInputLocationType();
   this->SetHaunchInputLocationType(haunchInputLocationType);
   pgsTypes::HaunchLayoutType haunchLayoutType = rOther.GetHaunchLayoutType();
   this->SetHaunchLayoutType(haunchLayoutType);
   pgsTypes::HaunchInputDistributionType haunchInputDistributionType = rOther.GetHaunchInputDistributionType();
   this->SetHaunchInputDistributionType(haunchInputDistributionType);

   if (haunchInputLocationType == pgsTypes::hilSame4Bridge || bMakeDeepCopy)
   {
      std::vector<Float64> haunches = rOther.GetDirectHaunchDepths();
      this->SetDirectHaunchDepths(haunches);
   }

   // Data at spans
   if ((haunchLayoutType == pgsTypes::hltAlongSpans && haunchInputLocationType != pgsTypes::hilSame4Bridge) || bMakeDeepCopy)
   {
      if (spanCnt != this->GetSpanCount())
      {
         ATLASSERT(0);
         return;
      }
      else
      {
         for (SpanIndexType iSpan = 0; iSpan < spanCnt; iSpan++)
         {
            const CSpanData2* pOtherSpan = rOther.GetSpan(iSpan);
            CSpanData2* pThisSpan = this->GetSpan(iSpan);
            GirderIndexType girderCnt = pOtherSpan->GetGirderCount();
            for (GirderIndexType iGirder = 0; iGirder < girderCnt; iGirder++)
            {
               std::vector<Float64> haunches = pOtherSpan->GetDirectHaunchDepths(iGirder,true);
               pThisSpan->SetDirectHaunchDepths(iGirder,haunches);
            }
         }
      }
   }

   // Haunches are at segments
   if ((haunchLayoutType == pgsTypes::hltAlongSegments && haunchInputLocationType != pgsTypes::hilSame4Bridge) || bMakeDeepCopy)
   {

      if (groupCnt != this->GetGirderGroupCount())
      {
         ATLASSERT(0);
         return;
      }
      else
      {
         for (GroupIndexType iGroup = 0; iGroup < groupCnt; iGroup++)
         {
            const CGirderGroupData* pOtherGroup = rOther.GetGirderGroup(iGroup);
            CGirderGroupData* pThisGroup = this->GetGirderGroup(iGroup);

            GirderIndexType girderCnt = pOtherGroup->GetGirderCount();
            if (girderCnt != pThisGroup->GetGirderCount())
            {
               ATLASSERT(0);
               return;
            }
            else
            {
               for (GirderIndexType iGirder = 0; iGirder < girderCnt; iGirder++)
               {
                  const CSplicedGirderData* pOtherGirder = pOtherGroup->GetGirder(iGirder);
                  CSplicedGirderData* pThisGirder = pThisGroup->GetGirder(iGirder);

                  pThisGirder->CopyHaunchData(*pOtherGirder);
               }
            }
         }
      }
   }
}

std::vector<pgsTypes::BoundaryConditionType> CBridgeDescription2::GetBoundaryConditionTypes(PierIndexType pierIdx) const
{
   std::vector<pgsTypes::BoundaryConditionType> connectionTypes;

   const CPierData2* pPier = GetPier(pierIdx);

   ATLASSERT(pPier->IsBoundaryPier());

   // "before deck" connections are only applicable if the bridge has a deck
   bool bHasDeck = IsStructuralDeck(m_Deck.DeckType) ? true : false;

   // This pier is on a group boundary (two groups frame into this pier).
   // All connection types are valid
   connectionTypes.push_back(pgsTypes::bctHinge);
   connectionTypes.push_back(pgsTypes::bctRoller);
   connectionTypes.push_back(pgsTypes::bctIntegralAfterDeck);
   if (bHasDeck)
   {
      connectionTypes.push_back(pgsTypes::bctIntegralBeforeDeck);
   }

   if ( pPier->GetPrevSpan() && pPier->GetNextSpan() )
   {
      // all these connection types require that there is a span on 
      // both sides of this pier
      connectionTypes.push_back(pgsTypes::bctContinuousAfterDeck);
      if (bHasDeck)
      {
         connectionTypes.push_back(pgsTypes::bctContinuousBeforeDeck);
      }

      connectionTypes.push_back(pgsTypes::bctIntegralAfterDeckHingeBack);

      if (bHasDeck)
      {
         connectionTypes.push_back(pgsTypes::bctIntegralBeforeDeckHingeBack);
      }

      connectionTypes.push_back(pgsTypes::bctIntegralAfterDeckHingeAhead);
         
      if (bHasDeck)
      {
         connectionTypes.push_back(pgsTypes::bctIntegralBeforeDeckHingeAhead);
      }
   }

   return connectionTypes;
}

std::vector<pgsTypes::PierSegmentConnectionType> CBridgeDescription2::GetPierSegmentConnectionTypes(PierIndexType pierIdx) const
{
   std::vector<pgsTypes::PierSegmentConnectionType> connectionTypes;

   const CPierData2* pPier = GetPier(pierIdx);
   ATLASSERT(pPier->IsInteriorPier()); // this must be an interior pier

   connectionTypes.push_back(pgsTypes::psctContinousClosureJoint);
   connectionTypes.push_back(pgsTypes::psctIntegralClosureJoint);
   connectionTypes.push_back(pgsTypes::psctContinuousSegment);
   connectionTypes.push_back(pgsTypes::psctIntegralSegment);

   return connectionTypes;
}

IndexType CBridgeDescription2::GetClosureJointCount() const
{
   // returns the number of closure joints for a single girder line... count is the same
   // for every girder line

   IndexType nClosures = 0;

   GirderIndexType gdrIdx = 0; // all girders in a group have the same number of closure joints.
                               // use index 0 as it is safest

   std::for_each(std::begin(m_GirderGroups), std::end(m_GirderGroups), [&nClosures, gdrIdx](auto* pGroup) {nClosures += pGroup->GetGirder(gdrIdx)->GetClosureJointCount(); });

   return nClosures;
}

CClosureJointData* CBridgeDescription2::GetClosureJoint(const CClosureKey& closureKey)
{
   CGirderGroupData* pGroup = GetGirderGroup(closureKey.groupIndex);
   CSplicedGirderData* pGirder = pGroup->GetGirder(closureKey.girderIndex);
   CClosureJointData* pClosure = pGirder->GetClosureJoint(closureKey.segmentIndex);
   return pClosure;
}

const CClosureJointData* CBridgeDescription2::GetClosureJoint(const CClosureKey& closureKey) const
{
   const CGirderGroupData* pGroup = GetGirderGroup(closureKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(closureKey.girderIndex);
   const CClosureJointData* pClosure = pGirder->GetClosureJoint(closureKey.segmentIndex);
   return pClosure;
}

CPrecastSegmentData* CBridgeDescription2::GetSegment(const CSegmentKey& segmentKey)
{
   CGirderGroupData* pGroup = GetGirderGroup(segmentKey.groupIndex);
   CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);
   return pSegment;
}

const CPrecastSegmentData* CBridgeDescription2::GetSegment(const CSegmentKey& segmentKey) const
{
   const CGirderGroupData* pGroup = GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);
   return pSegment;
}

void CBridgeDescription2::GetSegmentsAtTemporarySupport(SupportIndexType tsIdx,CSegmentKey* pLeftSegmentKey,CSegmentKey* pRightSegmentKey) const
{
   const CTemporarySupportData* pTS = GetTemporarySupport(tsIdx);
   const CClosureJointData* pClosure = pTS->GetClosureJoint(0);
   const CSpanData2* pSpan = pTS->GetSpan();
   const CGirderGroupData* pGroup = GetGirderGroup(pSpan);

   GroupIndexType grpIdx = pGroup->GetIndex();

   if ( pClosure )
   {
      const CPrecastSegmentData* pLeftSegment = pClosure->GetLeftSegment();
      const CPrecastSegmentData* pRightSegment = pClosure->GetRightSegment();

      CSegmentKey leftSegKey(grpIdx,INVALID_INDEX,pLeftSegment->GetIndex());
      CSegmentKey rightSegKey(grpIdx,INVALID_INDEX,pRightSegment->GetIndex());

      *pLeftSegmentKey = leftSegKey;
      *pRightSegmentKey = rightSegKey;
   }
   else
   {
      const CSplicedGirderData* pGirder = pGroup->GetGirder(0);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         std::vector<const CTemporarySupportData*> tempSupports(pSegment->GetTemporarySupports());
         for(const auto* pTempSupport : tempSupports)
         {
            if ( pTS->GetIndex() == pTempSupport->GetIndex() )
            {
               CSegmentKey segKey(grpIdx,INVALID_INDEX,segIdx);

               *pLeftSegmentKey = segKey;
               *pRightSegmentKey = segKey;

               return;
            }
         }
      }
   }
}

bool CBridgeDescription2::IsStable() const
{
   // Check the stability of each segment. The structure is stable if all segments are stable.
   // The structure is unstable if the stability of two adjacent segments could not be determined.
   // The stability of suspended segments cannot be determined without knowing the stability of the adjacent segments.
   //
   // Suspended Segment - Type 1 - End Span supported by strongback at one end
   //
   //   =============================o====================/
   //   ^                                      ^
   //
   // Suspended Segment - Type 2 - Drop-in supported by strongbacks at both ends
   //
   //     /=========================o================================o===========================/
   //                          ^                                         ^

   // Determine the stability of a segment by looking at the number of equations and number of unknowns
   // Number of Equations, Ne = 3j
   // Number of Unknowns,  Nu = 3m+r
   // j = # of joints (number of permanent piers + temporary supports)
   // m = # of members (number of joints-1)
   // r = # of reactions (number of permanent piers + erection towers)
   //
   // Ne < Nu : statically indeterminant and stable
   // Ne = Nu : statically determinant and stable
   // Ne > Nu : unstable
   //
   GroupIndexType nGroups = GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType gdrIdx = 0;

      bool bIsLastSegmentStable = true;

      const CGirderGroupData* pGroup = GetGirderGroup(grpIdx);
      const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         std::vector<const CPierData2*> vPiers = pSegment->GetPiers();
         std::vector<const CTemporarySupportData*> vTS = pSegment->GetTemporarySupports();

         IndexType nJoints = vPiers.size() + vTS.size();
         IndexType nMembers = nJoints-1;

         IndexType nReactions = vPiers.size();
         std::vector<const CTemporarySupportData*>::iterator tsIter(vTS.begin());
         std::vector<const CTemporarySupportData*>::iterator tsIterEnd(vTS.end());
         for ( ; tsIter != tsIterEnd; tsIter++ )
         {
            const CTemporarySupportData* pTS = *tsIter;
            if ( pTS->GetSupportType() == pgsTypes::ErectionTower )
            {
               nReactions++; // erection tower provide a reaction in the Y direction
            }
         }

         IndexType Ne = 3*nJoints;
         IndexType Nu = 3*nMembers+nReactions+1;
         bool bIsThisSegmentStable = Nu < Ne ? false : true;

         if ( segIdx == 0 )
         {
            bIsLastSegmentStable = bIsThisSegmentStable;
         }
         else
         {
            if ( !bIsLastSegmentStable && !bIsThisSegmentStable )
            {
               return false;
            }
            else
            {
               bIsLastSegmentStable = bIsThisSegmentStable;
            }
         }
      } // next segment
   } // next group

   return true;
}

bool CBridgeDescription2::IsValidSpan(SpanIndexType spanIdx) const
{
   return 0.0 < m_Spans[spanIdx]->GetSpanLength() ? true : false;
}

bool CBridgeDescription2::IsValidLayout() const
{
   for (const auto *pSpan : m_Spans)
   {
      if (!IsValidSpan(pSpan->GetIndex()))  return false;
   }

   return true;
}

bool CBridgeDescription2::IsValidBridge() const
{
   return IsValidLayout();
}

Float64 CBridgeDescription2::GetBridgeWidth() const
{
   const CDeckDescription2* pDeck = GetDeckDescription();

   if ( IsConstantWidthDeck(pDeck->GetDeckType()) )
   {
      // there isn't a deck, estimate bridge width by adding the girder spacings
      Float64 max_spacing_width = -DBL_MAX;

      if ( ::IsBridgeSpacing(GetGirderSpacingType()) )
      {
         // the same spacing is used in all spans
         Float64 s = GetGirderSpacing();
         if ( UseSameNumberOfGirdersInAllGroups() )
         {
            GirderIndexType nGirders = GetGirderCount();
            
            if ( UseSameGirderForEntireBridge() )
            {
               if ( ::IsAdjacentSpacing(GetGirderSpacingType()) )
               {
                  // beams are adjacent so spacing is actually a joint spacing...

                  // get the width of the girder
                  CComPtr<IBeamFactory> factory;
                  m_pGirderLibraryEntry->GetBeamFactory(&factory);
                  
                  CComPtr<IGirderSection> gdrSection;
                  factory->CreateGirderSection(nullptr,INVALID_ID,m_pGirderLibraryEntry->GetDimensions(),-1,-1,&gdrSection);

                  // Width is max of top and bottom.
                  Float64 Width, wleft, wrght;
                  gdrSection->get_TopWidth(&wleft,&wrght);
                  Width = wleft + wrght;
                  gdrSection->get_BottomWidth(&wleft,&wrght);
                  Width = max(Width, wleft + wrght);

                  if ( 1 < nGirders )
                  {
                     Float64 w = s*(nGirders-1) + nGirders*Width;
                     max_spacing_width = Max(max_spacing_width,w);
                  }
                  else
                  {
                     max_spacing_width = Max(max_spacing_width,Width);
                  }
               }
               else
               {
                  ATLASSERT(::IsSpreadSpacing(GetGirderSpacingType()));
                  if ( 1 < nGirders )
                  {
                     Float64 w = s*(nGirders-1);
                     max_spacing_width = Max(max_spacing_width,w);
                  }
               }
            }
            else
            {
               // Different girders are used in each group... need to add spacing for each
               // girder individually
               if ( ::IsAdjacentSpacing(GetGirderSpacingType()) )
               {
                  // beams are adjacent so spacing is actually a joint spacing...
                  GroupIndexType nGroups = GetGirderGroupCount();
                  for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
                  {
                     const CGirderGroupData* pGroup = GetGirderGroup(grpIdx);
                     nGirders = pGroup->GetGirderCount();
                     Float64 w = 0;
                     for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
                     {
                        // get the width of the girder
                        const GirderLibraryEntry* pGdrLibEntry = pGroup->GetGirderLibraryEntry(gdrIdx);
                        CComPtr<IBeamFactory> factory;
                        pGdrLibEntry->GetBeamFactory(&factory);
                        
                        CComPtr<IGirderSection> gdrSection;
                        factory->CreateGirderSection(nullptr,INVALID_ID,pGdrLibEntry->GetDimensions(),-1,-1,&gdrSection);

                        Float64 Width, wleft, wrght;
                        gdrSection->get_TopWidth(&wleft,&wrght);
                        Width = wleft + wrght;
                        gdrSection->get_BottomWidth(&wleft,&wrght);
                        Width = max(Width, wleft + wrght);

                        w += Width;
                     } // next girder

                     if ( 1 < nGirders )
                     {
                        w += s*(nGirders-1);
                     }
                     max_spacing_width = Max(max_spacing_width,w);
                  } // next group
               }
               else
               {
                  ATLASSERT(::IsSpreadSpacing(GetGirderSpacingType()));
                  if ( 1 < nGirders )
                  {
                     Float64 w = s*(nGirders-1);
                     max_spacing_width = Max(max_spacing_width,w);
                  }
               }
            }
         }
         else
         {
            // different number of girders in each group, need to compute spacing width for each group
            GroupIndexType nGroups = GetGirderGroupCount();
            for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
            {
               Float64 w = 0;
               const CGirderGroupData* pGroup = GetGirderGroup(grpIdx);
               GirderIndexType nGirders = pGroup->GetGirderCount();
               for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
               {
                  if ( ::IsAdjacentSpacing(GetGirderSpacingType()) )
                  {
                     // beams are adjacent so spacing is actually a joint spacing...
                     const GirderLibraryEntry* pGdrLibEntry = pGroup->GetGirderLibraryEntry(gdrIdx);

                     // get the width of the girder
                     CComPtr<IBeamFactory> factory;
                     pGdrLibEntry->GetBeamFactory(&factory);
                     
                     CComPtr<IGirderSection> gdrSection;
                     factory->CreateGirderSection(nullptr,INVALID_ID,pGdrLibEntry->GetDimensions(),-1,-1,&gdrSection);

                     Float64 Width, wleft, wrght;
                     gdrSection->get_TopWidth(&wleft,&wrght);
                     Width = wleft + wrght;
                     gdrSection->get_BottomWidth(&wleft,&wrght);
                     Width = max(Width, wleft + wrght);

                     w += Width;
                     if ( gdrIdx < nGirders-1 )
                     {
                        w += s;
                     }
                  }
                  else
                  {
                     ATLASSERT(::IsSpreadSpacing(GetGirderSpacingType()));
                     if ( gdrIdx < nGirders-1 )
                     {
                        w += s;
                     }
                  }
               } // next girder

               max_spacing_width = Max(max_spacing_width,w);
            } // next group
         }
      }
      else
      {
         PierIndexType nPiers = GetPierCount();
         for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
         {
            const CPierData2* pPier = GetPier(pierIdx);
            if ( pPier->HasSpacing() )
            {
               if ( !pPier->IsBoundaryPier() && pPier->GetClosureJoint(0) != nullptr )
               {
                  // if there is a closure joint, only back spacing is valid
                  const CGirderSpacing2* pSpacing = pPier->GetGirderSpacing(pgsTypes::Back);
                  Float64 w = pSpacing->GetSpacingWidth();
                  max_spacing_width = Max(max_spacing_width,w);
               }
               
               if ( pPier->GetPrevSpan() != nullptr )
               {
                  const CGirderSpacing2* pSpacing = pPier->GetGirderSpacing(pgsTypes::Back);
                  Float64 w = pSpacing->GetSpacingWidth();
                  max_spacing_width = Max(max_spacing_width,w);
               }

               if ( pPier->GetNextSpan() != nullptr )
               {
                  const CGirderSpacing2* pSpacing = pPier->GetGirderSpacing(pgsTypes::Ahead);
                  Float64 w = pSpacing->GetSpacingWidth();
                  max_spacing_width = Max(max_spacing_width,w);
               }
            }
         }

         SupportIndexType nTS = GetTemporarySupportCount();
         for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
         {
            const CTemporarySupportData* pTS = GetTemporarySupport(tsIdx);
            if ( pTS->HasSpacing() )
            {
               const CGirderSpacing2* pSpacing = pTS->GetSegmentSpacing();
               Float64 w = pSpacing->GetSpacingWidth();
               max_spacing_width = Max(max_spacing_width,w);
            }
         }
      }

      return max_spacing_width;
   }
   else
   {
      Float64 max_deck_width = pDeck->GetMaxWidth();
      return max_deck_width;
   }
}

void CBridgeDescription2::SetBearingType(pgsTypes::BearingType type)
{
   m_BearingType = type;
}

pgsTypes::BearingType CBridgeDescription2::GetBearingType() const
{
   return m_BearingType;
}

void CBridgeDescription2::SetBearingData(const CBearingData2& bearing)
{
   m_BearingData = bearing;
}

const CBearingData2* CBridgeDescription2::GetBearingData() const
{
   return &m_BearingData;
}

CBearingData2* CBridgeDescription2::GetBearingData()
{
   return &m_BearingData;
}

bool CBridgeDescription2::MoveBridge(PierIndexType pierIdx,Float64 newStation)
{
   // move pierIdx to newStation and keep all the span lengths constant
   CPierData2* pPier = GetPier(pierIdx);
   Float64 old_station = pPier->GetStation();
   Float64 deltaStation = newStation - old_station;
   if ( IsZero(deltaStation) )
   {
      return false;
   }

   CPierData2* pPrevPier = GetPier(0);
   pPrevPier->SetStation(pPrevPier->GetStation() + deltaStation);

   // move piers
   CSpanData2* pNextSpan = pPrevPier->GetNextSpan();
   while ( pNextSpan )
   {
      CPierData2* pNextPier = pNextSpan->GetNextPier();

      pNextPier->SetStation( pNextPier->GetStation() + deltaStation);
      
      pNextSpan = pNextPier->GetNextSpan();
   }

   // move temporary supports
   std::for_each(std::begin(m_TemporarySupports), std::end(m_TemporarySupports), [deltaStation](auto* pTS) {pTS->SetStation(pTS->GetStation() + deltaStation); });

   // move all the deck points
   std::for_each(std::begin(m_Deck.DeckEdgePoints), std::end(m_Deck.DeckEdgePoints), [deltaStation](auto& deckPoint) {deckPoint.Station += deltaStation; });

   return true;
}

bool CBridgeDescription2::MoveBridgeAdjustPrevSpan(PierIndexType pierIdx,Float64 newStation)
{
   // move pierIdx and all piers that come after it by delta
   // this will retain the length of all spans execpt for the one
   // immedately before the pier
   CPierData2* pPier = GetPier(pierIdx);
   Float64 old_station = pPier->GetStation();
   Float64 deltaStation = newStation - old_station;
   if ( IsZero(deltaStation) )
   {
      return false;
   }

   pPier->SetStation(old_station + deltaStation);

   CPierData2* pPrevPier = pPier;
   CSpanData2* pNextSpan = pPrevPier->GetNextSpan();
   while ( pNextSpan )
   {
      CPierData2* pNextPier = pNextSpan->GetNextPier();
      pNextPier->SetStation(pNextPier->GetStation() + deltaStation);
 
      pNextSpan = pNextPier->GetNextSpan();
   }

   // move temporary supports that occur after pierIdx
   std::for_each(std::begin(m_TemporarySupports), std::end(m_TemporarySupports), 
      [old_station, deltaStation](auto* pTS) {if (old_station <= pTS->GetStation()) pTS->SetStation(pTS->GetStation() + deltaStation); });

   return true;
}

bool CBridgeDescription2::MoveBridgeAdjustNextSpan(PierIndexType pierIdx,Float64 newStation)
{
   // move pierIdx and all piers that come before it by delta
   // this will retain the length of all spans execpt for the one
   // immedately after the pier
   CPierData2* pPier = GetPier(pierIdx);
   Float64 old_station = pPier->GetStation();
   Float64 deltaStation = newStation - old_station;
   if ( IsZero(deltaStation) )
   {
      return false;
   }

   pPier->SetStation( pPier->GetStation() + deltaStation);

   CPierData2* pNextPier = pPier;
   CSpanData2* pPrevSpan = pNextPier->GetPrevSpan();
   while ( pPrevSpan )
   {
      CPierData2* pPrevPier = pPrevSpan->GetPrevPier();
      pPrevPier->SetStation( pPrevPier->GetStation() + deltaStation);

      pPrevSpan = pPrevPier->GetPrevSpan();
   }

   // move temporary supports that occur before pierIdx
   std::for_each(std::begin(m_TemporarySupports), std::end(m_TemporarySupports), 
      [old_station, deltaStation](auto* pTS) {if (pTS->GetStation() <= old_station) pTS->SetStation(pTS->GetStation() + deltaStation); });

   return true;
}

bool CBridgeDescription2::MoveBridgeAdjustAdjacentSpans(PierIndexType pierIdx,Float64 newStation)
{
   // Changes the length of the spans before and after pierIdx.
   // be careful to not move the pier beyond the end of the adjacent span
   CPierData2* pPier = GetPier(pierIdx);
   Float64 old_station = pPier->GetStation();
   Float64 deltaStation = newStation - old_station;
   if ( IsZero(deltaStation) )
   {
      return false;
   }

   if ( pPier->GetPrevSpan() )
   {
      CPierData2* pPrevPier = pPier->GetPrevSpan()->GetPrevPier();
      if ( newStation <= pPrevPier->GetStation() )
      {
         ATLASSERT(false); // moving the pier too far
         return false;
      }
   }

   if ( pPier->GetNextSpan() )
   {
      CPierData2* pNextPier = pPier->GetNextSpan()->GetNextPier();
      if ( pNextPier->GetStation() <= newStation )
      {
         ATLASSERT(false); // moving the pier too far
         return false;
      }
   }

   pPier->SetStation(pPier->GetStation() + deltaStation);

   // since the overall length of the bridge doesn't change, there is no need to adjust
   // the location of the temporary supports
#pragma Reminder("REVIEW/UPDATE: pier could move such that the temporary support effectively changes spans")
   // if this happens the span/temp support referencing would get messed up (I think). review this case
   // and update as needed.

   return true;
}

void CBridgeDescription2::RenumberGroups()
{
   GroupIndexType grpIdx = 0;
   std::for_each(std::begin(m_GirderGroups), std::end(m_GirderGroups), [&grpIdx](auto* pGroup) {pGroup->SetIndex(grpIdx++); });
}

void CBridgeDescription2::RenumberSpans()
{
   // renumbers the spans and piers and updates all the prev/next pointers.
   auto spanIter = std::begin(m_Spans);
   auto spanIterEnd = std::end(m_Spans);
   auto pierIter = std::begin(m_Piers);
   auto pierIterEnd = std::end(m_Piers);

   SpanIndexType spanIdx = 0;
   PierIndexType pierIdx = 0;
   CSpanData2* pPrevSpan = nullptr;

   CPierData2* pPrevPier = *pierIter++;
   ATLASSERT(pPrevPier->GetBridgeDescription() == this);

   pPrevPier->SetIndex(pierIdx++);

   for ( ; spanIter != spanIterEnd && pierIter != pierIterEnd; spanIter++, pierIter++, spanIdx++, pierIdx++ )
   {
      CSpanData2* pThisSpan = *spanIter;
      CPierData2* pNextPier = *pierIter;

      ATLASSERT(pThisSpan->GetBridgeDescription() == this);
      ATLASSERT(pNextPier->GetBridgeDescription() == this);

      pThisSpan->SetIndex(spanIdx);
      pNextPier->SetIndex(pierIdx);

      pPrevPier->SetSpans(pPrevSpan,pThisSpan);
      pThisSpan->SetPiers(pPrevPier,pNextPier);

      pPrevPier = pNextPier;
      pPrevSpan = pThisSpan;
   }

   // last pier
   CPierData2* pLastPier = m_Piers.back();
   ATLASSERT(pLastPier->GetBridgeDescription() == this);

   pLastPier->SetSpans(pPrevSpan,nullptr);
}

void CBridgeDescription2::UpdateTemporarySupports()
{
   // sort temporary supports based on station
   std::sort(m_TemporarySupports.begin(), m_TemporarySupports.end(), [](auto* pTS1, auto* pTS2) {return *pTS1 < *pTS2; });

   // assign span based on location
   SupportIndexType tsIdx = 0;
   CSpanData2* pSpan = m_Spans[0];
   CPierData2* pPrevPier = pSpan->GetPrevPier();
   CPierData2* pNextPier = pSpan->GetNextPier();
   auto tsIter = std::begin(m_TemporarySupports);
   auto tsIterEnd = std::end(m_TemporarySupports);
   for ( ; tsIter != tsIterEnd; tsIter++ )
   {
      CTemporarySupportData* pTS = *tsIter;
      pTS->SetIndex(tsIdx++);

      Float64 tsStation = pTS->GetStation();

      bool bDone = false;
      do
      {
         if ( pPrevPier->GetStation() <= tsStation && tsStation <= pNextPier->GetStation() )
         {
            // temporary support is in this span
            pTS->SetSpan(pSpan);
            bDone = true;
         }
         else
         {
            // temporary support is not in this span, move to the next span
            pSpan = pNextPier->GetNextSpan();
            ATLASSERT(pSpan != nullptr); // ran out of spans before temporary supports

            if ( pSpan == nullptr )
            {
               bDone = true;
            }
            else
            {
               pPrevPier = pSpan->GetPrevPier();
               pNextPier = pSpan->GetNextPier();
            }
         }
      } while (!bDone );
   }
}

void CBridgeDescription2::RemoveNegMomentRebar(PierIndexType removeRebarPierIdx)
{
   auto begin = std::begin(m_Deck.DeckRebarData.NegMomentRebar);
   auto end = std::end(m_Deck.DeckRebarData.NegMomentRebar);
   auto last = std::remove_if(begin, end, [removeRebarPierIdx](auto& item) {return item.PierIdx == removeRebarPierIdx;});
   m_Deck.DeckRebarData.NegMomentRebar.erase(last, end);
}

HRESULT CBridgeDescription2::LoadOldBridgeDescription(Float64 version,IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   // Input is in an old format (the format is PGSuper before version 3.0 when we added PGSplice)
   // Use the old bridge description object to load the data
   ATLASSERT( version < 7 );
   std::unique_ptr<CBridgeDescription> pBridgeDesc(std::make_unique<CBridgeDescription>());
   HRESULT hr = pBridgeDesc->Load(version,pStrLoad,pProgress);
   if ( FAILED(hr))
   {
      return hr;
   }

   // The data was loaded correctly, now convert the old data into the proper format for this class
   pBridgeDesc->SetBridgeData(this);

   return S_OK;
}

void CBridgeDescription2::ClearGirderGroups()
{
   std::for_each(std::begin(m_GirderGroups), std::end(m_GirderGroups), [](auto* pGroup) {pGroup->Clear(); delete pGroup; pGroup = nullptr; });
   m_GirderGroups.clear();
}

void CBridgeDescription2::UpdateNextTemporarySupportID(SupportIDType tsID)
{
   m_TempSupportID = Max(m_TempSupportID,tsID+1);
}

SupportIDType CBridgeDescription2::GetNextTemporaryID(bool bIncrement)
{
   SegmentIDType tsID = m_TempSupportID;
   if ( bIncrement )
   {
      m_TempSupportID++;
   }

   return tsID;
}

void CBridgeDescription2::UpdateNextSegmentID(SegmentIDType segID)
{
   m_SegmentID = Max(m_SegmentID,segID+1);
}

SegmentIDType CBridgeDescription2::GetNextSegmentID(bool bIncrement)
{
   SegmentIDType segID = m_SegmentID;
   if ( bIncrement )
   {
      m_SegmentID++;
   }

   return segID;
}

void CBridgeDescription2::UpdateNextGirderID(GirderIDType gdrID)
{
   m_GirderID = Max(m_GirderID,gdrID+1);
}

GirderIDType CBridgeDescription2::GetNextGirderID(bool bIncrement)
{
   GirderIDType gdrID = m_GirderID;
   if ( bIncrement )
   {
      m_GirderID++;
   }

   return gdrID;
}

void CBridgeDescription2::UpdateNextPierID(PierIDType pierID)
{
   m_PierID = Max(m_PierID,pierID+1);
}

PierIDType CBridgeDescription2::GetNextPierID(bool bIncrement)
{
   PierIDType pierID = m_PierID;
   if ( bIncrement )
   {
      m_PierID++;
   }

   return pierID;
}

void CBridgeDescription2::UpdateNextGirderGroupID(GroupIDType grpID)
{
   m_GirderGroupID = Max(m_GirderGroupID,grpID+1);
}

GroupIDType CBridgeDescription2::GetNextGirderGroupID(bool bIncrement)
{
   GroupIDType grpID = m_GirderGroupID;
   if ( bIncrement )
   {
      m_GirderGroupID++;
   }

   return grpID;
}

bool CBridgeDescription2::HasLongitudinalJoints() const
{
   if (IsJointSpacing(m_GirderSpacingType))
   {
      // bridge has a joint spacing type... ask the factory
      const GirderLibraryEntry* pGirderLibraryEntry = nullptr;
      if (UseSameGirderForEntireBridge())
      {
         pGirderLibraryEntry = m_pGirderLibraryEntry;
      }
      else
      {
         const CGirderGroupData* pGroup = GetGirderGroup((GroupIndexType)0);
         const CSplicedGirderData* pGirder = pGroup->GetGirder(0);
         pGirderLibraryEntry = pGirder->GetGirderLibraryEntry();
      }

      CComPtr<IBeamFactory> beamFactory;
      pGirderLibraryEntry->GetBeamFactory(&beamFactory);
      return beamFactory->HasLongitudinalJoints();
   }
   else
   {
      // if bridge doesn't have a joint spacing type, there isn't a joint
      return false;
   }
}

bool CBridgeDescription2::HasStructuralLongitudinalJoints() const
{
   if (IsJointSpacing(m_GirderSpacingType))
   {
      // bridge has a joint spacing type... ask the factory
      const GirderLibraryEntry* pGirderLibraryEntry = nullptr;
      if (UseSameGirderForEntireBridge())
      {
         pGirderLibraryEntry = m_pGirderLibraryEntry;
      }
      else
      {
         const CGirderGroupData* pGroup = GetGirderGroup((GroupIndexType)0);
         const CSplicedGirderData* pGirder = pGroup->GetGirder(0);
         pGirderLibraryEntry = pGirder->GetGirderLibraryEntry();
      }
      
      CComPtr<IBeamFactory> beamFactory;
      pGirderLibraryEntry->GetBeamFactory(&beamFactory);
      if (beamFactory->HasLongitudinalJoints() && beamFactory->IsLongitudinalJointStructural(m_Deck.GetDeckType(), m_Deck.TransverseConnectivity))
      {
         // the bridge has longitudinal joints and the joints are structural
         if (IsBridgeSpacing(m_GirderSpacingType))
         {
            return IsZero(m_GirderSpacing) ? false : true; // if joint width is zero, then we don't have structural joints
         }
         else
         {
            return true; // if the spacing is defined girder by girder or span by span... we'll assume there is a non-zero joint spacing
         }
      }
      else
      {
         // longitudinal joints are not supported by the beam
         return false;
      }

      // NOTE: the logical is a little weired here, but a bridge can have joint spacing, but no longitudinal joint.
      // Examples would be box beams and voided slabs.
   }
   else
   {
      // if bridge doesn't have a joint spacing type, there isn't a joint
      return false;
   }
}

const CConcreteMaterial& CBridgeDescription2::GetLongitudinalJointMaterial() const
{
   return m_LongitudinalJointConcrete;
}

void CBridgeDescription2::SetLongitudinalJointMaterial(const CConcreteMaterial& material)
{
   m_LongitudinalJointConcrete = material;
}

void CBridgeDescription2::ForEachSegment(std::function<void(CPrecastSegmentData*,void*)>& fn,void* pData)
{
   for(auto* pGroup : m_GirderGroups)
   { 
      for (auto* pGirder : pGroup->m_Girders)
      {
         for (auto* pSegment : pGirder->m_Segments)
         {
            fn(pSegment,pData);
         }
      }
   }
}

#if defined _DEBUG
void CBridgeDescription2::AssertValid()
{
   // Check that all spans are attached to this bridge and the previous/next span connect
   // to the same pier
   SpanIndexType nSpans = GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      CSpanData2* pPrevSpan = GetSpan(spanIdx-1);
      CSpanData2* pThisSpan = GetSpan(spanIdx);
      CSpanData2* pNextSpan = GetSpan(spanIdx+1);

      pThisSpan->AssertValid(); // is this span valid?

      _ASSERT(pThisSpan->GetIndex() != INVALID_INDEX);

      _ASSERT( pThisSpan->GetBridgeDescription() == this );

      if ( pPrevSpan )
      {
         _ASSERT( pPrevSpan->GetNextPier() == pThisSpan->GetPrevPier() );
      }

      if ( pNextSpan )
      {
         _ASSERT( pThisSpan->GetNextPier() == pNextSpan->GetPrevPier() );
      }
   }

   // check that all piers are attached to this bridge
   // check that adjacent piers connect to the same span
   PierIndexType nPiers = GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      CPierData2* pPrevPier = GetPier(pierIdx-1);
      CPierData2* pThisPier = GetPier(pierIdx);
      CPierData2* pNextPier = GetPier(pierIdx+1);

      pThisPier->AssertValid(); // is this pier valid?

      _ASSERT(pThisPier->GetIndex() != INVALID_INDEX);
      _ASSERT(pThisPier->GetID() != INVALID_ID);

      _ASSERT(pThisPier->GetBridgeDescription() == this);

      if ( pPrevPier )
      {
         _ASSERT( pPrevPier->GetNextSpan() == pThisPier->GetPrevSpan() );
         _ASSERT( pPrevPier->GetStation() < pThisPier->GetStation() );
      }

      if ( pNextPier )
      {
         _ASSERT( pThisPier->GetNextSpan() == pNextPier->GetPrevSpan() );
         _ASSERT( pThisPier->GetStation() < pNextPier->GetStation() );
      }
   }

   // make sure all piers have an ID and that they are all unique
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      CPierData2* pPier = GetPier(pierIdx);
      PierIDType pierID = pPier->GetID();
      _ASSERT(pierID != INVALID_ID);
      for ( PierIndexType pierIdx2 = pierIdx+1; pierIdx2 < nPiers; pierIdx2++ )
      {
         CPierData2* pPier2 = GetPier(pierIdx2);
         PierIDType pierID2 = pPier2->GetID();
         _ASSERT(pierID2 != INVALID_ID);
         _ASSERT(pierID != pierID2);
      }
   }

   // Check that all groups are attached to this bridge and the previous/next group connect
   // to the same pier. Also check that the girder spacing match the number of girders in the group
   GroupIndexType nGroups = GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CGirderGroupData* pPrevGroup = GetGirderGroup(GroupIndexType(grpIdx-1));
      CGirderGroupData* pThisGroup = GetGirderGroup(GroupIndexType(grpIdx));
      CGirderGroupData* pNextGroup = GetGirderGroup(GroupIndexType(grpIdx+1));

      pThisGroup->AssertValid(); // is this group valid?

      _ASSERT(pThisGroup->GetID() != INVALID_ID);
      _ASSERT(pThisGroup->GetIndex() != INVALID_INDEX);

      _ASSERT( pThisGroup->GetBridgeDescription() == this );

      if ( pPrevGroup )
      {
         _ASSERT( pPrevGroup->GetPier(pgsTypes::metEnd) == pThisGroup->GetPier(pgsTypes::metStart) );

         // check girder count/girder spacing
         if ( pPrevGroup->GetGirderCount() != 0 )
         {
            _ASSERT( pPrevGroup->GetGirderCount() == pThisGroup->GetPier(pgsTypes::metStart)->GetGirderSpacing(pgsTypes::Back)->GetSpacingCount()+1);
         }
      }

      if ( pThisGroup->GetGirderCount() != 0 )
      {
         // check girder count/girder spacing
         _ASSERT(pThisGroup->GetGirderCount() == pThisGroup->GetPier(pgsTypes::metStart)->GetGirderSpacing(pgsTypes::Ahead)->GetSpacingCount()+1);
         _ASSERT(pThisGroup->GetGirderCount() == pThisGroup->GetPier(pgsTypes::metEnd  )->GetGirderSpacing(pgsTypes::Back )->GetSpacingCount()+1);
      }

      if ( pNextGroup )
      {
         _ASSERT( pThisGroup->GetPier(pgsTypes::metEnd) == pNextGroup->GetPier(pgsTypes::metStart) );

         // check girder count/girder spacing
         if ( pNextGroup->GetGirderCount() != 0 )
         {
            _ASSERT( pNextGroup->GetGirderCount() == pThisGroup->GetPier(pgsTypes::metEnd)->GetGirderSpacing(pgsTypes::Ahead)->GetSpacingCount()+1);
         }
      }
   }
}
#endif
