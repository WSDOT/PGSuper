///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\GirderData.h>
#include <WbflAtlExt.h>
#include <PGSuperException.h>

#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CBridgeDescription
****************************************************************************/


CBridgeDescription::CBridgeDescription()
{
   m_bSameNumberOfGirders = true;
   m_bSameGirderName      = true;

   m_MeasurementType     = pgsTypes::NormalToItem;
   m_MeasurementLocation = pgsTypes::AtCenterlinePier;

   m_AlignmentOffset = 0;

   m_SlabOffset     = ::ConvertToSysUnits( 10.0, unitMeasure::Inch );
   m_SlabOffsetType = pgsTypes::sotBridge;

   m_nGirders = 0;

   m_GirderSpacing     = ::ConvertToSysUnits(5.0,unitMeasure::Feet);
   m_GirderSpacingType = pgsTypes::sbsUniform;
   m_GirderOrientation = pgsTypes::Plumb;

   m_RefGirderIdx = INVALID_INDEX;
   m_RefGirderOffset = 0;
   m_RefGirderOffsetType = pgsTypes::omtBridge;


   m_pGirderLibraryEntry = NULL;

   m_LLDFMethod = pgsTypes::Calculated;

   m_Deck.SetBridgeDescription(this);
}

CBridgeDescription::CBridgeDescription(const CBridgeDescription& rOther)
{
   MakeCopy(rOther);
}

CBridgeDescription::~CBridgeDescription()
{
   Clear();
}

CBridgeDescription& CBridgeDescription::operator= (const CBridgeDescription& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CBridgeDescription::operator==(const CBridgeDescription& rOther) const
{
   if ( m_strGirderFamilyName != rOther.m_strGirderFamilyName )
      return false;

   if ( m_GirderOrientation != rOther.m_GirderOrientation )
      return false;

   if ( m_bSameGirderName != rOther.m_bSameGirderName )
      return false;

   if ( m_bSameGirderName &&  m_strGirderName != rOther.m_strGirderName )
      return false;
   
   if ( m_bSameNumberOfGirders != rOther.m_bSameNumberOfGirders )
      return false;

   if ( m_bSameNumberOfGirders && m_nGirders != rOther.m_nGirders )
      return false;

   if ( m_GirderSpacingType != rOther.m_GirderSpacingType )
      return false;

   if ( m_AlignmentOffset != rOther.m_AlignmentOffset )
      return false;

   if ( m_SlabOffsetType != rOther.m_SlabOffsetType )
      return false;

   if ( m_SlabOffsetType == pgsTypes::sotBridge )
   {
      if ( !IsEqual(m_SlabOffset,rOther.m_SlabOffset) )
         return false;
   }

   // the bridge-wide value of girder spacing only applies
   // if spacing is "sbsUniform"
   if ( m_GirderSpacingType == pgsTypes::sbsUniform || 
        m_GirderSpacingType == pgsTypes::sbsUniformAdjacent ||
        m_GirderSpacingType == pgsTypes::sbsConstantAdjacent )
   {
      if ( !IsEqual(m_GirderSpacing,rOther.m_GirderSpacing) )
         return false;

      if ( m_MeasurementLocation != rOther.m_MeasurementLocation )
         return false;

      if ( m_MeasurementType != rOther.m_MeasurementType )
         return false;

      if ( m_RefGirderIdx != rOther.m_RefGirderIdx )
         return false;

      if ( !IsEqual(m_RefGirderOffset,rOther.m_RefGirderOffset) )
         return false;

      if ( m_RefGirderOffsetType != rOther.m_RefGirderOffsetType )
         return false;
   }

   if ( m_Piers.size() != rOther.m_Piers.size() )
      return false;

   PierIndexType nPiers = m_Piers.size();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      if (*m_Piers[pierIdx] != *rOther.m_Piers[pierIdx] )
         return false;
   }

   if ( m_Spans.size() != rOther.m_Spans.size() )
      return false;

   SpanIndexType nSpans = m_Spans.size();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      if (*m_Spans[spanIdx] != *rOther.m_Spans[spanIdx] )
         return false;
   }

   if ( m_Deck != rOther.m_Deck )
      return false;

   if ( m_LeftRailingSystem != rOther.m_LeftRailingSystem )
      return false;

   if ( m_RightRailingSystem != rOther.m_RightRailingSystem )
      return false;

   if ( m_LLDFMethod != rOther.m_LLDFMethod )
      return false;

   return true;
}

bool CBridgeDescription::operator!=(const CBridgeDescription& rOther) const
{
   return !operator==(rOther);
}

