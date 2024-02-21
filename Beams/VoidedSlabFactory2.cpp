///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// VoidedSlab2Factory.cpp : Implementation of CVoidedSlab2Factory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "VoidedSlabFactory2.h"
#include "IBeamDistFactorEngineer.h"
#include "VoidedSlab2DistFactorEngineer.h"
#include "UBeamDistFactorEngineer.h"
#include "PsBeamLossEngineer.h"
#include "TimeStepLossEngineer.h"
#include "StrandMoverImpl.h"
#include <GeomModel\PrecastBeam.h>
#include <MathEx.h>
#include <sstream>
#include <algorithm>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\StatusCenter.h>
#include <IFace\Intervals.h>

#include <IFace\AgeAdjustedMaterial.h>
#include <Beams\Helper.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\StatusItem.h>

#include <psgLib/SectionPropertiesCriteria.h>
#include <psgLib/SpecificationCriteria.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVoidedSlab2Factory
HRESULT CVoidedSlab2Factory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.emplace_back(_T("H"));
   m_DimNames.emplace_back(_T("W"));
   m_DimNames.emplace_back(_T("Number_of_Voids"));
   m_DimNames.emplace_back(_T("D1"));
   m_DimNames.emplace_back(_T("D2"));
   m_DimNames.emplace_back(_T("H1"));
   m_DimNames.emplace_back(_T("H2"));
   m_DimNames.emplace_back(_T("S1"));
   m_DimNames.emplace_back(_T("S2"));
   m_DimNames.emplace_back(_T("C1"));
   m_DimNames.emplace_back(_T("C2"));
   m_DimNames.emplace_back(_T("C3"));
   m_DimNames.emplace_back(_T("Jmax"));
   m_DimNames.emplace_back(_T("EndBlockLength"));

   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(18.0,WBFL::Units::Measure::Inch)); // H
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(48.0,WBFL::Units::Measure::Inch)); // W
   m_DefaultDims.emplace_back(3);                                           // Number of Voids
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(10.0,WBFL::Units::Measure::Inch)); // D1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(10.0,WBFL::Units::Measure::Inch)); // D2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(12.0,WBFL::Units::Measure::Inch)); // H1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(12.0,WBFL::Units::Measure::Inch)); // H2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(12.5,WBFL::Units::Measure::Inch)); // S1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(12.5,WBFL::Units::Measure::Inch)); // S2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 3.0,WBFL::Units::Measure::Inch)); // C1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 3.0,WBFL::Units::Measure::Inch)); // C2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 1.0,WBFL::Units::Measure::Inch)); // C3
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(1.0,WBFL::Units::Measure::Inch));  // Max Joint Spacing
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(36.0,WBFL::Units::Measure::Inch)); // End Block Length

   // SI Units
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // H 
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W
   m_DimUnits[0].emplace_back((const WBFL::Units::Length*)BFDIMUNITSCALAR);// Number of Voids
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // H1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // H2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // S1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // S2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // C1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // C2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // C3
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // Max joint size
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // End Block Length

   // US Units
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // H 
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W
   m_DimUnits[1].emplace_back((const WBFL::Units::Length*)BFDIMUNITSCALAR);// Number of Voids
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // H1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // H2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // S1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // S2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // C1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // C2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // C3
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // Max joint size
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // End Block Length

   return S_OK;
}

void CVoidedSlab2Factory::CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const
{
   CComPtr<IVoidedSlabSection2> gdrSection;
   gdrSection.CoCreateInstance(CLSID_VoidedSlabSection2);
   CComPtr<IVoidedSlab2> beam;
   gdrSection->get_Beam(&beam);

   DimensionAndPositionBeam(dimensions, beam);

   gdrSection.QueryInterface(ppSection);
}

