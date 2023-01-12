///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSEXT_HAULINGANALYSISARTIFACT_H_
#define INCLUDED_PGSEXT_HAULINGANALYSISARTIFACT_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <PgsExt\LiftHaulConstants.h>
#include <PgsExt\ReportPointOfInterest.h>

#include <map>

#include <Stability\Stability.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
interface IEAFDisplayUnits;

class PGSEXTCLASS pgsHaulingAnalysisArtifact
{
public:
   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsHaulingAnalysisArtifact()
   {;}

   // GROUP: OPERATIONS
   virtual bool Passed(bool bIgnoreConfigurationLimits=false) const = 0;
   virtual bool Passed(pgsTypes::HaulingSlope slope) const = 0;
   virtual bool PassedStressCheck(pgsTypes::HaulingSlope slope) const = 0;
   virtual void GetRequiredConcreteStrength(pgsTypes::HaulingSlope slope,Float64 *pfcCompression,Float64 *pfcTension,Float64* pfcTensionWithRebar) const = 0;

   virtual void BuildHaulingCheckReport(const CSegmentKey& segmentKey, rptChapter* pChapter, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const = 0;
   virtual void BuildHaulingDetailsReport(const CSegmentKey& segmentKey, rptChapter* pChapter, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const = 0;

   virtual pgsHaulingAnalysisArtifact* Clone() const = 0;

#if defined _DEBUG
   virtual void Dump(dbgDumpContext& os) const = 0;
#endif

   virtual void Write1250Data(const CSegmentKey& segmentKey,std::_tofstream& resultsFile, std::_tofstream& poiFile,IBroker* pBroker, const std::_tstring& pid, const std::_tstring& bridgeId) const = 0;
};


/*****************************************************************************
CLASS 
   pgsWsdotHaulingAnalysisArtifact

   Artifact which holds the detailed results of a girder Hauling check


DESCRIPTION
   Artifact which holds the detailed results of a girder Hauling check

LOG
   rdp : 03.25.1999 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsWsdotHaulingAnalysisArtifact : public pgsHaulingAnalysisArtifact
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsWsdotHaulingAnalysisArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsWsdotHaulingAnalysisArtifact(const pgsWsdotHaulingAnalysisArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsWsdotHaulingAnalysisArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsWsdotHaulingAnalysisArtifact& operator = (const pgsWsdotHaulingAnalysisArtifact& rOther);

   // GROUP: OPERATIONS
   // virtual functions
   virtual bool Passed(bool bIgnoreConfigurationLimits = false) const override;
   virtual bool Passed(pgsTypes::HaulingSlope slope) const override;
   virtual bool PassedStressCheck(pgsTypes::HaulingSlope slope) const override;
   virtual void GetRequiredConcreteStrength(pgsTypes::HaulingSlope slope,Float64 *pfcCompression,Float64 *pfcTension, Float64* pfcTensionWithRebar) const override;

   virtual void BuildHaulingCheckReport(const CSegmentKey& segmentKey,rptChapter* pChapter, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const override;
   virtual void BuildHaulingDetailsReport(const CSegmentKey& segmentKey, rptChapter* pChapter, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const override;

   virtual pgsHaulingAnalysisArtifact* Clone() const override;

   virtual void Write1250Data(const CSegmentKey& segmentKey,std::_tofstream& resultsFile, std::_tofstream& poiFile,IBroker* pBroker, const std::_tstring& pid, const std::_tstring& bridgeId) const override;

   Float64 GetMinFsForCracking(pgsTypes::HaulingSlope slope) const;
   Float64 GetFsRollover(pgsTypes::HaulingSlope slope) const;
   Float64 GetFsFailure(pgsTypes::HaulingSlope slope) const;

   void SetHaulingCheckArtifact(const WBFL::Stability::HaulingCheckArtifact& haulingArtifact);
   const WBFL::Stability::HaulingCheckArtifact& GetHaulingCheckArtifact() const;

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsWsdotHaulingAnalysisArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsWsdotHaulingAnalysisArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS


   WBFL::Stability::HaulingCheckArtifact m_HaulingArtifact;

public:
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns true if the object is in a valid state, otherwise returns false.
   virtual bool AssertValid() const;

   //------------------------------------------------------------------------
   // Dumps the contents of the object to the given dump context.
   const WBFL::Stability::HaulingStabilityProblem* m_pStabilityProblem; // need this for dump
   virtual void Dump(dbgDumpContext& os) const override;
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
#endif // INCLUDED_PGSEXT_HAULINGANALYSISARTIFACT_H_