HRESULT CBridgeDescription::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;

   Clear();

   CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit("BridgeDescription");

      double version;
      pStrLoad->get_Version(&version);
      
      CComVariant var;
      var.vt = VT_BOOL;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property("GirderFamilyName",&var);
      m_strGirderFamilyName = OLE2A(var.bstrVal);

      if ( version < 6 )
      {
         // in version 5 and earlier... GirderFamilyName was actually a beam factory name
         // map the beam factory name to its actual girder family
         if ( m_strGirderFamilyName == "Precast I-Beam" )
         {
            m_strGirderFamilyName = "I-Beam";
         }
         else if ( m_strGirderFamilyName == "Nebraska NU Girder" )
         {
            m_strGirderFamilyName = "I-Beam";
         }
         else if ( m_strGirderFamilyName == "Precast U-Beam (WSDOT)" ||
                   m_strGirderFamilyName == "Precast U-Beam (TXDOT)" )
         {
            m_strGirderFamilyName = "U-Beam";
         }
         else if ( m_strGirderFamilyName == "Double Tee (WSDOT)" ||
                   m_strGirderFamilyName == "Double Tee (TxDOT)" )
         {
            m_strGirderFamilyName = "Double Tee";
         }
      }

      var.vt = VT_I4;
      hr = pStrLoad->get_Property("GirderOrientation",&var);
      m_GirderOrientation = (pgsTypes::GirderOrientationType)(var.lVal);

      var.vt = VT_BOOL;
      hr = pStrLoad->get_Property("UseSameGirderForEntireBridge",&var);
      m_bSameGirderName = (var.boolVal == VARIANT_TRUE ? true : false);

      if ( m_bSameGirderName )
      {
         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property("Girder",&var);
         m_strGirderName = OLE2A(var.bstrVal);
      }

      var.vt = VT_BOOL;
      hr = pStrLoad->get_Property("UseSameNumberOfGirdersInAllSpans",&var);
      m_bSameNumberOfGirders = (var.boolVal == VARIANT_TRUE ? true : false);

      if ( m_bSameNumberOfGirders )
      {
         var.vt = VT_I2;
         hr = pStrLoad->get_Property("GirderCount",&var);
         m_nGirders = (GirderIndexType)var.iVal;
      }

      if ( version < 2 )
      {
         hr = pStrLoad->get_Property("UseSameGirderSpacingForEntireBridge",&var);
         bool bSameGirderSpacing = (var.boolVal == VARIANT_TRUE ? true : false);
         if ( bSameGirderSpacing )
            m_GirderSpacingType = pgsTypes::sbsUniform;
         else
            m_GirderSpacingType = pgsTypes::sbsGeneral;
      }
      else
      {
         var.vt = VT_I4;
         hr = pStrLoad->get_Property("GirderSpacingType",&var);
         m_GirderSpacingType = (pgsTypes::SupportedBeamSpacing)(var.lVal);
      }

      if ( IsBridgeSpacing(m_GirderSpacingType) )
      {
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("GirderSpacing",&var);
         m_GirderSpacing = var.dblVal;

         var.vt = VT_I4;
         hr = pStrLoad->get_Property("MeasurementLocation",&var);
         m_MeasurementLocation = (pgsTypes::MeasurementLocation)(var.lVal);

         var.vt = VT_I4;
         hr = pStrLoad->get_Property("MeasurementType",&var);
         m_MeasurementType = (pgsTypes::MeasurementType)(var.lVal);      

         if ( 3 == version )
         {
            //added in version 3, removed in version 4, no longer used
            // just load the value and keep going
            var.vt = VT_I4;
            hr = pStrLoad->get_Property("WorkPointLocation",&var);
            //m_WorkPointLocation = (pgsTypes::WorkPointLocation)(var.lVal);      
         }

         if ( 4 <= version )
         {
            // added in version 4
            var.vt = VT_UI4;
            hr = pStrLoad->get_Property("RefGirder",&var);
            m_RefGirderIdx = (GirderIndexType)var.ulVal;

            hr = pStrLoad->get_Property("RefGirderOffsetType",&var);
            m_RefGirderOffsetType = (pgsTypes::OffsetMeasurementType)(var.lVal);

            var.vt = VT_R8;
            hr = pStrLoad->get_Property("RefGirderOffset",&var);
            m_RefGirderOffset = var.dblVal;
         }
      }

      var.vt = VT_I4;
      hr = pStrLoad->get_Property("LLDFMethod",&var);
      m_LLDFMethod = (pgsTypes::DistributionFactorMethod)(var.lVal);

      if ( 4 <= version )
      {
         // added in version 4
         var.vt = VT_R8;
         hr = pStrLoad->get_Property("AlignmentOffset",&var);
         m_AlignmentOffset = var.dblVal;
      }

      if ( 5 <= version )
      {
         // added in version 5
         var.vt = VT_UI4;
         hr = pStrLoad->get_Property("SlabOffsetType",&var);
         m_SlabOffsetType = (pgsTypes::SlabOffsetType)(var.lVal);
         if ( m_SlabOffsetType == pgsTypes::sotBridge )
         {
            var.vt = VT_R8;
            hr = pStrLoad->get_Property("SlabOffset",&var);
            m_SlabOffset = var.dblVal;
         }
      }

      hr = pStrLoad->BeginUnit("Piers");
      var.vt = VT_UI2;
      hr = pStrLoad->get_Property("PierCount",&var);
      PierIndexType nPiers = (PierIndexType)var.uiVal;
      PierIndexType pierIdx = 0;
      // allocate, number, and associated all piers with bridge before loading
      // this has to be done so that links can be restored property
      for ( pierIdx = 0; pierIdx < nPiers; pierIdx++ )
      {
         CPierData* pPier = new CPierData;
         pPier->SetBridgeDescription(this);
         pPier->SetPierIndex(pierIdx);
         m_Piers.push_back(pPier);
      }
      // load each pier
      for ( pierIdx = 0; pierIdx < nPiers; pierIdx++ )
      {
         CPierData* pPier = m_Piers[pierIdx];
         hr = pPier->Load(pStrLoad,pProgress);
      }
      pStrLoad->EndUnit();


      pStrLoad->BeginUnit("Spans");
      SpanIndexType nSpans = nPiers-1;
      for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
      {
         CSpanData* pSpan = new CSpanData;
         pSpan->SetBridgeDescription(this);
         pSpan->SetSpanIndex(spanIdx);
         hr = pSpan->Load(pStrLoad,pProgress);
         m_Spans.push_back(pSpan);
      }
      pStrLoad->EndUnit();

      RenumberSpans(); // numbers the spans, piers, and hooks up the pointers

      // load the deck parameters... the slab offset has been added to this data block (at version 5)
      // if the deck data block is older, the slab offset is for the entire bridge and it is read
      // by the deck object. we pass pointers to the slab offset type and slab offset so that
      // the deck can set them to the correct value. If the deck object is newer, these pointers
      // are not altered.
      hr = m_Deck.Load(pStrLoad,pProgress,&m_SlabOffsetType,&m_SlabOffset);

      if ( version < 2 )
      {
         // when we loaded the version 1 data above, we didn't know if we had spread or adjacent beams
         // looking at the deck type gives us that extra information
         if ( m_Deck.DeckType == pgsTypes::sdtCompositeOverlay || m_Deck.DeckType == pgsTypes::sdtNone )
         {
            if ( m_GirderSpacingType == pgsTypes::sbsUniform )
               m_GirderSpacingType = pgsTypes::sbsUniformAdjacent;
            else if ( m_GirderSpacing == pgsTypes::sbsGeneral )
               m_GirderSpacingType = pgsTypes::sbsGeneralAdjacent;
         }
      }

      Float64 railing_version;
      hr = pStrLoad->BeginUnit("LeftRailingSystem");
      pStrLoad->get_Version(&railing_version);
      hr = m_LeftRailingSystem.Load(pStrLoad,pProgress);

      if ( railing_version < 2 )
      {
         // if before version 2, it was assumed that the slab material was used
         // for the railing system.
         m_LeftRailingSystem.fc              = m_Deck.SlabFc;
         m_LeftRailingSystem.bUserEc         = m_Deck.SlabUserEc;
         m_LeftRailingSystem.Ec              = m_Deck.SlabEc;
         m_LeftRailingSystem.StrengthDensity = m_Deck.SlabStrengthDensity;
         m_LeftRailingSystem.WeightDensity   = m_Deck.SlabWeightDensity;
         m_LeftRailingSystem.K1              = m_Deck.SlabK1;
      }

      hr = pStrLoad->EndUnit();

      hr = pStrLoad->BeginUnit("RightRailingSystem");
      pStrLoad->get_Version(&railing_version);
      hr = m_RightRailingSystem.Load(pStrLoad,pProgress);

      if ( railing_version < 2 )
      {
         // if before version 2, it was assumed that the slab material was used
         // for the railing system.
         m_RightRailingSystem.fc              = m_Deck.SlabFc;
         m_RightRailingSystem.bUserEc         = m_Deck.SlabUserEc;
         m_RightRailingSystem.Ec              = m_Deck.SlabEc;
         m_RightRailingSystem.StrengthDensity = m_Deck.SlabStrengthDensity;
         m_RightRailingSystem.WeightDensity   = m_Deck.SlabWeightDensity;
         m_RightRailingSystem.K1              = m_Deck.SlabK1;
      }
      hr = pStrLoad->EndUnit();

      hr = pStrLoad->EndUnit(); // BridgeDescription

      CopyDown(m_bSameNumberOfGirders, 
               m_bSameGirderName, 
               IsBridgeSpacing(m_GirderSpacingType),
               m_SlabOffsetType == pgsTypes::sotBridge);
   }
   catch(...)
   {
      ATLASSERT(0);
   }

   AssertValid();
   return hr;
}