void CVoidedSlab2Factory::CreateSegment(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** ppSegment) const
{
   CComPtr<IVoidedSlabEndBlockSegment> segment;
   segment.CoCreateInstance(CLSID_VoidedSlabEndBlockSegment);

   // Build up the beam shape
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder     = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment    = pGirder->GetSegment(segmentKey.segmentIndex);

   const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();

   CComPtr<IGirderSection> gdrSection;
   CreateGirderSection(pBroker,statusGroupID,dimensions,-1,-1,&gdrSection);
   CComQIPtr<IVoidedSlabSection2> section(gdrSection);

   // if this is an exterior girder, remove the shear key block outs
   CComPtr<IVoidedSlab2> voidedSlabShape;
   section->get_Beam(&voidedSlabShape);
   if ( segmentKey.girderIndex == 0 )
   {
      voidedSlabShape->put_LeftBlockOut(VARIANT_FALSE);
   }

   if ( segmentKey.girderIndex == pGroup->GetGirderCount()-1 )
   {
      voidedSlabShape->put_RightBlockOut(VARIANT_FALSE);
   }

   IndexType nVoids;
   voidedSlabShape->get_VoidCount(&nVoids);
   if (0 < nVoids)
   {
      // only model end blocks if there are voids
      Float64 endBlockLength = GetDimension(dimensions, _T("EndBlockLength"));
      segment->put_EndBlockLength(etStart, endBlockLength);
      segment->put_EndBlockLength(etEnd, endBlockLength);
   }

   // Beam materials
   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   CComPtr<IMaterial> material;
   if ( pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      CComPtr<IAgeAdjustedMaterial> aaMaterial;
      BuildAgeAdjustedGirderMaterialModel(pBroker,pSegment,segment,&aaMaterial);
      aaMaterial.QueryInterface(&material);
   }
   else
   {
      GET_IFACE2(pBroker,IIntervals,pIntervals);
      GET_IFACE2(pBroker,IMaterials,pMaterial);
      material.CoCreateInstance(CLSID_Material);

      IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
      for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
      {
         Float64 E = pMaterial->GetSegmentEc(segmentKey,intervalIdx);
         Float64 D = pMaterial->GetSegmentWeightDensity(segmentKey,intervalIdx);

         material->put_E(intervalIdx,E);
         material->put_Density(intervalIdx,D);
      }
   }

   CComQIPtr<IShape> shape(section);
   ATLASSERT(shape);
   segment->AddShape(shape,material,nullptr);

   CComQIPtr<ISuperstructureMemberSegment> ssmbrSegment(segment);
   ssmbrSegment.CopyTo(ppSegment);
}

void CVoidedSlab2Factory::ConfigureSegment(IBroker* pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const
{
   // do nothing... all the configuration was done in CreateSegment
}

void CVoidedSlab2Factory::CreateSegmentShape(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const
{
   CComPtr<IVoidedSlab2> beam;
   beam.CoCreateInstance(CLSID_VoidedSlab2);

   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();

   DimensionAndPositionBeam(dimensions, beam);

   GET_IFACE2(pBroker, IBridge, pBridge);
   Float64 Lg = pBridge->GetSegmentLength(pSegment->GetSegmentKey());
   Float64 endBlockLength = GetDimension(dimensions, _T("EndBlockLength"));
   if (IsInEndBlock(Xs, sectionBias, endBlockLength, Lg))
   {
      beam->put_VoidCount(0);
   }

   beam.QueryInterface(ppShape);
}

Float64 CVoidedSlab2Factory::GetSegmentHeight(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();
   Float64 H = GetDimension(dimensions, _T("H"));
   return H;
}

void CVoidedSlab2Factory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,   POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE  );

   VERIFY(pPoiMgr->AddPointOfInterest(poiStart) != INVALID_ID);
   VERIFY(pPoiMgr->AddPointOfInterest(poiEnd) != INVALID_ID);

   // put section breaks just on either side of the end blocks/void interface
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();

   IndexType nVoids = (IndexType)pGdrEntry->GetDimension(_T("Number_of_Voids"));

   Float64 endBlockLength = pGdrEntry->GetDimension(_T("EndBlockLength"));

   if ( 0 < nVoids && !IsZero(endBlockLength) )
   {
      pgsPointOfInterest poiLeftFace1(segmentKey, endBlockLength, POI_SECTCHANGE_LEFTFACE);
      pgsPointOfInterest poiRightFace1(segmentKey, endBlockLength, POI_SECTCHANGE_RIGHTFACE);
      PoiIDType poiID = pPoiMgr->AddPointOfInterest( poiLeftFace1 );
      ATLASSERT(poiID != INVALID_ID);
      poiLeftFace1 = pPoiMgr->GetPointOfInterest(poiID);
      poiRightFace1.SetDistFromStart(poiLeftFace1.GetDistFromStart(),true);
      VERIFY(pPoiMgr->AddPointOfInterest(poiRightFace1) != INVALID_ID);

      pgsPointOfInterest poiRightFace2(segmentKey, gdrLength - endBlockLength, POI_SECTCHANGE_RIGHTFACE);
      pgsPointOfInterest poiLeftFace2(segmentKey, gdrLength - endBlockLength, POI_SECTCHANGE_LEFTFACE);
      poiID = pPoiMgr->AddPointOfInterest(poiRightFace2);
      ATLASSERT(poiID != INVALID_ID);
      poiRightFace2 = pPoiMgr->GetPointOfInterest(poiID);
      poiLeftFace2.SetDistFromStart(poiRightFace2.GetDistFromStart(),true);
      VERIFY(pPoiMgr->AddPointOfInterest(poiLeftFace2) != INVALID_ID);
   }
}

