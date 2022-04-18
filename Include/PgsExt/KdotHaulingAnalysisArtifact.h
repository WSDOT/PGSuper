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

// SYSTEM INCLUDES
//
// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <PgsExt\LiftHaulConstants.h>
#include <PgsExt\PointOfInterest.h>

#include <PgsExt\HaulingAnalysisArtifact.h>

#include <map>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
interface IEAFDisplayUnits;

#pragma once



/*****************************************************************************
CLASS 
   pgsKdotHaulingStressAnalysisArtifact

   Artifact that holds Hauling stress check results at a location.

DESCRIPTION
   Artifact that holds Hauling stress check results at a location.

LOG
   rdp : 06.26.2013 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsKdotHaulingStressAnalysisArtifact
{
public:

   // GROUP: DATA MEMBERS

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // constructor
   pgsKdotHaulingStressAnalysisArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsKdotHaulingStressAnalysisArtifact(const pgsKdotHaulingStressAnalysisArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsKdotHaulingStressAnalysisArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsKdotHaulingStressAnalysisArtifact& operator = (const pgsKdotHaulingStressAnalysisArtifact& rOther);

   // GROUP: OPERATIONS

   bool Passed() const;
   bool CompressionPassed() const;
   bool TensionPassed() const;

   // GROUP: ACCESS
   Float64 GetEffectiveHorizPsForce() const;
   void SetEffectiveHorizPsForce(Float64 f);

   Float64 GetEccentricityPsForce() const;
   void SetEccentricityPsForce(Float64 f);

   void SetMoment(Float64 moment);
   Float64 GetMoment() const;

   void GetTopFiberStress(Float64* pPs, Float64* pStress) const;
   void SetTopFiberStress(Float64 Ps, Float64 stress);

   void GetBottomFiberStress(Float64* pPs, Float64* pStress) const;
   void SetBottomFiberStress(Float64 Ps, Float64 stress);

   Float64 GetMaximumConcreteCompressiveStress() const;
   Float64 GetMaximumConcreteTensileStress() const;

   void SetAlternativeTensileStressParameters(Float64 Yna,   Float64 At,   Float64 T,  
                                              Float64 AsProvd,  Float64 AsReqd,  Float64 fAllow);

   void GetAlternativeTensileStressParameters(Float64* Yna,   Float64* At,   Float64* T,  
                                              Float64* AsProvd,  Float64* AsReqd,  Float64* fAllow) const;

   // Note there is only one capacity. This assumes that the entire section cannot be in tension, so only one capacity 
   // is required (for the tension side, or none if both are in compression).
   void GetConcreteTensileStress(Float64* fTop, Float64* fBottom, Float64* pCapacity) const;

   void SetCompressiveCapacity(Float64 fAllowableCompression);
   Float64 GetCompressiveCapacity() const;

   void SetRequiredConcreteStrength(Float64 fciComp,Float64 fciTensNoRebar,Float64 fciTensWithRebar);
   void GetRequiredConcreteStrength(Float64 *pfciComp,Float64 *pfciTensNoRebar,Float64 *pfciTensWithRebar) const;

   // GROUP: INQUIRY
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Dumps the contents of the class to the given stream.
   virtual void Dump(dbgDumpContext& os) const;
   #endif // _DEBUG

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsKdotHaulingStressAnalysisArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsKdotHaulingStressAnalysisArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   Float64 m_EffectiveHorizPsForce;
   Float64 m_EccentricityPsForce;

   Float64 m_Moment;
   Float64 m_TopFiberStressPrestress;
   Float64 m_TopFiberStress;
   Float64 m_BottomFiberStressPrestress;
   Float64 m_BottomFiberStress;

   // Alternate tensile stress values 
   Float64 m_Yna;
   Float64 m_At;
   Float64 m_T;
   Float64 m_AsReqd;
   Float64 m_AsProvd;
   Float64 m_fAllow;

   Float64 m_AllowableCompression;

   Float64 m_ReqdCompConcreteStrength;
   Float64 m_ReqdTensConcreteStrengthNoRebar;
   Float64 m_ReqdTensConcreteStrengthWithRebar;
   bool m_WasRebarReqd;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};



class PGSEXTCLASS pgsKdotHaulingAnalysisArtifact : public pgsHaulingAnalysisArtifact
{
public:
   // GROUP: LIFECYCLE
   //------------------------------------------------------------------------
   // Default constructor
   pgsKdotHaulingAnalysisArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsKdotHaulingAnalysisArtifact(const pgsKdotHaulingAnalysisArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsKdotHaulingAnalysisArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsKdotHaulingAnalysisArtifact& operator = (const pgsKdotHaulingAnalysisArtifact& rOther);

   // GROUP: OPERATIONS
   // virtual functions
   virtual bool Passed(bool bIgnoreConfigurationLimits = false) const override;
   virtual bool Passed(pgsTypes::HaulingSlope slope) const override;
   virtual bool PassedStressCheck(pgsTypes::HaulingSlope slope) const override;
   virtual void GetRequiredConcreteStrength(pgsTypes::HaulingSlope slope,Float64 *pfcCompression,Float64 *pfcTension, Float64* pfcTensionWithRebar) const override;

   Float64 GetLeadingOverhang() const;
   Float64 GetTrailingOverhang() const;

   virtual void BuildHaulingCheckReport(const CSegmentKey& segmentKey,rptChapter* pChapter, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const override;
   virtual void BuildHaulingDetailsReport(const CSegmentKey& segmentKey, rptChapter* pChapter, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const override;

   virtual pgsHaulingAnalysisArtifact* Clone() const override;

   virtual void Write1250Data(const CSegmentKey& segmentKey,std::_tofstream& resultsFile, std::_tofstream& poiFile,IBroker* pBroker, const std::_tstring& pid, const std::_tstring& bridgeId) const override;

   // GROUP: ACCESS
   Float64 GetGirderLength() const;
   void SetGirderLength(Float64 val);

   Float64 GetClearSpanBetweenSupportLocations() const;
   void SetClearSpanBetweenSupportLocations(Float64 val);

   void SetOverhangs(Float64 trailing,Float64 leading);

   void SetOverhangLimits(Float64 hardLimit, Float64 softLimit);
   void GetOverhangLimits(Float64* hardLimit, Float64* softLimit) const;

   void SetGirderWeight(Float64 wgt);
   Float64 GetGirderWeight() const;
   Float64 GetAvgGirderWeightPerLength() const;

   // Dynamic factors
   void SetGFactors(Float64 gOverhang, Float64 gInterior);
   void GetGFactors(Float64* gOverhang, Float64* gInterior) const;

   Float64 GetElasticModulusOfGirderConcrete() const;
   void SetElasticModulusOfGirderConcrete(Float64 val);

   // points of interest used for this hauling analysis
   std::vector<pgsPointOfInterest> GetHaulingPointsOfInterest() const;

   void AddHaulingStressAnalysisArtifact(const pgsPointOfInterest& poi,
                                      const pgsKdotHaulingStressAnalysisArtifact& artifact);
   const pgsKdotHaulingStressAnalysisArtifact* GetHaulingStressAnalysisArtifact(const pgsPointOfInterest& poi) const;

   // Outcome from design algorithm
   enum DesignOutcome {doNoDesignDone,      // No design was performed 
                       doFailed,            // No overhang succeeded
                       doSuccessInSoftZone, // Designed succeeded between 5' and 1/10 girder length
                       doSuccess};          // Succeeded within Min overhang requirments

   void SetDesignOutcome(DesignOutcome outcome);
   DesignOutcome GetDesignOutcome() const;

   // Design variables - only set if design performed
   void SetDesignOverhang(Float64);
   Float64 GetDesignOverhang() const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsKdotHaulingAnalysisArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsKdotHaulingAnalysisArtifact& rOther);

   void BuildRebarTable(IBroker* pBroker, rptChapter* pChapter, const CSegmentKey& segmentKey) const;

public:
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Dumps the contents of the object to the given dump context.
   virtual void Dump(dbgDumpContext& os) const override;
   #endif // _DEBUG

private:
   Float64 m_LeadingOverhang;
   Float64 m_TrailingOverhang;

   Float64 m_HardOverhangLimit;
   Float64 m_SoftOverhangLimit;

   Float64 m_GirderLength;
   Float64 m_GirderWeightPerLength;
   Float64 m_GirderWeight;

   Float64 m_gOverhang;
   Float64 m_gInterior;

   Float64 m_ClearSpanBetweenSupportLocations;
   Float64 m_ElasticModulusOfGirderConcrete;

   std::vector<pgsPointOfInterest> m_HaulingPois; // sorted same as below collection
   std::map<pgsPointOfInterest,pgsKdotHaulingStressAnalysisArtifact> m_HaulingStressAnalysisArtifacts;

   DesignOutcome m_DesignOutcome;
   Float64 m_DesignOverhang;
};

