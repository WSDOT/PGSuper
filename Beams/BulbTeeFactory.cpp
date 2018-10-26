///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// BulbTeeFactory.cpp : Implementation of CBulbTeeFactory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "BulbTeeFactory.h"
#include "BulbTeeDistFactorEngineer.h"
#include "PsBeamLossEngineer.h"
#include "TimeStepLossEngineer.h"
#include "StrandMoverImpl.h"
#include <GeomModel\PrecastBeam.h>
#include <MathEx.h>
#include <sstream>
#include <algorithm>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>

#include <PgsExt\BridgeDescription2.h>

#include <IFace\StatusCenter.h>
#include <PgsExt\StatusItem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBulbTeeFactory
HRESULT CBulbTeeFactory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.push_back(_T("C1"));
   m_DimNames.push_back(_T("D1"));
   m_DimNames.push_back(_T("D2"));
   m_DimNames.push_back(_T("D3"));
   m_DimNames.push_back(_T("D4"));
   m_DimNames.push_back(_T("D5"));
   m_DimNames.push_back(_T("D6"));
   m_DimNames.push_back(_T("D7"));
   m_DimNames.push_back(_T("D8"));
   m_DimNames.push_back(_T("T1"));
   m_DimNames.push_back(_T("T2"));
   m_DimNames.push_back(_T("W1"));
   m_DimNames.push_back(_T("W2"));
   m_DimNames.push_back(_T("W3"));
   m_DimNames.push_back(_T("W4"));
   m_DimNames.push_back(_T("Wmax"));
   m_DimNames.push_back(_T("Wmin"));

   std::sort(m_DimNames.begin(),m_DimNames.end());

   // Default beam is a W74G                                              
   m_DefaultDims.push_back(::ConvertToSysUnits(0.000,unitMeasure::Inch)); // C1
   m_DefaultDims.push_back(::ConvertToSysUnits(2.875,unitMeasure::Inch)); // D1
   m_DefaultDims.push_back(::ConvertToSysUnits(2.625,unitMeasure::Inch)); // D2
   m_DefaultDims.push_back(::ConvertToSysUnits(2.000,unitMeasure::Inch)); // D3
   m_DefaultDims.push_back(::ConvertToSysUnits(6.000,unitMeasure::Inch)); // D4
   m_DefaultDims.push_back(::ConvertToSysUnits(3.000,unitMeasure::Inch)); // D5
   m_DefaultDims.push_back(::ConvertToSysUnits(0.000,unitMeasure::Inch)); // D6
   m_DefaultDims.push_back(::ConvertToSysUnits(57.00,unitMeasure::Inch)); // D7
   m_DefaultDims.push_back(::ConvertToSysUnits( 0.00,unitMeasure::Inch)); // D8
   m_DefaultDims.push_back(::ConvertToSysUnits(6.000,unitMeasure::Inch)); // T1
   m_DefaultDims.push_back(::ConvertToSysUnits(6.000,unitMeasure::Inch)); // T2
   m_DefaultDims.push_back(::ConvertToSysUnits(16.50,unitMeasure::Inch)); // W1
   m_DefaultDims.push_back(::ConvertToSysUnits(2.000,unitMeasure::Inch)); // W2
   m_DefaultDims.push_back(::ConvertToSysUnits(9.500,unitMeasure::Inch)); // W3
   m_DefaultDims.push_back(::ConvertToSysUnits(0.000,unitMeasure::Inch)); // W4
   m_DefaultDims.push_back(::ConvertToSysUnits(6.000,unitMeasure::Feet)); // Wmax
   m_DefaultDims.push_back(::ConvertToSysUnits(4.000,unitMeasure::Feet)); // Wmin

   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // C1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D4
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D5
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D6
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D7
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D8
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W4
   m_DimUnits[0].push_back(&unitMeasure::Meter);      // Wmax
   m_DimUnits[0].push_back(&unitMeasure::Meter);      // Wmin

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // C1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D4
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D5
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D6
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D7
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D8
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W4
   m_DimUnits[1].push_back(&unitMeasure::Feet); // Wmax
   m_DimUnits[1].push_back(&unitMeasure::Feet); // Wmin

   return S_OK;
}

