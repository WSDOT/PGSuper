///////////////////////////////////////////////////////////////////////
// IEPluginExample
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

// PGSuperProjectImporter.cpp : Implementation of CPGSuperProjectImporter
#include "stdafx.h"
#include "IEPluginExample.h"
#include "PGSuperProjectImporter.h"

#include "PGSuperInterfaces.h"

#include <EAF\EAFAutoProgress.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\Helpers.h>

#include <psgLib/CreepCriteria.h>


HRESULT CPGSuperProjectImporter::FinalConstruct()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   VERIFY(m_Bitmap.LoadBitmap(IDB_IEPLUGIN));
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CPGSuperProjectImporter
STDMETHODIMP CPGSuperProjectImporter::GetItemText(BSTR*  bstrText) const
{
   CComBSTR bstrItemText("Example Importer");
   *bstrText = bstrItemText.Copy();
   return S_OK;
}

STDMETHODIMP CPGSuperProjectImporter::GetCLSID(CLSID* pCLSID) const
{
   *pCLSID = CLSID_PGSuperProjectImporter;
   return S_OK;
}

STDMETHODIMP CPGSuperProjectImporter::GetIcon(HICON* phIcon) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   *phIcon = AfxGetApp()->LoadIcon(IDI_IMPORTER);
   return S_OK;
}