void CVoidedSlab2Factory::CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedBeamSpacing* pSpacingType,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng) const
{
   GET_IFACE2(pBroker, IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   // use passed value if not null
   pgsTypes::SupportedDeckType deckType = (pDeckType!=nullptr) ? *pDeckType : pDeck->GetDeckType();
   pgsTypes::SupportedBeamSpacing spacingType = (pSpacingType!=nullptr) ? *pSpacingType : pBridgeDesc->GetGirderSpacingType();
   
   if (spacingType==pgsTypes::sbsUniformAdjacent || spacingType==pgsTypes::sbsGeneralAdjacent || spacingType==pgsTypes::sbsConstantAdjacent)
   {
      CComObject<CVoidedSlab2DistFactorEngineer>* pEngineer;
      CComObject<CVoidedSlab2DistFactorEngineer>::CreateInstance(&pEngineer);
      pEngineer->SetBroker(pBroker,statusGroupID);
      (*ppEng) = pEngineer;
      (*ppEng)->AddRef();
   }
   else
   {
      // this is a type b section... type b's are the same as type c's which are U-beams
      ATLASSERT( deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP );

      CComObject<CUBeamDistFactorEngineer>* pEngineer;
      CComObject<CUBeamDistFactorEngineer>::CreateInstance(&pEngineer);
      pEngineer->Init(true,true); // this is a type b cross section, and a spread slab
      pEngineer->SetBroker(pBroker,statusGroupID);
      (*ppEng) = pEngineer;
      (*ppEng)->AddRef();
   }
}

void CVoidedSlab2Factory::CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng) const
{
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      CComObject<CTimeStepLossEngineer>* pEngineer;
      CComObject<CTimeStepLossEngineer>::CreateInstance(&pEngineer);
      pEngineer->SetBroker(pBroker,statusGroupID);
      (*ppEng) = pEngineer;
      (*ppEng)->AddRef();
   }
   else
   {
      CComObject<CPsBeamLossEngineer>* pEngineer;
      CComObject<CPsBeamLossEngineer>::CreateInstance(&pEngineer);
       
      // depends on # of voids
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
      const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(girderKey.girderIndex)->GetGirderLibraryEntry();

      IndexType nVoids = (IndexType)pGdrEntry->GetDimension(_T("Number_of_Voids"));

      if ( nVoids == 0 )
      {
         pEngineer->Init(SolidSlab);
      }
      else
      {
         pEngineer->Init(SingleT);
      }

      pEngineer->SetBroker(pBroker,statusGroupID);
      (*ppEng) = pEngineer;
      (*ppEng)->AddRef();
   }
}