void CBulbTeeFactory::CreateGirderSection(IBroker* pBroker,StatusItemIDType statusID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection)
{
   CComPtr<IBulbTeeSection> gdrsection;
   gdrsection.CoCreateInstance(CLSID_BulbTeeSection);
   CComPtr<IBulbTee> beam;
   gdrsection->get_Beam(&beam);

   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,d7,d8;
   Float64 w1,w2,w3,w4,wmin,wmax;
   Float64 t1,t2;
   GetDimensions(dimensions,c1,d1,d2,d3,d4,d5,d6,d7,d8,w1,w2,w3,w4,wmin,wmax,t1,t2);
   beam->put_W1(w1);
   beam->put_W2(w2);
   beam->put_W3(w3);
   beam->put_W4(w4);

   if ( pBroker == NULL )
   {
      beam->put_W5(wmax);
   }
   else
   {
      // Assumes uniform girder spacing

      // use raw input here because requesting it from the bridge will cause an infinite loop.
      // bridge agent calls this during validation
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      ATLASSERT(pBridgeDesc->GetGirderSpacingType() == pgsTypes::sbsConstantAdjacent);
      Float64 spacing = pBridgeDesc->GetGirderSpacing();

      // if this is a fixed width section, then set the spacing equal to the width
      if ( IsEqual(wmin,wmax) )
         spacing = wmax;
         
      beam->put_W5(spacing);
   }

   beam->put_C1(c1);
   beam->put_D1(d1);
   beam->put_D2(d2);
   beam->put_D3(d3);
   beam->put_D4(d4);
   beam->put_D5(d5);
   beam->put_D6(d6);
   beam->put_D7(d7);
   beam->put_T1(t1);
   beam->put_T2(t2);

   gdrsection.QueryInterface(ppSection);
}

void CBulbTeeFactory::CreateGirderProfile(IBroker* pBroker,StatusItemIDType statusID,const CSegmentKey& segmentKey,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetSegmentLength(segmentKey);

   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,d7,d8;
   Float64 w1,w2,w3,w4,wmin,wmax;
   Float64 t1,t2;
   GetDimensions(dimensions,c1,d1,d2,d3,d4,d5,d6,d7,d8,w1,w2,w3,w4,wmin,wmax,t1,t2);

   Float64 height = d1 + d2 + d3 + d4 + d5 + d6 + d7;
   
   CComPtr<IPolyShape> polyShape;
   polyShape.CoCreateInstance(CLSID_PolyShape);
   
   polyShape->AddPoint(0,0);
   polyShape->AddPoint(length,0);
   polyShape->AddPoint(length,height+d8);
   polyShape->AddPoint(7*length/8,height+0.5625*d8);
   polyShape->AddPoint(6*length/8,height+0.2500*d8);
   polyShape->AddPoint(5*length/8,height+0.0625*d8);
   polyShape->AddPoint(4*length/8,height+0.0000*d8);
   polyShape->AddPoint(3*length/8,height+0.0625*d8);
   polyShape->AddPoint(2*length/8,height+0.2500*d8);
   polyShape->AddPoint(1*length/8,height+0.5625*d8);
   polyShape->AddPoint(0,height+d8);

   CComQIPtr<IXYPosition> position(polyShape);
   CComPtr<IPoint2d> topLeft;
   position->get_LocatorPoint(lpTopLeft,&topLeft);
   topLeft->Move(0,0);
   position->put_LocatorPoint(lpTopLeft,topLeft);

   polyShape->QueryInterface(ppShape);
}