STDMETHODIMP CPGSuperProjectImporter::Import(IBroker* pBroker)
{
   AfxMessageBox(_T("This project importer simulates importing data from an external source by creating a default bridge. A real project importer would connect to an external data source and programatically create a PGSuper model."),MB_OK);

   GET_IFACE2(pBroker,IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   pDisplayUnits->SetUnitMode(eafTypes::umUS);

   pProgress->UpdateMessage(_T("Building Bridge"));
   BuildBridge(pBroker);

   pProgress->UpdateMessage(_T("Initializing Girders"));
   InitGirderData(pBroker);

   pProgress->UpdateMessage(_T("Setting Project Criteria"));
   SetSpecification(pBroker);

   return S_OK;
}

void CPGSuperProjectImporter::BuildBridge(IBroker* pBroker)
{
   GET_IFACE2( pBroker, ILibraryNames, pLibNames );

   //
   // Build the bridge model
   // It will be a hard coded 100ft long bridge, with 5 girders at 6' spacing
   // Girder is the first girder of the first registered type
   //

   // We will build our bridge using the first girder defined for the first girder family that we find
   // 
   // get the names of the available girder families
   std::vector<std::_tstring> girderFamilyNames;
   pLibNames->EnumGirderFamilyNames(&girderFamilyNames);
   std::_tstring strGirderFamily, strGirderName;
   for (const auto& familyName : girderFamilyNames)
   {
      // find the first family that has girders defined
      std::vector<std::_tstring> names;
      pLibNames->EnumGirderNames(familyName.c_str(), &names);
      if (0 < names.size())
      {
         strGirderFamily = familyName;
         strGirderName = names.front();
         break;
      }
   }

   CBridgeDescription2 bridge;
   bridge.SetGirderFamilyName(strGirderFamily.c_str());

   // get the beam factory so we can get important information about the beam
   GET_IFACE2(pBroker,ILibrary,pLibrary);
   const GirderLibraryEntry* pGirderEntry = pLibrary->GetGirderEntry(strGirderName.c_str());

   CComPtr<IBeamFactory> beamFactory;
   pGirderEntry->GetBeamFactory(&beamFactory);

   // use the same girder for the entire bridge
   bridge.UseSameGirderForEntireBridge(true);
   bridge.SetGirderName(strGirderName.c_str()); // set the girder name
   bridge.SetGirderLibraryEntry(pGirderEntry); // associate it's definition

   // use 5 girders per span... and all spans have the same number of girders
   GirderIndexType num_girders = 5;
   bridge.UseSameNumberOfGirdersInAllGroups(true);
   bridge.SetGirderCount(num_girders);

   // use the first supported beam spacing type
   pgsTypes::SupportedBeamSpacings spacingTypes = beamFactory->GetSupportedBeamSpacings();
   pgsTypes::SupportedBeamSpacing spacingType = spacingTypes.front();

   pgsTypes::SupportedDeckTypes deckTypes = beamFactory->GetSupportedDeckTypes(spacingType);
   pgsTypes::SupportedDeckType deckType = deckTypes.front();

   Float64 minSpacing, maxSpacing;
   beamFactory->GetAllowableSpacingRange(pGirderEntry->GetDimensions(),deckType,spacingType,&minSpacing,&maxSpacing);
   Float64 girder_spacing = minSpacing;


   bridge.SetGirderSpacingType(spacingType);
   bridge.SetMeasurementType(pgsTypes::NormalToItem);
   bridge.SetMeasurementLocation(pgsTypes::AtPierLine);

   Float64 bridge_width = girder_spacing * num_girders; // overhang is half girder spacing

   if (IsAdjacentSpacing(spacingType))
   {
      ATLASSERT(IsJointSpacing(spacingType));

      bridge.SetGirderSpacing(girder_spacing); // girder spacing is also joint spacing for adjacent beams

      std::vector<pgsTypes::TopWidthType> topWidthTypes = beamFactory->GetSupportedTopWidthTypes();
      pgsTypes::TopWidthType topWidthType = topWidthTypes.front();

      Float64 left1, right1, left2, right2;
      beamFactory->GetAllowableTopWidthRange(topWidthType, pGirderEntry->GetDimensions(), &left1, &left2, &right1, &right2);

      if (IsTopWidthSpacing(spacingType))
      {
         // girder spacing is the top width
         bridge.SetGirderTopWidth(topWidthType, left1, right1);
      }

      bridge_width = num_girders * (left1 + right1) + (num_girders - 1) * girder_spacing; // girder spacing is joint width in this case
   }
   else
   {
      bridge.SetGirderSpacing(girder_spacing);
   }

   Float64 span_length = WBFL::Units::ConvertToSysUnits(100.0, WBFL::Units::Measure::Feet); // set span length to 100ft


   bridge.CreateFirstSpan(nullptr,nullptr,nullptr,INVALID_INDEX); // creates 2 piers and a span
   bridge.SetSpanLength(0,span_length); 

   bridge.SetSlabOffset(WBFL::Units::ConvertToSysUnits(12.0,WBFL::Units::Measure::Inch));
   bridge.SetFillet(WBFL::Units::ConvertToSysUnits(0.75,WBFL::Units::Measure::Inch));

   //
   // define the bridge deck
   //

   CDeckPoint point;
   if (deckType==pgsTypes::sdtCompositeCIP || deckType==pgsTypes::sdtCompositeSIP)
   {
      point.LeftEdge  = bridge_width/2.0;
      point.RightEdge = bridge_width/2.0;
      point.Station = 0.0;
      point.MeasurementType = pgsTypes::omtBridge;
   }

   CDeckDescription2 deck;
   deck.SetDeckType(deckType);

   switch( deckType )
   {
   case pgsTypes::sdtCompositeCIP:
      deck.GrossDepth        = WBFL::Units::ConvertToSysUnits(8.0,WBFL::Units::Measure::Inch);
      deck.OverhangTaper[pgsTypes::stLeft] = pgsTypes::dotTopTopFlange;
      deck.OverhangTaper[pgsTypes::stRight] = pgsTypes::dotTopTopFlange;
      deck.OverhangEdgeDepth[pgsTypes::stLeft] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::Inch);
      deck.OverhangEdgeDepth[pgsTypes::stRight] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::Inch);
      deck.DeckEdgePoints.push_back(point);
      break;

   case pgsTypes::sdtCompositeSIP:
      deck.GrossDepth        = WBFL::Units::ConvertToSysUnits(4.0,WBFL::Units::Measure::Inch);
      deck.PanelDepth        = WBFL::Units::ConvertToSysUnits(4.0,WBFL::Units::Measure::Inch);
      deck.PanelSupport      = WBFL::Units::ConvertToSysUnits(2.0,WBFL::Units::Measure::Inch);
      deck.OverhangTaper[pgsTypes::stLeft] = pgsTypes::dotTopTopFlange;
      deck.OverhangTaper[pgsTypes::stRight] = pgsTypes::dotTopTopFlange;
      deck.OverhangEdgeDepth[pgsTypes::stLeft] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::Inch);
      deck.OverhangEdgeDepth[pgsTypes::stRight] = WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::Inch);
      deck.DeckEdgePoints.push_back(point);
      break;

   case pgsTypes::sdtCompositeOverlay:
   case pgsTypes::sdtNonstructuralOverlay:
      deck.GrossDepth        = WBFL::Units::ConvertToSysUnits(3.0,WBFL::Units::Measure::Inch);
      break;

   case pgsTypes::sdtNone:
      break;

   default:
      ATLASSERT(false); // shoudl never get here
   }

   deck.WearingSurface = pgsTypes::wstFutureOverlay;
   if ( spacingType == pgsTypes::sbsUniformAdjacent )
      deck.TransverseConnectivity = pgsTypes::atcConnectedAsUnit;

   deck.Concrete.Fc               = WBFL::Units::ConvertToSysUnits(5.0,WBFL::Units::Measure::KSI);
   deck.Concrete.WeightDensity    = WBFL::Units::ConvertToSysUnits(160.0,WBFL::Units::Measure::LbfPerFeet3); 
   deck.Concrete.StrengthDensity  = WBFL::Units::ConvertToSysUnits(160.0,WBFL::Units::Measure::LbfPerFeet3); 
   deck.Concrete.MaxAggregateSize = WBFL::Units::ConvertToSysUnits(0.5,WBFL::Units::Measure::Inch); 

   deck.Concrete.EcK1 = 1.0;
   deck.Concrete.EcK2 = 1.0;
   deck.Concrete.CreepK1 = 1.0;
   deck.Concrete.CreepK2 = 1.0;
   deck.Concrete.ShrinkageK1 = 1.0;
   deck.Concrete.ShrinkageK2 = 1.0;

   deck.Concrete.bUserEc = false;
   deck.Concrete.Ec = WBFL::LRFD::ConcreteUtil::ModE((WBFL::Materials::ConcreteType)deck.Concrete.Type,deck.Concrete.Fc,deck.Concrete.StrengthDensity,false);
   deck.bInputAsDepthAndDensity = false;
   deck.OverlayWeight = WBFL::Units::ConvertToSysUnits(0.025,WBFL::Units::Measure::KSF);

   *bridge.GetDeckDescription() = deck;


   //
   // define the traffic barriers
   //
   std::vector<std::_tstring> barrierNames;
   pLibNames->EnumTrafficBarrierNames(&barrierNames);
   std::_tstring strBarrierName = barrierNames[0];

   bridge.GetLeftRailingSystem()->strExteriorRailing = strBarrierName;
   bridge.GetRightRailingSystem()->strExteriorRailing = strBarrierName;

   InitTimelineManager(pBroker,bridge);

   /// Assign the bridge model to PGSuper
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   pIBridgeDesc->SetBridgeDescription(bridge);
}

