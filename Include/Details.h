///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#ifndef INCLUDE_DETAILS_H_
#define INCLUDE_DETAILS_H_

#include <PGSuperTypes.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <Lrfd\Lrfd.h>
#include <WBFLRCCapacity.h>

struct SECTIONHAUNCH
{
   pgsPointOfInterest PointOfInterest;
   Float64 Station;
   Float64 Offset;
   Float64 tSlab;
   Float64 Fillet;
   Float64 ElevAlignment;
   Float64 ElevGirder;
   Float64 CrownSlope;
   Float64 GirderOrientation;
   Float64 Wtop;
   Float64 C;
   Float64 D;
   Float64 CamberEffect;
   Float64 GirderOrientationEffect;
   Float64 ProfileEffect;
   Float64 RequiredHaunchDepth;
   Float64 ElevTopGirder;
   Float64 TopSlabToTopGirder;
};

struct HAUNCHDETAILS
{
   std::vector<SECTIONHAUNCH> Haunch;
   Float64 RequiredSlabOffset; // "A" Dimension
   Float64 HaunchDiff; // maximum difference in haunch thickness
};

struct MOMENTCAPACITYDETAILS
{
   Float64 Mn;        // Nominal moment capacity
   Float64 Phi;       // Strength reduction factor
   Float64 PPR;       // Partial prestress ratio at this section
   Float64 MomentArm; // Distance between dc and de
   Float64 c;         // Distance from extreme compression fiber to the neutral axis
   Float64 dc;        // Distance from extreme compression fiber to the resultant compressive force
   Float64 de;        // Distance from extreme compression fiber to the resultant tensile force (used to compute c/de)
   Float64 de_shear;  // Distance from extreme compression fiber to the resultant tensile force for only those strands in tension (used for shear)
   Float64 C;         // Resultant compressive force
   Float64 T;         // Resultant tensile force

   int Method;        // LRFD_METHOD or WSDOT_METHOD
                      // WSDOT_METHOD = variable phi factor
                      // LRFD_METHOD = over reinforce capacity per C5.7.3.3.1

   // WSDOT_METHOD
   Float64 dt;        // Depth from extreme compression fiber to cg of lowest piece of reinforcement
   Float64 et;        // Net tensile strain
   Float64 etl;       // Tension Control Strain Limit
   Float64 ecl;       // Compression Control Strain Limit

   Float64 fps_avg;   // Average stress in strands at nominal resistance
   Float64 fpt_avg;   // Average stress in tendons at nominal resistance

   // LRFD_METHOD 
   // For C5.7.3.3.1... Capacity of over reinforced section
   bool    bOverReinforced; // True if section is over reinforced
   bool    bRectSection;    // True if rectangular section behavior
   Float64 Beta1Slab;       // Beta1 for slab only... B1 and f'c of slab are used in these calcs
   Float64 FcSlab;
   Float64 hf;
   Float64 b;
   Float64 bw;
   Float64 MnMin;           // Minimum nominal capacity of a over reinforced section (Eqn C5.7.3.3.1-1 or 2)

   Float64 fpe_ps; // Effective prestress
   Float64 eps_initial; // Initial strain in strands

   std::vector<Float64> fpe_pt; // Effective prestress
   std::vector<Float64> ept_initial; // Initial strain in strands

   // solution object provides the full equilibrium state of the moment
   // capacity solution
   CComPtr<IMomentCapacitySolution> CapacitySolution;
};

struct CRACKINGMOMENTDETAILS
{
   Float64 Mcr;  // Cracking moment
   Float64 Mdnc; // Dead load moment on non-composite girder
   Float64 fr;   // Rupture stress
   Float64 fcpe; // Stress at bottom of non-composite girder due to prestress
   Float64 Sb;   // Bottom section modulus of non-composite girder
   Float64 Sbc;  // Bottom section modulus of composite girder
   Float64 McrLimit; // Limiting cracking moment ... per 2nd Edition + 2003 interims (changed in 2005 interims)

   Float64 g1,g2,g3; // gamma factors from LRFD 5.7.3.3.2 (LRFD 6th Edition, 2012)
};