void CBulbTeeFactory::CreateSegment(IBroker* pBroker,StatusItemIDType statusID,const CSegmentKey& segmentKey,ISuperstructureMember* ssmbr)
{
   CComPtr<ISuperstructureMemberSegment> segment;

   bool bPrismatic = IsPrismatic(pBroker,segmentKey);
   if ( bPrismatic )
   {
      // prismatic
      segment.CoCreateInstance(CLSID_PrismaticSuperstructureMemberSegment);
   }
   else
   {
      // non-prismatic
      segment.CoCreateInstance(CLSID_ThickenedFlangeBulbTeeSegment);
   }

   ATLASSERT(segment != NULL);

   // Build up the beam shape
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();

   // Beam materials
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IMaterials,pMaterial);
   CComPtr<IMaterial> material;
   material.CoCreateInstance(CLSID_Material);

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      Float64 E = pMaterial->GetSegmentEc(segmentKey,intervalIdx);
      Float64 D = pMaterial->GetSegmentWeightDensity(segmentKey,intervalIdx);

      material->put_E(intervalIdx,E);
      material->put_Density(intervalIdx,D);
   }



   if ( bPrismatic )
   {
      CComPtr<IGirderSection> gdrSection;
      CreateGirderSection(pBroker,statusID,dimensions,-1,-1,&gdrSection);
      
      CComQIPtr<IPrismaticSuperstructureMemberSegment> prisSegment(segment);
      ATLASSERT(prisSegment);

      CComQIPtr<IShape> shape(gdrSection);
      ATLASSERT(shape);

      prisSegment->AddShape(shape,material,NULL);
   }
   else
   {
      CComPtr<IGirderSection> gdrSection;
      CreateGirderSection(pBroker,statusID,dimensions,-1,-1,&gdrSection);

      CComQIPtr<IThickenedFlangeBulbTeeSegment> bulbTeeSegment(segment);
      CComQIPtr<IBulbTeeSection> bulbTeeSection(gdrSection);

      Float64 flangeThickening = GetDimension(dimensions,_T("D8"));
      bulbTeeSegment->put_FlangeThickening(flangeThickening);


      CComQIPtr<IShape> shape(gdrSection);
      ATLASSERT(shape);

      bulbTeeSegment->AddShape(shape,material,NULL);
   }


   ssmbr->AddSegment(segment);
}

void CBulbTeeFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,   POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE  );

   pPoiMgr->AddPointOfInterest(poiStart);
   pPoiMgr->AddPointOfInterest(poiEnd);

   if ( !IsPrismatic(pBroker,segmentKey) )
   {
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,1*gdrLength/8,POI_SECTCHANGE_TRANSITION ) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,2*gdrLength/8,POI_SECTCHANGE_TRANSITION ) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,3*gdrLength/8,POI_SECTCHANGE_TRANSITION ) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,4*gdrLength/8,POI_SECTCHANGE_TRANSITION ) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,5*gdrLength/8,POI_SECTCHANGE_TRANSITION ) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,6*gdrLength/8,POI_SECTCHANGE_TRANSITION ) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,7*gdrLength/8,POI_SECTCHANGE_TRANSITION ) );
   }
}

