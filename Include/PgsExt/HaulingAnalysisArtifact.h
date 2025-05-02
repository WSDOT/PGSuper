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

#pragma once

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\LiftHaulConstants.h>
#include <PgsExt\ReportPointOfInterest.h>
#include <map>
#include <Stability\Stability.h>

class IEAFDisplayUnits;

class PGSEXTCLASS pgsHaulingAnalysisArtifact
{
public:
   virtual ~pgsHaulingAnalysisArtifact() = default;

   // GROUP: OPERATIONS
   virtual bool Passed(bool bIgnoreConfigurationLimits=false) const = 0;
   virtual bool Passed(WBFL::Stability::HaulingSlope slope) const = 0;
   virtual bool PassedStressCheck(WBFL::Stability::HaulingSlope slope) const = 0;
   virtual void GetRequiredConcreteStrength(WBFL::Stability::HaulingSlope slope,Float64 *pfcCompression,Float64 *pfcTension,Float64* pfcTensionWithRebar) const = 0;

   virtual void BuildHaulingCheckReport(const CSegmentKey& segmentKey, rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const = 0;
   virtual void BuildHaulingDetailsReport(const CSegmentKey& segmentKey, rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const = 0;

   virtual pgsHaulingAnalysisArtifact* Clone() const = 0;

#if defined _DEBUG
   virtual void Dump(WBFL::Debug::LogContext& os) const = 0;
#endif

   virtual void Write1250Data(const CSegmentKey& segmentKey,std::_tofstream& resultsFile, std::_tofstream& poiFile, std::shared_ptr<WBFL::EAF::Broker> pBroker, const std::_tstring& pid, const std::_tstring& bridgeId) const = 0;
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
   pgsWsdotHaulingAnalysisArtifact() = default;
   pgsWsdotHaulingAnalysisArtifact(const pgsWsdotHaulingAnalysisArtifact& rOther) = default;
   virtual ~pgsWsdotHaulingAnalysisArtifact() = default;

   pgsWsdotHaulingAnalysisArtifact& operator = (const pgsWsdotHaulingAnalysisArtifact& rOther) = default;

   bool Passed(bool bIgnoreConfigurationLimits = false) const override;
   bool Passed(WBFL::Stability::HaulingSlope slope) const override;
   bool PassedStressCheck(WBFL::Stability::HaulingSlope slope) const override;
   void GetRequiredConcreteStrength(WBFL::Stability::HaulingSlope slope,Float64 *pfcCompression,Float64 *pfcTension, Float64* pfcTensionWithRebar) const override;

   void BuildHaulingCheckReport(const CSegmentKey& segmentKey,rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const override;
   void BuildHaulingDetailsReport(const CSegmentKey& segmentKey, rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const override;

   pgsHaulingAnalysisArtifact* Clone() const override;

   void Write1250Data(const CSegmentKey& segmentKey,std::_tofstream& resultsFile, std::_tofstream& poiFile, std::shared_ptr<WBFL::EAF::Broker> pBroker, const std::_tstring& pid, const std::_tstring& bridgeId) const override;

   Float64 GetMinFsForCracking(WBFL::Stability::HaulingSlope slope) const;
   Float64 GetFsRollover(WBFL::Stability::HaulingSlope slope) const;
   Float64 GetFsFailure(WBFL::Stability::HaulingSlope slope) const;

   void SetHaulingCheckArtifact(const WBFL::Stability::HaulingCheckArtifact& haulingArtifact);
   const WBFL::Stability::HaulingCheckArtifact& GetHaulingCheckArtifact() const;

private:
   WBFL::Stability::HaulingCheckArtifact m_HaulingArtifact;

public:
   // GROUP: DEBUG
#if defined _DEBUG
   const WBFL::Stability::HaulingStabilityProblem* m_pStabilityProblem = nullptr; // need this for dump
   void Dump(WBFL::Debug::LogContext& os) const override;
#endif // _DEBUG
};