void CPGSuperProjectImporter::SetSpecification(IBroker* pBroker)
{
   GET_IFACE2( pBroker, ILibraryNames, pLibNames );

   std::vector<std::_tstring> specs;
   pLibNames->EnumSpecNames( &specs );

   GET_IFACE2( pBroker, ISpecification, pSpec );
   pSpec->SetSpecification( specs[0] );
}

void CPGSuperProjectImporter::InitGirderData(IBroker* pBroker)
{
   const auto* pPool = WBFL::LRFD::StrandPool::GetInstance();
   const auto* pStrand = pPool->GetStrand(
      WBFL::Materials::PsStrand::Grade::Gr1860,
      WBFL::Materials::PsStrand::Type::LowRelaxation,
      WBFL::Materials::PsStrand::Coating::None,
      WBFL::Materials::PsStrand::Size::D1524);

   GET_IFACE2( pBroker, IBridgeDescription,pIBridgeDesc);
   GET_IFACE2( pBroker, ISegmentData, pSegmentData);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);
            pSegmentData->SetStrandMaterial(segmentKey,pgsTypes::Straight, pStrand);
            pSegmentData->SetStrandMaterial(segmentKey,pgsTypes::Harped,   pStrand);
            pSegmentData->SetStrandMaterial(segmentKey,pgsTypes::Temporary,pStrand);
         }
      }
   }
}