void CVoidedSlab2Factory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions,  Float64 Hg,
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const
{
   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;

   CComQIPtr<IConfigureStrandMover> configurer(sm);

   // Set the shapes for harped strand bounds 
   // Voided slabs don't normally support harped strands, so the question
   Float64 H,W,D1,D2,H1,H2,S1,S2,C1,C2,C3,J,EndBlockLength;
   WebIndexType N;
   GetDimensions(dimensions,H,W,D1,D2,H1,H2,S1,S2,C1,C2,C3,N,J,EndBlockLength);

   Float64 width = W;
   Float64 depth = (Hg < 0 ? H : Hg);

   if (N==0)
   {
      // easy part, no voids
      Float64 hook_offset = 0.0;

      CComPtr<IShape> shape;
      MakeRectangle(width, depth, hook_offset, 0.00, &shape);

      hr = configurer->AddRegion(shape, 0.0);
      ATLASSERT (SUCCEEDED(hr));
   }
   else
   {
      // multiple voids, put rectangles between them
      IndexType nIntVoids, nExtVoids;
      if ( N == 0 )
      {
         nIntVoids = 0;
         nExtVoids = 0;
      }
      else if ( N == 1 )
      {
         nIntVoids = 0;
         nExtVoids = 1;
      }
      else
      {
         nExtVoids = 2;
         nIntVoids = N - nExtVoids;
      }

      Float64 t_ext;
      if ( nIntVoids == 0 )
      {
         t_ext = (width - (nExtVoids-1)*S1 - D1)/2;
      }
      else
      {
         t_ext = (width - (nIntVoids-1)*S2 - 2*S1 - D1)/2;
      }

      // thickness of interior "web" (between interior voids)
      Float64 t_int;
      if ( nIntVoids == 0 )
      {
         t_int = 0;
      }
      else
      {
         t_int = S2 - D2;
      }

      // thickness of "web" between interior and exterior voids)
      Float64 t_ext_int;
      if ( nIntVoids == 0 )
      {
         t_ext_int = S1 - D1;
      }
      else
      {
         t_ext_int = S1 - D1/2 - D2/2;
      }

      Float64 end_loc = (width-t_ext)/2.0; 

      // rectangles at ends
      CComPtr<IShape> shapel, shaper;
      MakeRectangle(t_ext, depth - C1 - C2, -end_loc, -(C1+C2), &shapel);
      MakeRectangle(t_ext, depth - C1 - C2,  end_loc, -(C1+C2), &shaper);

      hr = configurer->AddRegion(shapel, 0.0);
      ATLASSERT (SUCCEEDED(hr));
      
      hr = configurer->AddRegion(shaper, 0.0);
      ATLASSERT (SUCCEEDED(hr));

      if ( nIntVoids == 0 && nExtVoids == 2 )
      {
         CComPtr<IShape> shape;
         MakeRectangle(t_ext_int, depth, 0.00, 0.00, &shape);
         hr = configurer->AddRegion(shape, 0.0);
         ATLASSERT (SUCCEEDED(hr));
      }
      else if ( 0 < nIntVoids )
      {
         // rectangles between interior and exterior voids
         Float64 loc = width/2 - t_ext - D1 - t_ext_int/2;
         shapel.Release();
         shaper.Release();

         MakeRectangle(t_ext_int, depth, -loc, 0.00, &shapel);
         MakeRectangle(t_ext_int, depth,  loc, 0.00, &shaper);

         hr = configurer->AddRegion(shapel, 0.0);
         ATLASSERT (SUCCEEDED(hr));
         
         hr = configurer->AddRegion(shaper, 0.0);
         ATLASSERT (SUCCEEDED(hr));

         // retangles between interior voids
         loc = width/2 - t_ext - D1 - t_ext_int - D2 - t_int/2;
         loc *= -1;
         for ( Uint16 i = 1; i < nIntVoids; i++ )
         {
            CComPtr<IShape> shape;
            MakeRectangle(t_int,depth,loc,0.00,&shape);
            hr = configurer->AddRegion(shape, 0.0);
            ATLASSERT (SUCCEEDED(hr));

            loc += S2;
         }
      }
   }

   // set vertical offset bounds and increments
   Float64 hptb  = hpTopFace     == IBeamFactory::BeamBottom ? hpTopLimit     - depth : -hpTopLimit;
   Float64 hpbb  = hpBottomFace  == IBeamFactory::BeamBottom ? hpBottomLimit  - depth : -hpBottomLimit;
   Float64 endtb = endTopFace    == IBeamFactory::BeamBottom ? endTopLimit    - depth : -endTopLimit;
   Float64 endbb = endBottomFace == IBeamFactory::BeamBottom ? endBottomLimit - depth : -endBottomLimit;

   hr = configurer->SetHarpedStrandOffsetBounds(0, depth, endtb, endbb, hptb, hpbb, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));
}

const std::vector<std::_tstring>& CVoidedSlab2Factory::GetDimensionNames() const
{
   return m_DimNames;
}

const std::vector<Float64>& CVoidedSlab2Factory::GetDefaultDimensions() const
{
   return m_DefaultDims;
}

