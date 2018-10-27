///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
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

// TaperedIBeamFactory.cpp : Implementation of CTaperedIBeamFactory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "TaperedIBeamFactory.h"
#include "IBeamDistFactorEngineer.h"
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

#include <IFace\AgeAdjustedMaterial.h>
#include <Beams\Helper.h>

#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTaperedIBeamFactory
HRESULT CTaperedIBeamFactory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.emplace_back(_T("C1"));
   m_DimNames.emplace_back(_T("D1"));
   m_DimNames.emplace_back(_T("D2"));
   m_DimNames.emplace_back(_T("D3"));
   m_DimNames.emplace_back(_T("D4"));
   m_DimNames.emplace_back(_T("D5"));
   m_DimNames.emplace_back(_T("D6"));
   m_DimNames.emplace_back(_T("D7_Start"));
   m_DimNames.emplace_back(_T("D7_End"));
   m_DimNames.emplace_back(_T("T1"));
   m_DimNames.emplace_back(_T("T2"));
   m_DimNames.emplace_back(_T("W1"));
   m_DimNames.emplace_back(_T("W2"));
   m_DimNames.emplace_back(_T("W3"));
   m_DimNames.emplace_back(_T("W4"));

//   std::sort(m_DimNames.begin(),m_DimNames.end());

   // Default beam is a WF74/83G
   m_DefaultDims.emplace_back(::ConvertToSysUnits( 0.000,unitMeasure::Inch)); // C1
   m_DefaultDims.emplace_back(::ConvertToSysUnits( 3.000,unitMeasure::Inch)); // D1
   m_DefaultDims.emplace_back(::ConvertToSysUnits( 3.000,unitMeasure::Inch)); // D2
   m_DefaultDims.emplace_back(::ConvertToSysUnits( 3.000,unitMeasure::Inch)); // D3
   m_DefaultDims.emplace_back(::ConvertToSysUnits( 5.125,unitMeasure::Inch)); // D4
   m_DefaultDims.emplace_back(::ConvertToSysUnits( 4.500,unitMeasure::Inch)); // D5
   m_DefaultDims.emplace_back(::ConvertToSysUnits( 3.000,unitMeasure::Inch)); // D6
   m_DefaultDims.emplace_back(::ConvertToSysUnits(52.375,unitMeasure::Inch)); // D7 start
   m_DefaultDims.emplace_back(::ConvertToSysUnits(61.000,unitMeasure::Inch)); // D7 end
   m_DefaultDims.emplace_back(::ConvertToSysUnits( 6.125,unitMeasure::Inch)); // T1
   m_DefaultDims.emplace_back(::ConvertToSysUnits( 6.125,unitMeasure::Inch)); // T2
   m_DefaultDims.emplace_back(::ConvertToSysUnits(18.4375,unitMeasure::Inch)); // W1
   m_DefaultDims.emplace_back(::ConvertToSysUnits( 3.000,unitMeasure::Inch)); // W2
   m_DefaultDims.emplace_back(::ConvertToSysUnits(13.125,unitMeasure::Inch)); // W3
   m_DefaultDims.emplace_back(::ConvertToSysUnits( 3.000,unitMeasure::Inch)); // W4

   // SI Units
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // C1
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D1
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D2
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D3
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D4
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D5
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D6
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D7 start
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D7 end
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // T1
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // T2
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // W1
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // W2
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // W3
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // W4

   // US Units
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // C1
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D1
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D2
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D3
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D4
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D5
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D6
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D7 start
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D7 end
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // T1
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // T2
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // W1
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // W2
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // W3
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // W4

   return S_OK;
}

void CTaperedIBeamFactory::CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const
{
   CreateGirderSection(pBroker,statusGroupID,pgsTypes::metStart,dimensions,ppSection);
}

