///////////////////////////////////////////////////////////////////////
// IEPluginExample
// Copyright © 1999-2018  Washington State Department of Transportation
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

STDMETHODIMP CPGSuperProjectImporter::GetIcon(HICON* phIcon) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   *phIcon = AfxGetApp()->LoadIcon(IDI_IMPORTER);
   return S_OK;
}

STDMETHODIMP CPGSuperProjectImporter::Import(IBroker* pBroker)
{
//   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   int st = AfxMessageBox(_T("Simulate importing data from an external source by creating a default bridge. Click Yes to do it!"),MB_YESNO);

   if (st==IDYES)
   {
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
   else
   {
      return E_FAIL;
   }
}

void CPGSuperProjectImporter::BuildBridge(IBroker* pBroker)
{
   GET_IFACE2( pBroker, ILibraryNames, pLibNames );

   //
   // Build the bridge model
   // It will be a hard coded 100ft long bridge, with 5 girders at 6' spacing
   // Girder is the first girder of the first registered type
   //

   // get the names of the available girder families
   std::vector<std::_tstring> girderFamilyNames;
   pLibNames->EnumGirderFamilyNames(&girderFamilyNames);
   std::vector<std::_tstring>::iterator familyIter;
   std::_tstring strGirderFamily;
   std::_tstring strGirderName ;
   for ( familyIter = girderFamilyNames.begin(); familyIter != girderFamilyNames.end(); familyIter++ )
   {
      strGirderFamily = *familyIter;

      // get the the name of one of the girders in this family
      std::vector<std::_tstring> names;
      pLibNames->EnumGirderNames(strGirderFamily.c_str(), &names );

      std::vector<std::_tstring>::iterator nameIter;
      for ( nameIter = names.begin(); nameIter != names.end(); nameIter++ )
      {
         strGirderName = *nameIter;

         // we've got the first girder in the first family for whatever library is loaded in PGSuper
         // force the loops to complete
         nameIter = names.end()-1;
         familyIter = girderFamilyNames.end()-1;
      }
   }

   CBridgeDescription2 bridge;
   bridge.SetGirderFamilyName(strGirderFamily.c_str());

   GirderIndexType num_girders = 5;
   Float64 girder_spacing = ::ConvertToSysUnits(6.0,unitMeasure::Feet);
   Float64 span_length = ::ConvertToSysUnits(100.0,unitMeasure::Feet); // set span length to 100ft

   Float64 bridge_width = girder_spacing * num_girders; // overhang is half girder spacing

   bridge.UseSameGirderForEntireBridge(true);
   bridge.SetGirderName(strGirderName.c_str());

   bridge.UseSameNumberOfGirdersInAllGroups(true);
   bridge.SetGirderCount(num_girders);


   GET_IFACE2(pBroker,ILibrary,pLibrary);
   const GirderLibraryEntry* pGirderEntry = pLibrary->GetGirderEntry(strGirderName.c_str());
   CComPtr<IBeamFactory> beamFactory;
   pGirderEntry->GetBeamFactory(&beamFactory);

   pgsTypes::SupportedBeamSpacings spacingTypes = beamFactory->GetSupportedBeamSpacings();

   pgsTypes::SupportedBeamSpacing spacingType = spacingTypes.front();

   bridge.SetGirderSpacingType(spacingType);
   bridge.SetGirderSpacing( girder_spacing  );
   bridge.SetMeasurementType( pgsTypes::NormalToItem );
   bridge.SetMeasurementLocation( pgsTypes::AtPierLine );

   bridge.CreateFirstSpan(nullptr,nullptr,nullptr,0); // creates 2 piers and a span
   bridge.SetSpanLength(0,span_length); 

   bridge.SetSlabOffset(::ConvertToSysUnits(12.0,unitMeasure::Inch));
   bridge.SetFillet(::ConvertToSysUnits(0.75,unitMeasure::Inch));

   //
   // define the bridge deck
   //

   GET_IFACE2(pBroker,ILibrary,pLib);
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName.c_str());
   CComPtr<IBeamFactory> factory;
   pGdrEntry->GetBeamFactory(&factory);
   pgsTypes::SupportedBeamSpacings beamSpacings = factory->GetSupportedBeamSpacings();
   pgsTypes::SupportedBeamSpacing beamSpacing = beamSpacings[0];
   pgsTypes::SupportedDeckTypes deckTypes = factory->GetSupportedDeckTypes(beamSpacing);
   pgsTypes::SupportedDeckType deckType = deckTypes[0];

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
      deck.GrossDepth        = ::ConvertToSysUnits(8.0,unitMeasure::Inch);
      deck.OverhangTaper[pgsTypes::stLeft] = pgsTypes::dotTopTopFlange;
      deck.OverhangTaper[pgsTypes::stRight] = pgsTypes::dotTopTopFlange;
      deck.OverhangEdgeDepth[pgsTypes::stLeft] = ::ConvertToSysUnits(6.0, unitMeasure::Inch);
      deck.OverhangEdgeDepth[pgsTypes::stRight] = ::ConvertToSysUnits(6.0, unitMeasure::Inch);
      deck.DeckEdgePoints.push_back(point);
      break;

   case pgsTypes::sdtCompositeSIP:
      deck.GrossDepth        = ::ConvertToSysUnits(4.0,unitMeasure::Inch);
      deck.PanelDepth        = ::ConvertToSysUnits(4.0,unitMeasure::Inch);
      deck.PanelSupport      = ::ConvertToSysUnits(2.0,unitMeasure::Inch);
      deck.OverhangTaper[pgsTypes::stLeft] = pgsTypes::dotTopTopFlange;
      deck.OverhangTaper[pgsTypes::stRight] = pgsTypes::dotTopTopFlange;
      deck.OverhangEdgeDepth[pgsTypes::stLeft] = ::ConvertToSysUnits(6.0, unitMeasure::Inch);
      deck.OverhangEdgeDepth[pgsTypes::stRight] = ::ConvertToSysUnits(6.0, unitMeasure::Inch);
      deck.DeckEdgePoints.push_back(point);
      break;

   case pgsTypes::sdtCompositeOverlay:
   case pgsTypes::sdtNonstructuralOverlay:
      deck.GrossDepth        = ::ConvertToSysUnits(3.0,unitMeasure::Inch);
      break;

   case pgsTypes::sdtNone:
      break;

   default:
      ATLASSERT(false); // shoudl never get here
   }

   deck.WearingSurface = pgsTypes::wstFutureOverlay;
   if ( beamSpacing == pgsTypes::sbsUniformAdjacent )
      deck.TransverseConnectivity = pgsTypes::atcConnectedAsUnit;

   deck.Concrete.Fc               = ::ConvertToSysUnits(5.0,unitMeasure::KSI);
   deck.Concrete.WeightDensity    = ::ConvertToSysUnits(160.0,unitMeasure::LbfPerFeet3); 
   deck.Concrete.StrengthDensity  = ::ConvertToSysUnits(160.0,unitMeasure::LbfPerFeet3); 
   deck.Concrete.MaxAggregateSize = ::ConvertToSysUnits(0.5,unitMeasure::Inch); 

   deck.Concrete.EcK1 = 1.0;
   deck.Concrete.EcK2 = 1.0;
   deck.Concrete.CreepK1 = 1.0;
   deck.Concrete.CreepK2 = 1.0;
   deck.Concrete.ShrinkageK1 = 1.0;
   deck.Concrete.ShrinkageK2 = 1.0;

   deck.Concrete.bUserEc = false;
   deck.Concrete.Ec = lrfdConcreteUtil::ModE(deck.Concrete.Fc,deck.Concrete.StrengthDensity,false);
   deck.bInputAsDepthAndDensity = false;
   deck.OverlayWeight = ::ConvertToSysUnits(0.025,unitMeasure::KSF);

   *bridge.GetDeckDescription() = deck;


   //
   // define the traffic barriers
   //
   std::vector<std::_tstring> barrierNames;
   pLibNames->EnumTrafficBarrierNames(&barrierNames);
   std::_tstring strBarrierName = barrierNames[0];

   bridge.GetLeftRailingSystem()->strExteriorRailing = strBarrierName;
   bridge.GetRightRailingSystem()->strExteriorRailing = strBarrierName;


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
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   const matPsStrand* pStrand = pPool->GetStrand(matPsStrand::Gr1860,matPsStrand::LowRelaxation,matPsStrand::None,matPsStrand::D1524);

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
