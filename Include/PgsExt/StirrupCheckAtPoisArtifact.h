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

#if !defined INCLUDED_PGSEXT_SHEARARTIFACT_H_
#define INCLUDED_PGSEXT_SHEARARTIFACT_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#if !defined INCLUDED_SYSTEM_SECTIONVALUE_H_
#include <System\SectionValue.h>
#endif

#if !defined INCLUDED_PGSEXT_POIARTIFACTKEY_H_
#include <PgsExt\PoiArtifactKey.h>
#endif

#include <Materials/Rebar.h>

#include <PgsExt\PointOfInterest.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//


/*****************************************************************************
CLASS 
   pgsLongReinfShearArtifact

   Artifact that holds design/check results for area of steel for shear cap.


DESCRIPTION
   Artifact that holds design/check results for area of steel for shear cap.

LOG
   rab : 5.31.1999 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsLongReinfShearArtifact
{
public:
   pgsLongReinfShearArtifact() = default;
   pgsLongReinfShearArtifact(const pgsLongReinfShearArtifact& rOther) = default;
   ~pgsLongReinfShearArtifact() = default;

   pgsLongReinfShearArtifact& operator = (const pgsLongReinfShearArtifact& rOther) = default;

   Float64 GetFy() const;
   void SetFy(Float64 fy);

   Float64 GetEs() const;
   void SetEs(Float64 Es);

   Float64 GetAs() const;
   void SetAs(Float64 as);

   Float64 GetAps() const;
   void SetAps(Float64 aps);

   Float64 GetFps() const;
   void SetFps(Float64 fps);

   Float64 GetAptSegment() const;
   void SetAptSegment(Float64 apt);

   Float64 GetFptSegment() const;
   void SetFptSegment(Float64 fpt);

   Float64 GetAptGirder() const;
   void SetAptGirder(Float64 apt);

   Float64 GetFptGirder() const;
   void SetFptGirder(Float64 fpt);

   Float64 GetMu() const;
   void SetMu(Float64 mu);

   Float64 GetMr() const;
   void SetMr(Float64 mr);

   Float64 GetDv() const;
   void SetDv(Float64 dv);

   Float64 GetFlexuralPhi() const;
   void SetFlexuralPhi(Float64 phi);

   Float64 GetNu() const;
   void SetNu(Float64 nu);

   Float64 GetAxialPhi() const;
   void SetAxialPhi(Float64 phi);

   Float64 GetVu() const;
   void SetVu(Float64 vu);

   Float64 GetShearPhi() const;
   void SetShearPhi(Float64 phi);

   Float64 GetVs() const;
   void SetVs(Float64 vs);

   Float64 GetVp() const;
   void SetVp(Float64 vp);

   Float64 GetTheta() const;
   void SetTheta(Float64 theta);

   Uint16 GetEquation() const;
   void SetEquation(Uint16 eqn); // 1 = 5.7.3.5-1, 2 = 5.7.3.5-2 (pre2017: 5.8.3.5)

   // is long shear check at poi applicable (not applicable outside of face of support)
   bool IsApplicable() const;
   void SetApplicability(bool isApplicable);

   void IsFHWAUHPC(bool bIsUHPC);
   bool IsFHWAUHPC() const;

   void SetCrackLocalizationStrain(Float64 etloc);
   Float64 GetCrackLocalizationStrain() const;

   // This is actually the tension stress limit, gamma_u*ft,cr
   void SetDesignEffectiveConcreteStrength(Float64 ft);
   Float64 GetDesignEffectiveConcreteStrength() const;

   void SetAct(Float64 Act);
   Float64 GetAct() const;


   // LRFD 9th Edition added the requirement that Apsfps > Asfy
   // set this parameter to true if this requirement is to be checked
   bool PretensionForceMustExceedBarForce() const;
   void PretensionForceMustExceedBarForce(bool bExceed);

   Float64 GetPretensionForce() const;
   Float64 GetRebarForce() const;
   Float64 GetConcreteForce() const;


   // lrfd demand and capacity criteria
   void SetDemandForce(Float64 demand);
   Float64 GetDemandForce() const;

   Float64 GetCapacityForce() const;

   bool PassedPretensionForce() const; // 9th edition requirement for Apsfps > Asfy
   bool PassedCapacity() const;
   bool Passed() const;

   // GROUP: LIFECYCLE


private:
   Float64 m_Fy{0.0};
   Float64 m_Es{ 0.0 };
   Float64 m_As{ 0.0 };
   Float64 m_Aps{ 0.0 };
   Float64 m_Fps{ 0.0 };
   Float64 m_AptSegment{ 0.0 };
   Float64 m_FptSegment{ 0.0 };
   Float64 m_AptGirder{ 0.0 };
   Float64 m_FptGirder{ 0.0 };
   Float64 m_Mu{ 0.0 };
   Float64 m_Mr{ 0.0 };
   Float64 m_Dv{ 0.0 };
   Float64 m_FlexuralPhi{ 0.0 };
   Float64 m_Nu{ 0.0 };
   Float64 m_AxialPhi{ 0.0 };
   Float64 m_Vu{ 0.0 };
   Float64 m_ShearPhi{ 0.0 };
   Float64 m_Vs{ 0.0 };
   Float64 m_Vp{ 0.0 };
   Float64 m_Theta{ 0.0 };
   Float64 m_DemandForce{ 0.0 };
   Uint16  m_Equation{ 0 };
   bool    m_bIsApplicable{ false };
   bool    m_bPretensionForceLimit{false};

   // FHWA UHPC parameters
   bool    m_bFHWAUHPC{ false };
   Float64 m_Act{ 0.0 };
   Float64 m_ft{ 0.0 }; // gamma_u * ft,cr = tension stress limit
   Float64 m_etloc{ 0.0 }; // crack localization strain
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

/*****************************************************************************
CLASS 
   pgsVerticalShearArtifact

   Artifact that holds vertical shear design/check results.


DESCRIPTION
   Artifact that holds vertical shear design/check results.

LOG
   rab : 12.29.1997 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsVerticalShearArtifact
{
public:
   // GROUP: DATA MEMBERS

   // if the support reaction is compressive then
   // the strength check between the support and the 
   // critical section is not required
   bool IsApplicable() const;
   void IsApplicable(bool bApplicable);

   // for sections where strength is not applicable, strut and tie analysis may be
   bool IsStrutAndTieRequired() const;
   void IsStrutAndTieRequired(bool bRequired);

   // 5.7.2.3 (pre2017: 5.8.2.4)
   void SetAreStirrupsReqd(bool reqd);
   bool GetAreStirrupsReqd() const;

   void SetAreStirrupsProvided(bool provd);
   bool GetAreStirrupsProvided() const;

   void SetAvOverSReqd(Float64 reqd);
   Float64 GetAvOverSReqd() const;

   void SetDemand(Float64 demand);
   Float64 GetDemand() const;
   void SetCapacity(Float64 cap);
   Float64 GetCapacity() const;

   // Use this function for locations outside of the CSS. This allows checking
   // whether Av/S is decreasing in the end regions
   void SetEndSpacing(Float64 AvS_provided,Float64 AvS_at_CS);
   void GetEndSpacing(Float64* pAvS_provided,Float64* pAvS_at_CS);

   // Returns true if the check for this artifact occurs in a critical section zone
   bool IsInCriticalSectionZone() const;

   // returns true if this is an end region poi and Av/s is less that at CSS
   bool DidAvsDecreaseAtEnd() const;

   bool   Passed() const;

   // GROUP: LIFECYCLE

   pgsVerticalShearArtifact() = default;
   pgsVerticalShearArtifact(const pgsVerticalShearArtifact& rOther) = default;
   ~pgsVerticalShearArtifact() = default;

   pgsVerticalShearArtifact& operator = (const pgsVerticalShearArtifact& rOther) = default;

   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns <b>true</b> if the class is in a valid state, otherwise returns
   // <b>false</b>.
   virtual bool AssertValid() const;

   //------------------------------------------------------------------------
   // Dumps the contents of the class to the given stream.
   virtual void Dump(WBFL::Debug::LogContext& os) const;
   #endif // _DEBUG

private:
   // GROUP: DATA MEMBERS
   bool m_bIsApplicable{true};
   bool m_bIsStrutAndTieRequired{false};
   bool m_bEndSpacingApplicable{false};

   // 5.7.2.3 (pre2017: 5.8.2.4)
   bool m_AreStirrupsReqd{true};
   bool m_AreStirrupsProvided{false};

   Float64 m_AvOverSReqd{ 0.0 };

   Float64 m_Demand{ 0.0 };
   Float64 m_Capacity{ 0.0 };

   Float64 m_AvSprovided{ 0.0 };
   Float64 m_AvSatCS{ 0.0 };
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

/*****************************************************************************
CLASS 
   pgsHorizontalShearArtifact

   Artifact that holds shear design/check results.


DESCRIPTION
   Artifact that holds shear design/check results.

LOG
   rab : 12.29.1997 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsHorizontalShearArtifact
{
public:
   pgsHorizontalShearArtifact() = default;
   pgsHorizontalShearArtifact(const pgsHorizontalShearArtifact& rOther) = default;
   ~pgsHorizontalShearArtifact() = default;

   pgsHorizontalShearArtifact& operator = (const pgsHorizontalShearArtifact& rOther) = default;

   // GROUP: OPERATIONS

   // Not applicable if there is no composite slab
   bool IsApplicable() const;
   void SetApplicability(bool isApplicable);

   // additional stirrups in top flange
   Float64 GetAvfAdditional() const { return m_AvfAdditional; }
   void SetAvfAdditional(Float64 avf) { m_AvfAdditional = avf; }
   Float64 GetSAdditional() const;
   void SetSAdditional(Float64 s);

   // stirrups in girder
   Float64 GetAvfGirder() const { return m_AvfGirder; }
   void SetAvfGirder(Float64 avf) { m_AvfGirder = avf; }
   Float64 GetSGirder() const;
   void SetSGirder(Float64 s);

   // composite stirrup Av/S
   Float64 GetAvOverS() const;
   // max stirrup spacing for min spacing check
   Float64 GetSpacing() const;

   Float64 GetNormalCompressionForce() const { return m_NormalCompressionForce; }
   void SetNormalCompressionForce(Float64 force) { m_NormalCompressionForce = force; }
   Float64 GetNormalCompressionForceLoadFactor() const { return m_gamma_Pc; }
   void SetNormalCompressionForceLoadFactor(Float64 gamma) { m_gamma_Pc = gamma; }
   Float64 GetAcv() const {return m_Acv;}
   void SetAcv(Float64 Acv) {m_Acv = Acv;}
   Float64 GetCohesionFactor() const {return m_CohesionFactor;}
   void SetCohesionFactor(Float64 factor) {m_CohesionFactor = factor;}
   Float64 GetFrictionFactor() const {return m_FrictionFactor;}
   void SetFrictionFactor(Float64 factor) {m_FrictionFactor = factor;}
   Float64 GetK1() const;
   void SetK1(Float64 k1);
   Float64 GetK2() const;
   void SetK2(Float64 k2);
   Float64 GetPhi() const {return m_Phi;}
   void SetPhi(Float64 phi) {m_Phi = phi;}
   // capacities from eqn's 5.7.4.3-1,2,3 (pre2017: 5.8.4.1)
   void GetVn(Float64* pVn1, Float64* pVn2, Float64* pVn3) const;
   void SetVn(Float64 Vn1, Float64 Vn2, Float64 Vn3);
   Float64 GetFc() const;
   void SetFc(Float64 fc);

   Float64 GetDemand() const {return m_UltimateHorizontalShear;}
   void SetDemand(Float64 shear) {m_UltimateHorizontalShear = shear;}
   Float64 GetNominalCapacity() const;
   Float64 GetCapacity() const;

   // additional data for 5.7.4.3-1-4 (pre2017: 5.8.4.1)
   Float64 GetBv() const;
   void SetBv(Float64 bv);
   Float64 GetSmax() const;
   void SetSmax(Float64 sMax);
   Float64 GetFy() const;
   void SetFy(Float64 fy);
   bool WasFyLimited() const;
   void WasFyLimited(bool bWasLimited);
   bool Is5_7_4_1_4Applicable() const;
   
   Float64 GetAvOverSMin_5_7_4_2_1() const;
   void SetAvOverSMin_5_7_4_2_1(Float64 fmin);
   Float64 GetAvOverSMin_5_7_4_1_3() const;
   void SetAvOverSMin_5_7_4_1_3(Float64 fmin);
   Float64 GetAvOverSMin() const;
   void SetAvOverSMin(Float64 fmin);
   Float64 GetNumLegs() const;
   void SetNumLegs(Float64 legs);
   Float64 GetNumLegsReqd() const;
   void SetNumLegsReqd(Float64 legs);
   Float64 GetVsAvg() const;      // Average shear stress. Note: This value is vni prior to 2007 and
   void SetVsAvg(Float64 vsavg);  // vui afterwards 
   Float64 GetVsLimit() const; // max shear strength at which 5.7.4.3-4 (pre2017: 5.8.4.1-4) is not applicable
   void SetVsLimit(Float64 vs);
   bool DoAllPrimaryStirrupsEngageDeck() const;
   void SetDoAllPrimaryStirrupsEngageDeck(bool doEngage);
   bool IsTopFlangeRoughened() const;
   void SetIsTopFlangeRoughened(bool doIsRough);


   // data for computing shear stress
   Float64 GetVu() const;
   void SetVu(const Float64& vu);
   Float64 GetDv() const;
   void SetDv(Float64 dv);
   Float64 GetI() const;
   void SetI(Float64 i);
   Float64 GetQ() const;
   void SetQ(Float64 q);

   // Use this function for locations outside of the CSS. This allows checking
   // whether Av/S is decreasing in the end zones
   void SetEndSpacing(Float64 AvS_provided,Float64 AvS_at_CS);
   void GetEndSpacing(Float64* pAvS_provided,Float64* pAvS_at_CS);

   // Returns true if this is an end region poi and Av/s is less than at CSS
   bool DidAvsDecreaseAtEnd() const;

   // Data for design algorithm, if needed
   Float64 GetAvOverSReqd() const;
   void SetAvOverSReqd(const Float64& vu);

   bool SpacingPassed() const;
   int  MinReinforcementPassed() const;
   bool StrengthPassed() const;

   bool   Passed() const;

   // GROUP: ACCESS
   // GROUP: INQUIRY
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns <b>true</b> if the class is in a valid state, otherwise returns
   // <b>false</b>.
   virtual bool AssertValid() const;

   //------------------------------------------------------------------------
   // Dumps the contents of the class to the given stream.
   virtual void Dump(WBFL::Debug::LogContext& os) const;
   #endif // _DEBUG

private:
   // GROUP: DATA MEMBERS
   bool    m_IsApplicable{false};
   Float64 m_AvfAdditional{ 0.0 };
   Float64 m_SAdditional{ 0.0 };
   Float64 m_AvfGirder{ 0.0 };
   Float64 m_SGirder{ 0.0 };
   Float64 m_UltimateHorizontalShear{ 0.0 };
   Float64 m_NormalCompressionForce{ 0.0 };
   Float64 m_gamma_Pc{ 0.0 };
   Float64 m_Acv{ 0.0 };
   Float64 m_CohesionFactor{ 0.0 };
   Float64 m_FrictionFactor{ 0.0 };
   Float64 m_K1{ 0.0 }, m_K2{ 0.0 }; // strength factors added in LRFD 2007
   Float64 m_Phi{ 0.0 };
   Float64 m_Vn1{ 0.0 };
   Float64 m_Vn2{ 0.0 };
   Float64 m_Vn3{ 0.0 };
   Float64 m_Fc{ 0.0 }; // F'c used to compute in Eqn 5.7.4.3-2 (pre2017: 5.8.4.1-2)
   bool m_bDoAllPrimaryStirrupsEngageDeck{false};
   bool m_bIsTopFlangeRoughened{false};

   Float64 m_Bv{ 0.0 };
   Float64 m_Smax{ 0.0 }; // maximum allowable spacing [5.7.4.5 (pre2017: 5.8.4.2)]
   Float64 m_Fy{ 0.0 };
   bool    m_bWasFyLimited{ false }; // true if fy is limited to 60 ksi per LRFD2013 5.7.4.3 (pre2017: 5.8.4.1)
   Float64 m_AvOverSMin_5_7_4_2_1{ 0.0 };
   Float64 m_AvOverSMin_5_7_4_1_3{ 0.0 };
   Float64 m_AvOverSMin{ 0.0 };
   Float64 m_NumLegs{ 0.0 };
   Float64 m_NumLegsReqd{ 0.0 };
   Float64 m_VsAvg{ 0.0 };   // average shear strength
   Float64 m_VsLimit{ 0.0 }; // max shear strength at which 5.7.4.3-4 (pre2017: 5.8.4.1-4) is not applicable

   // parameters for checking end zone locations
   bool m_bEndSpacingApplicable{false};
   Float64 m_AvSprovided{ 0.0 };
   Float64 m_AvSatCS{ 0.0 };


   // parameters used to compute horizonal shear from vertical shear
   Float64 m_Dv{ 0.0 };
   Float64 m_I{ 0.0 };
   Float64 m_Q{ 0.0 };
   Float64 m_Vu{ 0.0 }; // vertical shear

   Float64 m_AvsReqd{ 0.0 };
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//


/*****************************************************************************
CLASS 
   pgsStirrupDetailArtifact

   Artifact that holds shear design/check results.


DESCRIPTION
   Artifact that holds shear design/check results.

LOG
   rab : 12.29.1997 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsStirrupDetailArtifact
{
public:
   pgsStirrupDetailArtifact() = default;
   pgsStirrupDetailArtifact(const pgsStirrupDetailArtifact& rOther) = default;
   ~pgsStirrupDetailArtifact() = default;

   pgsStirrupDetailArtifact& operator = (const pgsStirrupDetailArtifact& rOther) = default;


   // GROUP: OPERATIONS
   void SetAfter1999(bool val) {m_After1999=val;}
   bool GetAfter1999() const {return m_After1999;}
   void SetFy(Float64 fy) { m_Fy = fy; }
   Float64 GetFy() const { return m_Fy; }
   void SetFc(Float64 fc) { m_Fc = fc; }
   Float64 GetFc() const { return m_Fc; }
   void SetAvsMin(Float64 avsMin) {m_AvsMin = avsMin;}
   Float64 GetAvsMin() const {return m_AvsMin;}
   void SetAvs(Float64 avs) {m_Avs = avs;}
   Float64 GetAvs() const {return m_Avs;}
   void SetSMax(Float64 s) {m_SMax = s;}
   Float64 GetSMax() const {return m_SMax;}
   void SetSMin(Float64 s) {m_SMin = s;}
   Float64 GetSMin() const {return m_SMin;}
   void SetBarSize(WBFL::Materials::Rebar::Size s) {m_BarSize = s;}
   WBFL::Materials::Rebar::Size GetBarSize() const {return m_BarSize;}
   void SetS(Float64 s) {m_S = s;}
   Float64 GetS() const {return m_S;}
   void SetBv(Float64 bv) {m_Bv = bv;}
   Float64 GetBv() const {return m_Bv;}
   void SetDv(Float64 dv) {m_Dv = dv;}
   Float64 GetDv() const {return m_Dv;}
   void SetTheta(Float64 theta) { m_Theta = theta; }
   Float64 GetTheta() const { return m_Theta; }

   // Prior to 1999 Vu is a force
   void SetVu(Float64 Vu) {m_Vu = Vu;}
   Float64 GetVu() const {/*ATLASSERT(!m_After1999)*/; return m_Vu;}
   void SetVuLimit(Float64 Vu) {m_VuLimit = Vu;}
   Float64 GetVuLimit() const {/*ATLASSERT(!m_After1999)*/; return m_VuLimit;}
   // 1999 and later vu is stress
   void Setvu(Float64 vu) {/*ATLASSERT(m_After1999)*/; m_vu = vu;}
   Float64 Getvu() const {/*ATLASSERT(m_After1999)*/; return m_vu;}
   void SetvuLimit(Float64 vu) {/*ATLASSERT(m_After1999)*/; m_vuLimit = vu;}
   Float64 GetvuLimit() const {/*ATLASSERT(m_After1999)*/; return m_vuLimit;}

   void SetApplicability(bool isApp) {m_IsApplicable=isApp;}
   bool IsApplicable() const {return m_IsApplicable;};
   void SetIsInCriticalSectionZone(bool isApp) {m_IsInCritialSectionZone=isApp;}
   bool IsInCriticalSectionZone() const {return m_IsInCritialSectionZone;};
   bool   Passed() const;

   // GROUP: ACCESS
   // GROUP: INQUIRY
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns <b>true</b> if the class is in a valid state, otherwise returns
   // <b>false</b>.
   virtual bool AssertValid() const;
   //------------------------------------------------------------------------
   // Dumps the contents of the class to the given stream.
   virtual void Dump(WBFL::Debug::LogContext& os) const;
   #endif // _DEBUG

