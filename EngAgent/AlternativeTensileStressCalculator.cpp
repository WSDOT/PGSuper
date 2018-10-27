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

#include "StdAfx.h"
#include "AlternativeTensileStressCalculator.h"
#include <IFace\Bridge.h>

/****************************************************************************
CLASS
   pgsAlternativeTensileStressCalculator
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

pgsAlternativeTensileStressCalculator::pgsAlternativeTensileStressCalculator(const CSegmentKey& segmentKey, IntervalIndexType intervalIdx,
                                                                             IBridge* pBridge,IGirder* pGirder,
                                         IShapes* pShapes,ISectionProperties* pSectProps, ILongRebarGeometry* pRebarGeom,
                                         IMaterials* pMaterials,IPointOfInterest* pPoi,bool bLimitBarStress,Float64 fsMax,
                                         bool bGirderStresses) :
m_pBridge(pBridge),
m_pGirder(pGirder),
m_pShapes(pShapes),
m_pSectProps(pSectProps),
m_pRebarGeom(pRebarGeom),
m_pMaterials(pMaterials),
m_pPoi(pPoi)
{
   m_IntervalIdx = intervalIdx;
   m_bLimitBarStress = bLimitBarStress;
   m_fsMax = fsMax;
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

void pgsAlternativeTensileStressCalculator::SetBarStressLimit(Float64 fsMax)
{
   m_fsMax = fsMax;
}

Float64 pgsAlternativeTensileStressCalculator::GetBarStressLimit() const
{
   return m_fsMax;
}

Float64 pgsAlternativeTensileStressCalculator::ComputeAlternativeStressRequirements(
                                        const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,
                                        Float64 fTop, Float64 fBot, 
                                        Float64 fAllowableWithoutRebar, Float64 fAllowableWithRebar,
                                        Float64 *pYna, Float64 *pAreaTens, Float64 *pT, 
                                        Float64 *pAsProvd, Float64 *pAsReqd, bool* pIsAdequateRebar)
{
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
      CClosureKey closureKey;
      if ( m_pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         m_pMaterials->GetClosureJointLongitudinalRebarProperties(closureKey,&Es,&fy,&fu);
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
   if ( m_bLimitBarStress && m_fsMax < allowable_bar_stress )
   {
       allowable_bar_stress = m_fsMax;
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

   // NOTE: This function assumes that longitudinal rebar does not change during design.
   // This will need to be revisited if we start designing longitudinal rebar.

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
         if ( pConfig == nullptr )
         {
            CClosureKey closureKey;
            if ( m_pPoi->IsInClosureJoint(poi,&closureKey) )
            {
               fci       = m_pMaterials->GetClosureJointDesignFc(closureKey,m_IntervalIdx);
               conc_type = m_pMaterials->GetClosureJointConcreteType(closureKey);
               isfct     = m_pMaterials->DoesClosureJointConcreteHaveAggSplittingStrength(closureKey);
               fct       = isfct ? m_pMaterials->GetClosureJointConcreteAggSplittingStrength(closureKey) : 0.0;
            }
            else
            {
               fci       = m_pMaterials->GetSegmentDesignFc(segmentKey,m_IntervalIdx);
               conc_type = m_pMaterials->GetSegmentConcreteType(segmentKey);
               isfct     = m_pMaterials->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
               fct       = isfct ? m_pMaterials->GetSegmentConcreteAggSplittingStrength(segmentKey) : 0.0;
            }
         }
         else
         {
            fci       = pConfig->Fci;
            conc_type = pConfig->ConcType;
            isfct     = pConfig->bHasFct;
            fct       = pConfig->Fct;
         }

         CComPtr<IEnumRebarSectionItem> enumItems;
         rebar_section->get__EnumRebarSectionItem(&enumItems);

         CComPtr<IRebarSectionItem> item;
         while ( enumItems->Next(1,&item,nullptr) != S_FALSE )
         {
            CComPtr<IRebar> rebar;
            item->get_Rebar(&rebar);
            Float64 as;
            rebar->get_NominalArea(&as);

            // Adjust bar area for development
            Float64 dev_length_factor = m_pRebarGeom->GetDevLengthFactor(poi, item, conc_type, fci, isfct, fct);

            as *= dev_length_factor;

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

            item.Release();
         }
      }
      else
      {
         // deck stresses
#pragma Reminder("UPDATE: need to account for development length")
         if ( stressLoc == slAllTens )
         {
            AsProvd += m_pRebarGeom->GetAsTopMat(poi,pgsTypes::drbAll,pgsTypes::drcAll);
            AsProvd += m_pRebarGeom->GetAsBottomMat(poi,pgsTypes::drbAll,pgsTypes::drcAll);
         }
         else if ( stressLoc == slTopTens )
         {
            if ( Yna <= m_pRebarGeom->GetTopMatLocation(poi,pgsTypes::drbAll,pgsTypes::drcAll) )
            {
               AsProvd += m_pRebarGeom->GetAsTopMat(poi,pgsTypes::drbAll,pgsTypes::drcAll);
            }

            if ( Yna <= m_pRebarGeom->GetBottomMatLocation(poi,pgsTypes::drbAll,pgsTypes::drcAll) )
            {
               AsProvd += m_pRebarGeom->GetAsBottomMat(poi,pgsTypes::drbAll,pgsTypes::drcAll);
            }
         }
         else if ( stressLoc == slBotTens )
         {
            if ( m_pRebarGeom->GetTopMatLocation(poi,pgsTypes::drbAll,pgsTypes::drcAll) <= Yna )
            {
               AsProvd += m_pRebarGeom->GetAsTopMat(poi,pgsTypes::drbAll,pgsTypes::drcAll);
            }

            if ( m_pRebarGeom->GetBottomMatLocation(poi,pgsTypes::drbAll,pgsTypes::drcAll) <= Yna )
            {
               AsProvd += m_pRebarGeom->GetAsBottomMat(poi,pgsTypes::drbAll,pgsTypes::drcAll);
            }
         }
      }
   }

   // Now we can determine which allowable we can use
   Float64 fAllowable;
   if (AsReqd <= AsProvd)
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

void pgsAlternativeTensileStressCalculator::ComputeReqdFcTens(const CSegmentKey& segmentKey,Float64 ft, // stress demand
                       Float64 rcsT, bool rcsBfmax, Float64 rcsFmax, Float64 rcsTalt, // allowable stress coeff's
                       Float64* pFcNo,Float64* pFcWithRebar)
{
   if ( 0 < ft )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IMaterials,pMaterials);
      Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);

      // Without rebar
      if ( rcsBfmax && (rcsFmax < ft) )
      {
         // allowable stress is limited and we hit the limit
         *pFcNo = -99999;
      }
      else
      {
         *pFcNo = pow(ft/(lambda*rcsT),2);
      }

      // With rebar
      *pFcWithRebar = pow(ft/(lambda*rcsTalt),2);

   }
   else
   {
      // Compression
      *pFcNo = 0.0;
      *pFcWithRebar = 0.0;
   }
}