void CBulbTeeFactory::CreateDistFactorEngineer(IBroker* pBroker,StatusItemIDType statusID,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng)
{
   GET_IFACE2(pBroker, ISpecification,    pSpec);
   GET_IFACE2(pBroker, ILibrary,          pLib);
   GET_IFACE2(pBroker, IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Int16 method = pSpecEntry->GetLiveLoadDistributionMethod();

   // use passed values if not null
   pgsTypes::SupportedDeckType deckType = (pDeckType!=NULL) ? *pDeckType : pDeck->DeckType;

   ATLASSERT(deckType == pgsTypes::sdtNone || deckType == pgsTypes::sdtCompositeOverlay); // no spread bulb t's

   pgsTypes::AdjacentTransverseConnectivity connect = (pConnect!=NULL) ? *pConnect : pDeck->TransverseConnectivity;

   // for composite attached beams, we want to use WSDOT type k
   bool useIBeam = ( method==LLDF_WSDOT && connect == pgsTypes::atcConnectedAsUnit && deckType == pgsTypes::sdtCompositeOverlay );

   CComObject<CBulbTeeDistFactorEngineer>* pEngineer;
   CComObject<CBulbTeeDistFactorEngineer>::CreateInstance(&pEngineer);

   pEngineer->Init(useIBeam);
   pEngineer->SetBroker(pBroker,statusID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CBulbTeeFactory::CreatePsLossEngineer(IBroker* pBroker,StatusItemIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng)
{
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
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
       pEngineer->Init(IBeam);
       pEngineer->SetBroker(pBroker,statusGroupID);
       (*ppEng) = pEngineer;
       (*ppEng)->AddRef();
   }
}

void CBulbTeeFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions,  Float64 Hg,
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover)
{
   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;


   // set the shape for harped strand bounds - only in the thinest part of the web
   CComPtr<IRectangle> harp_rect;
   hr = harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));

   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,d7,d8;
   Float64 w1,w2,w3,w4,wmin,wmax;
   Float64 t1,t2;
   GetDimensions(dimensions,c1,d1,d2,d3,d4,d5,d6,d7,d8,w1,w2,w3,w4,wmin,wmax,t1,t2);

   Float64 width = Min(t1,t2);
   Float64 depth = (Hg < 0 ? d1 + d2 + d3 + d4 + d5 + d6 + d7 : Hg);

   harp_rect->put_Width(width);
   harp_rect->put_Height(depth);

   CComPtr<IPoint2d> hook;
   hook.CoCreateInstance(CLSID_Point2d);
   hook->Move(0, -depth/2.0);

   harp_rect->putref_HookPoint(hook);

   CComPtr<IShape> shape;
   harp_rect->get_Shape(&shape);

   CComQIPtr<IConfigureStrandMover> configurer(sm);
   hr = configurer->AddRegion(shape, 0.0);
   ATLASSERT (SUCCEEDED(hr));

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