const std::vector<const WBFL::Units::Length*>& CVoidedSlab2Factory::GetDimensionUnits(bool bSIUnits) const
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CVoidedSlab2Factory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSI,std::_tstring* strErrMsg) const
{
   Float64 H,W,D1,D2,H1,H2,S1,S2,C1,C2,C3,J,EndBlockLength;
   WebIndexType N;
   GetDimensions(dimensions,H,W,D1,D2,H1,H2,S1,S2,C1,C2,C3,N,J,EndBlockLength);

   if ( H <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("Height must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("Width must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( N < 0 )
   {
      std::_tostringstream os;
      os << _T("Invalid Number of Voids") << std::ends;
      *strErrMsg = os.str();
      return false;
   }
   else if (N == 0)
   {
      if ( !IsZero(D1) || !IsZero(D2) )
      {
         std::_tostringstream os;
         os << _T("Void Diameter Must Be Zero If No Voids") << std::ends;
         *strErrMsg = os.str();
         return false;
      }

      if ( !IsZero(S1) || !IsZero(S2) )
      {
         std::_tostringstream os;
         os << _T("Invalid - Void Spacing Must Be Zero If No Voids") << std::ends;
         *strErrMsg = os.str();
         return false;
      }
   }
   else // (N > 0)
   {
      if ( N == 1 && !IsZero(S1) )
      {
         std::_tostringstream os;
         os << _T("Invalid - Exterior Void Spacing Must Be Zero") << std::ends;
         *strErrMsg = os.str();
         return false;
      }

      if ( (N == 1 || N == 2) && !IsZero(D2) )
      {
         std::_tostringstream os;
         os << _T("Invalid - Interior Void Diameter Must Be Zero") << std::ends;
         *strErrMsg = os.str();
         return false;
      }

      if ( (N == 1 || N == 2) && !IsZero(S2) )
      {
         std::_tostringstream os;
         os << _T("Invalid - Interior Void Spacing Must Be Zero") << std::ends;
         *strErrMsg = os.str();
         return false;
      }

      if ( D1 <= 0.0 )
      {
         std::_tostringstream os;
         os << _T("Exterior Void Diameter Must Be Greater Than Zero") << std::ends;
         *strErrMsg = os.str();
         return false;
      }

      if ( H <= D1 )
      {
         std::_tostringstream os;
         os << _T("Exterior Void Diameter must be less than slab height") << std::ends;
         *strErrMsg = os.str();
         return false;
      }


      if ( 2 < N )
      {
         if ( D2 <= 0.0 )
         {
            std::_tostringstream os;
            os << _T("Interior Void Diameter Must Be Greater Than Zero") << std::ends;
            *strErrMsg = os.str();
            return false;
         }

         if ( H <= D2 )
         {
            std::_tostringstream os;
            os << _T("Interior Void Diameter must be less than slab height") << std::ends;
            *strErrMsg = os.str();
            return false;
         }
      }

      if ( N == 1 )
      {
         if ( W <= D1 )
         {
            std::_tostringstream os;
            os << _T("Void Diameter must be less than slab width") << std::ends;
            *strErrMsg = os.str();
            return false;
         }

         if ( W <= D1 )
         {
            std::_tostringstream os;
            os << _T("Void Diameter must be less than slab width") << std::ends;
            *strErrMsg = os.str();
            return false;
         }
      }
      else
      {
         if ( N == 2 )
         {
            if ( W <= S1 + D1 )
            {
               std::_tostringstream os;
               os << _T("Slab must be wider than width occupied by voids") << std::ends;
               *strErrMsg = os.str();
               return false;
            }

            if ( S1 <= D1 )
            {
               std::_tostringstream os;
               os << _T("Void spacing must be greater than void diameter") << std::ends;
               *strErrMsg = os.str();
               return false;
            }
         }
         else
         {
            if ( W <= (N-3)*S2 + 2*S1 + D1 )
            {
               std::_tostringstream os;
               os << _T("Slab must be wider than width occupied by voids") << std::ends;
               *strErrMsg = os.str();
               return false;
            }

            if ( S1 <= D1/2+D2/2 )
            {
               std::_tostringstream os;
               os << _T("Void spacing must be greater than void diameter") << std::ends;
               *strErrMsg = os.str();
               return false;
            }

            if ( 3 < N && S2 <= D2 )
            {
               std::_tostringstream os;
               os << _T("Void spacing must be greater than void diameter") << std::ends;
               *strErrMsg = os.str();
               return false;
            }
         }
      }
   }

   if ( J < 0.0 )
   {
      std::_tostringstream os;
      os << _T("Maximum joint size must be zero or greater") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   return true;
}

void CVoidedSlab2Factory::SaveSectionDimensions(WBFL::System::IStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions) const
{
   pSave->BeginUnit(_T("VoidedSlab2Dimensions"),3.0);
   for(const auto& name : m_DimNames)
   {
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CVoidedSlab2Factory::LoadSectionDimensions(WBFL::System::IStructuredLoad* pLoad) const
{
   Float64 parent_version;
   if ( pLoad->GetParentUnit() == _T("GirderLibraryEntry") )
   {
      parent_version = pLoad->GetParentVersion();
   }
   else
   {
      parent_version = pLoad->GetVersion();
   }

   IBeamFactory::Dimensions dimensions;

   Float64 dimVersion = 1.0;
   if ( 14 <= parent_version )
   {
      if ( pLoad->BeginUnit(_T("VoidedSlab2Dimensions")) )
      {
         dimVersion = pLoad->GetVersion();
      }
      else
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }

   for(const auto& name : m_DimNames)
   {
      Float64 value = 0;
      if ( !pLoad->Property(name.c_str(),&value) )
      {
         if ( name == _T("C3") && dimVersion < 3)
         {
            // C3 was introduced in dimVersion 3
            // if version is less, it is ok to fail and continue
            value = 0;
         }
         else
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }

      dimensions.emplace_back(name,value);
   }

   if ( 14 <= parent_version && !pLoad->EndUnit() )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   return dimensions;
}

bool CVoidedSlab2Factory::IsPrismatic(const IBeamFactory::Dimensions& dimensions) const
{
   Float64 endBlockLength = GetDimension(dimensions,_T("EndBlockLength"));
   return IsZero(endBlockLength) ? true : false;
}

bool CVoidedSlab2Factory::IsPrismatic(const CSegmentKey& segmentKey) const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();
   return IsPrismatic(dimensions);
}

bool CVoidedSlab2Factory::IsSymmetric(const CSegmentKey& segmentKey) const
{
   return true;
}

std::_tstring CVoidedSlab2Factory::GetImage() const
{
   return std::_tstring(_T("VoidedSlab2.jpg"));
}

std::_tstring CVoidedSlab2Factory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeOverlay:
      strImage = _T("VoidedSlab_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage = _T("VoidedSlab_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CVoidedSlab2Factory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("+Mn_VoidedSlab_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("+Mn_VoidedSlab_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CVoidedSlab2Factory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("-Mn_VoidedSlab_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("-Mn_VoidedSlab_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CVoidedSlab2Factory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("Vn_VoidedSlab_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("Vn_VoidedSlab_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CVoidedSlab2Factory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();
   if (section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth ||
      WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition())
   {
      return _T("VoidedSlab_Effective_Flange_Width_Interior_Girder_2008.gif");
   }
   else
   {
      return _T("VoidedSlab_Effective_Flange_Width_Interior_Girder.gif");
   }
}

std::_tstring CVoidedSlab2Factory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();
   if (section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth ||
      WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition())
   {
      return _T("VoidedSlab_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("VoidedSlab_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID CVoidedSlab2Factory::GetCLSID() const
{
   return CLSID_VoidedSlab2Factory;
}

std::_tstring CVoidedSlab2Factory::GetName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

CLSID CVoidedSlab2Factory::GetFamilyCLSID() const
{
   return CLSID_SlabBeamFamily;
}

std::_tstring CVoidedSlab2Factory::GetGirderFamilyName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CVoidedSlab2Factory::GetPublisher() const
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring CVoidedSlab2Factory::GetPublisherContactInformation() const
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE CVoidedSlab2Factory::GetResourceInstance() const
{
   return _Module.GetResourceInstance();
}

LPCTSTR CVoidedSlab2Factory::GetImageResourceName() const
{
   return _T("VOIDEDSLAB2");
}

HICON  CVoidedSlab2Factory::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_VOIDEDSLAB2) );
}

void CVoidedSlab2Factory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                      Float64& H,Float64& W,Float64& D1,Float64& D2,Float64& H1,Float64& H2,Float64& S1,Float64& S2,Float64& C1,Float64& C2,Float64& C3,WebIndexType& N,Float64& J,Float64& EndBlockLength) const
{
   H = GetDimension(dimensions,_T("H"));
   W = GetDimension(dimensions,_T("W"));
   D1 = GetDimension(dimensions,_T("D1"));
   D2 = GetDimension(dimensions,_T("D2"));
   H1 = GetDimension(dimensions,_T("H1"));
   H2 = GetDimension(dimensions,_T("H2"));
   S1 = GetDimension(dimensions,_T("S1"));
   S2 = GetDimension(dimensions,_T("S2"));
   C1 = GetDimension(dimensions,_T("C1"));
   C2 = GetDimension(dimensions,_T("C2"));
   C3 = GetDimension(dimensions,_T("C3"));
   N = (WebIndexType)GetDimension(dimensions,_T("Number_of_Voids"));
   J = GetDimension(dimensions,_T("Jmax"));
   EndBlockLength = GetDimension(dimensions,_T("EndBlockLength"));
}

Float64 CVoidedSlab2Factory::GetDimension(const IBeamFactory::Dimensions& dimensions, const std::_tstring& name) const
{
   for (const auto& dim : dimensions)
   {
      if ( name == dim.first )
      {
         return dim.second;
      }
   }

   ATLASSERT(false); // should never get here
   return -99999;
}

pgsTypes::SupportedDeckTypes CVoidedSlab2Factory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const
{
   pgsTypes::SupportedDeckTypes sdt;
   switch(sbs)
   {
   case pgsTypes::sbsUniform:
   case pgsTypes::sbsGeneral:
      sdt.push_back(pgsTypes::sdtCompositeCIP);
      sdt.push_back(pgsTypes::sdtCompositeSIP);
      break;

   case pgsTypes::sbsUniformAdjacent:
   case pgsTypes::sbsGeneralAdjacent:
      sdt.push_back(pgsTypes::sdtCompositeCIP);
      sdt.push_back(pgsTypes::sdtCompositeOverlay);
      //sdt.push_back(pgsTypes::sdtNonstructuralOverlay);
      sdt.push_back(pgsTypes::sdtNone);
      break;

   default:
      ATLASSERT(false);
   }
   return sdt;
}

pgsTypes::SupportedBeamSpacings CVoidedSlab2Factory::GetSupportedBeamSpacings() const
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniformAdjacent);
   sbs.push_back(pgsTypes::sbsGeneralAdjacent);
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);

   return sbs;
}

bool CVoidedSlab2Factory::IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::SupportedBeamSpacings sbs = GetSupportedBeamSpacings();
   auto found = std::find(sbs.cbegin(), sbs.cend(), spacingType);
   return found == sbs.end() ? false : true;
}

bool CVoidedSlab2Factory::ConvertBeamSpacing(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const
{
   if (spacingType == pgsTypes::sbsUniform)
   {
      // we have uniform spacing, but can only get here if there no deck... we actually want uniform adjacent
      *pNewSpacingType = pgsTypes::sbsUniformAdjacent;
      *pNewSpacing = spacing;
      *pNewTopWidth = 0.0;
      return true;
   }
   else if (spacingType == pgsTypes::sbsGeneral)
   {
      // we have general spacing, but can only get here if there no deck... we actually want general adjacent
      *pNewSpacingType = pgsTypes::sbsGeneralAdjacent;
      *pNewSpacing = spacing;
      *pNewTopWidth = 0.0;
      return true;
   }
   return false;
}

pgsTypes::WorkPointLocations CVoidedSlab2Factory::GetSupportedWorkPointLocations(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::WorkPointLocations wpls;
   wpls.push_back(pgsTypes::wplTopGirder);
   wpls.push_back(pgsTypes::wplBottomGirder);

   return wpls;
}

bool CVoidedSlab2Factory::IsSupportedWorkPointLocation(pgsTypes::SupportedBeamSpacing spacingType, pgsTypes::WorkPointLocation wpType) const
{
   pgsTypes::WorkPointLocations sbs = GetSupportedWorkPointLocations(spacingType);
   auto found = std::find(sbs.cbegin(), sbs.cend(), wpType);
   return found == sbs.end() ? false : true;
}

std::vector<pgsTypes::GirderOrientationType> CVoidedSlab2Factory::GetSupportedGirderOrientation() const
{
   std::vector<pgsTypes::GirderOrientationType> types{ pgsTypes::Plumb, pgsTypes::StartNormal,pgsTypes::MidspanNormal,pgsTypes::EndNormal,pgsTypes::Balanced };
   return types;
}

bool CVoidedSlab2Factory::IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return true;
}

pgsTypes::GirderOrientationType CVoidedSlab2Factory::ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return orientation;
}