struct MINMOMENTCAPDETAILS
{
   std::_tstring LimitState; // Limit State for the minimum magnitude of Mu
   Float64 Mr;     // Nominal resistance (phi*Mn)
   Float64 MrMin;  // Minimum nominal resistance Max(MrMin1,MrMin2)
   Float64 MrMin1; // 1.2Mcr
   Float64 MrMin2; // 1.33Mu
   Float64 Mcr;
   Float64 Mu;
};

struct SHEARCAPACITYDETAILS
{
   SHEARCAPACITYDETAILS()
   {
      memset((void*)this,0,sizeof(SHEARCAPACITYDETAILS));
   };

   // [IN]
   ShearCapacityMethod Method; // General or Simplfied per LRFD 5.8.3.4.3 (Vci/Vcw - added to LRFD in 2007)
   Float64 Nu;
   Float64 Mu;
   Float64 RealMu; // Actual Mu computed from structural analysis. Same as Mu if MuLimitUsed is false
   Float64 PhiMu; // capacity reduction factor for moment
   bool    MuLimitUsed; // true if Mu set equal to Vu*dv per 2000 version of LRFD
   Float64 Vi; // Vu that goes with Mu
   Float64 Vu; // Maximum Vu at section
   Float64 Vd; // Vdc + Vdw
   Float64 Vp;  // vertical component of prestress Vps + Vpt
   Float64 Vps; // vertical component of prestress due to strands
   Float64 Vpt; // vertical component of prestress due to tendons
   Float64 Phi;
   Float64 dv;
   Float64 bv;
   Float64 fpeps;
   Float64 fpept;
   Float64 fpc;
   Float64 Es;
   Float64 As;
   Float64 Eps;
   Float64 Aps;
   Float64 Ept;
   Float64 Apt;
   Float64 Ec;
   Float64 Ac;
   Float64 fc;
   Float64 Av;
   Float64 fy;
   Float64 S;
   Float64 Alpha;
   Float64 de;
   Float64 h;
   Float64 ag; // max aggregate size
   Float64 sx; // spacing between layers of longitudinal cracking steel
   Float64 MomentArm;
   CRACKINGMOMENTDETAILS McrDetails;
   bool    bTensionBottom; // true if the flexural tension side is on the bottom of the girder

   // [OUT]
   Float64 fpops; // fpo for strand
   Float64 fpopt; // fpo for tendon
   bool ShearInRange; // If this is true, the applied shear was in range so
                      // shear capacity could be calculated. Otherwise all
                      // values below to Vn1 are not defined.
   Float64 vu;  // Shear stress per 5.8.9.2
   Float64 vufc; // Shear stress / f'c
   Float64 ex;
   Float64 Fe;  // -1 if not applicable
   Float64 Beta;
   Int16   BetaEqn; // Equation used to compute Beta (only applicable since LRFD 2009)
   Int16   BetaThetaTable; // Table used to compute Beta and Theta
   Float64 sxe; // [E5.8.3.4.2-5]
   Float64 sxe_tbl;
   Float64 Theta;
   Float64 Vc;
   Float64 Vs;
   Float64 Vn1;  // [E5.8.3.3-1]
   Float64 Vn2;  // [E5.8.3.3-2]
   Float64 Vn;   // Nominal shear resistance
   Float64 pVn;  // Factored nominal shear resistance
   Float64 VuLimit; // Limiting Vu where stirrups are required [E5.8.2.4-1]
   bool bStirrupsReqd;
   Int16 Equation; // Equation used to comupte ex (Only applicable after LRFD 1999)
   Float64 vfc_tbl;
   Float64 ex_tbl;

   Float64 VciMin;
   Float64 VciCalc;
   Float64 Vci;
   Float64 Vcw;

   Float64 VsReqd;
   Float64 AvOverS_Reqd;

   pgsTypes::ConcreteType ConcreteType;
   bool bHasFct;
   Float64 fct;
};

// fpc - strand stress for shear capacity calculation
struct FPCDETAILS
{
   Float64 eps; // Eccentricity of prestress strand
   Float64 Pps; // Prestress force
   Float64 ept; // Eccentricity of post-tension strand
   Float64 Ppt; // Post-tension force
   Float64 Ag;  // Area of non-composite girder
   Float64 Ig;  // Moment of inertia of non-composite girder
   Float64 Ybg; // Ybottom of girder
   Float64 Ybc; // Ybottom of composite girder
   Float64 c;   // Distance to stress point from non-comp neutral axis
   Float64 Mg;  // Dead load moment on the girder
   Float64 fpc; // Stress at composite cg due to dl and ps on nc girder.
};