std::vector<std::_tstring> CBulbTeeFactory::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<Float64> CBulbTeeFactory::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CBulbTeeFactory::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CBulbTeeFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg)
{
   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,d7,d8;
   Float64 w1,w2,w3,w4,wmin,wmax;
   Float64 t1,t2;
   GetDimensions(dimensions,c1,d1,d2,d3,d4,d5,d6,d7,d8,w1,w2,w3,w4,wmin,wmax,t1,t2);

// 0  D1  
// 1  D2
// 2  D3
// 3  D4
// 4  D5
// 5  D6
// 6  D7
// 7  D8
// 8  T1
// 9  T2
// 10 W1
// 11 W2
// 12 W3
// 13 W4
// 14 Wmax
// 15 Wmin
   if ( c1 < 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][0];
      std::_tostringstream os;
      os << _T("C1 must be zero or greater ") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][0];
      std::_tostringstream os;
      os << _T("D1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d2 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("D2 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d3 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("D3 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d4 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][3];
      std::_tostringstream os;
      os << _T("D4 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d5 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("D5 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d6 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("D6 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d7 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][6];
      std::_tostringstream os;
      os << _T("D7 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d8 < 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][7];
      std::_tostringstream os;
      os << _T("D8 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][10];
      std::_tostringstream os;
      os << _T("W1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( w2 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("W2 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w3 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][12];
      std::_tostringstream os;
      os << _T("W3 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( w4 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("W4 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }
   
   if ( t1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][8];
      std::_tostringstream os;
      os << _T("T1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   
   
   if ( t2 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][9];
      std::_tostringstream os;
      os << _T("T2 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( c1 > d4 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][9];
      std::_tostringstream os;
      os << _T("C1 must be less than D4 ") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if (wmin>wmax)
   {
      std::_tostringstream os;
      os << _T("Wmin must be greater than or equal to Wmax") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   Float64 inp_toler = ::ConvertToSysUnits(2.0, unitMeasure::Millimeter);

   Float64 min_topflange = t2+2.0*(w3+w4);
   if ( wmin + inp_toler < min_topflange )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][15];
      Float64 mf_u = ::ConvertFromSysUnits(min_topflange,*pUnit);

      std::_tostringstream os;
      os << _T("Wmin must be greater than or equal to bottom flange width = ")<<mf_u<< pUnit->UnitTag() <<_T(" = T2 + 2.0*(W3+W4)") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   min_topflange = t1+2.0*(w1+w2);
   if ( wmin + inp_toler < min_topflange  )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][15];
      Float64 mf_u = ::ConvertFromSysUnits(min_topflange,*pUnit);

      std::_tostringstream os;
      os << _T("Wmin must be greater than or equal to T1 + 2.0*(W1 + W2) = ")<<mf_u<< pUnit->UnitTag() <<_T(" = ") << std::ends;
      *strErrMsg = os.str();
      return false;
   }


   return true;
}

void CBulbTeeFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::_tstring>::iterator iter;
   pSave->BeginUnit(_T("BulbTeeDimensions"),3.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CBulbTeeFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
{
   Float64 parent_version;
   if ( pLoad->GetParentUnit() == _T("GirderLibraryEntry") )
      parent_version = pLoad->GetParentVersion();
   else
      parent_version = pLoad->GetVersion();


   IBeamFactory::Dimensions dimensions;

   if ( 14 <= parent_version && !pLoad->BeginUnit(_T("BulbTeeDimensions")) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   Float64 dimVersion = pLoad->GetVersion();

   std::vector<std::_tstring>::iterator iter;
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value;
      if ( !pLoad->Property(name.c_str(),&value) )
      {
         if ( (parent_version < 14 || (14 <= parent_version && dimVersion < 2)) && name == _T("D8") )
            value = 0;
         else if ( (parent_version < 14 || (14 <= parent_version && dimVersion < 3))&& name == _T("C1") )
            value = 0;
         else
            THROW_LOAD(InvalidFileFormat,pLoad);

      }
      dimensions.push_back( std::make_pair(name,value) );
   }

   if ( 14 <= parent_version && !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   return dimensions;
}

bool CBulbTeeFactory::IsPrismatic(IBroker* pBroker,const CSegmentKey& segmentKey)
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();
   Float64 d8 = GetDimension(dimensions,_T("D8"));

   return IsZero(d8) ? true : false;
}

bool CBulbTeeFactory::IsSymmetric(IBroker* pBroker,const CSegmentKey& segmentKey)
{
   return true;
}

Float64 CBulbTeeFactory::GetInternalSurfaceAreaOfVoids(IBroker* pBroker,const CSegmentKey& segmentKey)
{
   return 0;
}

std::_tstring CBulbTeeFactory::GetImage()
{
   return std::_tstring(_T("BulbTee.png"));
}

std::_tstring CBulbTeeFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("BulbTee_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("BulbTee_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CBulbTeeFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("+Mn_BulbTee_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("+Mn_BulbTee_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CBulbTeeFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("-Mn_BulbTee_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("-Mn_BulbTee_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CBulbTeeFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("Vn_BulbTee_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("Vn_BulbTee_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CBulbTeeFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   return _T("BulbTee_Effective_Flange_Width_Interior_Girder.gif");
}

std::_tstring CBulbTeeFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("BulbTee_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("BulbTee_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID CBulbTeeFactory::GetCLSID()
{
   return CLSID_BulbTeeFactory;
}

std::_tstring CBulbTeeFactory::GetName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

CLSID CBulbTeeFactory::GetFamilyCLSID()
{
   return CLSID_DeckBulbTeeBeamFamily;
}

std::_tstring CBulbTeeFactory::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CBulbTeeFactory::GetPublisher()
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring CBulbTeeFactory::GetPublisherContactInformation()
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE CBulbTeeFactory::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

LPCTSTR CBulbTeeFactory::GetImageResourceName()
{
   return _T("BULBTEE");
}

HICON  CBulbTeeFactory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_BULBTEE) );
}

void CBulbTeeFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions, Float64& c1,
                                  Float64& d1,Float64& d2,Float64& d3,Float64& d4,Float64& d5,Float64& d6,Float64& d7,Float64& d8,
                                  Float64& w1,Float64& w2,Float64& w3,Float64& w4,Float64& wmin,Float64& wmax,
                                  Float64& t1,Float64& t2)
{
   c1 = GetDimension(dimensions,_T("C1"));
   d1 = GetDimension(dimensions,_T("D1"));
   d2 = GetDimension(dimensions,_T("D2"));
   d3 = GetDimension(dimensions,_T("D3"));
   d4 = GetDimension(dimensions,_T("D4"));
   d5 = GetDimension(dimensions,_T("D5"));
   d6 = GetDimension(dimensions,_T("D6"));
   d7 = GetDimension(dimensions,_T("D7"));
   d8 = GetDimension(dimensions,_T("D8"));
   w1 = GetDimension(dimensions,_T("W1"));
   w2 = GetDimension(dimensions,_T("W2"));
   w3 = GetDimension(dimensions,_T("W3"));
   w4 = GetDimension(dimensions,_T("W4"));
   wmin = GetDimension(dimensions,_T("Wmin"));
   wmax = GetDimension(dimensions,_T("Wmax"));
   t1 = GetDimension(dimensions,_T("T1"));
   t2 = GetDimension(dimensions,_T("T2"));
}

Float64 CBulbTeeFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name)
{
   Dimensions::const_iterator iter;
   for ( iter = dimensions.begin(); iter != dimensions.end(); iter++ )
   {
      const Dimension& dim = *iter;
      if ( name == dim.first )
         return dim.second;
   }

   ATLASSERT(false); // should never get here
   return -99999;
}

pgsTypes::SupportedDeckTypes CBulbTeeFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
{
   pgsTypes::SupportedDeckTypes sdt;

   switch(sbs)
   {
   case pgsTypes::sbsConstantAdjacent:
      sdt.push_back(pgsTypes::sdtCompositeOverlay);
      sdt.push_back(pgsTypes::sdtNone);
      break;

   default:
      ATLASSERT(false);
   }

   return sdt;
}

pgsTypes::SupportedBeamSpacings CBulbTeeFactory::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsConstantAdjacent);
   return sbs;
}

pgsTypes::SupportedDiaphragmTypes CBulbTeeFactory::GetSupportedDiaphragms()
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes CBulbTeeFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type)
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

void CBulbTeeFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
                                               pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing)
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   Float64 Wmax = GetDimension(dimensions,_T("Wmax"));
   Float64 Wmin = GetDimension(dimensions,_T("Wmin"));

   if (sdt == pgsTypes::sdtCompositeOverlay || sdt == pgsTypes::sdtNone)
   {
      if (sbs == pgsTypes::sbsConstantAdjacent)
      {
         *minSpacing = Wmin;
         *maxSpacing = Wmax;
      }
      else
      {
         ATLASSERT(false);
      }
   }
   else
   {
      ATLASSERT(false);
   };
}

WebIndexType CBulbTeeFactory::GetWebCount(const IBeamFactory::Dimensions& dimensions)
{
   return 1;
}

Float64 CBulbTeeFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   Float64 D1 = GetDimension(dimensions,_T("D1"));
   Float64 D2 = GetDimension(dimensions,_T("D2"));
   Float64 D3 = GetDimension(dimensions,_T("D3"));
   Float64 D4 = GetDimension(dimensions,_T("D4"));
   Float64 D5 = GetDimension(dimensions,_T("D5"));
   Float64 D6 = GetDimension(dimensions,_T("D6"));
   Float64 D7 = GetDimension(dimensions,_T("D7"));

   return D1 + D2 + D3 + D4 + D5 + D6 + D7;
}

Float64 CBulbTeeFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return GetDimension(dimensions,_T("Wmax"));
}

bool CBulbTeeFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType)
{
   return false;
}

void CBulbTeeFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

GirderIndexType CBulbTeeFactory::GetMinimumBeamCount()
{
   return 2;
}