private:
   // GROUP: DATA MEMBERS
   bool m_After1999{true}; // after 1999 spec
   Float64 m_Fy{0.0};
   Float64 m_Fc{ 0.0 };
   Float64 m_AvsMin{ 0.0 };
   Float64 m_Avs{ 0.0 };
   Float64 m_SMax{ 0.0 };
   Float64 m_SMin{ 0.0 };
   WBFL::Materials::Rebar::Size m_BarSize{WBFL::Materials::Rebar::Size::bsNone};
   Float64 m_S{ 0.0 };
   Float64 m_Bv{ 0.0 };
   Float64 m_Dv{ 0.0 };
   Float64 m_Vu{ 0.0 }; // 1999 and earlier used shear force for comparison
   Float64 m_VuLimit{ 0.0 };
   Float64 m_vu{ 0.0 }; // post 1999 we use shear stress per 5.7.2.8 (pre2017: 5.8.2.9)
   Float64 m_vuLimit{ 0.0 };
   bool m_IsApplicable{false};
   bool m_IsInCritialSectionZone{false};
   Float64 m_Theta{ 0.0 }; // for FWHA UHPC see GS 1.7.2.6
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
   pgsStirrupCheckAtPoisArtifact

   Artifact that holds shear design/check results.


DESCRIPTION
   Artifact that holds shear design/check results.