HRESULT CBridgeDescription::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;
   pStrSave->BeginUnit("BridgeDescription",6.0);

   // GirderFamilyName was actually a beam factory name in version 5... in version 6 of this
   // data block, it is really a girder family name
   pStrSave->put_Property("GirderFamilyName",CComVariant(CComBSTR(m_strGirderFamilyName.c_str())));

   pStrSave->put_Property("GirderOrientation",CComVariant(m_GirderOrientation));

   pStrSave->put_Property("UseSameGirderForEntireBridge",CComVariant(m_bSameGirderName));
   if ( m_bSameGirderName )
   {
      pStrSave->put_Property("Girder",CComVariant(CComBSTR(m_strGirderName.c_str())));
   }

   pStrSave->put_Property("UseSameNumberOfGirdersInAllSpans",CComVariant(m_bSameNumberOfGirders) );
   if ( m_bSameNumberOfGirders )
   {
      pStrSave->put_Property("GirderCount",CComVariant(m_nGirders));
   }

   pStrSave->put_Property("GirderSpacingType",CComVariant(m_GirderSpacingType) ); // changed in version 2
   if ( IsBridgeSpacing(m_GirderSpacingType) )
   {
      pStrSave->put_Property("GirderSpacing",       CComVariant(m_GirderSpacing));
      pStrSave->put_Property("MeasurementLocation", CComVariant(m_MeasurementLocation));
      pStrSave->put_Property("MeasurementType",     CComVariant(m_MeasurementType));

      // added in version 4
      pStrSave->put_Property("RefGirder",CComVariant(m_RefGirderIdx));
      pStrSave->put_Property("RefGirderOffsetType",CComVariant(m_RefGirderOffsetType));
      pStrSave->put_Property("RefGirderOffset",CComVariant(m_RefGirderOffset));
   }

   hr = pStrSave->put_Property("LLDFMethod",CComVariant(m_LLDFMethod));

   hr = pStrSave->put_Property("AlignmentOffset",CComVariant(m_AlignmentOffset)); // added in version 4

   // added in version 5
   hr = pStrSave->put_Property("SlabOffsetType",CComVariant(m_SlabOffsetType));
   if ( m_SlabOffsetType == pgsTypes::sotBridge )
      hr = pStrSave->put_Property("SlabOffset",CComVariant(m_SlabOffset));
   
   pStrSave->BeginUnit("Piers",1.0);
   pStrSave->put_Property("PierCount",CComVariant((long)m_Piers.size()));
   std::vector<CPierData*>::iterator pierIter;
   for ( pierIter = m_Piers.begin(); pierIter != m_Piers.end(); pierIter++ )
   {
      CPierData* pPier = *pierIter;
      pPier->Save(pStrSave,pProgress);
   }
   pStrSave->EndUnit();


   pStrSave->BeginUnit("Spans",1.0);
   std::vector<CSpanData*>::iterator spanIter;
   for ( spanIter = m_Spans.begin(); spanIter != m_Spans.end(); spanIter++ )
   {
      CSpanData* pSpan = *spanIter;
      pSpan->Save(pStrSave,pProgress);
   }
   pStrSave->EndUnit();


   m_Deck.Save(pStrSave,pProgress);

   pStrSave->BeginUnit("LeftRailingSystem",2.0);
   m_LeftRailingSystem.Save(pStrSave,pProgress);
   pStrSave->EndUnit();

   pStrSave->BeginUnit("RightRailingSystem",2.0);
   m_RightRailingSystem.Save(pStrSave,pProgress);
   pStrSave->EndUnit();
   
   pStrSave->EndUnit(); // BridgeDescription
   return hr;
}

