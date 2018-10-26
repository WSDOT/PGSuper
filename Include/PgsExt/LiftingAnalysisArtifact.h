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

#ifndef INCLUDED_PGSEXT_LIFTINGANALYSISARTIFACT_H_
#define INCLUDED_PGSEXT_LIFTINGANALYSISARTIFACT_H_

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
class pgsLiftingAnalysisArtifact;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsLiftingStressAnalysisArtifact

   Artifact that holds lifting stress Analysis results at a location.


DESCRIPTION
   Artifact that holds lifting stress Analysis results at a location.


COPYRIGHT
   Copyright (c) 1999
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 03.26.1999 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsLiftingStressAnalysisArtifact
{
public:

   // GROUP: DATA MEMBERS

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // constructor
   pgsLiftingStressAnalysisArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsLiftingStressAnalysisArtifact(const pgsLiftingStressAnalysisArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsLiftingStressAnalysisArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsLiftingStressAnalysisArtifact& operator = (const pgsLiftingStressAnalysisArtifact& rOther);

   // GROUP: OPERATIONS
   bool   Passed() const;
   bool   CompressionPassed() const;
   bool   TensionPassed() const;

   // Get maximized stresses based on minimum C/D ratios
   void GetMaxCompressiveStress(Float64* fTop, Float64* fBottom, Float64* Capacity) const;
   void GetMaxTensileStress(Float64* fTop, Float64* fBottom, Float64* CapacityTop, Float64* CapacityBottom) const;

   // GROUP: ACCESS

   Float64 GetEffectiveHorizPsForce() const;
   void SetEffectiveHorizPsForce(Float64 f);

   Float64 GetEccentricityPsForce() const;
   void SetEccentricityPsForce(Float64 f);

   void GetMomentImpact(Float64* pUpward,  Float64* pNoImpact, Float64* pDownward) const;
   void SetMomentImpact(Float64 upward,  Float64 NoImpact, Float64 downward);

   void GetTopFiberStress(Float64* pPs, Float64* pUpward,  Float64* pNoImpact, Float64* pDownward) const;
   void SetTopFiberStress(Float64 Ps, Float64 upward,  Float64 noImpact, Float64 downward);

   void GetBottomFiberStress(Float64* pPs, Float64* pUpward,  Float64* pNoImpact, Float64* pDownward) const;
   void SetBottomFiberStress(Float64 Ps, Float64 upward,  Float64 noImpact, Float64 downward);

   Float64 GetMaximumConcreteCompressiveStress() const;
   Float64 GetMaximumConcreteTensileStress() const;

   void SetAlternativeTensileStressParameters(ImpactDir impact, Float64 Yna,   Float64 At,   Float64 T,  
                                              Float64 AsProvd,  Float64 AsReqd,  Float64 fAllow);

   void GetAlternativeTensileStressParameters(ImpactDir impact, Float64* Yna,   Float64* At,   Float64* T,  
                                              Float64* AsProvd,  Float64* AsReqd,  Float64* fAllow) const;

   void SetCompressiveCapacity(Float64 fAllowableCompression);
   void GetCompressiveCapacity(Float64* fAllowableCompression) const;

   // Tensile capacity for each impact direction
   void GetTensileCapacities(Float64* pUpward,  Float64* pNoImpact, Float64* pDownward) const;

   void SetRequiredConcreteStrength(Float64 fciComp,Float64 fciTensNoRebar,Float64 fciTensWithRebar);
   void GetRequiredConcreteStrength(Float64 *pfciComp,Float64 *pfciTensNoRebar,Float64 *pfciTensWithRebar) const;

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
   Float64 m_EffectiveHorizPsForce;
   Float64 m_EccentricityPsForce;
   Float64 m_MomentUpward;
   Float64 m_MomentNoImpact;
   Float64 m_MomentDownward;
   Float64 m_TopFiberStressPrestress;      // top fiber stress due to prestress
   Float64 m_TopFiberStressUpward;         // top fiber stress due to DL+Impact+Prestress
   Float64 m_TopFiberStressNoImpact;
   Float64 m_TopFiberStressDownward;
   Float64 m_BottomFiberStressPrestress;
   Float64 m_BottomFiberStressUpward;
   Float64 m_BottomFiberStressNoImpact;
   Float64 m_BottomFiberStressDownward;

   // Alternate tensile stress values for each impact direction
   Float64 m_Yna[SIZE_OF_IMPACTDIR];
   Float64 m_At[SIZE_OF_IMPACTDIR];
   Float64 m_T[SIZE_OF_IMPACTDIR];
   Float64 m_AsReqd[SIZE_OF_IMPACTDIR];
   Float64 m_AsProvd[SIZE_OF_IMPACTDIR];
   Float64 m_fAllow[SIZE_OF_IMPACTDIR];

   Float64 m_AllowableCompression;
   Float64 m_ReqdCompConcreteStrength;
   Float64 m_ReqdTensConcreteStrengthNoRebar;
   Float64 m_ReqdTensConcreteStrengthWithRebar;
   bool m_WasRebarReqd;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsLiftingStressAnalysisArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsLiftingStressAnalysisArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS

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
   pgsLiftingCrackingAnalysisArtifact

   Artifact that holds lifting cracking Analysis results at a location.


DESCRIPTION
   Artifact that holds lifting cracking Analysis results at a location.


COPYRIGHT
   Copyright (c) 1999
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 03.26.1999 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsLiftingCrackingAnalysisArtifact
{
public:
   // GROUP: DATA MEMBERS

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsLiftingCrackingAnalysisArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsLiftingCrackingAnalysisArtifact(const pgsLiftingCrackingAnalysisArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsLiftingCrackingAnalysisArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsLiftingCrackingAnalysisArtifact& operator = (const pgsLiftingCrackingAnalysisArtifact& rOther);

   // GROUP: OPERATIONS
   bool Passed() const;

   // GROUP: ACCESS
   Float64 GetVerticalMoment() const;
   void SetVerticalMoment(Float64 m);

   Float64 GetLateralMoment() const;
   void SetLateralMoment(Float64 m);

   Float64 GetThetaCrackingMax() const;
   void SetThetaCrackingMax(Float64 t);

   // which flange governs for lateral moment, theta, and fs.
   CrackedFlange GetCrackedFlange() const;
   void SetCrackedFlange(CrackedFlange flange);

   // stress used to calc lateral moment
   Float64 GetLateralMomentStress() const;
   void SetLateralMomentStress(Float64 mom);

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
   void MakeCopy(const pgsLiftingCrackingAnalysisArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsLiftingCrackingAnalysisArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   Float64 m_VerticalMoment;
   Float64 m_LateralMoment;
   Float64 m_ThetaCrackingMax;
   Float64 m_FsCracking;
   Float64 m_AllowableFsForCracking;
   CrackedFlange m_CrackedFlange;
   Float64 m_LateralMomentStress;

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
   pgsLiftingAnalysisArtifact

   Artifact which holds the detailed results of a girder lifting Analysis


DESCRIPTION
   Artifact which holds the detailed results of a girder lifting Analysis


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 03.25.1999 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsLiftingAnalysisArtifact
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsLiftingAnalysisArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsLiftingAnalysisArtifact(const pgsLiftingAnalysisArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsLiftingAnalysisArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsLiftingAnalysisArtifact& operator = (const pgsLiftingAnalysisArtifact& rOther);

   // GROUP: OPERATIONS
   bool Passed() const;
   bool PassedFailureCheck() const;
   bool PassedStressCheck() const;

   // GROUP: ACCESS

   // Is girder stable for lifting? - If yr is less than zero, cg of girder
   // is above pick point and girder cannot be lifted without overturning.
   //
   // NOTE: If this function returns false, the cracking and failure calculations
   // cannot be performed and there will be no cracking or failure artifacts.
   bool IsGirderStable() const;
   void SetIsGirderStable(bool isStable);

   // factors of safety
   Float64 GetMinFsForCracking() const;

   // allowable FS's
   Float64 GetAllowableFsForCracking() const;
   void SetAllowableFsForCracking(Float64 val);

   Float64 GetAllowableFsForFailure() const;
   void SetAllowableFsForFailure(Float64 val);

   Float64 GetThetaFailureMax() const;
   void SetThetaFailureMax(Float64 val);

   // basic factor of safety against failure. This is the FS
   // computed from the equation.
   Float64 GetBasicFsFailure() const;
   void SetBasicFsFailure(Float64 val);

   // This is the actual FS against failure. If FSf < min FScr, FSf = min FScr
   Float64 GetFsFailure() const;
   void SetFsFailure(Float64 val);

   // girder properties
   Float64 GetGirderLength() const;
   void SetGirderLength(Float64 val);

   Float64 GetXFerLength() const;
   void SetXFerLength(Float64 val);

   Float64 GetAvgGirderWeightPerLength() const;
   void    SetGirderWeight(Float64 gdrWgt);
   Float64 GetGirderWeight() const;

   Float64 GetClearSpanBetweenPickPoints() const;
   void SetClearSpanBetweenPickPoints(Float64 val);

   Float64 GetLeftOverhang() const;
   Float64 GetRightOverhang() const;
   void SetOverhangs(Float64 left,Float64 right);

   Float64 GetVerticalDistanceFromPickPointToGirderCg() const;
   void SetVerticalDistanceFromPickPointToGirderCg(Float64 val);

   Float64 GetUpwardImpact() const;
   void SetUpwardImpact(Float64 val);

   Float64 GetDownwardImpact() const;
   void SetDownwardImpact(Float64 val);

   Float64 GetSweepTolerance() const;
   void SetSweepTolerance(Float64 val);

   Float64 GetLiftingDeviceTolerance() const;
   void SetLiftingDeviceTolerance(Float64 val);

   Float64 GetConcreteStrength() const;
   void SetConcreteStrength(Float64 val);

   Float64 GetModRupture() const;
   void SetModRupture(Float64 val);

   Float64 GetModRuptureCoefficient() const;
   void SetModRuptureCoefficient(Float64 val);

   Float64 GetElasticModulusOfGirderConcrete() const;
   void SetElasticModulusOfGirderConcrete(Float64 val);

   Float64 GetAxialCompressiveForceDueToInclinationOfLiftingCables() const;
   void SetAxialCompressiveForceDueToInclinationOfLiftingCables(Float64 val);

   Float64 GetMomentInGirderDueToInclinationOfLiftingCables() const;
   void SetMomentInGirderDueToInclinationOfLiftingCables(Float64 val);

   Float64 GetInclinationOfLiftingCables() const;
   void SetInclinationOfLiftingCables(Float64 val);

   Float64 GetEccentricityDueToSweep() const;
   void SetEccentricityDueToSweep(Float64 val);

   Float64 GetEccentricityDueToPlacementTolerance() const;
   void SetEccentricityDueToPlacementTolerance(Float64 val);

   Float64 GetOffsetFactor() const;
   void SetOffsetFactor(Float64 val);

   Float64 GetTotalInitialEccentricity() const;
   void SetTotalInitialEccentricity(Float64 val);

   Float64 GetCamberDueToSelfWeight() const;
   void SetCamberDueToSelfWeight(Float64 val);

   Float64 GetCamberDueToPrestress() const;
   void SetCamberDueToPrestress(Float64 val);

   Float64 GetTotalCamberAtLifting() const;
   void SetTotalCamberAtLifting(Float64 val);

   Float64 GetAdjustedYr() const;
   void SetAdjustedYr(Float64 val);

   Float64 GetZo() const;
   void SetZo(Float64 zo);

   Float64 GetIx() const;
   void SetIx(Float64 ix);

   Float64 GetIy() const;
   void SetIy(Float64 iy);

   Float64 GetZoPrime() const;
   void SetZoPrime(Float64 zo);

   Float64 GetInitialTiltAngle() const;
   void SetInitialTiltAngle(Float64 val);

   // points of interest used for this lifing analysis
   void SetLiftingPointsOfInterest(const std::vector<pgsPointOfInterest>& rPois);
   std::vector<pgsPointOfInterest> GetLiftingPointsOfInterest() const;

   // returns the top and bottom girder stresses for the supplied vector of locations
   void GetGirderStress(std::vector<Float64> locs,bool bMin,bool bIncludePrestress,std::vector<Float64>& fTop,std::vector<Float64>& fBot) const;

   void GetMinMaxStresses(Float64* minStress, Float64* maxStress,Float64* minDistFromStart,Float64* maxDistFromStart) const;

   // get max top tension and bottom compression stresses (minus prestress) in end zone and mid-zone
   void GetMidZoneMinMaxRawStresses(Float64 leftHp, Float64 rightHp, Float64* topStress, Float64* botStress,Float64* topDistFromStart,Float64* botDistFromStart) const;

   // returns the maximum girder dead load stress at the top and bottom of the girder at the controlling
   // location of point of prestress transfer and lift point.
   void GetEndZoneMinMaxRawStresses(Float64* topStress, Float64* botStress,Float64* topDistFromStart,Float64* botDistFromStart) const;

   // get max top (tension) and bottom (compression) stresses (minus prestress) at all analysis points along girder
   struct MaxdLiftingStresses
   {
   public:
      pgsPointOfInterest m_LiftingPoi;
      Float64 m_PrestressForce;
      Float64 m_TopMaxStress;
      Float64 m_BottomMinStress;

      MaxdLiftingStresses(const pgsPointOfInterest& rPoi, Float64 prestressForce, Float64 topMaxStress, Float64 bottomMinStress):
      m_LiftingPoi(rPoi), m_PrestressForce(prestressForce), m_TopMaxStress(topMaxStress), m_BottomMinStress(bottomMinStress)
      {;}
   };
   typedef std::vector<MaxdLiftingStresses>     MaxLiftingStressCollection;
   typedef MaxLiftingStressCollection::iterator MaxLiftingStressIterator;

   void GetMinMaxLiftingStresses(MaxLiftingStressCollection& rMaxStresses) const;

   void AddLiftingStressAnalysisArtifact(Float64 distFromStart,
                                         const pgsLiftingStressAnalysisArtifact& artifact);
   const pgsLiftingStressAnalysisArtifact* GetLiftingStressAnalysisArtifact(Float64 distFromStart) const;

   void AddLiftingCrackingAnalysisArtifact(Float64 distFromStart,
                                           const pgsLiftingCrackingAnalysisArtifact& artifact);
   const pgsLiftingCrackingAnalysisArtifact* GetLiftingCrackingAnalysisArtifact(Float64 distFromStart) const;

   void GetRequiredConcreteStrength(Float64 *pfcCompression,Float64 *pfcTensionNoRebar,Float64 *pfcTensionWithRebar) const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsLiftingAnalysisArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsLiftingAnalysisArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS

   Float64 m_GirderLength;
   Float64 m_GirderWeight;
   Float64 m_ClearSpanBetweenPickPoints;
   Float64 m_VerticalDistanceFromPickPointToGirderCg;
   Float64 m_UpwardImpact;
   Float64 m_DownwardImpact;
   Float64 m_SweepTolerance;
   Float64 m_LiftingDeviceTolerance;
   Float64 m_Fci;
   Float64 m_ModulusOfRupture;
   Float64 m_Krupture;
   Float64 m_ElasticModulusOfGirderConcrete;
   Float64 m_AxialCompressiveForceDueToInclinationOfLiftingCables;
   Float64 m_MomentInGirderDueToInclinationOfLiftingCables;
   Float64 m_InclinationOfLiftingCables;
   Float64 m_EccentricityDueToSweep;
   Float64 m_EccentricityDueToPlacementTolerance;
   Float64 m_OffsetFactor;
   Float64 m_TotalInitialEccentricity;
   Float64 m_CamberDueToSelfWeight;
   Float64 m_CamberDueToPrestress;
   Float64 m_AdjustedTotalCamberAtLifting;
   Float64 m_AdjustedYr;
   Float64 m_Ix;
   Float64 m_Iy;
   Float64 m_Zo;
   Float64 m_ZoPrime;
   Float64 m_InitialTiltAngle;

   Float64 m_ThetaFailureMax;

   Float64 m_BasicFsFailure;
   Float64 m_FsFailure;

   Float64 m_LeftOverhang;
   Float64 m_RightOverhang;

   Float64 m_XFerLength;

   bool m_IsStable;

   Float64 m_AllowableFsForCracking;
   Float64 m_AllowableFsForFailure;

   std::vector<pgsPointOfInterest> m_LiftingPois; // sorted same as below collections
   std::map<Float64,pgsLiftingStressAnalysisArtifact,Float64_less> m_LiftingStressAnalysisArtifacts;
   std::map<Float64,pgsLiftingCrackingAnalysisArtifact,Float64_less> m_LiftingCrackingAnalysisArtifacts;

public:
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

#endif // INCLUDED_PGSEXT_LIFTINGANALYSISARTIFACT_H_