// data at poi's used to interpolate critical section
struct CRITSECTIONDETAILSATPOI
{
   CRITSECTIONDETAILSATPOI() 
   { 
      // Critical section POI must be exact... do not merge them with other POI
      Poi.CanMerge(false); 
   }

   enum IntersectionType {DvIntersection, ThetaIntersection, NoIntersection};
   IntersectionType Intersection;

   pgsPointOfInterest Poi;
   Float64            DistFromFOS;
   Float64            Dv;
   bool               InRange; // false if theta calculation couldn't be done
   Float64            Theta;
   Float64            CotanThetaDv05;

   bool operator<(const CRITSECTIONDETAILSATPOI& other)
   {   return Poi < other.Poi;  }
};

// Critical Section for shear details
struct CRITSECTDETAILS
{
   // if there is an uplift reaction, the critical section is at the face of support
   bool bAtFaceOfSupport;
   pgsPointOfInterest poiFaceOfSupport;

   PierIndexType PierIdx; // index of the pier this critical section is associated with
   pgsTypes::PierFaceType PierFace;
   Float64 Start, End; // start/end of the critical section zone (the zone over which shear
   // is influenced by direct compression). Measured from start of segment

   // data at all pois
   std::vector<CRITSECTIONDETAILSATPOI> PoiData;

   // intersection locations
   CRITSECTIONDETAILSATPOI CsDv;   // section locations from dv calculation
   CRITSECTIONDETAILSATPOI CsDvt;  // section locations from .5*dv*cot(theta) calculation
   CRITSECTIONDETAILSATPOI* pCriticalSection; // pointer to the actual critical section

   CRITSECTDETAILS()
   {
      bAtFaceOfSupport = false;
      PierIdx = INVALID_INDEX;
      PierFace = pgsTypes::Ahead;
      Start = 0;
      End = 0;
      pCriticalSection = NULL;
   }

   CRITSECTDETAILS(const CRITSECTDETAILS& other)
   {
      bAtFaceOfSupport = other.bAtFaceOfSupport;
      poiFaceOfSupport = other.poiFaceOfSupport;
      PierIdx          = other.PierIdx;
      PierFace         = other.PierFace;
      Start            = other.Start;
      End              = other.End;
      PoiData          = other.PoiData;
      CsDv             = other.CsDv;
      CsDvt            = other.CsDvt;

      if ( &other.CsDv == other.pCriticalSection )
      {
         pCriticalSection = &CsDv;
      }
      else if ( &other.CsDvt == other.pCriticalSection )
      {
         pCriticalSection = &CsDvt;
      }
      else
      {
         pCriticalSection = NULL;
      }
   }

   const pgsPointOfInterest& GetPointOfInterest() const { return bAtFaceOfSupport ? poiFaceOfSupport : pCriticalSection->Poi; }
   void SetPointOfInterest(const pgsPointOfInterest& poi)
   {
      if ( bAtFaceOfSupport )
      {
         poiFaceOfSupport = poi;
      }
      else
      {
         pCriticalSection->Poi = poi;
      }
   }
};

struct CREEPCOEFFICIENTDETAILS
{
   Uint32 Method;          // a CREEP_xxx constant
   Uint32 Spec; // spec type... pre 2005 or 2005 and later... CREEP_SPEC_XXXX constant
   Float64 Ct; // This is the creep coefficient

   Float64 ti; // concrete age at time of loading
   Float64 t;  // amount of time load has been applied to concrete

   // The remaining parameters are for LRFD method
   Uint32 CuringMethod;
   Float64 VSratio; // Volume to surface ratio
   Float64 Fc;      // Concrete strength at time of loading
   Float64 H;       // Relative Humidity

   // before 2005 interim
   Float64 kf;
   Float64 kc;

   // 2005 and later
   Float64 kvs;
   Float64 khc;
   //Float64 kf; // using the kf above
   Float64 ktd;

   Float64 K1, K2; // 2005 and later, from NCHRP Report 496
};

struct INCREMENTALCREEPDETAILS
{
   boost::shared_ptr<matConcreteBaseCreepDetails> pStartDetails;
   boost::shared_ptr<matConcreteBaseCreepDetails> pEndDetails;
};