void CBridgeDescription::MakeCopy(const CBridgeDescription& rOther)
{
   m_bSameNumberOfGirders     = rOther.m_bSameNumberOfGirders;
   m_bSameGirderName          = rOther.m_bSameGirderName;

   m_LeftRailingSystem  = rOther.m_LeftRailingSystem;
   m_RightRailingSystem = rOther.m_RightRailingSystem;

   m_AlignmentOffset     = rOther.m_AlignmentOffset;
   m_GirderSpacingType   = rOther.m_GirderSpacingType;
   m_GirderSpacing       = rOther.m_GirderSpacing;
   m_RefGirderIdx        = rOther.m_RefGirderIdx;
   m_RefGirderOffset     = rOther.m_RefGirderOffset;
   m_RefGirderOffsetType = rOther.m_RefGirderOffsetType;

   m_nGirders            = rOther.m_nGirders;
   m_strGirderName       = rOther.m_strGirderName;
   m_strGirderFamilyName = rOther.m_strGirderFamilyName;
   m_GirderOrientation   = rOther.m_GirderOrientation;

   m_SlabOffset          = rOther.m_SlabOffset;
   m_SlabOffsetType      = rOther.m_SlabOffsetType;

   m_pGirderLibraryEntry = rOther.m_pGirderLibraryEntry;

   m_MeasurementType     = rOther.m_MeasurementType;
   m_MeasurementLocation = rOther.m_MeasurementLocation;

   // clear out the deck, spans and piers... then rebuild
   Clear();

   m_Deck               = rOther.m_Deck;

   std::vector<CPierData*>::const_iterator pierIter = rOther.m_Piers.begin();
   const CPierData* pFirstPier = *pierIter;
   pierIter++;

   std::vector<CSpanData*>::const_iterator spanIter = rOther.m_Spans.begin();

   bool bFirst = true;
   for ( ; pierIter != rOther.m_Piers.end(); pierIter++, spanIter++ )
   {
      const CSpanData* pSpan = *spanIter;
      const CPierData* pPier = *pierIter;

      if ( bFirst )
      {
         CreateFirstSpan(pFirstPier,pSpan,pPier);
         bFirst = false;
      }
      else
      {
         AppendSpan(pSpan,pPier);
      }
   }

   m_LLDFMethod = rOther.m_LLDFMethod;

   AssertValid();
}

void CBridgeDescription::MakeAssignment(const CBridgeDescription& rOther)
{
   MakeCopy( rOther );
}

CDeckDescription* CBridgeDescription::GetDeckDescription()
{
   return &m_Deck;
}

const CDeckDescription* CBridgeDescription::GetDeckDescription() const
{
   return &m_Deck;
}

CRailingSystem* CBridgeDescription::GetLeftRailingSystem()
{
   return &m_LeftRailingSystem;
}

const CRailingSystem* CBridgeDescription::GetLeftRailingSystem() const
{
   return &m_LeftRailingSystem;
}

CRailingSystem* CBridgeDescription::GetRightRailingSystem()
{
   return &m_RightRailingSystem;
}

const CRailingSystem* CBridgeDescription::GetRightRailingSystem() const
{
   return &m_RightRailingSystem;
}

void CBridgeDescription::Clear()
{
   std::vector<CPierData*>::iterator pierIter;
   for ( pierIter = m_Piers.begin(); pierIter != m_Piers.end(); pierIter++ )
   {
      CPierData* pPierData = *pierIter;
      delete pPierData;
   }
   m_Piers.clear();

   std::vector<CSpanData*>::iterator spanIter;
   for ( spanIter = m_Spans.begin(); spanIter != m_Spans.end(); spanIter++ )
   {
      CSpanData* pSpanData = *spanIter;
      delete pSpanData;
   }
   m_Spans.clear();

   m_Deck.DeckEdgePoints.clear();
}

void CBridgeDescription::CreateFirstSpan(const CPierData* pFirstPier,const CSpanData* pFirstSpan,const CPierData* pNextPier)
{
   _ASSERT( 0 == m_Piers.size() && 0 == m_Spans.size() ); // this call should only be made once

   CPierData* firstPier;
   CSpanData* firstSpan;
   CPierData* nextPier;

   if ( pFirstPier )
      firstPier = new CPierData(*pFirstPier);
   else
      firstPier = new CPierData();

   if ( pFirstSpan )
      firstSpan = new CSpanData(*pFirstSpan);
   else
      firstSpan = new CSpanData();

   if ( pNextPier )
   {
      nextPier = new CPierData(*pNextPier);
   }
   else
   {
      nextPier = new CPierData();
      nextPier->SetStation( firstPier->GetStation() + ::ConvertToSysUnits(100.0,unitMeasure::Feet) );
   }

   firstPier->SetBridgeDescription(this);
   firstSpan->SetBridgeDescription(this);
   nextPier->SetBridgeDescription(this);

   m_Piers.push_back(firstPier);
   m_Spans.push_back(firstSpan);
   m_Piers.push_back(nextPier);

   RenumberSpans();

   if ( !pFirstSpan )
      firstSpan->SetGirderCount(m_nGirders);

   AssertValid();
}

void CBridgeDescription::AppendSpan(const CSpanData* pSpanData,const CPierData* pPierData)
{
   // Use pier stationing to determine span length
   InsertSpan(-1,pgsTypes::Ahead,-1.0,pSpanData,pPierData);
}

