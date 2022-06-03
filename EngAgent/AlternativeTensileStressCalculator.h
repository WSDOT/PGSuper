///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#pragma once
#include <PgsExt\PgsExt.h>

interface IBridge;
interface IGirder;
interface IShapes;
interface ISectionProperties;
interface ILongRebarGeometry;
interface IMaterials;
interface IPointOfInterest;

/*****************************************************************************
CLASS 
   pgsAlternativeTensileStressCalculator


DESCRIPTION
   Utility class for dealing with alternative tensile stress in casting yard and at lifting

LOG
   rdp : 03.25.2013 : Created file
*****************************************************************************/
class pgsAlternativeTensileStressCalculator
{
public:
   // GROUP: LIFECYCLE
   //------------------------------------------------------------------------
   // Constructor
   pgsAlternativeTensileStressCalculator(const CSegmentKey& segmentKey, IntervalIndexType intervalIdx,IBridge* pBridge,IGirder* pGirder,
                                         IShapes* pShapes,ISectionProperties* pSectProps, ILongRebarGeometry* pRebarGeom,
                                         IMaterials* pMaterials,IPointOfInterest* pPoi,bool bLimitBarStress,Float64 fsMax,
                                         bool bGirderStresses);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsAlternativeTensileStressCalculator()
   {;}

   // if true, the stress in the mild reinforcement is limited to the bar stress limit, otherwise it is not.
   void LimitBarStress(bool bLimit);
   bool LimitBarStress() const;

   // set/get the bar stress limit
   void SetBarStressLimit(Float64 fsMax);
   Float64 GetBarStressLimit() const;

   Float64 ComputeAlternativeStressRequirements(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,
                                                Float64 fTop, Float64 fBot, 
                                                Float64 fAllowableWithoutRebar, Float64 fAllowableWithRebar,
                                                Float64 *pYna, Float64 *pAreaTens, Float64 *pT, 
                                                Float64 *pAsProvd, Float64 *pAsReqd, bool* pIsAdequateRebar);

   static void ComputeReqdFcTens(const CSegmentKey& segmentKey,Float64 ft, // stress demand
                          Float64 rcsT, bool rcsBfmax, Float64 rcsFmax, Float64 rcsTalt, // allowable stress coeff's
                          Float64* pFcNo,Float64* pFcWithRebar);

private:
   pgsAlternativeTensileStressCalculator(); // no default constructor

   // GROUP: DATA MEMBERS

   // these are weak references
   IBridge* m_pBridge;
   IGirder* m_pGirder;
   IShapes* m_pShapes;
   ISectionProperties* m_pSectProps;
   ILongRebarGeometry* m_pRebarGeom;
   IMaterials* m_pMaterials;
   IPointOfInterest* m_pPoi;

   bool m_bLimitBarStress;
   Float64 m_fsMax;

   bool m_bGirderStresses;
   IntervalIndexType m_IntervalIdx;
};
