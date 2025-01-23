///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#include "StdAfx.h"

#include <IFace\PointOfInterest.h>
#include <IFace\StatusCenter.h>
#include <IFace\GirderHandling.h>
#include <IFace\PrestressForce.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\GirderHandlingSpecCriteria.h>

#include <EAF\EAFDisplayUnits.h>

#include <PGSuperException.h>

#include <PgsExt\StatusItem.h>
#include <PgsExt\GirderModelFactory.H>

#include "GirderHandlingChecker.h"
#include "WsdotGirderHaulingChecker.h"
#include "KdotGirderHaulingChecker.h"

#include "StatusItems.h"
#include "PGSuperUnits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   pgsGirderHandlingChecker
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////


//======================== LIFECYCLE  =======================================
pgsGirderHandlingChecker::pgsGirderHandlingChecker(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;
}

pgsGirderHandlingChecker::~pgsGirderHandlingChecker()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================

pgsGirderHaulingChecker* pgsGirderHandlingChecker::CreateGirderHaulingChecker()
{
   GET_IFACE(ISegmentHaulingSpecCriteria,pSpec);

   pgsTypes::HaulingAnalysisMethod method = pSpec->GetHaulingAnalysisMethod();

   if (method==pgsTypes::HaulingAnalysisMethod::WSDOT)
   {
      return new pgsWsdotGirderHaulingChecker(m_pBroker, m_StatusGroupID);
   }
   else
   {
      return new pgsKdotGirderHaulingChecker(m_pBroker, m_StatusGroupID);
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================


void pgsGirderHandlingChecker::ComputeMoments(IBroker* pBroker, pgsGirderModelFactory* pGirderModelFactory,
                                              const CSegmentKey& segmentKey,
                                              IntervalIndexType intervalIdx,
                                              Float64 leftOH,Float64 glen,Float64 rightOH,
                                              Float64 E,
                                              PoiAttributeType poiReference,
                                              const PoiList& rpoiVec,
                                              std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection)
{
   CComPtr<IFem2dModel> pModel;
   pgsPoiPairMap poiMap;

   // need left and right support locations measured from the left end of the girder
   Float64 leftSupportLocation = leftOH;
   Float64 rightSupportLocation = glen - rightOH;
   LoadCaseIDType lcid = 0;
   pGirderModelFactory->CreateGirderModel(pBroker,intervalIdx,segmentKey,leftSupportLocation,rightSupportLocation,glen,E,lcid,true,true,rpoiVec,&pModel,&poiMap);

   // Get results
   CComQIPtr<IFem2dModelResults> results(pModel);
   pmomVec->clear();
   *pMidSpanDeflection = 0.0;
   bool found_mid = false;

   Float64 dx,dy,rz;
   for ( const pgsPointOfInterest& poi : rpoiVec)
   {
      Float64 fx,fy,mz;
      PoiIDPairType femPoiID = poiMap.GetModelPoi(poi);
      HRESULT hr = results->ComputePOIForces(lcid,femPoiID.first,mftLeft,lotMember,&fx,&fy,&mz);
      ATLASSERT(SUCCEEDED(hr));
      pmomVec->push_back(mz);


      if (poi.IsMidSpan(poiReference))
      {
         ATLASSERT(found_mid == false);
         // poi should be at the half-way point between the supports
         ATLASSERT( IsEqual(poi.GetDistFromStart(),leftOH + (glen-leftOH-rightOH)/2,0.001) );

         hr = results->ComputePOIDeflections(lcid,femPoiID.first,lotMember,&dx,&dy,&rz);
         ATLASSERT(SUCCEEDED(hr));

         *pMidSpanDeflection = dy;
         found_mid = true;
      }
   }

   ATLASSERT(found_mid); // must have a point at mid-span for calc to work right
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The function below computes alternate tensile stress for the biaxial case. This is no longer used because inclined
// girders do not use the alternate increased tensile stress. They only use the fracture modulus.
// If we change our mind some day, this function will be useful.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
void pgsGirderHandlingChecker::GetRequirementsForAlternativeTensileStress(const pgsPointOfInterest& poi,Float64 ftu,Float64 ftd,Float64 fbu,Float64 fbd,Float64* pY,Float64* pA,Float64* pT,Float64* pAs)
{
    GET_IFACE(IGirder,pGirder);
    GET_IFACE(ISectProp2,pSectProps);
    GET_IFACE(IBridgeMaterial,pMaterial);

    GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
    bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

    Float64 Es, fs, fu;
    pMaterial->GetLongitudinalRebarProperties(span,gdr,&Es,&fs,&fu);
    fs *= 0.5;

    Float64 fsMax = (bUnitsSI ? WBFL::Units::ConvertToSysUnits(206.0,WBFL::Units::Measure::MPa) : WBFL::Units::ConvertToSysUnits(30.0,WBFL::Units::Measure::KSI) );
    if ( fsMax < fs )
       fs = fsMax;

    // Determine As requirement for alternative allowable tensile stress
    Float64 H = pGirder->GetHeight(poi);

    Float64 fTop = (ftu + ftd)/2;
    Float64 fBot = (fbu + fbd)/2;

    Float64 T = 0;
    Float64 At = 0;
    Float64 As = 0;
    Float64 Yna = -1;  // < 0 means it is not on the cross section
    if ( IsLE(fTop,0.) && IsLE(fBot,0.) )
    {
       // compression over entire cross section
       T = 0;
    }
    else if ( IsLE(0.,fTop) && IsLE(0.,fBot) )
    {
       // tension over entire cross section
       At = pSectProps->GetAg(pgsTypes::CastingYard,poi);
       Float64 fAvg = (fTop + fBot)/2;
       T = fAvg * At;
    }
    else
    {
       // Location of neutral axis from Bottom of Girder
//       Yna = (IsZero(fBot) ? 0 : H - (fTop*H/(fTop-fBot)) );

       CComPtr<IShape> shape;
       pSectProps->GetGirderShape(poi,false,&shape);

       CComQIPtr<IXYPosition> position(shape);
       CComPtr<IPoint2d> tc,bc;
       position->get_LocatorPoint(lpTopCenter,   &tc);
       position->get_LocatorPoint(lpBottomCenter,&bc);

       Float64 Xtop,Ytop;
       Float64 Xbot,Ybot;
       tc->get_X(&Xtop);
       tc->get_Y(&Ytop);

       bc->get_X(&Xbot);
       bc->get_Y(&Ybot);

       Float64 Wt, Wb;
       Wt = pGirder->GetTopWidth(poi);
       Wb = pGirder->GetBottomWidth(poi);

       // create a 3D plane to represent the stress plane
       CComPtr<IPlane3d> plane;
       plane.CoCreateInstance(CLSID_Plane3d);
       CComPtr<IPoint3d> p1,p2,p3;
       p1.CoCreateInstance(CLSID_Point3d);
       p2.CoCreateInstance(CLSID_Point3d);
       p3.CoCreateInstance(CLSID_Point3d);

       p1->Move( Xtop+Wt/2,Ytop,ftu);
       p2->Move( Xtop-Wt/2,Ytop,ftd);
       p3->Move( Xbot+Wb/2,Ybot,fbu);

       plane->ThroughPoints(p1,p2,p3);

       // Determine neutral axis line by finding two points where z(stress) is zero
       Float64 ya,yb;
       plane->GetY(-10000,0.00,&ya);
       plane->GetY( 10000,0.00,&yb);

       CComPtr<IPoint2d> pa,pb;
       pa.CoCreateInstance(CLSID_Point2d);
       pb.CoCreateInstance(CLSID_Point2d);
       pa->Move(-10000,ya);
       pb->Move( 10000,yb);

       CComPtr<ILine2d> line;
       line.CoCreateInstance(CLSID_Line2d);

       Float64 fAvg;

       // line clips away left hand side
       if ( 0 <= fTop && fBot <= 0 )
       {
           // Tension top, compression bottom
           // line needs to go right to left
          line->ThroughPoints(pb,pa);

          fAvg = fTop / 2;
       }
       else if ( fTop <= 0 && 0 <= fBot )
       {
           // Tension bottom
           // line needs to go left to right
          line->ThroughPoints(pa,pb);

          fAvg = fBot / 2;
       }

       CComPtr<IShape> clipped_shape;
       shape->ClipWithLine(line,&clipped_shape);

       CComPtr<IShapeProperties> props;
       clipped_shape->get_ShapeProperties(&props);

       props->get_Area(&At);

       T = fAvg * At;
    }

    As = T/fs;

    *pY = Yna;
    *pA = At;
    *pT = T;
    *pAs = As;
}
*/