void CBridgeDescription::InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace,Float64 newSpanLength, const CSpanData* pSpanData,const CPierData* pPierData)
{
   ASSERT( 2 <= m_Piers.size() && 1 <= m_Spans.size() ); // if this first, then the call to CreateFirstSpan hasn't been made yet

   // Negative span length means that we take stationing from piers - better have pier data
   if ( newSpanLength<=0  )
   {
      if (!pPierData)
      {
         ASSERT(0); // Assert will warn above about this case, take care if it happens
         newSpanLength = ::ConvertToSysUnits(100.0,unitMeasure::Feet);
      }
   }

   // if refPierIdx < 0 then treat this as an append
   if ( refPierIdx == ALL_PIERS )
   {
      refPierIdx = m_Piers.size()-1;
   }

   // Index for our new span
   SpanIndexType newSpanIdx;
   if (refPierIdx==0)
   {
      newSpanIdx = 0;
   }
   else
   {
      newSpanIdx = pierFace==pgsTypes::Back ? refPierIdx-1 : refPierIdx;
   }

   // Copy properties from the span on the side of the pier in question (if it exists)
   SpanIndexType refSpanIdx;
   if (refPierIdx<=0)
   {
      refSpanIdx = 0;
   }
   else if (refPierIdx >= (PierIndexType)m_Piers.size()-1)
   {
      refSpanIdx = m_Piers.size()-2;
   }
   else if (pierFace==pgsTypes::Back)
   {
      refSpanIdx = refPierIdx-1;
   }
   else
   {
      ASSERT(pierFace==pgsTypes::Ahead);
      refSpanIdx = refPierIdx;
   }

   // Create the new span and pier objects
   CSpanData* pNewSpan;
   CPierData* pNewPier;

   if ( pSpanData )
      pNewSpan = new CSpanData(*pSpanData);
   else
      pNewSpan = new CSpanData(*m_Spans[refSpanIdx]);

   if ( pPierData )
   {
      pNewPier = new CPierData(*pPierData);
   }
   else
   {
      pNewPier = new CPierData(*m_Piers[refPierIdx]);
   }

   // assign bridge model to new span and pier
   pNewSpan->SetBridgeDescription(this);
   pNewPier->SetBridgeDescription(this);

   // store the new span and pier
   m_Spans.insert(m_Spans.begin()+newSpanIdx,pNewSpan); 

   std::vector<CPierData*>::iterator backPierIter; // pier just to the back of our new span
   if ( pierFace == pgsTypes::Back )
   {
      // insert new pier before the reference pier
      backPierIter = m_Piers.insert(m_Piers.begin() + refPierIdx, pNewPier );
   }
   else
   {
      // insert new pier after the reference pier
      backPierIter = m_Piers.insert(m_Piers.begin() + refPierIdx + 1, pNewPier);
      backPierIter--;
   }

   // renumbers spans and sets the pier<-->span<-->pier pointers
   RenumberSpans();

   CPierData* pPrevPier = pNewSpan->GetPrevPier();
   if ( pPrevPier->GetConnectionLibraryEntry(pgsTypes::Ahead) == NULL )
   {
      // the prev pier doesn't have a connection on this span's side of the pier
      // copy from the back side
      pPrevPier->SetConnection(pgsTypes::Ahead,             pPrevPier->GetConnection(pgsTypes::Back) );
      pPrevPier->SetConnectionLibraryEntry(pgsTypes::Ahead, pPrevPier->GetConnectionLibraryEntry(pgsTypes::Back) );
   }

   CPierData* pNextPier = pNewSpan->GetNextPier();
   if ( pNextPier->GetConnectionLibraryEntry(pgsTypes::Back) == NULL )
   {
      // the next pier doesn't have a connection on this span's side of the pier
      // copy from the back side
      pNextPier->SetConnection(pgsTypes::Back,             pNextPier->GetConnection(pgsTypes::Ahead) );
      pNextPier->SetConnectionLibraryEntry(pgsTypes::Back, pNextPier->GetConnectionLibraryEntry(pgsTypes::Ahead) );
   }

   // offset all piers after the new pier by the length of the new span
   if (newSpanLength>0.0)
   {
      std::vector<CPierData*>::iterator pierIter;
      for ( pierIter = backPierIter + 1; pierIter != m_Piers.end(); pierIter++ )
      {
         CPierData* pPier = *pierIter;
         pPier->SetStation( pPier->GetStation() + newSpanLength);
      }
   }

   if ( !pSpanData && m_bSameNumberOfGirders )
      pNewSpan->SetGirderCount(m_nGirders);

   AssertValid();
}

void CBridgeDescription::RemoveSpan(SpanIndexType spanIdx,pgsTypes::RemovePierType rmPierType)
{
   CSpanData* pPrevSpan = GetSpan(spanIdx-1);
   CSpanData* pSpan     = GetSpan(spanIdx);
   CSpanData* pNextSpan = GetSpan(spanIdx+1);

   CPierData* pPrevPier = pSpan->GetPrevPier();
   CPierData* pNextPier = pSpan->GetNextPier();

   double span_length = pSpan->GetSpanLength();
   PierIndexType removePierIdx;

   if ( rmPierType == pgsTypes::PrevPier )
   {
      removePierIdx = spanIdx;
      m_Spans.erase(m_Spans.begin()+spanIdx);
      m_Piers.erase(m_Piers.begin()+removePierIdx);
      delete pPrevPier;
      delete pSpan;
   }
   else
   {
      removePierIdx = spanIdx + 1;
      m_Spans.erase(m_Spans.begin()+spanIdx);
      m_Piers.erase(m_Piers.begin()+removePierIdx);
      delete pSpan;
      delete pNextPier;
   }

   RenumberSpans();

   // offset all piers after the pir that was removed by the length of the span that was removed
   std::vector<CPierData*>::iterator pierIter;
   for ( pierIter = m_Piers.begin()+removePierIdx; pierIter != m_Piers.end(); pierIter++ )
   {
      CPierData* pPier = *pierIter;
      pPier->SetStation( pPier->GetStation() - span_length );
   }

   AssertValid();
}

PierIndexType CBridgeDescription::GetPierCount() const
{
   return m_Piers.size();
}

SpanIndexType CBridgeDescription::GetSpanCount() const
{
   return m_Spans.size();
}

CPierData* CBridgeDescription::GetPier(PierIndexType pierIdx)
{
   if (0 <= pierIdx && pierIdx < (PierIndexType)m_Piers.size() )
     return m_Piers[pierIdx];

   return NULL;
}

const CPierData* CBridgeDescription::GetPier(PierIndexType pierIdx) const
{
   if (0 <= pierIdx && pierIdx < (PierIndexType)m_Piers.size() )
      return m_Piers[pierIdx];

   return NULL;
}

CSpanData* CBridgeDescription::GetSpan(SpanIndexType spanIdx)
{
   if ( 0 <= spanIdx && spanIdx < (SpanIndexType)m_Spans.size() )
     return m_Spans[spanIdx];

   return NULL;
}

const CSpanData* CBridgeDescription::GetSpan(SpanIndexType spanIdx) const
{
   if ( 0 <= spanIdx && spanIdx < (SpanIndexType)m_Spans.size() )
      return m_Spans[spanIdx];

   return NULL;
}

void CBridgeDescription::UseSameNumberOfGirdersInAllSpans(bool bSame) 
{
   m_bSameNumberOfGirders = bSame;
}

bool CBridgeDescription::UseSameNumberOfGirdersInAllSpans() const
{
   return m_bSameNumberOfGirders;
}

void CBridgeDescription::SetGirderCount(GirderIndexType nGirders)
{
   m_nGirders = nGirders;

   if ( m_bSameNumberOfGirders )
   {
      std::vector<CSpanData*>::iterator iter;
      for ( iter = m_Spans.begin(); iter != m_Spans.end(); iter++ )
      {
         CSpanData* pSpan = *iter;
         pSpan->SetGirderCount(nGirders);
      }
   }
}

GirderIndexType CBridgeDescription::GetGirderCount() const
{
   return m_nGirders;
}

void CBridgeDescription::SetGirderFamilyName(const char* strName)
{
   m_strGirderFamilyName = strName;
}

const char* CBridgeDescription::GetGirderFamilyName() const
{
   return m_strGirderFamilyName.c_str();
}

void CBridgeDescription::UseSameGirderForEntireBridge(bool bSame)
{
   m_bSameGirderName = bSame;
}

