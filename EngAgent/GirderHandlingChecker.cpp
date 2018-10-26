///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include <IFace\PointOfInterest.h>
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
                                              const CSegmentKey& segmentKey,
                                              IntervalIndexType intervalIdx,
                                              Float64 leftOH,Float64 glen,Float64 rightOH,
                                              Float64 E,
                                              PoiAttributeType poiReference,
                                              const std::vector<pgsPointOfInterest>& rpoiVec,
                                              std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection)
{
   CComPtr<IFem2dModel> pModel;
   pgsPoiMap poiMap;

   // need left and right support locations measured from the left end of the girder
   Float64 leftSupportLocation = leftOH;
   Float64 rightSupportLocation = glen - rightOH;
   LoadCaseIDType lcid = 0;
   pGirderModelFactory->CreateGirderModel(pBroker,intervalIdx,segmentKey,leftSupportLocation,rightSupportLocation,E,lcid,true,rpoiVec,&pModel,&poiMap);

   // Get results
   CComQIPtr<IFem2dModelResults> results(pModel);
   pmomVec->clear();
   *pMidSpanDeflection = 0.0;
   bool found_mid = false;

   Float64 dx,dy,rz;
   std::vector<pgsPointOfInterest>::const_iterator poiIter(rpoiVec.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(rpoiVec.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi = *poiIter;
      Float64 fx,fy,mz;
      PoiIDType femPoiID = poiMap.GetModelPoi(poi);
      HRESULT hr = results->ComputePOIForces(lcid,femPoiID,mftLeft,lotMember,&fx,&fy,&mz);
      ATLASSERT(SUCCEEDED(hr));
      pmomVec->push_back(mz);


      if (poi.IsMidSpan(poiReference))
      {
         ATLASSERT(found_mid == false);
         // poi should be at the half-way point between the supports
         ATLASSERT( IsEqual(poi.GetDistFromStart(),leftOH + (glen-leftOH-rightOH)/2) );

         hr = results->ComputePOIDeflections(lcid,femPoiID,lotMember,&dx,&dy,&rz);
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

pgsAlternativeTensileStressCalculator::pgsAlternativeTensileStressCalculator(const CSegmentKey& segmentKey, IntervalIndexType intervalIdx,
                                                                             IBridge* pBridge,IGirder* pGirder,
                                         IShapes* pShapes,ISectionProperties* pSectProps, ILongRebarGeometry* pRebarGeom,
                                         IMaterials* pMaterials,bool bLimitBarStress,
                                         bool bSISpec,bool bGirderStresses) :
m_pBridge(pBridge),
m_pGirder(pGirder),
m_pShapes(pShapes),
m_pSectProps(pSectProps),
m_pRebarGeom(pRebarGeom),
m_pMaterials(pMaterials)
{
   m_IntervalIdx = intervalIdx;
   m_bLimitBarStress = bLimitBarStress;
   m_bSISpec = bSISpec;
   m_bGirderStresses = bGirderStresses;
}

void pgsAlternativeTensileStressCalculator::LimitBarStress(bool bLimit)
{
   m_bLimitBarStress = bLimit;
}

bool pgsAlternativeTensileStressCalculator::LimitBarStress() const
{
   return m_bLimitBarStress;
}

Float64 pgsAlternativeTensileStressCalculator::ComputeAlternativeStressRequirements(
                                        const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,
                                        Float64 fTop, Float64 fBot, 
                                        Float64 fAllowableWithoutRebar, Float64 fAllowableWithRebar,
                                        Float64 *pYna, Float64 *pAreaTens, Float64 *pT, 
                                        Float64 *pAsProvd, Float64 *pAsReqd, bool* pIsAdequateRebar)
{
#pragma Reminder("UDPATE: need to consider which component has tension... girder or deck")
   // fTop and fBot could be for the girder or for the deck
   // if for the deck, deck properties and deck steel is needed. 
   // this method doesn't do that yet (see case for slAllTens... uses girder area only)

   // Determine neutral axis location and mild steel requirement for alternative tensile stress
   typedef enum {slAllTens, slAllComp, slTopTens, slBotTens} StressLocation;
   StressLocation stressLoc;
   Float64 Yna = -1;

   Float64 H;
   if ( m_bGirderStresses )
   {
      H = m_pGirder->GetHeight(poi);
   }
   else
   {
      H = m_pBridge->GetStructuralSlabDepth(poi);
   }

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // Determine bar stress
   Float64 Es, fy, fu;
   if ( m_bGirderStresses )
   {
      if ( poi.HasAttribute(POI_CLOSURE) )
      {
         m_pMaterials->GetClosureJointLongitudinalRebarProperties(segmentKey,&Es,&fy,&fu);
      }
      else
      {
         m_pMaterials->GetSegmentLongitudinalRebarProperties(segmentKey,&Es,&fy,&fu);
      }
   }
   else
   {
      m_pMaterials->GetDeckRebarProperties(&Es,&fy,&fu);
   }

   // Max bar stress for computing higher allowable temporary tensile (5.9.4.1.2)
   Float64 allowable_bar_stress = 0.5*fy;
   Float64 fsMax = (m_bSISpec ? ::ConvertToSysUnits(206.0,unitMeasure::MPa) : ::ConvertToSysUnits(30.0,unitMeasure::KSI) );
   if ( m_bLimitBarStress && fsMax < allowable_bar_stress )
   {
       allowable_bar_stress = fsMax;
   }

   if ( fTop <= TOLERANCE && fBot <= TOLERANCE )
   {
      // compression over entire cross section
      stressLoc = slAllComp;
   }
   else if ( 0.0 <= fTop && 0.0 <= fBot )
   {
       // tension over entire cross section
      stressLoc = slAllTens;
   }
   else
   {
      ATLASSERT( BinarySign(fBot) != BinarySign(fTop) );

      stressLoc = 0.0 <= fBot ? slBotTens : slTopTens;

      // Location of neutral axis from Bottom of Girder/Deck
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
      if ( m_bGirderStresses )
      {
         AreaTens = m_pSectProps->GetAg(m_IntervalIdx,poi);
      }
      else
      {
         AreaTens = m_pSectProps->GetTributaryDeckArea(poi);
      }

       Float64 fAvg = (fTop + fBot)/2;
       T = fAvg * AreaTens;

       ATLASSERT( T != 0 );
   }
   else
   {
      if (m_bGirderStresses )
      {
         // Clip shape to determine concrete tension area
         CComPtr<IShape> shape;
         m_pShapes->GetSegmentShape(m_IntervalIdx,poi,false,pgsTypes::scGirder,&shape);

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
      }
      else
      {
         Float64 fAvg;
         Float64 Weff = m_pSectProps->GetTributaryFlangeWidth(poi);
         if ( stressLoc == slTopTens )
         {
            AreaTens = (H-Yna)*Weff;
            fAvg = fTop/2;
         }
         else
         {
            AreaTens = Yna*Weff;
            fAvg = fBot/2;
         }

         T = fAvg * AreaTens;
      }


      ATLASSERT( T != 0 );
   }

   // Area of steel required to meet higher tensile stress requirement
   Float64 AsReqd = T/allowable_bar_stress;
   ATLASSERT( 0 <= AsReqd );

// This will need to be revisited if we start designing longitudinal rebar
#pragma Reminder("This function assumes that longitudinal rebar does not change during design")

   // Compute area of rebar actually provided in tension zone. Reduce values for development
   Float64 AsProvd = 0.0; // As provided
   if ( stressLoc != slAllComp )
   {
      if (m_bGirderStresses )
      {
         CComPtr<IRebarSection> rebar_section;
         m_pRebarGeom->GetRebars(poi,&rebar_section);

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
            if ( poi.HasAttribute(POI_CLOSURE) )
            {
               fci       = m_pMaterials->GetClosureJointFc(segmentKey,m_IntervalIdx);
               conc_type = m_pMaterials->GetClosureJointConcreteType(segmentKey);
               isfct     = m_pMaterials->DoesClosureJointConcreteHaveAggSplittingStrength(segmentKey);
               fct       = isfct ? m_pMaterials->GetClosureJointConcreteAggSplittingStrength(segmentKey) : 0.0;
            }
            else
            {
               fci       = m_pMaterials->GetSegmentFc(segmentKey,m_IntervalIdx);
               conc_type = m_pMaterials->GetSegmentConcreteType(segmentKey);
               isfct     = m_pMaterials->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
               fct       = isfct ? m_pMaterials->GetSegmentConcreteAggSplittingStrength(segmentKey) : 0.0;
            }
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

            if ( IsGE(1.0,dev_length_factor) ) // Bars must be fully developed before higher 
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
                  location->get_Y(&y); // measured from top of girder in Girder Section Coordinates
                  // need y to be measured from bottom of girder
                  y += H;

                  // Add bar if it's on right side of NA
                  if ( stressLoc == slTopTens && Yna < y)
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
      else
      {
         // deck stresses
#pragma Reminder("UPDATE: need to account for development length")
         if ( stressLoc == slAllTens )
         {
            AsProvd += m_pRebarGeom->GetAsTopMat(poi,ILongRebarGeometry::All);
            AsProvd += m_pRebarGeom->GetAsBottomMat(poi,ILongRebarGeometry::All);
         }
         else if ( stressLoc == slTopTens )
         {
            if ( Yna <= m_pRebarGeom->GetTopMatLocation(poi,ILongRebarGeometry::All) )
            {
               AsProvd += m_pRebarGeom->GetAsTopMat(poi,ILongRebarGeometry::All);
            }

            if ( Yna <= m_pRebarGeom->GetBottomMatLocation(poi,ILongRebarGeometry::All) )
            {
               AsProvd += m_pRebarGeom->GetAsBottomMat(poi,ILongRebarGeometry::All);
            }
         }
         else if ( stressLoc == slBotTens )
         {
            if ( m_pRebarGeom->GetTopMatLocation(poi,ILongRebarGeometry::All) <= Yna )
            {
               AsProvd += m_pRebarGeom->GetAsTopMat(poi,ILongRebarGeometry::All);
            }

            if ( m_pRebarGeom->GetBottomMatLocation(poi,ILongRebarGeometry::All) <= Yna )
            {
               AsProvd += m_pRebarGeom->GetAsBottomMat(poi,ILongRebarGeometry::All);
            }
         }
      }
   }

   // Now we can determine which allowable we can use
   Float64 fAllowable;
   if (AsReqd < AsProvd)
   {
      fAllowable = fAllowableWithRebar;
      *pIsAdequateRebar = true;
   }
   else
   {
      fAllowable = fAllowableWithoutRebar;
      *pIsAdequateRebar = false;
   }

   *pYna      = Yna;
   *pAreaTens = AreaTens;
   *pT        = T;
   *pAsProvd  = AsProvd;
   *pAsReqd   = AsReqd;

   return fAllowable;
}

void pgsAlternativeTensileStressCalculator::ComputeReqdFcTens(Float64 ft, // stress demand
                       Float64 rcsT, bool rcsBfmax, Float64 rcsFmax, Float64 rcsTalt, // allowable stress coeff's
                       Float64* pFcNo,Float64* pFcWithRebar)
{
   if ( 0 < ft )
   {
      // Without rebar
      if ( rcsBfmax && (rcsFmax < ft) )
      {
         // allowable stress is limited and we hit the limit
         *pFcNo = -1;
      }
      else
      {
         *pFcNo = pow(ft/rcsT,2);
      }

      // With rebar
      *pFcWithRebar = pow(ft/rcsTalt,2);

   }
   else
   {
      // Compression
      *pFcNo = 0.0;
      *pFcWithRebar = 0.0;
   }
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
       pSectProp2->GetGirderShape(poi,false,&shape);

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