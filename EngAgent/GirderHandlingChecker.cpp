///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <PgsExt\PgsExtLib.h>

#include <IFace\GirderHandlingPointOfInterest.h>
#include <IFace\Bridge.h>
#include <IFace\StatusCenter.h>
#include <IFace\GirderHandling.h>
#include <IFace\PrestressForce.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\GirderHandlingSpecCriteria.h>

#include <EAF\EAFDisplayUnits.h>

#include "..\PGSuperException.h"

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
   GET_IFACE(IGirderHaulingSpecCriteria,pSpec);

   pgsTypes::HaulingAnalysisMethod method = pSpec->GetHaulingAnalysisMethod();

   if (method==pgsTypes::hmWSDOT)
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

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsGirderHandlingChecker::AssertValid() const
{
//#pragma Reminder("TODO: Implement the AssertValid method for pgsGirderHandlingChecker")
   return true;
}

void pgsGirderHandlingChecker::Dump(dbgDumpContext& os) const
{
//#pragma Reminder("TODO: Implement the Dump method for pgsGirderHandlingChecker")
   os << "Dump for pgsGirderHandlingChecker" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsGirderHandlingChecker::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsGirderHandlingChecker");

//#pragma Reminder("TODO: Implement the TestMe method for pgsGirderHandlingChecker")
   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsGirderHandlingChecker");

   TESTME_EPILOG("GirderHandlingChecker");
}
#endif // _UNITTEST


void pgsGirderHandlingChecker::ComputeMoments(IBroker* pBroker, pgsGirderModelFactory* pGirderModelFactory,
                                              SpanIndexType span,GirderIndexType gdr,
                                              pgsTypes::Stage stage,
                                              Float64 leftOH,Float64 glen,Float64 rightOH,
                                              Float64 E,
                                              const std::vector<pgsPointOfInterest>& rpoiVec,
                                              std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection)
{
   CComPtr<IFem2dModel> pModel;
   pgsPoiMap poiMap;

   // need left and right support locations measured from the left end of the girder
   Float64 leftSupportLocation = leftOH;
   Float64 rightSupportLocation = glen - rightOH;
   LoadCaseIDType lcid = 0;
   pGirderModelFactory->CreateGirderModel(pBroker,span,gdr,leftSupportLocation,rightSupportLocation,E,lcid,true,rpoiVec,&pModel,&poiMap);

   // Get results
   CComQIPtr<IFem2dModelResults> results(pModel);
   pmomVec->clear();
   *pMidSpanDeflection = 0.0;
   bool found_mid = false;

   Float64 dx,dy,rz;
   for (std::vector<pgsPointOfInterest>::const_iterator poiIter = rpoiVec.begin(); poiIter != rpoiVec.end(); poiIter++)
   {
      const pgsPointOfInterest& poi = *poiIter;
      Float64 fx,fy,mz;
      PoiIDType femPoiID = poiMap.GetModelPoi(poi);
      HRESULT hr = results->ComputePOIForces(0,femPoiID,mftLeft,lotMember,&fx,&fy,&mz);
      ATLASSERT(SUCCEEDED(hr));
      pmomVec->push_back(mz);


      if (poi.IsMidSpan(stage))
      {
         ATLASSERT(found_mid == false);
         ATLASSERT( IsEqual(poi.GetDistFromStart(),glen/2) );

         hr = results->ComputePOIDisplacements(lcid,femPoiID,lotMember,&dx,&dy,&rz);
         ATLASSERT(SUCCEEDED(hr));

         *pMidSpanDeflection = dy;
         found_mid = true;
      }
   }

   ATLASSERT(found_mid); // must have a point at mid-span for calc to work right
}