bool CBridgeDescription::UseSameGirderForEntireBridge() const
{
   return m_bSameGirderName;
}

const char* CBridgeDescription::GetGirderName() const
{
   return m_strGirderName.c_str();
}

void CBridgeDescription::SetGirderName(const char* strName)
{
   if ( m_strGirderName != strName )
   {
      // girder name changed...
      m_strGirderName = strName;

      // need to reset prestressing data
      CSpanData* pSpan = GetSpan(0);
      while ( pSpan )
      {
         CGirderTypes girderTypes = *(pSpan->GetGirderTypes());
         GirderIndexType nGirders = pSpan->GetGirderCount();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            CGirderData& girderData = girderTypes.GetGirderData(gdrIdx);
            girderData.ResetPrestressData();
            pSpan->SetGirderTypes(girderTypes);
         }

         CPierData* pNextPier = pSpan->GetNextPier();
         pSpan = pNextPier->GetNextSpan();
      }
   }
}

const GirderLibraryEntry* CBridgeDescription::GetGirderLibraryEntry() const
{
   return m_pGirderLibraryEntry;
}

void CBridgeDescription::SetGirderLibraryEntry(const GirderLibraryEntry* pEntry)
{
   m_pGirderLibraryEntry = pEntry;
}

void CBridgeDescription::SetGirderOrientation(pgsTypes::GirderOrientationType gdrOrientation)
{
   m_GirderOrientation = gdrOrientation;
}

pgsTypes::GirderOrientationType CBridgeDescription::GetGirderOrientation() const
{
   return m_GirderOrientation;
}

void CBridgeDescription::SetGirderSpacing(double spacing)
{
   m_GirderSpacing = spacing;
}

double CBridgeDescription::GetGirderSpacing() const
{
   return m_GirderSpacing;
}

void CBridgeDescription::SetGirderSpacingType(pgsTypes::SupportedBeamSpacing sbs)
{
   m_GirderSpacingType = sbs;
}

pgsTypes::SupportedBeamSpacing CBridgeDescription::GetGirderSpacingType() const
{
   return m_GirderSpacingType;
}

void CBridgeDescription::SetMeasurementType(pgsTypes::MeasurementType mt)
{
   m_MeasurementType = mt;
}

pgsTypes::MeasurementType CBridgeDescription::GetMeasurementType() const
{
   return m_MeasurementType;
}

void CBridgeDescription::SetMeasurementLocation(pgsTypes::MeasurementLocation ml)
{
   m_MeasurementLocation = ml;
}

pgsTypes::MeasurementLocation CBridgeDescription::GetMeasurementLocation() const
{
   return m_MeasurementLocation;
}

bool CBridgeDescription::SetSpanLength(SpanIndexType spanIdx,double newLength)
{
   _ASSERT( 0 < newLength );
   CSpanData* pSpan = GetSpan(spanIdx);
   double length = pSpan->GetSpanLength();
   double deltaL = newLength - length;

   if ( IsZero(deltaL) )
      return false;

   while ( pSpan )
   {
      CPierData* pNextPier = pSpan->GetNextPier();
      pNextPier->SetStation( pNextPier->GetStation() + deltaL);

      pSpan = pNextPier->GetNextSpan();
   }

   return true;
}

void CBridgeDescription::SetRefGirder(GirderIndexType refGdrIdx)
{
   m_RefGirderIdx = refGdrIdx;
}

GirderIndexType CBridgeDescription::GetRefGirder() const
{
   return m_RefGirderIdx;
}

void CBridgeDescription::SetRefGirderOffset(double offset)
{
   m_RefGirderOffset = offset;
}

double CBridgeDescription::GetRefGirderOffset() const
{
   return m_RefGirderOffset;
}

void CBridgeDescription::SetRefGirderOffsetType(pgsTypes::OffsetMeasurementType offsetType)
{
   m_RefGirderOffsetType = offsetType;
}

pgsTypes::OffsetMeasurementType CBridgeDescription::GetRefGirderOffsetType() const
{
   return m_RefGirderOffsetType;
}

void CBridgeDescription::SetAlignmentOffset(double alignmentOffset)
{
   m_AlignmentOffset = alignmentOffset;
}

double CBridgeDescription::GetAlignmentOffset() const
{
   return m_AlignmentOffset;
}

void CBridgeDescription::SetSlabOffsetType(pgsTypes::SlabOffsetType slabOffsetType)
{
   m_SlabOffsetType = slabOffsetType;
}

pgsTypes::SlabOffsetType CBridgeDescription::GetSlabOffsetType() const
{
   return m_SlabOffsetType;
}

void CBridgeDescription::SetSlabOffset(Float64 slabOffset)
{
   m_SlabOffset = slabOffset;
}

Float64 CBridgeDescription::GetSlabOffset() const
{
   return m_SlabOffset;
}

Float64 CBridgeDescription::GetMaxSlabOffset() const
{
   if ( m_SlabOffsetType == pgsTypes::sotBridge )
      return m_SlabOffset;

   const CSpanData* pSpan = GetSpan(0);
   Float64 maxSlabOffset = 0;
   do
   {
      const CGirderTypes* pGirderTypes = pSpan->GetGirderTypes();
      GirderIndexType nGirders = pGirderTypes->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         Float64 startSlabOffset = pGirderTypes->GetSlabOffset(gdrIdx,pgsTypes::metStart);
         Float64 endSlabOffset   = pGirderTypes->GetSlabOffset(gdrIdx,pgsTypes::metEnd);

         maxSlabOffset = ::Max3(maxSlabOffset,startSlabOffset,endSlabOffset);
      }

      pSpan = pSpan->GetNextPier()->GetNextSpan();
   } while (pSpan);

   return maxSlabOffset;
}

void CBridgeDescription::SetDistributionFactorMethod(pgsTypes::DistributionFactorMethod method)
{
   m_LLDFMethod = method;
}

pgsTypes::DistributionFactorMethod CBridgeDescription::GetDistributionFactorMethod() const
{
   return m_LLDFMethod;
}

bool CBridgeDescription::MovePier(PierIndexType pierIdx,double newStation,pgsTypes::MovePierOption moveOption)
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