void CPGSuperProjectImporter::InitTimelineManager(IBroker* pBroker, CBridgeDescription2& bridge)
{
   // NOTE: The actual timing doesn't matter since we aren't doing a true time-step analysis
   // We will just use reasonable times so the sequence is correct
   CTimelineManager timelineMgr;

   GET_IFACE2(pBroker, ILibrary, pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   // get a list of all the segment IDs. it is needed in a couple locations below
   std::set<SegmentIDType> segmentIDs;
   GroupIndexType nGroups = bridge.GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      const CGirderGroupData* pGroup = bridge.GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            SegmentIDType segmentID = pSegment->GetID();
            std::pair<std::set<SegmentIDType>::iterator, bool> result = segmentIDs.insert(segmentID);
            ATLASSERT(result.second == true);
         }
      }
   }

   // Casting yard stage... starts at day 0 when strands are stressed
   // The activities in this stage includes prestress release, lifting and storage
   const auto& creep_criteria = pSpecEntry->GetCreepCriteria();
   std::unique_ptr<CTimelineEvent> pTimelineEvent = std::make_unique<CTimelineEvent>();
   pTimelineEvent->SetDay(0);
   pTimelineEvent->SetDescription(_T("Construct Girders, Erect Piers"));
   pTimelineEvent->GetConstructSegmentsActivity().Enable();
   pTimelineEvent->GetConstructSegmentsActivity().SetTotalCuringDuration(WBFL::Units::ConvertFromSysUnits(creep_criteria.XferTime, WBFL::Units::Measure::Day));
   pTimelineEvent->GetConstructSegmentsActivity().SetRelaxationTime(WBFL::Units::ConvertFromSysUnits(creep_criteria.XferTime, WBFL::Units::Measure::Day));
   pTimelineEvent->GetConstructSegmentsActivity().AddSegments(segmentIDs);

   // assume piers are erected at the same time girders are being constructed
   pTimelineEvent->GetErectPiersActivity().Enable();
   PierIndexType nPiers = bridge.GetPierCount();
   for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
   {
      const CPierData2* pPier = bridge.GetPier(pierIdx);
      PierIDType pierID = pPier->GetID();
      pTimelineEvent->GetErectPiersActivity().AddPier(pierID);
   }

   EventIndexType eventIdx;
   timelineMgr.AddTimelineEvent(pTimelineEvent.release(), true, &eventIdx);

   // Erect girders. It is assumed that girders are transported, erected, and temporary strands 
   // are removed all on the same day. Assuming max construction sequence (D120). The actual
   // don't matter unless the user switches to time-step analysis.
   pTimelineEvent = std::make_unique<CTimelineEvent>();
   Float64 day = WBFL::Units::ConvertFromSysUnits(creep_criteria.XferTime + creep_criteria.CreepDuration1Max, WBFL::Units::Measure::Day);
   Float64 maxDay = 28.0;
   day = Max(day, maxDay);
   maxDay += 1.0;
   pTimelineEvent->SetDay(day);
   pTimelineEvent->SetDescription(_T("Erect Girders"));
   pTimelineEvent->GetErectSegmentsActivity().Enable();
   pTimelineEvent->GetErectSegmentsActivity().AddSegments(segmentIDs);
   timelineMgr.AddTimelineEvent(pTimelineEvent.get(), true, &eventIdx);
   maxDay = pTimelineEvent->GetDay();
   pTimelineEvent.release();

   pgsTypes::SupportedDeckType deckType = bridge.GetDeckDescription()->GetDeckType();

   Float64 deck_diaphragm_curing_duration = 0; // we assume composite deck locks in creep deflections. Girder creep occurs over the curing duration. Set the duration to 0 day to avoid creep deflection
   if (IsNonstructuralDeck(deckType))
   {
      // deck is non-composite or there is no deck so creep can continue
      deck_diaphragm_curing_duration = Min(WBFL::Units::ConvertFromSysUnits(creep_criteria.TotalCreepDuration - creep_criteria.CreepDuration2Max, WBFL::Units::Measure::Day), 28.0);
   }

   if (IsJointSpacing(bridge.GetGirderSpacingType()) && bridge.HasStructuralLongitudinalJoints())
   {
      // No deck
      // 1) Diaphragms
      // 2) Joints

      // deck or overlay
      // 1) Diaphragms
      // 2) Joints
      // 3) Deck
      pTimelineEvent = std::make_unique<CTimelineEvent>();
      day = WBFL::Units::ConvertFromSysUnits(creep_criteria.XferTime + creep_criteria.CreepDuration2Max, WBFL::Units::Measure::Day);
      day = Max(day, maxDay);
      pTimelineEvent->SetDay(day);
      pTimelineEvent->SetDescription(_T("Cast Diaphragms"));
      pTimelineEvent->GetApplyLoadActivity().ApplyIntermediateDiaphragmLoad();
      timelineMgr.AddTimelineEvent(pTimelineEvent.get(), true, &eventIdx);
      maxDay = pTimelineEvent->GetDay() + 1;
      pTimelineEvent.release();

      pTimelineEvent = std::make_unique<CTimelineEvent>();
      day = WBFL::Units::ConvertFromSysUnits(creep_criteria.XferTime + creep_criteria.CreepDuration2Max, WBFL::Units::Measure::Day) + 1.0;
      day = Max(day, maxDay);
      pTimelineEvent->SetDay(day);
      pTimelineEvent->SetDescription(_T("Cast Longitudinal Joints"));

      pTimelineEvent->GetCastLongitudinalJointActivity().Enable();
      pTimelineEvent->GetCastLongitudinalJointActivity().SetTotalCuringDuration(1.0); // day
      pTimelineEvent->GetCastLongitudinalJointActivity().SetActiveCuringDuration(1.0); // day
      timelineMgr.AddTimelineEvent(pTimelineEvent.get(), true, &eventIdx);
      maxDay = pTimelineEvent->GetDay() + 1;
      pTimelineEvent.release();

      if (deckType != pgsTypes::sdtNone)
      {
         pTimelineEvent = std::make_unique<CTimelineEvent>();
         day = WBFL::Units::ConvertFromSysUnits(creep_criteria.XferTime + creep_criteria.CreepDuration2Max, WBFL::Units::Measure::Day) + 2.0;
         day = Max(day, maxDay);
         pTimelineEvent->SetDay(day);


         pTimelineEvent->SetDescription(GetCastDeckEventName(deckType));
         pTimelineEvent->GetCastDeckActivity().Enable();
         pTimelineEvent->GetCastDeckActivity().SetCastingType(CCastDeckActivity::Continuous); // this is the only option supported for PGSuper models
         pTimelineEvent->GetCastDeckActivity().SetTotalCuringDuration(deck_diaphragm_curing_duration); // day
         pTimelineEvent->GetCastDeckActivity().SetActiveCuringDuration(deck_diaphragm_curing_duration); // day
         timelineMgr.AddTimelineEvent(pTimelineEvent.get(), true, &eventIdx);
         maxDay = pTimelineEvent->GetDay() + 1;
         pTimelineEvent.release();
      }
   }
   else
   {
      // Cast deck & diaphragms
      pTimelineEvent = std::make_unique<CTimelineEvent>();
      day = WBFL::Units::ConvertFromSysUnits(creep_criteria.XferTime + creep_criteria.CreepDuration2Max, WBFL::Units::Measure::Day);
      day = Max(day, maxDay);
      pTimelineEvent->SetDay(day);
      pTimelineEvent->SetDescription(_T("Cast Diaphragms"));

      pTimelineEvent->GetApplyLoadActivity().ApplyIntermediateDiaphragmLoad();
      timelineMgr.AddTimelineEvent(pTimelineEvent.get(), true, &eventIdx);
      maxDay = pTimelineEvent->GetDay() + 1;
      pTimelineEvent.release();

      if (deckType != pgsTypes::sdtNone)
      {
         pTimelineEvent = std::make_unique<CTimelineEvent>();
         day = WBFL::Units::ConvertFromSysUnits(creep_criteria.XferTime + creep_criteria.CreepDuration2Max, WBFL::Units::Measure::Day);
         day = Max(day, maxDay);
         pTimelineEvent->SetDay(day);
         pTimelineEvent->SetDescription(GetCastDeckEventName(deckType));
         pTimelineEvent->GetCastDeckActivity().Enable();
         pTimelineEvent->GetCastDeckActivity().SetCastingType(CCastDeckActivity::Continuous); // this is the only option supported for PGSuper models
         pTimelineEvent->GetCastDeckActivity().SetTotalCuringDuration(deck_diaphragm_curing_duration); // day
         pTimelineEvent->GetCastDeckActivity().SetActiveCuringDuration(deck_diaphragm_curing_duration); // day
         timelineMgr.AddTimelineEvent(pTimelineEvent.get(), true, &eventIdx);
         maxDay = pTimelineEvent->GetDay() + 1;
         pTimelineEvent.release();
      }
   }

   // traffic barrier/superimposed dead loads
   pTimelineEvent = std::make_unique<CTimelineEvent>();
   day = WBFL::Units::ConvertFromSysUnits(creep_criteria.XferTime + creep_criteria.CreepDuration2Max, WBFL::Units::Measure::Day) + deck_diaphragm_curing_duration;
   day = Max(day, maxDay);
   pTimelineEvent->SetDay(day); // deck is continuous
   pTimelineEvent->GetApplyLoadActivity().ApplyRailingSystemLoad();

   pgsTypes::WearingSurfaceType wearingSurface = bridge.GetDeckDescription()->WearingSurface;
   if (wearingSurface == pgsTypes::wstSacrificialDepth || wearingSurface == pgsTypes::wstOverlay)
   {
      pTimelineEvent->SetDescription(_T("Final without Live Load"));
      if (wearingSurface == pgsTypes::wstOverlay)
      {
         pTimelineEvent->GetApplyLoadActivity().ApplyOverlayLoad();
      }
   }
   else
   {
      pTimelineEvent->SetDescription(_T("Install Railing System"));
   }
   timelineMgr.AddTimelineEvent(pTimelineEvent.get(), true, &eventIdx);
   maxDay = pTimelineEvent->GetDay() + 1;
   pTimelineEvent.release();

   if (wearingSurface == pgsTypes::wstFutureOverlay)
   {
      pTimelineEvent = std::make_unique<CTimelineEvent>();
      day = WBFL::Units::ConvertFromSysUnits(creep_criteria.XferTime + creep_criteria.CreepDuration2Max, WBFL::Units::Measure::Day) + deck_diaphragm_curing_duration + 1.0;
      day = Max(day, maxDay);
      pTimelineEvent->SetDay(day);
      pTimelineEvent->SetDescription(_T("Final without Live Load"));
      pTimelineEvent->GetApplyLoadActivity().ApplyOverlayLoad();
      timelineMgr.AddTimelineEvent(pTimelineEvent.get(), true, &eventIdx);
      maxDay = pTimelineEvent->GetDay() + 1;
      pTimelineEvent.release();
   }

   // live load
   pTimelineEvent = std::make_unique<CTimelineEvent>();
   day = WBFL::Units::ConvertFromSysUnits(creep_criteria.XferTime + creep_criteria.CreepDuration2Max, WBFL::Units::Measure::Day) + deck_diaphragm_curing_duration + 1.0;
   day = Max(day, maxDay);
   pTimelineEvent->SetDay(day);
   pTimelineEvent->SetDescription(_T("Final with Live Load"));
   pTimelineEvent->GetApplyLoadActivity().ApplyLiveLoad();
   pTimelineEvent->GetApplyLoadActivity().ApplyRatingLiveLoad();
   timelineMgr.AddTimelineEvent(pTimelineEvent.get(), true, &eventIdx);
   maxDay = pTimelineEvent->GetDay() + 1;
   pTimelineEvent.release();

   bridge.SetTimelineManager(&timelineMgr);
}