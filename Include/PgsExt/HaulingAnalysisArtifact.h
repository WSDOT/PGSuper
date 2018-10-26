///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <PgsExt\PointOfInterest.h>

#include <map>

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
   virtual bool Passed() const =0;
   virtual bool PassedStressCheck() const =0;
   virtual void GetRequiredConcreteStrength(Float64 *pfcCompression,Float64 *pfcTensionNoRebar,Float64 *pfcTensionWithRebar) const =0;

   virtual Float64 GetLeadingOverhang() const =0;
   virtual Float64 GetTrailingOverhang() const =0;

   virtual void BuildHaulingCheckReport(SpanIndexType span,GirderIndexType girder, rptChapter* pChapter, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const =0;
   virtual void BuildHaulingDetailsReport(SpanIndexType span,GirderIndexType girder, rptChapter* pChapter, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const =0;

   virtual pgsHaulingAnalysisArtifact* Clone() const =0;

#if defined _DEBUG
   virtual void Dump(dbgDumpContext& os) const =0;
#endif

   virtual void Write1250Data(SpanIndexType span,GirderIndexType girder,std::_tofstream& resultsFile, std::_tofstream& poiFile,IBroker* pBroker, const std::_tstring& pid, const std::_tstring& bridgeId) const =0;
};


// MISCELLANEOUS
//


/*****************************************************************************
CLASS 
   pgsWsdotHaulingStressAnalysisArtifact

   Artifact that holds Hauling stress check results at a location.


DESCRIPTION
   Artifact that holds Hauling stress check results at a location.


COPYRIGHT
   Copyright (c) 1999
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 03.26.1999 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsWsdotHaulingStressAnalysisArtifact
{
public:

   // GROUP: DATA MEMBERS

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // constructor
   pgsWsdotHaulingStressAnalysisArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsWsdotHaulingStressAnalysisArtifact(const pgsWsdotHaulingStressAnalysisArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsWsdotHaulingStressAnalysisArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsWsdotHaulingStressAnalysisArtifact& operator = (const pgsWsdotHaulingStressAnalysisArtifact& rOther);

   // GROUP: OPERATIONS

   bool Passed() const;
   bool CompressionPassed() const;
   bool TensionPassed() const;

   // GROUP: ACCESS
   Float64 GetEffectiveHorizPsForce() const;
   void SetEffectiveHorizPsForce(Float64 f);

   Float64 GetEccentricityPsForce() const;
   void SetEccentricityPsForce(Float64 f);

   Float64 GetLateralMoment() const;
   void SetLateralMoment(Float64 mom);

   void GetMomentImpact(Float64* pUpward,  Float64* pNoImpact, Float64* pDownward) const;
   void SetMomentImpact(Float64 upward,  Float64 NoImpact, Float64 downward);

   void GetTopFiberStress(Float64* pPs, Float64* pUpward,  Float64* pNoImpact, Float64* pDownward) const;
   void SetTopFiberStress(Float64 Ps, Float64 upward,  Float64 noImpact, Float64 downward);

   void GetBottomFiberStress(Float64* pPs, Float64* pUpward,  Float64* pNoImpact, Float64* pDownward) const;
   void SetBottomFiberStress(Float64 Ps, Float64 upward,  Float64 noImpact, Float64 downward);

   // Get maximized stresses for plumb girder with impact based on minimum C/D ratios
   void GetMaxPlumbCompressiveStress(Float64* fTop, Float64* fBottom, Float64* Capacity) const;
   void GetMaxPlumbTensileStress(Float64* fTop, Float64* fBottom, Float64* CapacityTop, Float64* CapacityBottom) const;

   void GetInclinedGirderStresses(Float64* pftu,Float64* pftd,Float64* pfbu,Float64* pfbd) const;
   void SetInclinedGirderStresses(Float64 ftu,Float64 ftd,Float64 fbu,Float64 fbd);

   Float64 GetMaximumInclinedConcreteCompressiveStress() const;
   Float64 GetMaximumInclinedConcreteTensileStress() const;
   Float64 GetMaximumConcreteCompressiveStress() const;
   Float64 GetMaximumConcreteTensileStress() const;

   void SetAlternativeTensileStressParameters(ImpactDir impact, Float64 Yna,   Float64 At,   Float64 T,  
                                              Float64 AsProvd,  Float64 AsReqd,  Float64 fAllow);

   void GetAlternativeTensileStressParameters(ImpactDir impact, Float64* Yna,   Float64* At,   Float64* T,  
                                              Float64* AsProvd,  Float64* AsReqd,  Float64* fAllow) const;

   // Tensile capacity for each impact direction
   void GetTensileCapacities(Float64* pUpward,  Float64* pNoImpact, Float64* pDownward) const;

   void SetRequiredConcreteStrength(Float64 fciComp,Float64 fciTensNoRebar,Float64 fciTensWithRebar);
   void GetRequiredConcreteStrength(Float64 *pfciComp,Float64 *pfciTensNoRebar,Float64 *pfciTensWithRebar) const;

   void SetCompressiveCapacity(Float64 fAllowableCompression);
   Float64 GetCompressiveCapacity() const;

   void SetInclinedTensileCapacity(Float64 fAllowable);
   Float64 GetInclinedTensileCapacity() const;

   // GROUP: INQUIRY
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns <b>true</b> if the class is in a valid state, otherwise returns
   // <b>false</b>.
   virtual bool AssertValid() const;

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
   void MakeCopy(const pgsWsdotHaulingStressAnalysisArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsWsdotHaulingStressAnalysisArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   Float64 m_EffectiveHorizPsForce;
   Float64 m_EccentricityPsForce;

   Float64 m_LateralMoment;

   Float64 m_MomentUpward;
   Float64 m_MomentNoImpact;
   Float64 m_MomentDownward;
   Float64 m_TopFiberStressPrestress;
   Float64 m_TopFiberStressUpward;
   Float64 m_TopFiberStressNoImpact;
   Float64 m_TopFiberStressDownward;
   Float64 m_BottomFiberStressPrestress;
   Float64 m_BottomFiberStressUpward;
   Float64 m_BottomFiberStressNoImpact;
   Float64 m_BottomFiberStressDownward;

   Float64 m_ftu,m_ftd,m_fbu,m_fbd;

   // Alternate tensile stress values for each impact direction
   Float64 m_Yna[SIZE_OF_IMPACTDIR];
   Float64 m_At[SIZE_OF_IMPACTDIR];
   Float64 m_T[SIZE_OF_IMPACTDIR];
   Float64 m_AsReqd[SIZE_OF_IMPACTDIR];
   Float64 m_AsProvd[SIZE_OF_IMPACTDIR];
   Float64 m_fAllow[SIZE_OF_IMPACTDIR];

   Float64 m_AllowableCompression;
   Float64 m_AllowableInclinedTension;
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

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

/*****************************************************************************
CLASS 
   pgsWsdotHaulingCrackingAnalysisArtifact

   Artifact that holds Hauling cracking check results at a location.


DESCRIPTION
   Artifact that holds Hauling cracking check results at a location.


COPYRIGHT
   Copyright (c) 1999
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 03.26.1999 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsWsdotHaulingCrackingAnalysisArtifact
{
public:
   // GROUP: DATA MEMBERS

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsWsdotHaulingCrackingAnalysisArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsWsdotHaulingCrackingAnalysisArtifact(const pgsWsdotHaulingCrackingAnalysisArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsWsdotHaulingCrackingAnalysisArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsWsdotHaulingCrackingAnalysisArtifact& operator = (const pgsWsdotHaulingCrackingAnalysisArtifact& rOther);

   // GROUP: OPERATIONS
   bool   Passed() const;

   // GROUP: ACCESS

   Float64 GetVerticalMoment() const;
   void SetVerticalMoment(Float64 m);

   Float64 GetLateralMomentStress() const;
   void SetLateralMomentStress(Float64 mom);

   // which flange governs for lateral moment, theta, and fs.
   CrackedFlange GetCrackedFlange() const;
   void SetCrackedFlange(CrackedFlange flange);

   Float64 GetLateralMoment() const;
   void SetLateralMoment(Float64 m);

   Float64 GetThetaCrackingMax() const;
   void SetThetaCrackingMax(Float64 t);

   Float64 GetFsCracking() const;
   void SetFsCracking(Float64 fs);

   Float64 GetAllowableFsForCracking() const;
   void SetAllowableFsForCracking(Float64 val);

   // GROUP: INQUIRY
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns <b>true</b> if the class is in a valid state, otherwise returns
   // <b>false</b>.
   virtual bool AssertValid() const;

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
   void MakeCopy(const pgsWsdotHaulingCrackingAnalysisArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsWsdotHaulingCrackingAnalysisArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS

   // GROUP: LIFECYCLE

   Float64 m_VerticalMoment;
   Float64 m_LateralMoment;
   Float64 m_LateralMomentStress;
   Float64 m_ThetaCrackingMax;
   Float64 m_FsCracking;
   Float64 m_AllowableFsForCracking;
   CrackedFlange m_CrackedFlange;

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

/*****************************************************************************
CLASS 
   pgsWsdotHaulingAnalysisArtifact

   Artifact which holds the detailed results of a girder Hauling check


DESCRIPTION
   Artifact which holds the detailed results of a girder Hauling check


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

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
   virtual bool Passed() const;
   virtual bool PassedStressCheck() const;
   virtual void GetRequiredConcreteStrength(Float64 *pfcCompression,Float64 *pfcTensionNoRebar,Float64 *pfcTensionWithRebar) const;

   virtual Float64 GetLeadingOverhang() const;
   virtual Float64 GetTrailingOverhang() const;

   virtual void BuildHaulingCheckReport(SpanIndexType span,GirderIndexType girder,rptChapter* pChapter, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const;
   virtual void BuildHaulingDetailsReport(SpanIndexType span,GirderIndexType girder, rptChapter* pChapter, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const;

   virtual pgsHaulingAnalysisArtifact* Clone() const;

   virtual void Write1250Data(SpanIndexType span,GirderIndexType girder,std::_tofstream& resultsFile, std::_tofstream& poiFile,IBroker* pBroker, const std::_tstring& pid, const std::_tstring& bridgeId) const;

   // GROUP: ACCESS

   // allowable FS's
   Float64 GetAllowableFsForCracking() const;
   void SetAllowableFsForCracking(Float64 val);

   Float64 GetAllowableFsForRollover() const;
   void SetAllowableFsForRollover(Float64 val);

   Float64 GetAllowableSpanBetweenSupportLocations() const;
   void SetAllowableSpanBetweenSupportLocations(Float64 val);

   Float64 GetAllowableLeadingOverhang() const;
   void SetAllowableLeadingOverhang(Float64 val);

   Float64 GetMaxGirderWgt() const;
   void SetMaxGirderWgt(Float64 maxWgt);

   // factors of safety
   Float64 GetMinFsForCracking() const;

   Float64 GetFsRollover() const;
   void SetFsRollover(Float64 val);

   // girder properties
   Float64 GetGirderLength() const;
   void SetGirderLength(Float64 val);

   Float64 GetClearSpanBetweenSupportLocations() const;
   void SetClearSpanBetweenSupportLocations(Float64 val);

   void SetOverhangs(Float64 trailing,Float64 leading);

   void SetGirderWeight(Float64 wgt);
   Float64 GetGirderWeight() const;
   Float64 GetAvgGirderWeightPerLength() const;

   Float64 GetHeightOfRollCenterAboveRoadway() const;
   void SetHeightOfRollCenterAboveRoadway(Float64 val);

   Float64 GetHeightOfCgOfGirderAboveRollCenter() const;
   void SetHeightOfCgOfGirderAboveRollCenter(Float64 val);

   Float64 GetRollStiffnessOfvehicle() const;
   void SetRollStiffnessOfvehicle(Float64 val);

   Float64 GetAxleWidth() const;
   void SetAxleWidth(Float64 val);

   Float64 GetRoadwaySuperelevation() const;
   void SetRoadwaySuperelevation(Float64 val);

   Float64 GetUpwardImpact() const;
   void SetUpwardImpact(Float64 val);

   Float64 GetDownwardImpact() const;
   void SetDownwardImpact(Float64 val);

   Float64 GetSweepTolerance() const;
   void SetSweepTolerance(Float64 val);

   Float64 GetSupportPlacementTolerance() const;
   void SetSupportPlacementTolerance(Float64 val);

   Float64 GetConcreteStrength() const;
   void SetConcreteStrength(Float64 val);

   Float64 GetModRupture() const;
   void SetModRupture(Float64 val);

   Float64 GetModRuptureCoefficient() const;
   void SetModRuptureCoefficient(Float64 val);

   Float64 GetElasticModulusOfGirderConcrete() const;
   void SetElasticModulusOfGirderConcrete(Float64 val);

   Float64 GetEccentricityDueToSweep() const;
   void SetEccentricityDueToSweep(Float64 val);

   Float64 GetEccentricityDueToPlacementTolerance() const;
   void SetEccentricityDueToPlacementTolerance(Float64 val);

   Float64 GetOffsetFactor() const;
   void SetOffsetFactor(Float64 val);

   Float64 GetTotalInitialEccentricity() const;
   void SetTotalInitialEccentricity(Float64 val);

   Float64 GetRadiusOfStability() const;
   void SetRadiusOfStability(Float64 val);

   Float64 GetIx() const;
   void SetIx(Float64 ix);

   Float64 GetIy() const;
   void SetIy(Float64 iy);

   Float64 GetZo() const;
   void SetZo(Float64 zo);

   Float64 GetZoPrime() const;
   void SetZoPrime(Float64 zo);

   Float64 GetThetaRolloverMax() const;
   void SetThetaRolloverMax(Float64 t);

   Float64 GetEqualibriumAngle() const;
   void SetEqualibriumAngle(Float64 val);

   // points of interest used for this hauling analysis
   void SetHaulingPointsOfInterest(const std::vector<pgsPointOfInterest>& rPois);
   std::vector<pgsPointOfInterest> GetHaulingPointsOfInterest() const;

   void GetMinMaxStresses(Float64* minStress, Float64* maxStress) const;
   void GetMinMaxInclinedStresses(Float64* pftuMin,Float64* pftdMin,Float64* pfbuMin,Float64* pfbdMin,
                                  Float64* pftuMax,Float64* pftdMax,Float64* pfbuMax,Float64* pfbdMax) const;

   // get max top (tension) and bottom (compression) stresses (minus prestress) at all analysis points along girder
   struct MaxdHaulingStresses
   {
   public:
      pgsPointOfInterest m_HaulingPoi;
      Float64 m_PrestressForce;
      Float64 m_TopMaxStress;
      Float64 m_BottomMinStress;

      MaxdHaulingStresses(const pgsPointOfInterest& rPoi, Float64 prestressForce, Float64 topMaxStress, Float64 bottomMinStress):
      m_HaulingPoi(rPoi), m_PrestressForce(prestressForce), m_TopMaxStress(topMaxStress), m_BottomMinStress(bottomMinStress)
      {;}
   };
   typedef std::vector<MaxdHaulingStresses>     MaxHaulingStressCollection;
   typedef MaxHaulingStressCollection::iterator MaxHaulingStressIterator;

   void GetMinMaxHaulingStresses(MaxHaulingStressCollection& rMaxStresses) const;

   void AddHaulingStressAnalysisArtifact(Float64 distFromStart,
                                      const pgsWsdotHaulingStressAnalysisArtifact& artifact);
   const pgsWsdotHaulingStressAnalysisArtifact* GetHaulingStressAnalysisArtifact(Float64 distFromStart) const;

   void AddHaulingCrackingAnalysisArtifact(Float64 distFromStart,
                                      const pgsWsdotHaulingCrackingAnalysisArtifact& artifact);
   const pgsWsdotHaulingCrackingAnalysisArtifact* GetHaulingCrackingAnalysisArtifact(Float64 distFromStart) const;

   void SetAllowableTensileConcreteStressParameters(Float64 f,bool bMax,Float64 fmax);
   void SetAllowableCompressionFactor(Float64 c);
   void SetAlternativeTensileConcreteStressFactor(Float64 f);

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsWsdotHaulingAnalysisArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsWsdotHaulingAnalysisArtifact& rOther);

   // reporting functions
   bool BuildImpactedStressTable(SpanIndexType span,GirderIndexType girder, rptChapter* pChapter,
                                 IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const;
   void BuildInclinedStressTable(SpanIndexType span,GirderIndexType girder,rptChapter* pChapter,
                                 IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const;
   void BuildOtherTables(rptChapter* pChapter,
                         IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits) const;

   void BuildRebarTable(IBroker* pBroker, rptChapter* pChapter, SpanIndexType span, GirderIndexType girder, 
                        ImpactDir dir) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS

   Float64 m_GirderLength;
   Float64 m_GirderWeightPerLength;
   Float64 m_ClearSpanBetweenSupportLocations;
   Float64 m_HeightOfRollCenterAboveRoadway;
   Float64 m_HeightOfCgOfGirderAboveRollCenter;
   Float64 m_RollStiffnessOfvehicle;
   Float64 m_AxleWidth;
   Float64 m_RoadwaySuperelevation;
   Float64 m_UpwardImpact;
   Float64 m_DownwardImpact;
   Float64 m_SweepTolerance;
   Float64 m_SupportPlacementTolerance;
   Float64 m_Fc;
   Float64 m_ModulusOfRupture;
   Float64 m_Krupture;
   Float64 m_ElasticModulusOfGirderConcrete;
   Float64 m_EccentricityDueToSweep;
   Float64 m_EccentricityDueToPlacementTolerance;
   Float64 m_OffsetFactor;
   Float64 m_TotalInitialEccentricity;
   Float64 m_RadiusOfStability;
   Float64 m_Ix;
   Float64 m_Iy;
   Float64 m_Zo;
   Float64 m_ZoPrime;
   Float64 m_ThetaRolloverMax;
   Float64 m_EqualibriumAngle;

   Float64 m_FsRollover;

   Float64 m_LeadingOverhang;
   Float64 m_TrailingOverhang;

   std::vector<pgsPointOfInterest> m_HaulingPois; // sorted same as below collections
   std::map<Float64,pgsWsdotHaulingStressAnalysisArtifact,Float64_less> m_HaulingStressAnalysisArtifacts;
   std::map<Float64,pgsWsdotHaulingCrackingAnalysisArtifact,Float64_less> m_HaulingCrackingAnalysisArtifacts;

   Float64 m_GirderWeight; // total girder weight

   // allowable stress parameters
   Float64 m_C;
   Float64 m_T;
   bool m_bfmax;
   Float64 m_fmax;
   Float64 m_Talt;

   Float64 m_AllowableSpanBetweenSupportLocations;
   Float64 m_AllowableLeadingOverhang;
   Float64 m_AllowableFsForCracking;
   Float64 m_AllowableFsForRollover;
   Float64 m_MaxGirderWgt;

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
#endif // INCLUDED_PGSEXT_HAULINGANALYSISARTIFACT_H_