struct INCREMENTALSHRINKAGEDETAILS
{
   INCREMENTALSHRINKAGEDETAILS() : esi(0) {}
   boost::shared_ptr<matConcreteBaseShrinkageDetails> pStartDetails;
   boost::shared_ptr<matConcreteBaseShrinkageDetails> pEndDetails;
   Float64 esi;
};

// Details of incremental relaxation computation used for time-step analysis
struct INCREMENTALRELAXATIONDETAILS
{
   INCREMENTALRELAXATIONDETAILS()
   {
      memset((void*)this,0,sizeof(INCREMENTALRELAXATIONDETAILS));
   }

   // common parameters
   Float64 fpi; // effective prestress at the start of the interval
   Float64 fpy;
   Float64 fpu;
   Float64 tStart;
   Float64 tEnd;
   Float64 epoxyFactor;

   // These parameters are for AASHTO and ACI209 models
   Float64 K;

   // These parameters are for CEB-FEP model
   Float64 p;
   Float64 k;

   // Incremental relaxation
   Float64 fr;
};

#define TIMESTEP_CR  0
#define TIMESTEP_SH  1
#define TIMESTEP_RE  2

// This struct holds the computation details for a cross section of a concrete part
// for a specific interval for a time step loss analysis
// The concrete part could be a girder segment, closure joint, or deck
struct TIME_STEP_CONCRETE
{
   //
   // TIME STEP ANALYSIS INPUT PARAMETERS
   //

   // Net Section Properties of concrete part
   Float64 An;  // Area
   Float64 Yn;  // Centroid measured in Girder Section Coordinates
   Float64 In;  // Moment of inertia
   Float64 H;   // Height of concrete part
   Float64 E;   // Modulus of Elasticity used for computing transformed section properties

   // Creep Strains during this interval due to loads applied in previous intervals
   struct CREEP_STRAIN
   {
      Float64 P;
      Float64 E; // modulus of elasticity used to compute creep strain (Not age adjusted)
      Float64 A;
      Float64 Cs; // C(i+1/2,j)
      Float64 Ce; // C(i-1/2,j)
      Float64 Xs; // concrete Aging coefficient X(i+1/2,j)
      Float64 Xe; // concrete Aging coefficient X(i-1/2,j)
      Float64 e;
      CREEP_STRAIN()
      {
         P = 0;
         E = 0;
         A = 0;
         Cs = 0;
         Ce = 0;
         Xs = 1;
         Xe = 1;
         e = 0;
      }
   };

   struct CREEP_CURVATURE
   {
      Float64 M;
      Float64 E; // modulus of elasticity used to compute creep curvature (Not age adjusted)
      Float64 I;
      Float64 Cs; // C(i+1/2,j)
      Float64 Ce; // C(i-1/2,j)
      Float64 Xs; // concrete Aging coefficient X(i+1/2,j)
      Float64 Xe; // concrete Aging coefficient X(i-1/2,j)
      Float64 r;
      CREEP_CURVATURE()
      {
         M = 0;
         E = 0;
         I = 0;
         Cs = 0;
         Ce = 0;
         Xs = 1;
         Xe = 1;
         r = 0;
      }
   };
   std::vector<CREEP_STRAIN> ec;    // = (dN(j)/(AE(j))*[X(i+1/2,j)*C(i+1/2,j) - X(i-1/2,j*)C(i-1/2,j)]
   std::vector<CREEP_CURVATURE> rc; // = (dM(j)/(IE(j))*[X(i+1/2,j)*C(i+1/2,j) - X(i-1/2,j)*C(i-1/2,j)]
   std::vector<INCREMENTALCREEPDETAILS> Creep; // creep coefficient details

   // Unrestrained deformations due to creep and shrinkage in concrete during this interval
   INCREMENTALSHRINKAGEDETAILS Shrinkage;
   Float64 eci; // sum of (dN(j)/(AE(j))*[C(i+1/2,j) - C(i-1/2,j)]
   Float64 rci; // sum of (dM(j)/(IE(j))*[C(i+1/2,j) - C(i-1/2,j)]

   // Restraining force (force required to cause deformations that are
   // equal and opposite to the unrestrained deformations)
   Float64 PrCreep; // = -E*An*e
   Float64 MrCreep; // = -E*In*r
   Float64 PrShrinkage; // = -E*An*esi