pgsTypes::SupportedDiaphragmTypes CVoidedSlab2Factory::GetSupportedDiaphragms() const
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes CVoidedSlab2Factory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const
{
   pgsTypes::SupportedDiaphragmLocationTypes locations;
   switch(type)
   {
   case pgsTypes::dtCastInPlace :
      locations.push_back(pgsTypes::dltExternal);
      break;

   default:
      ATLASSERT(false);
   }

   return locations;
}

void CVoidedSlab2Factory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   Float64 gw = GetDimension(dimensions,_T("W"));
   Float64 J  = GetDimension(dimensions,_T("Jmax"));

   if(sbs == pgsTypes::sbsUniform || sbs == pgsTypes::sbsGeneral)
   {
      *minSpacing = gw;
      *maxSpacing = MAX_GIRDER_SPACING;
   }
   else if(sbs == pgsTypes::sbsUniformAdjacent || sbs == pgsTypes::sbsGeneralAdjacent)
   {
      // for this spacing type, we have joint spacing... spacing range is the range of joint width
      *minSpacing = 0;
      *maxSpacing = J;
   }
   else
   {
      ATLASSERT(false); // shouldn't get here
   }
}

WebIndexType CVoidedSlab2Factory::GetWebCount(const IBeamFactory::Dimensions& dimensions) const
{
   WebIndexType nv = (WebIndexType)GetDimension(dimensions,_T("Number_of_Voids"));
   return nv+1;
}

