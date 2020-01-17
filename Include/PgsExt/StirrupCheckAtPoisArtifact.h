///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#include <Material\Rebar.h>

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
   // GROUP: DATA MEMBERS

   Float64 GetFy() const;
   void SetFy(Float64 fy);

   Float64 GetAs() const;
   void SetAs(Float64 as);

   Float64 GetAps() const;
   void SetAps(Float64 aps);

   Float64 GetFps() const;
   void SetFps(Float64 fps);

   Float64 GetApt() const;
   void SetApt(Float64 apt);

   Float64 GetFpt() const;
   void SetFpt(Float64 fpt);

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

   // lrfd demand and capacity criteria
   void SetDemandForce(Float64 area);
   Float64 GetDemandForce() const;
   void SetCapacityForce(Float64 area);
   Float64 GetCapacityForce() const;

   bool   Passed() const;

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsLongReinfShearArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsLongReinfShearArtifact(const pgsLongReinfShearArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsLongReinfShearArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsLongReinfShearArtifact& operator = (const pgsLongReinfShearArtifact& rOther);

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
   virtual void Dump(dbgDumpContext& os) const;
   #endif // _DEBUG

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsLongReinfShearArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsLongReinfShearArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   Float64 m_Fy;
   Float64 m_As;
   Float64 m_Aps;
   Float64 m_Fps;
   Float64 m_Apt;
   Float64 m_Fpt;
   Float64 m_Mu;
   Float64 m_Mr;
   Float64 m_Dv;
   Float64 m_FlexuralPhi;
   Float64 m_Nu;
   Float64 m_AxialPhi;
   Float64 m_Vu;
   Float64 m_ShearPhi;
   Float64 m_Vs;
   Float64 m_Vp;
   Float64 m_Theta;
   Float64 m_DemandForce;
   Float64 m_CapacityForce;
   Uint16  m_Equation;
   bool    m_IsApplicable;

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

   //------------------------------------------------------------------------
   // Default constructor
   pgsVerticalShearArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsVerticalShearArtifact(const pgsVerticalShearArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsVerticalShearArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsVerticalShearArtifact& operator = (const pgsVerticalShearArtifact& rOther);

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
   virtual void Dump(dbgDumpContext& os) const;
   #endif // _DEBUG

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsVerticalShearArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsVerticalShearArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   bool m_bIsApplicable;
   bool m_bIsStrutAndTieRequired;
   bool m_bEndSpacingApplicable;

   // 5.7.2.3 (pre2017: 5.8.2.4)
   bool m_AreStirrupsReqd;
   bool m_AreStirrupsProvided;

   Float64 m_AvOverSReqd;

   Float64 m_Demand;
   Float64 m_Capacity;

   Float64 m_AvSprovided;
   Float64 m_AvSatCS;

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
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsHorizontalShearArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsHorizontalShearArtifact(const pgsHorizontalShearArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsHorizontalShearArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsHorizontalShearArtifact& operator = (const pgsHorizontalShearArtifact& rOther);

   // GROUP: OPERATIONS

   // Not applicable if there is no composite slab
   bool IsApplicable() const;
   void SetApplicability(bool isApplicable);

   // additional stirrups in top flange
   Float64 GetAvfAdditional() const {return m_AvfAdditional;}
   void SetAvfAdditional(Float64 avf) {m_AvfAdditional = avf;}
   Float64 GetSAdditional() const;
   void SetSAdditional(Float64 s);

   // stirrups in girder
   Float64 GetAvfGirder() const {return m_AvfGirder;}
   void SetAvfGirder(Float64 avf) {m_AvfGirder = avf;}
   Float64 GetSGirder() const;
   void SetSGirder(Float64 s);

   // composite stirrup Av/S
   Float64 GetAvOverS() const;
   // max stirrup spacing for min spacing check
   Float64 GetSpacing() const;

   Float64 GetNormalCompressionForce() const {return m_NormalCompressionForce;}
   void SetNormalCompressionForce(Float64 force) {m_NormalCompressionForce = force;}
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
   virtual void Dump(dbgDumpContext& os) const;
   #endif // _DEBUG

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsHorizontalShearArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsHorizontalShearArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   bool    m_IsApplicable;
   Float64 m_AvfAdditional;
   Float64 m_SAdditional;
   Float64 m_AvfGirder;
   Float64 m_SGirder;
   Float64 m_UltimateHorizontalShear;
   Float64 m_NormalCompressionForce;
   Float64 m_Acv;
   Float64 m_CohesionFactor;
   Float64 m_FrictionFactor;
   Float64 m_K1, m_K2; // strength factors added in LRFD 2007
   Float64 m_Phi;
   Float64 m_Vn1;
   Float64 m_Vn2;
   Float64 m_Vn3;
   Float64 m_Fc; // F'c used to compute in Eqn 5.7.4.3-2 (pre2017: 5.8.4.1-2)
   bool m_bDoAllPrimaryStirrupsEngageDeck;
   bool m_bIsTopFlangeRoughened;

   Float64 m_Bv;
   Float64 m_Smax; // maximum allowable spacing [5.7.4.5 (pre2017: 5.8.4.2)]
   Float64 m_Fy;
   bool    m_bWasFyLimited; // true if fy is limited to 60 ksi per LRFD2013 5.7.4.3 (pre2017: 5.8.4.1)
   Float64 m_AvOverSMin_5_7_4_2_1;
   Float64 m_AvOverSMin_5_7_4_1_3;
   Float64 m_AvOverSMin;
   Float64 m_NumLegs;
   Float64 m_NumLegsReqd;
   Float64 m_VsAvg;   // average shear strength
   Float64 m_VsLimit; // max shear strength at which 5.7.4.3-4 (pre2017: 5.8.4.1-4) is not applicable

   // parameters for checking end zone locations
   bool m_bEndSpacingApplicable;
   Float64 m_AvSprovided;
   Float64 m_AvSatCS;


   // parameters used to compute horizonal shear from vertical shear
   Float64 m_Dv;
   Float64 m_I;
   Float64 m_Q;
   Float64 m_Vu; // vertical shear

   Float64 m_AvsReqd;



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
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsStirrupDetailArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsStirrupDetailArtifact(const pgsStirrupDetailArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsStirrupDetailArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsStirrupDetailArtifact& operator = (const pgsStirrupDetailArtifact& rOther);


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
   void SetBarSize(matRebar::Size s) {m_BarSize = s;}
   matRebar::Size GetBarSize() const {return m_BarSize;}
   void SetS(Float64 s) {m_S = s;}
   Float64 GetS() const {return m_S;}
   void SetBv(Float64 bv) {m_Bv = bv;}
   Float64 GetBv() const {return m_Bv;}
   void SetDv(Float64 dv) {m_Dv = dv;}
   Float64 GetDv() const {return m_Dv;}

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
   virtual void Dump(dbgDumpContext& os) const;
   #endif // _DEBUG

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsStirrupDetailArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsStirrupDetailArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   bool m_After1999; // after 1999 spec
   Float64 m_Fy;
   Float64 m_Fc;
   Float64 m_AvsMin;
   Float64 m_Avs;
   Float64 m_SMax;
   Float64 m_SMin;
   matRebar::Size m_BarSize;
   Float64 m_S;
   Float64 m_Bv;
   Float64 m_Dv;
   Float64 m_Vu; // 1999 and earlier used shear force for comparison
   Float64 m_VuLimit;
   Float64 m_vu; // post 1999 we use shear stress per 5.7.2.8 (pre2017: 5.8.2.9)
   Float64 m_vuLimit;
   bool m_IsApplicable;
   bool m_IsInCritialSectionZone;
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

   //------------------------------------------------------------------------
   // Default constructor
   pgsStirrupCheckAtPoisArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsStirrupCheckAtPoisArtifact(const pgsStirrupCheckAtPoisArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsStirrupCheckAtPoisArtifact();

   pgsStirrupCheckAtPoisArtifact& operator = (const pgsStirrupCheckAtPoisArtifact& rOther);
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
   virtual void Dump(dbgDumpContext& os) const;
#endif // _DEBUG

protected:
   void MakeCopy(const pgsStirrupCheckAtPoisArtifact& rOther);
   void MakeAssignment(const pgsStirrupCheckAtPoisArtifact& rOther);

private:
   pgsPointOfInterest         m_Poi;
   pgsVerticalShearArtifact   m_VerticalShearArtifact;
   pgsHorizontalShearArtifact m_HorizontalShearArtifact;
   pgsStirrupDetailArtifact   m_StirrupDetailArtifact;
   pgsLongReinfShearArtifact  m_LongReinfShearArtifact;
};

#endif // INCLUDED_PGSEXT_SHEARARTIFACT_H_