   //
   // TIME STEP ANALYSIS OUTPUT PARAMETERS
   //

   Float64 dei[19];
   Float64 de;

   Float64 ei[19];
   Float64 e;

   Float64 dri[19];
   Float64 dr;

   Float64 ri[19];
   Float64 r;

   // Force on this concrete part due to elastic effects during this interval
   Float64 dPi[19]; // index is one of the pgsTypes::ProductForceType enum values
   Float64 dMi[19];
   Float64 dP, dM; // summation of dPi and dMi

   // Force on this concrete part at the end of this interval
   Float64 Pi[19]; // = (P in previous interval) + dP;
   Float64 Mi[19]; // = (M in previous interval) + dM;
   Float64 P, M; // summation of Pi and Mi

   // Stress at the end of this interval = stress at end of previous interval + dP/An + dM*y/In 
   // where y is the depth from the top of the concrete part
   Float64 f[2][19][2]; // first index is one of the pgsTypes::FaceType enums, second index is one of the pgsTypes::ProductForceType enum values
                        // third index is one of the ResultsType enum values

   // Stress in this due to live load
   Float64 fLLMin[2]; // index is pgsTypes::FaceType
   Float64 fLLMax[2];

   TIME_STEP_CONCRETE()
   {
      An = 0;
      Yn = 0;
      In = 0;
      H  = 0;
      E  = 0;

      eci = 0;
      rci = 0;

      PrCreep = 0;
      MrCreep = 0;

      PrShrinkage = 0;

      de = 0;
      e  = 0;

      dr = 0;
      r  = 0;

      dP = 0;
      dM = 0;
      P = 0;
      M = 0;

      int n = sizeof(dPi)/sizeof(dPi[0]);
      for ( int i = 0; i < n ; i++ )
      {
         dei[i] = 0;
         ei[i]  = 0;

         dri[i] = 0;
         ri[i]  = 0;

         dPi[i] = 0;
         dMi[i] = 0;

         Pi[i] = 0;
         Mi[i] = 0;

         f[pgsTypes::TopFace][i][0] = 0;
         f[pgsTypes::TopFace][i][1] = 0;

         f[pgsTypes::BottomFace][i][0] = 0;
         f[pgsTypes::BottomFace][i][1] = 0;
      }

      fLLMin[pgsTypes::TopFace]    = 0;
      fLLMin[pgsTypes::BottomFace] = 0;
      fLLMax[pgsTypes::TopFace]    = 0;
      fLLMax[pgsTypes::BottomFace] = 0;
   }
};

// This struct holds the computation details for a strand part
// for a specific interval for a time step loss analysis
// The strand part could be a pretensioned or posttension strand
struct TIME_STEP_STRAND
{
   //
   // TIME STEP ANALYSIS INPUT PARAMETERS
   //

   // Time Parameters of the interval
   Float64 tEnd; // time since stressing of strand to end of this interval

   // Geometric Properties
   Float64 As;
   Float64 Ys; // centroid of strand, measured in Girder Section Coordinates
   Float64 E;

   // Relaxation
   INCREMENTALRELAXATIONDETAILS Relaxation;
   Float64 er; // apparent deformation due to relaxation (fr/Eps)
   Float64 PrRelaxation; // restraining force (= -Eps*Ap*er = -fr*As)

   Float64 Pj;  // jacking force  (includes deductions for anchors set and friction if PT)
   Float64 fpj; // jacking stress (includes deductions for anchors set and friction if PT)

   //
   // TIME STEP ANALYSIS OUTPUT PARAMETERS
   //
   Float64 dei[19]; // change in strain in strand due to deformations in this interval
   Float64 de; // summation of dei

   Float64 dPi[19]; // change in force in strand due to deformations in this interval
   Float64 dP; // summation of dPi

   Float64 ei[19]; // strain in strand at end of this interval = (e previous interval + de)
   Float64 e; // summation of ei
   
   Float64 Pi[19]; // force in strand at end of this interval = (P previous interval + dP)
   Float64 P; // summation of Pi

   // Loss/Gain during this interval (change in effective prestress this interval)
   Float64 dfpei[19]; // = dP/Aps
   Float64 dfpe; // summation of dfpei

   // Effective prestress
   Float64 fpe; // = fpj + Sum(dfpe for all intervals up to and including this interval)
   // also fpe = fpe (previous interval) + dfpe (this interval)

