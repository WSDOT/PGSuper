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

#ifndef INCLUDED_PGSEXT_GIRDERHANDLINGCHECKER_H_
#define INCLUDED_PGSEXT_GIRDERHANDLINGCHECKER_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#include <PgsExt\PgsExtExp.h>
#include <PgsExt\HaulingAnalysisArtifact.h>
#include <PgsExt\LiftingAnalysisArtifact.h>
#include <PgsExt\PoiMap.h>
#include <PgsExt\GirderModelFactory.h>

#include <IFace\GirderHandlingPointOfInterest.h>

#include <EAF\EAFUtilities.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// Virtual members of polymorphic hauling checker
class pgsGirderHaulingChecker
{
public:
   virtual pgsHaulingAnalysisArtifact* CheckHauling(SpanIndexType span,GirderIndexType gdr, SHARED_LOGFILE LOGFILE)=0;
   virtual pgsHaulingAnalysisArtifact* AnalyzeHauling(SpanIndexType span,GirderIndexType gdr)=0;
   virtual pgsHaulingAnalysisArtifact* AnalyzeHauling(SpanIndexType span,GirderIndexType gdr,const HANDLINGCONFIG& config,IGirderHaulingDesignPointsOfInterest* pPOId)=0;
   virtual pgsHaulingAnalysisArtifact* DesignHauling(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,bool bDesignForEqualOverhangs,bool bIgnoreConfigurationLimits,IGirderHaulingDesignPointsOfInterest* pPOId, bool* bSuccess, SHARED_LOGFILE LOGFILE)=0;
};


// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsGirderHandlingChecker

   Design Checker Factory for girder lifting and hauling


DESCRIPTION
   Design Checker Factory for girder lifting and hauling


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 03.25.1999 : Created file
*****************************************************************************/

class pgsGirderHandlingChecker
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Constructor
   pgsGirderHandlingChecker(IBroker* pBroker,StatusGroupIDType statusGroupID);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsGirderHandlingChecker();

   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   // Factory Method to create the appropriate hauling checker
   pgsGirderHaulingChecker* CreateGirderHaulingChecker();

   // Utility functions for the checking classes
   static void ComputeMoments(IBroker* pBroker, pgsGirderModelFactory* pGirderModelFactory, SpanIndexType span,GirderIndexType gdr,
                       pgsTypes::Stage stage,
                       Float64 leftOH,Float64 glen,Float64 rightOH,
                       Float64 E, 
                       const std::vector<pgsPointOfInterest>& rpoiVec,
                       std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection);

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;

   // GROUP: LIFECYCLE
   // can't construct without a broker
   pgsGirderHandlingChecker();

   // Prevent accidental copying and assignment
   pgsGirderHandlingChecker(const pgsGirderHandlingChecker&);
   pgsGirderHandlingChecker& operator=(const pgsGirderHandlingChecker&);

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

public:
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns true if the object is in a valid state, otherwise returns false.
   virtual bool AssertValid() const;

   //------------------------------------------------------------------------
   // Dumps the contents of the object to the given dump context.
   virtual void Dump(dbgDumpContext& os) const;
   #endif // _DEBUG

   #if defined _UNITTEST
   //------------------------------------------------------------------------
   // Runs a self-diagnostic test.  Returns true if the test passed,
   // otherwise false.
   static bool TestMe(dbgLog& rlog);
   #endif // _UNITTEST
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

/*****************************************************************************
CLASS 
   pgsAlternativeTensileStressCalculator


DESCRIPTION
   Utility class for dealing with alternative tensile stress in casting yard and at lifting


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 03.25.2013 : Created file
*****************************************************************************/

class pgsAlternativeTensileStressCalculator
{
public:
   // GROUP: LIFECYCLE
   //------------------------------------------------------------------------
   // Constructor
   pgsAlternativeTensileStressCalculator(SpanIndexType span,GirderIndexType gdr, IGirder* pGirder,
                                         ISectProp2* pSectProp2, ILongRebarGeometry* pRebarGeom,
                                         IBridgeMaterial* pMaterial, IBridgeMaterialEx* pMaterialEx,
                                         bool bSIUnits);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsAlternativeTensileStressCalculator()
   {;}

   Float64 ComputeAlternativeStressRequirements(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,
                                                Float64 fTop, Float64 fBot, 
                                                Float64 lowAllowTens, Float64 highAllowTens,
                                                Float64 *pYna, Float64 *pAreaTens, Float64 *pT, 
                                                Float64 *pAsProvd, Float64 *pAsReqd, bool* pIsAdequateRebar);

   static void ComputeReqdFcTens(SpanIndexType span,GirderIndexType gdr,
                          Float64 ft, // stress demand
                          Float64 rcsT, bool rcsBfmax, Float64 rcsFmax, Float64 rcsTalt, // allowable stress coeff's
                          Float64* pFcNo,Float64* pFcWithRebar)
   {
      if ( 0 < ft )
      {
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IBridgeMaterial,pMat);
         Float64 lambda = pMat->GetLambdaGdr(span,gdr);

         // Without rebar
         if ( rcsBfmax &&  ft>rcsFmax)
         {
            // allowable stress is limited and we hit the limit
            *pFcNo = -1;
         }
         else
         {
            *pFcNo = pow(ft/(rcsT*lambda),2);
         }

         // With rebar
         *pFcWithRebar = pow(ft/(rcsTalt*lambda),2);

      }
      else
      {
         // Compression
         *pFcNo = 0.0;
         *pFcWithRebar = 0.0;
      }
   }

private:
   pgsAlternativeTensileStressCalculator(); // no default constructor

   // GROUP: DATA MEMBERS
   IGirder* m_pGirder;
   ISectProp2* m_pSectProp2;
   ILongRebarGeometry* m_pRebarGeom;
   IBridgeMaterial* m_pMaterial;
   IBridgeMaterialEx* m_pMaterialEx;
   Float64 m_AllowableFs; // 

};


#endif // INCLUDED_PGSEXT_GIRDERHANDLINGCHECKER_H_