void CTaperedIBeamFactory::CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,pgsTypes::MemberEndType end,const IBeamFactory::Dimensions& dimensions,IGirderSection** ppSection) const
{
   CComPtr<IFlangedGirderSection> gdrSection;
   gdrSection.CoCreateInstance(CLSID_FlangedGirderSection);

   CComPtr<IPrecastBeam> beam;
   gdrSection->get_Beam(&beam);

   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,d7s,d7e;
   Float64 w1,w2,w3,w4;
   Float64 t1,t2;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,d7s,d7e,w1,w2,w3,w4,t1,t2,c1);
   beam->put_W1(w1);
   beam->put_W2(w2);
   beam->put_W3(w3);
   beam->put_W4(w4);
   beam->put_D1(d1);
   beam->put_D2(d2);
   beam->put_D3(d3);
   beam->put_D4(d4);
   beam->put_D5(d5);
   beam->put_D6(d6);

   if ( end == pgsTypes::metStart )
      beam->put_D7(d7s);
   else
      beam->put_D7(d7e);

   beam->put_T1(t1);
   beam->put_T2(t2);
   beam->put_C1(c1);

   gdrSection.QueryInterface(ppSection);
}

void CTaperedIBeamFactory::CreateGirderProfile(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,const IBeamFactory::Dimensions& dimensions,IShape** ppShape) const
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetSegmentLength(segmentKey);

   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,d7s,d7e;
   Float64 w1,w2,w3,w4;
   Float64 t1,t2;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,d7s,d7e,w1,w2,w3,w4,t1,t2,c1);

   Float64 height_start = d1 + d2 + d3 + d4 + d5 + d6 + d7s;
   Float64 height_end   = d1 + d2 + d3 + d4 + d5 + d6 + d7e;

   CComPtr<IPolyShape> shape;
   shape.CoCreateInstance(CLSID_PolyShape);
   shape->AddPoint(0,0); // top left corner of shape
   shape->AddPoint(length,0);
   shape->AddPoint(length,-height_end);
   shape->AddPoint(0,-height_start);

   shape->QueryInterface(ppShape);
}

void CTaperedIBeamFactory::CreateSegment(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** ppSegment) const
{
   CComPtr<ISuperstructureMemberSegment> segment;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder     = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment    = pGirder->GetSegment(segmentKey.segmentIndex);

   const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();

   bool bPrismatic = IsPrismatic(segmentKey);
   if ( bPrismatic )
   {
      // prismatic
      segment.CoCreateInstance(CLSID_PrismaticSuperstructureMemberSegment);
   }
   else
   {
      // non-prismatic
      segment.CoCreateInstance(CLSID_TaperedGirderSegment);
   }

   ATLASSERT(segment != nullptr);

   // Build up the beam shape

   // Beam materials
   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   CComPtr<IMaterial> material;
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
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

   // add shapes to the segment
   if ( bPrismatic )
   {
      CComPtr<IGirderSection> gdrSection;
      CreateGirderSection(pBroker,statusGroupID,dimensions,-1,-1,&gdrSection);

      CComQIPtr<IPrismaticSuperstructureMemberSegment> prisSegment(segment);
      ATLASSERT(prisSegment);

      CComQIPtr<IShape> shape(gdrSection);
      ATLASSERT(shape);

      prisSegment->AddShape(shape,material,nullptr);
   }
   else
   {
      CComQIPtr<ITaperedGirderSegment> taperedSegment(segment);
      CComPtr<IFlangedGirderSection> flangedSection[2];
      CComPtr<IShape> shape[2];
      for ( int i = 0; i < 2; i++ )
      {
         CComPtr<IGirderSection> gdrSection;
         CreateGirderSection(pBroker,statusGroupID,(pgsTypes::MemberEndType)i,dimensions,&gdrSection);

         gdrSection.QueryInterface(&flangedSection[i]);
         flangedSection[i].QueryInterface(&shape[i]);
      }

      taperedSegment->AddShape( shape[etStart], shape[etEnd], material, nullptr );
   }

   segment.CopyTo(ppSegment);
}

void CTaperedIBeamFactory::ConfigureSegment(IBroker* pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const
{
   // do nothing... all the configuration was done in CreateSegment
}

void CTaperedIBeamFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();

#if defined _DEBUG
   std::_tstring strGirderName = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderName();
   ATLASSERT( strGirderName == pGdrEntry->GetName() );
#endif

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   PoiAttributeType attrib = POI_SECTCHANGE_TRANSITION;
   Float64 start_length = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 end_length   = pBridge->GetSegmentEndEndDistance(segmentKey);

   for ( int i = 0; i < 11; i++ )
   {
      Float64 x = i*gdrLength/10;

      if ( x < start_length || gdrLength-end_length < x)
      {
         pgsPointOfInterest poi(segmentKey,x,attrib);
         pPoiMgr->AddPointOfInterest( poi );
      }
      else
      {
         pgsPointOfInterest poi(segmentKey,x,attrib);
         pPoiMgr->AddPointOfInterest(poi);
      }
   }
}