LOG
   rab : 12.29.1997 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsStirrupCheckAtPoisArtifact
{
public:
   // vertical shear check results
   void SetVerticalShearArtifact(const pgsVerticalShearArtifact& artifact);
   const pgsVerticalShearArtifact* GetVerticalShearArtifact() const;
   void SetHorizontalShearArtifact(const pgsHorizontalShearArtifact& artifact);
   const pgsHorizontalShearArtifact* GetHorizontalShearArtifact() const;
   void SetStirrupDetailArtifact(const pgsStirrupDetailArtifact& artifact);
   const pgsStirrupDetailArtifact* GetStirrupDetailArtifact() const;
   void SetLongReinfShearArtifact( const pgsLongReinfShearArtifact& artifact);
   const pgsLongReinfShearArtifact* GetLongReinfShearArtifact() const;

   bool   Passed() const;

   pgsStirrupCheckAtPoisArtifact() = default;
   pgsStirrupCheckAtPoisArtifact(const pgsStirrupCheckAtPoisArtifact& rOther) = default;

   ~pgsStirrupCheckAtPoisArtifact() = default;

   pgsStirrupCheckAtPoisArtifact& operator = (const pgsStirrupCheckAtPoisArtifact& rOther) = default;
   bool operator<(const pgsStirrupCheckAtPoisArtifact& artifact) const;


   const pgsPointOfInterest& GetPointOfInterest() const;
   void SetPointOfInterest(const pgsPointOfInterest& poi);

#if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns <b>true</b> if the class is in a valid state, otherwise returns
   // <b>false</b>.
   virtual bool AssertValid() const;

   //------------------------------------------------------------------------
   // Dumps the contents of the class to the given stream.
   virtual void Dump(WBFL::Debug::LogContext& os) const;
#endif // _DEBUG


private:
   pgsPointOfInterest         m_Poi;
   pgsVerticalShearArtifact   m_VerticalShearArtifact;
   pgsHorizontalShearArtifact m_HorizontalShearArtifact;
   pgsStirrupDetailArtifact   m_StirrupDetailArtifact;
   pgsLongReinfShearArtifact  m_LongReinfShearArtifact;
};

#endif // INCLUDED_PGSEXT_SHEARARTIFACT_H_