   // sum of losses up to and including this interval
   // this is an effective loss that includes time-dependent and elastic effects
   Float64 loss; // = Sum(dfpe for all intervals up to and including this interval)
   // also loss = loss (previous interval) + dfpe (this interval)

   // Elastic effect of live load on effective prestress
   Float64 dFllMin;
   Float64 fpeLLMin; // fpe + dFll;
   Float64 lossLLMin; // loss - dFll;

   Float64 dFllMax;
   Float64 fpeLLMax; // fpe + dFll;
   Float64 lossLLMax; // loss - dFll;

   // This value can be checked by (-Pj+P) = fpe*Aps
   TIME_STEP_STRAND()
   {
      tEnd = 0;
      As = 0;
      Ys = 0;
      E = 0;
      er = 0;
      PrRelaxation = 0;

      Pj  = 0;
      fpj = 0;

      dP = 0;
      P = 0;

      de = 0;
      e = 0;

      int n = sizeof(dPi)/sizeof(dPi[0]);
      for ( int i = 0; i < n; i++ )
      {
         dei[i] = 0;
         ei[i] = 0;

         dPi[i] = 0;
         Pi[i]  = 0;

         dfpei[i] = 0;
      }

      dfpe = 0;
      fpe  = 0;
      loss = 0;

      dFllMin   = 0;
      fpeLLMin  = 0;
      lossLLMin = 0;

      dFllMax   = 0;
      fpeLLMax  = 0;
      lossLLMax = 0;
   }
};

// This struct holds the computation details for a rebar part
// for a specific interval for a time step loss analysis
// The rebar part could be deck or girder segment rebar
struct TIME_STEP_REBAR
{
   //
   // TIME STEP ANALYSIS INPUT PARAMETERS
   //

   // Geometric Properties
   Float64 As;
   Float64 Ys; // centroid of rebar, measured in Girder Section Coordinates
   Float64 E;

   //
   // TIME STEP ANALYSIS OUTPUT PARAMETERS
   //
   Float64 dei[19]; // change in strain in bar due to deformations in this interval
   Float64 de; // summation of dei

   Float64 dPi[19]; // change in force in bar during this interval
   Float64 dP; // summation of dPi

   Float64 ei[19]; // strain in bar at end of this interval = (e previous interval + de)
   Float64 e; // summation of ei

   Float64 Pi[19]; // force in rebar at end of this interval = (P previous interval + dP)
   Float64 P; // summation of Pi

   TIME_STEP_REBAR()
   {
      As = 0;
      Ys = 0;
      E = 0;

      dP = 0;
      P = 0;

      de = 0;
      e = 0;

      int n = sizeof(dPi)/sizeof(dPi[0]);
      for ( int i = 0; i < n; i++ )
      {
         dei[i] = 0;
         ei[i] = 0;

         dPi[i] = 0;
         Pi[i]  = 0;
      }
   }
};

// This struct holds the computation details for a specific interval 
// for a time step loss analysis
struct TIME_STEP_DETAILS
{
   //
   // TIME STEP ANALYSIS INPUT PARAMETERS
   //
   // (not really needed, but will help with debugging)
   IntervalIndexType intervalIdx;
   Float64 tStart, tMiddle, tEnd; // time at start, middle, end of interval

   // Transformed Section Properties for this interval (based on age adjusted modulus of the girder)
   // Section Properties are taken about the centroid of the transformed section
   // The centroid, Ytr, is in Girder Section Coordinate (measured from top of girder, up is positive)
   Float64 Atr, Ytr, Itr;

   Float64 E; // modulus used to transform properties into an equivalent material

   // Change in total loading on the section due to externally applied loads during this interval
   // Array index is one of the pgsTypes::ProductForceType enum values
   // upto and including pgsTypes::pftRelaxation
   Float64 dPi[19], dMi[19];

   // total change in loading on the section (summation of dPi and dMi)
   Float64 dP, dM;

   // Total loading on the section due to externally applied loads in all intervals upto
   // and including this interval. Array index is one of the pgsTypes::ProductForceType enum values
   // upto and including pgsTypes::pftRelaxation
   Float64 Pi[19], Mi[19];

   // total change in loading on the section (summation of Pi and Mi)
   Float64 P, M;