void CTaperedIBeamFactory::CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedBeamSpacing* pSpacingType,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng) const
{
   CComObject<CIBeamDistFactorEngineer>* pEngineer;
   CComObject<CIBeamDistFactorEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CTaperedIBeamFactory::CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng) const
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

void CTaperedIBeamFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, Float64 Hg, 
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const
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
   Float64 d1,d2,d3,d4,d5,d6,d7s,d7e;
   Float64 w1,w2,w3,w4;
   Float64 t1,t2;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,d7s,d7e,w1,w2,w3,w4,t1,t2,c1);

   Float64 width = Min(t1,t2);
   Float64 depth = (Hg < 0 ? d1 + d2 + d3 + d4 + d5 + d6 + d7s : Hg);

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

const std::vector<std::_tstring>& CTaperedIBeamFactory::GetDimensionNames() const
{
   return m_DimNames;
}

const std::vector<Float64>& CTaperedIBeamFactory::GetDefaultDimensions() const
{
   return m_DefaultDims;
}

const std::vector<const unitLength*>& CTaperedIBeamFactory::GetDimensionUnits(bool bSIUnits) const
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CTaperedIBeamFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg) const
{
   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,d7s,d7e;
   Float64 w1,w2,w3,w4;
   Float64 t1,t2;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,d7s,d7e,w1,w2,w3,w4,t1,t2,c1);

// C1  0
// D1  1
// D2  2
// D3  3
// D4  4
// D5  5
// D6  6
// D7Start  7
// D7End    8
// T1  9
// T2  10
// W1  11
// W2  12
// W3  13
// W4  14

   if ( d1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][1];
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
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][4];
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

   if ( d7s <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][7];
      std::_tostringstream os;
      os << _T("D7 at Start must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d7e <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][8];
      std::_tostringstream os;
      os << _T("D7 at End must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][11];
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
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][13];
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
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][9];
      std::_tostringstream os;
      os << _T("T1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   
   
   if ( t2 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][10];
      std::_tostringstream os;
      os << _T("T2 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( c1 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("C1 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( c1 > d4 )
   {
      std::_tostringstream os;
      os << _T("C1 must be greater than D4") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   return true;
}

void CTaperedIBeamFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions) const
{
   pSave->BeginUnit(_T("TaperedIBeamDimensions"),1.0);
   for(const auto& name : m_DimNames)
   {
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CTaperedIBeamFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad) const
{
   Float64 parent_version;
   if (pLoad->GetParentUnit() == _T("GirderLibraryEntry"))
   {
      parent_version = pLoad->GetParentVersion();
   }
   else
   {
      parent_version = pLoad->GetVersion();
   }

   IBeamFactory::Dimensions dimensions;

   if (!pLoad->BeginUnit(_T("TaperedIBeamDimensions")))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   for(const auto& name : m_DimNames)
   {
      Float64 value;
      if (!pLoad->Property(name.c_str(), &value))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      dimensions.emplace_back(name,value);
   }

   if (!pLoad->EndUnit())
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   return dimensions;
}

bool CTaperedIBeamFactory::IsPrismatic(const IBeamFactory::Dimensions& dimensions) const
{
   Float64 d7s = GetDimension(dimensions,_T("D7_Start"));
   Float64 d7e = GetDimension(dimensions,_T("D7_End"));

   bool bPrismatic = true;
   if ( IsEqual(d7s,d7e) )
   {
      // prismatic
      bPrismatic = true;
   }
   else
   {
      // non-prismatic
      bPrismatic = false;
   }

   return bPrismatic;
}

bool CTaperedIBeamFactory::IsPrismatic(const CSegmentKey& segmentKey) const
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

bool CTaperedIBeamFactory::IsSymmetric(const CSegmentKey& segmentKey) const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();

   Float64 d7s = GetDimension(dimensions,_T("D7_Start"));
   Float64 d7e = GetDimension(dimensions,_T("D7_End"));

   bool bSymmetric = true;
   if ( IsEqual(d7s,d7e) )
   {
      // symmetric
      bSymmetric = true;
   }
   else
   {
      // non-symmetric
      bSymmetric = false;
   }

   return bSymmetric;
}

Float64 CTaperedIBeamFactory::GetInternalSurfaceAreaOfVoids(IBroker* pBroker,const CSegmentKey& segmentKey) const
{
   return 0;
}

std::_tstring CTaperedIBeamFactory::GetImage() const
{
   return std::_tstring(_T("TaperedIBeam.jpg"));
}

std::_tstring CTaperedIBeamFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
      strImage =  _T("IBeam_Slab_CIP.gif");
      break;

   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("IBeam_Slab_SIP.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CTaperedIBeamFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("+Mn_IBeam_Composite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CTaperedIBeamFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("-Mn_IBeam_Composite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CTaperedIBeamFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("Vn_IBeam.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CTaperedIBeamFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("IBeam_Effective_Flange_Width_Interior_Girder_2008.gif");
   }
   else
   {
      return _T("IBeam_Effective_Flange_Width_Interior_Girder.gif");
   }
}

std::_tstring CTaperedIBeamFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("IBeam_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("IBeam_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID CTaperedIBeamFactory::GetCLSID() const
{
   return CLSID_TaperedIBeamFactory;
}

std::_tstring CTaperedIBeamFactory::GetName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

CLSID CTaperedIBeamFactory::GetFamilyCLSID() const
{
   return CLSID_WFBeamFamily;
}

std::_tstring CTaperedIBeamFactory::GetGirderFamilyName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CTaperedIBeamFactory::GetPublisher() const
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring CTaperedIBeamFactory::GetPublisherContactInformation() const
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE CTaperedIBeamFactory::GetResourceInstance() const
{
   return _Module.GetResourceInstance();
}

LPCTSTR CTaperedIBeamFactory::GetImageResourceName() const
{
   return _T("TAPEREDIBEAM");
}

HICON  CTaperedIBeamFactory::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_IBEAM) );
}

void CTaperedIBeamFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                  Float64& d1,Float64& d2,Float64& d3,Float64& d4,Float64& d5,Float64& d6,Float64& d7s,Float64& d7e,
                                  Float64& w1,Float64& w2,Float64& w3,Float64& w4,
                                  Float64& t1,Float64& t2, Float64& c1) const
{
   d1 = GetDimension(dimensions,_T("D1"));
   d2 = GetDimension(dimensions,_T("D2"));
   d3 = GetDimension(dimensions,_T("D3"));
   d4 = GetDimension(dimensions,_T("D4"));
   d5 = GetDimension(dimensions,_T("D5"));
   d6 = GetDimension(dimensions,_T("D6"));
   d7s = GetDimension(dimensions,_T("D7_Start"));
   d7e = GetDimension(dimensions,_T("D7_End"));
   w1 = GetDimension(dimensions,_T("W1"));
   w2 = GetDimension(dimensions,_T("W2"));
   w3 = GetDimension(dimensions,_T("W3"));
   w4 = GetDimension(dimensions,_T("W4"));
   t1 = GetDimension(dimensions,_T("T1"));
   t2 = GetDimension(dimensions,_T("T2"));
   c1 = GetDimension(dimensions,_T("C1")); 
}

Float64 CTaperedIBeamFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name) const
{
   for (const auto& dim : dimensions)
   {
      if (name == dim.first)
      {
         return dim.second;
      }
   }

   ATLASSERT(false); // should never get here
   return -99999;
}

pgsTypes::SupportedDeckTypes CTaperedIBeamFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const
{
   pgsTypes::SupportedDeckTypes sdt;
   switch(sbs)
   {
   case pgsTypes::sbsUniform:
   case pgsTypes::sbsGeneral:
      sdt.push_back(pgsTypes::sdtCompositeCIP);
      sdt.push_back(pgsTypes::sdtCompositeSIP);
      break;

   default:
      ATLASSERT(false);
   }
   return sdt;
}

pgsTypes::SupportedBeamSpacings CTaperedIBeamFactory::GetSupportedBeamSpacings() const
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);

   return sbs;
}