bool CBridgeDescription::MoveBridge(PierIndexType pierIdx,double newStation)
{
   // move pierIdx to newStation and keep all the span lengths constant
   CPierData* pPier = GetPier(pierIdx);
   double old_station = pPier->GetStation();
   double deltaStation = newStation - old_station;
   if ( IsZero(deltaStation) )
      return false;

   CPierData* pPrevPier = GetPier(0);
   pPrevPier->SetStation(pPrevPier->GetStation() + deltaStation);

   CSpanData* pNextSpan = pPrevPier->GetNextSpan();
   while ( pNextSpan )
   {
      CPierData* pNextPier = pNextSpan->GetNextPier();

      pNextPier->SetStation( pNextPier->GetStation() + deltaStation);
      
      pNextSpan = pNextPier->GetNextSpan();
   }

   // move all the deck points
   std::vector<CDeckPoint>::iterator iter;
   for ( iter = m_Deck.DeckEdgePoints.begin(); iter != m_Deck.DeckEdgePoints.end(); iter++ )
   {
      CDeckPoint& deckPoint = *iter;
      deckPoint.Station += deltaStation;
   }


   return true;
}

bool CBridgeDescription::MoveBridgeAdjustPrevSpan(PierIndexType pierIdx,double newStation)
{
   // move pierIdx and all piers that come after it by delta
   // this will retain the length of all spans execpt for the one
   // immedately before the pier
   CPierData* pPier = GetPier(pierIdx);
   double old_station = pPier->GetStation();
   double deltaStation = newStation - old_station;
   if ( IsZero(deltaStation) )
      return false;

   pPier->SetStation(old_station + deltaStation);

   CPierData* pPrevPier = pPier;
   CSpanData* pNextSpan = pPrevPier->GetNextSpan();
   while ( pNextSpan )
   {
      CPierData* pNextPier = pNextSpan->GetNextPier();
      pNextPier->SetStation(pNextPier->GetStation() + deltaStation);
 
      pNextSpan = pNextPier->GetNextSpan();
   }

   return true;
}

bool CBridgeDescription::MoveBridgeAdjustNextSpan(PierIndexType pierIdx,double newStation)
{
   // move pierIdx and all piers that come before it by delta
   // this will retain the length of all spans execpt for the one
   // immedately after the pier
   CPierData* pPier = GetPier(pierIdx);
   double old_station = pPier->GetStation();
   double deltaStation = newStation - old_station;
   if ( IsZero(deltaStation) )
      return false;

   pPier->SetStation( pPier->GetStation() + deltaStation);

   CPierData* pNextPier = pPier;
   CSpanData* pPrevSpan = pNextPier->GetPrevSpan();
   while ( pPrevSpan )
   {
      CPierData* pPrevPier = pPrevSpan->GetPrevPier();
      pPrevPier->SetStation( pPrevPier->GetStation() + deltaStation);

      pPrevSpan = pPrevPier->GetPrevSpan();
   }

   return true;
}

bool CBridgeDescription::MoveBridgeAdjustAdjacentSpans(PierIndexType pierIdx,double newStation)
{
   CPierData* pPier = GetPier(pierIdx);
   double old_station = pPier->GetStation();
   double deltaStation = newStation - old_station;
   if ( IsZero(deltaStation) )
      return false;

   pPier->SetStation(pPier->GetStation() + deltaStation);

   return true;
}

void CBridgeDescription::CopyDown(bool bGirderCount,bool bGirderType,bool bSpacing,bool bSlabOffset)
{
   // NOTE: If you are adding data to be copied in this function, you will want to also make changes
   //       to ReconcileEdits
   CSpanData* pSpan = GetSpan(0);
   while ( pSpan )
   {
      if ( bGirderCount )
         pSpan->SetGirderCount( m_nGirders );

      if ( bSpacing )
      {
         pSpan->GetGirderSpacing(pgsTypes::metStart)->JoinAll(0);
         pSpan->GetGirderSpacing(pgsTypes::metStart)->SetGirderSpacing(0,m_GirderSpacing);
         pSpan->GetGirderSpacing(pgsTypes::metStart)->SetMeasurementLocation(m_MeasurementLocation);
         pSpan->GetGirderSpacing(pgsTypes::metStart)->SetMeasurementType(m_MeasurementType);
         pSpan->GetGirderSpacing(pgsTypes::metStart)->SetRefGirder(m_RefGirderIdx);
         pSpan->GetGirderSpacing(pgsTypes::metStart)->SetRefGirderOffset(m_RefGirderOffset);
         pSpan->GetGirderSpacing(pgsTypes::metStart)->SetRefGirderOffsetType(m_RefGirderOffsetType);

         pSpan->GetGirderSpacing(pgsTypes::metEnd)->JoinAll(0);
         pSpan->GetGirderSpacing(pgsTypes::metEnd)->SetGirderSpacing(0,m_GirderSpacing);
         pSpan->GetGirderSpacing(pgsTypes::metEnd)->SetMeasurementLocation(m_MeasurementLocation);
         pSpan->GetGirderSpacing(pgsTypes::metEnd)->SetMeasurementType(m_MeasurementType);
         pSpan->GetGirderSpacing(pgsTypes::metEnd)->SetRefGirder(m_RefGirderIdx);
         pSpan->GetGirderSpacing(pgsTypes::metEnd)->SetRefGirderOffset(m_RefGirderOffset);
         pSpan->GetGirderSpacing(pgsTypes::metEnd)->SetRefGirderOffsetType(m_RefGirderOffsetType);
      }

      if ( bGirderType )
      {
         CGirderTypes* pGirderTypes = pSpan->GetGirderTypes();
         pGirderTypes->JoinAll(0);
         pGirderTypes->SetGirderName(0,m_strGirderName.c_str());
         pGirderTypes->SetGirderLibraryEntry(0, m_pGirderLibraryEntry );
      }

      if ( bSlabOffset )
      {
         CGirderTypes* pGirderTypes = pSpan->GetGirderTypes();
         GirderIndexType nGirders = pGirderTypes->GetGirderCount();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            pGirderTypes->SetSlabOffset(gdrIdx,pgsTypes::metStart,m_SlabOffset);
            pGirderTypes->SetSlabOffset(gdrIdx,pgsTypes::metEnd,  m_SlabOffset);
         }
      }

      pSpan = pSpan->GetNextPier()->GetNextSpan();
   }
}