   // Time step parameters for girder and deck
   TIME_STEP_CONCRETE Girder;
   TIME_STEP_CONCRETE Deck;

   // Time step parameters for strands and tendons
#if defined LUMP_STRANDS
   TIME_STEP_STRAND Strands[3]; // pgsTypes::StrandType (Straight, Harped, Temporary)
#else
   std::vector<TIME_STEP_STRAND> Strands[3]; // pgsTypes::StrandType (Straight, Harped, Temporary)
#endif
   std::vector<TIME_STEP_STRAND> Tendons; // one per duct

   // Time step parameters for rebar
   // access first array with pgsTypes::DeckRebarMatType and second with pgsTypes::DeckRebarBarType
   // e.g DeckRebar[pgsTypes::drmTop][pgsTypes::drbIndividual]
   // Don't use pgsTypes::drbAll
   TIME_STEP_REBAR DeckRebar[2][2];

   // Time step parameters for girder rebar
   std::vector<TIME_STEP_REBAR> GirderRebar;

   // Forces required to totally restrain the cross section for initial strains occuring during this interval
   Float64 Pr[3], Mr[3]; // index is one of the TIMESTEP_XXX constants

   // Initial Strains 
   Float64 e[3]; // index is one of the TIMESTEP_XXX constants
   Float64 r[3];

   // Deformation due to externally applied loads and restraining forces in this interval
   Float64 der[19]; // axial strain
   Float64 drr[19]; // curvature

   // Total deformation due to externally applied loads and restraining forces
   Float64 er[19]; // axial strain
   Float64 rr[19]; // curvature

   // Check equilibrium
   Float64 dPext, dPint; // change in external and internal axial force during this interval (dPext == dPint)
   Float64 dMext, dMint; // change in external and internal moment during this interval (dMext == dMint)
   Float64 Pext, Pint;   // sum of external and internal axial forces through the end of this interval (Pext == Pint)
   Float64 Mext, Mint;   // sum of external and internal moments through the end of this interval (Mext == Mint)

   TIME_STEP_DETAILS()
   {
      intervalIdx = INVALID_INDEX;
      tStart  = 0;
      tMiddle = 0;
      tEnd    = 0;

      Atr = 0;
      Ytr = 0;
      Itr = 0;
      E   = 0;

      int n = sizeof(dPi)/sizeof(dPi[0]);
      for ( int i = 0; i < n ; i++ )
      {
         dPi[i] = 0;
         dMi[i] = 0;

         Pi[i] = 0;
         Mi[i] = 0;

         der[i] = 0;
         drr[i] = 0;

         er[i] = 0;
         rr[i] = 0;
      }

      for (int i = 0; i < 3; i++)
      {
         Pr[i] = 0;
         Mr[i] = 0;

         e[i] = 0;
         r[i] = 0;
      }

      dP = 0;
      dM = 0;

      P = 0;
      M = 0;

      dPext = 0;
      dPint = 0;
      dMext = 0;
      dMint = 0;
      Pext  = 0;
      Pint  = 0;
      Mext  = 0;
      Mint  = 0;
   }
};

// This struct holds the computation details for the basic
// anchor set parameters. The actual anchor set at a POI
// is stored in FRICTIONLOSSDETAILS. This struct just has
// the parameters for the seating wedge
struct ANCHORSETDETAILS
{
   ANCHORSETDETAILS()
   { 
      girderKey = CGirderKey(INVALID_INDEX,INVALID_INDEX); 
      ductIdx = INVALID_INDEX;
      for ( int i = 0; i < 2; i++ )
      {
         Lset[i]  = 0;
         dfpAT[i] = 0;
         dfpS[i]  = 0;
      }
   }

   // Key
   CGirderKey girderKey;
   DuctIndexType ductIdx;

   // Value
   // Array index is pgsTypes::MemberEndType
   Float64 Lset[2]; // Anchor set zone length
   Float64 dfpAT[2]; // Loss of effective stress at anchorage due to seating
   Float64 dfpS[2];  // Loss of effective stress at end of anchor set zone length due to seating
                     // This is typically zero except when the anchor set zone is longer than the tendon
};

// This struct holds the computation details for friction and anchor set
// losses at a POI
struct FRICTIONLOSSDETAILS
{
   Float64 alpha; // total angular change from jacking end to this POI
   Float64 X;     // distance from start of tendon to this POI
   Float64 dfpF;  // friction loss at this POI
   Float64 dfpA;  // anchor set loss at this POI
};