bool CTaperedIBeamFactory::IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::SupportedBeamSpacings sbs = GetSupportedBeamSpacings();
   auto found = std::find(sbs.cbegin(), sbs.cend(), spacingType);
   return found == sbs.end() ? false : true;
}

bool CTaperedIBeamFactory::ConvertBeamSpacing(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const
{
   return false;
}

std::vector<pgsTypes::GirderOrientationType> CTaperedIBeamFactory::GetSupportedGirderOrientation() const
{
   std::vector<pgsTypes::GirderOrientationType> types{ pgsTypes::Plumb/*, pgsTypes::StartNormal,pgsTypes::MidspanNormal,pgsTypes::EndNormal*/ };
   return types;
}

bool CTaperedIBeamFactory::IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return orientation == pgsTypes::Plumb ? true : false;
}

pgsTypes::GirderOrientationType CTaperedIBeamFactory::ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return pgsTypes::Plumb;
}

pgsTypes::SupportedDiaphragmTypes CTaperedIBeamFactory::GetSupportedDiaphragms() const
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes CTaperedIBeamFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const
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

void CTaperedIBeamFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   Float64 T1 = GetDimension(dimensions,_T("T1"));
   Float64 T2 = GetDimension(dimensions,_T("T2"));
   Float64 W1 = GetDimension(dimensions,_T("W1"));
   Float64 W2 = GetDimension(dimensions,_T("W2"));
   Float64 W3 = GetDimension(dimensions,_T("W3"));
   Float64 W4 = GetDimension(dimensions,_T("W4"));

   Float64 top_w = T1 + 2.0*(W1+W2);
   Float64 bot_w = T2 + 2.0*(W3+W4);

   Float64 gw = Max(top_w, bot_w);


   if ( sdt == pgsTypes::sdtCompositeCIP || sdt == pgsTypes::sdtCompositeSIP )
   {
      if(sbs == pgsTypes::sbsUniform || sbs == pgsTypes::sbsGeneral)
      {
         *minSpacing = gw;
         *maxSpacing = MAX_GIRDER_SPACING;
      }
      else
      {
         ATLASSERT(false); // shouldn't get here
      }
   }
   else
   {
      ATLASSERT(false); // shouldn't get here
   }
}

WebIndexType CTaperedIBeamFactory::GetWebCount(const IBeamFactory::Dimensions& dimensions) const
{
   return 1;
}

Float64 CTaperedIBeamFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   Float64 D1 = GetDimension(dimensions,_T("D1"));
   Float64 D2 = GetDimension(dimensions,_T("D2"));
   Float64 D3 = GetDimension(dimensions,_T("D3"));
   Float64 D4 = GetDimension(dimensions,_T("D4"));
   Float64 D5 = GetDimension(dimensions,_T("D5"));
   Float64 D6 = GetDimension(dimensions,_T("D6"));
   Float64 D7_start = GetDimension(dimensions,_T("D7_Start"));
   Float64 D7_end   = GetDimension(dimensions,_T("D7_End"));

   return D1 + D2 + D3 + D4 + D5 + D6 + (endType == pgsTypes::metStart ? D7_start : D7_end);
}

Float64 CTaperedIBeamFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   Float64 W1 = GetDimension(dimensions,_T("W1"));
   Float64 W2 = GetDimension(dimensions,_T("W2"));
   Float64 W3 = GetDimension(dimensions,_T("W3"));
   Float64 W4 = GetDimension(dimensions,_T("W4"));
   Float64 T1 = GetDimension(dimensions,_T("T1"));
   Float64 T2 = GetDimension(dimensions,_T("T2"));

   Float64 top = 2*(W1+W2) + T1;
   Float64 bot = 2*(W3+W4) + T2;

   return Max(top,bot);
}

bool CTaperedIBeamFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const
{
   return false;
}

void CTaperedIBeamFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

bool CTaperedIBeamFactory::HasLongitudinalJoints() const
{
   return false;
}

bool CTaperedIBeamFactory::IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const
{
   return false;
}

bool CTaperedIBeamFactory::HasTopFlangeThickening() const
{
   return false;
}

bool CTaperedIBeamFactory::CanPrecamber() const
{
   return false;
}

GirderIndexType CTaperedIBeamFactory::GetMinimumBeamCount() const
{
   return 2;
}