void CBridgeDescription::ReconcileEdits(IBroker* pBroker, const CBridgeDescription* pOriginal)
{
   // Note: that *this is the data that has been edited and pOriginal represents
   //       *this before editing.

   // First step is to check if data has been changed at the bridge level. If so,
   // copy down to the individual span/girder level

   bool copyGirderCount = false;
   if (this->m_bSameNumberOfGirders)
   {
     if(!pOriginal->m_bSameNumberOfGirders ||
        this->m_nGirders != pOriginal->m_nGirders)
     {
        copyGirderCount = true;
     }
   }

   bool copyGirderType = false;
   if (this->m_bSameGirderName)
   {
      if (!pOriginal->m_bSameGirderName ||
          this->m_strGirderName != pOriginal->m_strGirderName)
      {
         copyGirderType = true;
      }
   }
   
   bool copySpacing = false;
   if (IsBridgeSpacing(this->m_GirderSpacingType))
   {
      if (!IsBridgeSpacing(pOriginal->m_GirderSpacingType) ||
          this->m_GirderSpacing != pOriginal->m_GirderSpacing)
      {
         copySpacing = true;
      }
   }

   bool copySlabOffset = false;
   if (this->m_SlabOffsetType==pgsTypes::sotBridge)
   {
      if (pOriginal->m_SlabOffsetType!=pgsTypes::sotBridge ||
          this->m_SlabOffset != pOriginal->m_SlabOffset)
      {
         copySlabOffset = true;
      }
   }

   // Copy bridge data down to spans if needed
   CopyDown(copyGirderCount ,copyGirderType, copySpacing, copySlabOffset);

   // Next step is to refill seed data for girder stirrups or long rebar
   // for any girders that have changed types
   // get shear information from library
   GET_IFACE2( pBroker, ILibrary, pLib );

   // NOTE: The logic here isn't, and probably can't be perfect. If spans or girder groups are added and 
   //       shuffled, it's impossible to compare with the original configuration. The default here
   //       is, if in doubt, use seed data
   std::vector<CSpanData*>::const_iterator origSpanIter = pOriginal->m_Spans.begin();
   for(std::vector<CSpanData*>::iterator thisSpanIter = m_Spans.begin(); thisSpanIter!=m_Spans.end(); thisSpanIter++)
   {
      if(origSpanIter != pOriginal->m_Spans.end())
         origSpanIter++;

      CSpanData* pthisSpan = *thisSpanIter;
      CSpanData* pOrigSpan = (origSpanIter==pOriginal->m_Spans.end() ? NULL : *origSpanIter);

      GroupIndexType thisNGroups = pthisSpan->GetGirderTypes()->GetGirderGroupCount();
      GroupIndexType origNGroups = (pOrigSpan!=NULL ? pOrigSpan->GetGirderTypes()->GetGirderGroupCount() : 0);

      for(GroupIndexType iGroup = 0; iGroup< thisNGroups; iGroup++)
      {
         std::string thisGirderName;
         GirderIndexType nthisGstart, nthisGend;
         pthisSpan->GetGirderTypes()->GetGirderGroup(iGroup, &nthisGstart, &nthisGend, thisGirderName);

         std::string origGirderName;
         if (iGroup < origNGroups)
         {
            GirderIndexType norigGstart, norigGend;
            pOrigSpan->GetGirderTypes()->GetGirderGroup(iGroup, &norigGstart, &norigGend, origGirderName);
         }

         if (thisGirderName != origGirderName)
         {
            // Enough evidence here that the girder type was changed - refill with seed data
            const GirderLibraryEntry* pGird = pLib->GetGirderEntry( thisGirderName.c_str());
            ASSERT(pGird!=0);

            for (GirderIndexType igdr=nthisGstart; igdr<=nthisGend; igdr++)
            {
               CGirderData& thisGdrData = pthisSpan->GetGirderTypes()->GetGirderData(igdr);

               thisGdrData.ShearData.CopyGirderEntryData( *pGird );

               thisGdrData.LongitudinalRebarData.CopyGirderEntryData( *pGird );
            }
         }
      }
   }
}


void CBridgeDescription::RenumberSpans()
{
   // renumbers the spans and piers and updates all the prev/next pointers.
   std::vector<CSpanData*>::iterator spanIter = m_Spans.begin();
   std::vector<CPierData*>::iterator pierIter = m_Piers.begin();

   SpanIndexType spanIdx = 0;
   PierIndexType pierIdx = 0;
   CSpanData* pPrevSpan = NULL;

   CPierData* pPrevPier = *pierIter++;
   pPrevPier->SetPierIndex(pierIdx++);

   for ( ; spanIter != m_Spans.end() && pierIter != m_Piers.end(); spanIter++, pierIter++, spanIdx++, pierIdx++ )
   {
      CSpanData* pThisSpan = *spanIter;
      CPierData* pNextPier = *pierIter;

      pThisSpan->SetSpanIndex(spanIdx);
      pNextPier->SetPierIndex(pierIdx);

      pPrevPier->SetSpans(pPrevSpan,pThisSpan);
      pThisSpan->SetPiers(pPrevPier,pNextPier);

      pPrevPier = pNextPier;
      pPrevSpan = pThisSpan;
   }

   // last pier
   CPierData* pLastPier = m_Piers.back();
   pLastPier->SetSpans(pPrevSpan,NULL);

   // Make sure connection data at ends of bridge are cleared
   CPierData* pPier = m_Piers.front();
   pPier->SetConnectionLibraryEntry(pgsTypes::Back, NULL);
   pPier = m_Piers.back();
   pPier->SetConnectionLibraryEntry(pgsTypes::Ahead, NULL);
}

void CBridgeDescription::AssertValid()
{
#if defined _DEBUG
   SpanIndexType nSpans = GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      CSpanData* pPrevSpan = GetSpan(spanIdx-1);
      CSpanData* pThisSpan = GetSpan(spanIdx);
      CSpanData* pNextSpan = GetSpan(spanIdx+1);

      if ( pPrevSpan )
      {
         _ASSERT( pPrevSpan->GetNextPier() == pThisSpan->GetPrevPier() );
      }

      if ( pNextSpan )
      {
         _ASSERT( pThisSpan->GetNextPier() == pNextSpan->GetPrevPier() );
      }
   }

   PierIndexType nPiers = GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      CPierData* pPrevPier = GetPier(pierIdx-1);
      CPierData* pThisPier = GetPier(pierIdx);
      CPierData* pNextPier = GetPier(pierIdx+1);

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
#endif
}