Float64 CVoidedSlab2Factory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   return GetDimension(dimensions,_T("H"));
}

Float64 CVoidedSlab2Factory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   return GetDimension(dimensions,_T("W"));
}

void CVoidedSlab2Factory::GetBeamTopWidth(const IBeamFactory::Dimensions& dimensions, pgsTypes::MemberEndType endType, Float64* pLeftWidth, Float64* pRightWidth) const
{
   Float64 W = GetDimension(dimensions,_T("W"));
   Float64 C1 = GetDimension(dimensions,_T("C1"));

   Float64 top = W - 2*C1;
   top /= 2.0;

   *pLeftWidth = top;
   *pRightWidth = top;
}

bool CVoidedSlab2Factory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const
{
   return false;
}

void CVoidedSlab2Factory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

bool CVoidedSlab2Factory::HasLongitudinalJoints() const
{
   return false;
}

bool CVoidedSlab2Factory::IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const
{
   return false;
}

bool CVoidedSlab2Factory::HasTopFlangeThickening() const
{
   return false;
}

GirderIndexType CVoidedSlab2Factory::GetMinimumBeamCount() const
{
   return 1;
}

bool CVoidedSlab2Factory::CanPrecamber() const
{
   return false;
}

void CVoidedSlab2Factory::DimensionAndPositionBeam(const IBeamFactory::Dimensions& dimensions, IVoidedSlab2* pBeam) const
{
   Float64 H, W, D1, D2, H1, H2, S1, S2, C1, C2, C3, J, EndBlockLength;
   IndexType N;
   GetDimensions(dimensions, H, W, D1, D2, H1, H2, S1, S2, C1, C2, C3, N, J, EndBlockLength);

   pBeam->put_Height(H);
   pBeam->put_Width(W);
   pBeam->put_C1(C1);
   pBeam->put_C2(C2);
   pBeam->put_C3(C3);
   pBeam->put_ExteriorVoidDiameter(D1);
   pBeam->put_InteriorVoidDiameter(D2);
   pBeam->put_ExteriorVoidElevation(H1);
   pBeam->put_InteriorVoidElevation(H2);
   pBeam->put_ExteriorVoidSpacing(S1);
   pBeam->put_InteriorVoidSpacing(S2);
   pBeam->put_VoidCount(N);


   // Hook point is at bottom center of bounding box.
   // Adjust hook point so top center of bounding box is at (0,0)
   Float64 Hg;
   pBeam->get_Height(&Hg);

   CComPtr<IPoint2d> hookPt;
   pBeam->get_HookPoint(&hookPt);
   hookPt->Offset(0, -Hg);
}