// This struct holds the computation details for prestress losses
// at a POI
struct LOSSDETAILS
{
   LOSSDETAILS() {;}

   LOSSDETAILS(const LOSSDETAILS& other)
   { MakeCopy(other); }

   LOSSDETAILS& operator=(const LOSSDETAILS& other)
   { return MakeCopy(other); }

   LOSSDETAILS& MakeCopy(const LOSSDETAILS& other)
   {
      LossMethod = other.LossMethod;
      pLosses = other.pLosses;

      FrictionLossDetails = other.FrictionLossDetails;

      TimeStepDetails = other.TimeStepDetails;

#if defined _DEBUG
      POI = other.POI;
#endif

      return *this;
   }

   // Method for computing prestress losses... the value of this parameter
   // defines which of the loss details given below are applicable
   pgsTypes::LossMethod LossMethod;

   ///////////////////////////////////////////////////////////////////////////////
   // Losses computed by LRFD Approximate or Refined methods or general lump sum
   ///////////////////////////////////////////////////////////////////////////////

   // THESE LOSS OBJECTS NOT VALID WITH TIME_STEP METHOD

   // LRFD Method Losses
   // Base class can be casted to derived class to get details. You know who you are!
   boost::shared_ptr<const lrfdLosses> pLosses;


   ///////////////////////////////////////////////////////////////////////////////
   // Losses computed by Time Step Method
   ///////////////////////////////////////////////////////////////////////////////

   // Friction and Anchor Set Losses
   // vector index is the duct index
   std::vector<FRICTIONLOSSDETAILS> FrictionLossDetails;

   // vector index in an interval index
   std::vector<TIME_STEP_DETAILS> TimeStepDetails;

#if defined _DEBUG
   pgsPointOfInterest POI; // this is the POI that this loss details applies to
#endif
};

struct XFERLENGTHDETAILS
{
   bool bMinuteValue; // if true, the transfer length was set to a very small value
   Float64 db; // strand diameter
   Int16 ndb; // number of strand diameters used for xfer length
   bool bEpoxy; // is strand grit epoxy coated
   Float64 lt; // transfer length
};

struct STRANDDEVLENGTHDETAILS
{
    // details of bonded and debonded strand development and transfer
    // length calculations... see LRFD 5.11.4.1  and 5.11.4.2
    Float64 db; // strand diameter
    Float64 fpe;
    Float64 fps;
    Float64 k;
    Float64 ld; // development length
    XFERLENGTHDETAILS ltDetails; // transfer length
};

#define NO_TTS          0 // lifting without TTS
#define PS_TTS          1 // lifting with prestressed TTS
#define PT_TTS_OPTIONAL 2 // post-tensioned TTS, lifting at NO_TTS lift point (if f'ci < lifting strength for NO_TTS, can lift at a lower strength if PT_TTS used)
#define PT_TTS_REQUIRED 3 // post-tensioned TTS, lifting at PS_TTS lift point (TTS required for lifting)

struct FABRICATIONOPTIMIZATIONDETAILS
{
   // Current temporary strand information
   StrandIndexType Nt;
   Float64 Pjack;
   pgsTypes::TTSUsage TempStrandUsage;

   Float64 Fci_FormStripping_WithoutTTS;

   // Required strength and support locations for lifting
   // index is one of the xx_TTS constants above
   Float64 Fci[4];
   Float64 L[4];

   bool bTempStrandsRequiredForShipping;

   Float64 Fc; // concrete strength required for shipping

   // Shipping - equal overhang limits
   Float64 Lmin;
   Float64 Lmax;

   // Shipping - unequal overhangs
   Float64 LUmin; // minimum overhang length
   Float64 LUmax; // maximum overhang length
   Float64 LUsum; // sum of overhang lengths

   FABRICATIONOPTIMIZATIONDETAILS()
   {
      memset((void*)this,0,sizeof(FABRICATIONOPTIMIZATIONDETAILS));
   };
};

struct CRACKEDSECTIONDETAILS
{
   Float64 c;   // distance from top of section to crack (crack depth)
   Float64 Icr; // based on girder material
   CComPtr<ICrackedSectionSolution> CrackedSectionSolution;
};

#endif // INCLUDE_DETAILS_H_