/****************************************************************************
CLASS
   pgsAlternativeTensileStressCalculator
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

pgsAlternativeTensileStressCalculator::pgsAlternativeTensileStressCalculator(SpanIndexType span,GirderIndexType gdr, IGirder* pGirder,
                                         ISectProp2* pSectProp2, ILongRebarGeometry* pRebarGeom,
                                         IBridgeMaterial* pMaterial, IBridgeMaterialEx* pMaterialEx, bool bSIUnits):
m_pGirder(pGirder),
m_pSectProp2(pSectProp2),
m_pRebarGeom(pRebarGeom),
m_pMaterial(pMaterial),
m_pMaterialEx(pMaterialEx)
{
   // Precompute some values
   Float64 Es, fy, fu;
   pMaterial->GetLongitudinalRebarProperties(span,gdr,&Es,&fy,&fu);

   // Max bar stress for computing higher allowable temporary tensile (5.9.4.1.2)
   Float64 fs = 0.5*fy;
   Float64 fsMax = (bSIUnits ? ::ConvertToSysUnits(206.0,unitMeasure::MPa) : ::ConvertToSysUnits(30.0,unitMeasure::KSI) );
   if ( fsMax < fs )
       fs = fsMax;

   m_AllowableFs = fs; 
}

Float64 pgsAlternativeTensileStressCalculator::ComputeAlternativeStressRequirements(
                                        const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,
                                        Float64 fTop, Float64 fBot, 
                                        Float64 lowAllowTens, Float64 highAllowTens,
                                        Float64 *pYna, Float64 *pAreaTens, Float64 *pT, 
                                        Float64 *pAsProvd, Float64 *pAsReqd, bool* pIsAdequateRebar)
{
   // Determine neutral axis location and mild steel requirement for alternative tensile stress
   typedef enum {slAllTens, slAllComp, slTopTens, slBotTens} StressLocation;
   StressLocation stressLoc;
   Float64 Yna = -1;
   Float64 H = m_pGirder->GetHeight(poi);

   if ( fTop <= TOLERANCE && fBot <= TOLERANCE )
   {
      // compression over entire cross section
      stressLoc = slAllComp;
   }
   else if ( 0 <= fTop && 0 <= fBot )
   {
       // tension over entire cross section
      stressLoc = slAllTens;
   }
   else
   {
      ATLASSERT( BinarySign(fBot) != BinarySign(fTop) );

      stressLoc = fBot>0.0 ? slBotTens : slTopTens;

      // Location of neutral axis from Bottom of Girder
      Yna = (IsZero(fBot) ? 0 : H - (fTop*H/(fTop-fBot)) );

      ATLASSERT( 0 <= Yna );
   }

   // Compute area on concrete in tension and total tension
   Float64 AreaTens; // area of concrete in tension
   Float64 T;        // tension
   if ( stressLoc == slAllComp )
   {
       // Compression over entire cross section
      AreaTens = 0.0;
      T = 0.0;
   }
   else if ( stressLoc == slAllTens )
   {
       // Tension over entire cross section
       AreaTens = m_pSectProp2->GetAg(pgsTypes::CastingYard,poi);
       Float64 fAvg = (fTop + fBot)/2;
       T = fAvg * AreaTens;

       ATLASSERT( T != 0 );
   }
   else
   {
      // Clip shape to determine concrete tension area
      CComPtr<IShape> shape;
      m_pSectProp2->GetGirderShape(poi,pgsTypes::CastingYard,false,&shape);

      CComQIPtr<IXYPosition> position(shape);
      CComPtr<IPoint2d> bc;
      position->get_LocatorPoint(lpBottomCenter,&bc);
      Float64 Y;
      bc->get_Y(&Y);

      CComPtr<ILine2d> line;
      line.CoCreateInstance(CLSID_Line2d);
      CComPtr<IPoint2d> p1, p2;
      p1.CoCreateInstance(CLSID_Point2d);
      p2.CoCreateInstance(CLSID_Point2d);
      p1->Move(-10000,Y+Yna);
      p2->Move( 10000,Y+Yna);

      Float64 fAvg;

      if ( stressLoc == slTopTens )
      {
          // Tension top, compression bottom
          // line needs to go right to left
         line->ThroughPoints(p2,p1);

         fAvg = fTop / 2;
      }
      else
      {
         // Compression Top, Tension Bottom
         // line needs to go left to right
         ATLASSERT(stressLoc==slBotTens);
         line->ThroughPoints(p1,p2);

         fAvg = fBot / 2;
      }

      CComPtr<IShape> clipped_shape;
      shape->ClipWithLine(line,&clipped_shape);

      if ( clipped_shape )
      {
         CComPtr<IShapeProperties> props;
         clipped_shape->get_ShapeProperties(&props);

         props->get_Area(&AreaTens);
      }
      else
      {
         AreaTens = 0.0;
      }

      T = fAvg * AreaTens;

      ATLASSERT( T != 0 );
   }

   // Area of steel required to meet higher tensile stress requirement
   Float64 AsReqd = T/m_AllowableFs;
   ATLASSERT( 0 <= AsReqd );

// This will need to be revisited if we start designing longitudinal rebar
#pragma Reminder("This function assumes that longitudinal rebar does not change during design")

   // Compute area of rebar actually provided in tension zone. Reduce values for development
   Float64 AsProvd = 0.0; // As provided
   if ( stressLoc != slAllComp )
   {
      CComPtr<IRebarSection> rebar_section;
      m_pRebarGeom->GetRebars(poi,&rebar_section);
      
      SpanIndexType span = poi.GetSpan();
      GirderIndexType gdr = poi.GetGirder();

      pgsTypes::ConcreteType conc_type;
      Float64 fci, fct;
      bool isfct;
      if (pConfig!=NULL)
      {
         fci       = pConfig->Fci;
         conc_type = pConfig->ConcType;
         isfct     = pConfig->bHasFct;
         fct       = pConfig->Fct;
      }
      else
      {
         fci       = m_pMaterial->GetFciGdr(span, gdr);
         conc_type = m_pMaterialEx->GetGdrConcreteType(span,gdr);
         isfct     = m_pMaterialEx->DoesGdrConcreteHaveAggSplittingStrength(span,gdr);
         fct       = isfct ? m_pMaterialEx->GetGdrConcreteAggSplittingStrength(span,gdr) : 0.0;
      }

      CComPtr<IEnumRebarSectionItem> enumItems;
      rebar_section->get__EnumRebarSectionItem(&enumItems);

      CComPtr<IRebarSectionItem> item;
      while ( enumItems->Next(1,&item,NULL) != S_FALSE )
      {
         CComPtr<IRebar> rebar;
         item->get_Rebar(&rebar);
         Float64 as;
         rebar->get_NominalArea(&as);

         Float64 dev_length_factor = m_pRebarGeom->GetDevLengthFactor(item, conc_type, fci, isfct, fct);

         if (dev_length_factor >= 1.0-1.0e-05) // Bars must be fully developed before higher 
                                               // allowable stress can be used.
                                               // Apply a small tolerance.
         {
            if (stressLoc == slAllTens)
            {
               // all bars in tension - just add
               AsProvd += as;
            }
            else
            {
               CComPtr<IPoint2d> location;
               item->get_Location(&location);

               Float64 x,y;
               location->get_X(&x);
               location->get_Y(&y);
               // Add bar if it's on right side of NA
               if ( stressLoc == slTopTens && y > Yna)
               {
                  AsProvd += as;
               }
               else if ( stressLoc == slBotTens && y < Yna)
               {
                  AsProvd += as;
               }
            }
         }

         item.Release();
      }
   }

   // Now we can determine which allowable we can use
   Float64 fAllowable;
   if (AsProvd > AsReqd)
   {
      fAllowable = highAllowTens;
      *pIsAdequateRebar = true;
   }
   else
   {
      fAllowable = lowAllowTens;
      *pIsAdequateRebar = false;
   }

   *pYna = Yna;
   *pAreaTens = AreaTens;
   *pT = T;
   *pAsProvd = AsProvd;
   *pAsReqd = AsReqd;

   return fAllowable;
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
    GET_IFACE(ISectProp2,pSectProp2);
    GET_IFACE(IBridgeMaterial,pMaterial);

    GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
    bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

    Float64 Es, fs, fu;
    pMaterial->GetLongitudinalRebarProperties(span,gdr,&Es,&fs,&fu);
    fs *= 0.5;

    Float64 fsMax = (bUnitsSI ? ::ConvertToSysUnits(206.0,unitMeasure::MPa) : ::ConvertToSysUnits(30.0,unitMeasure::KSI) );
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
       At = pSectProp2->GetAg(pgsTypes::CastingYard,poi);
       Float64 fAvg = (fTop + fBot)/2;
       T = fAvg * At;
    }
    else
    {
       // Location of neutral axis from Bottom of Girder
//       Yna = (IsZero(fBot) ? 0 : H - (fTop*H/(fTop-fBot)) );

       CComPtr<IShape> shape;
       pSectProp2->GetGirderShape(poi,pgsTypes::CastingYard,false,&shape);

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