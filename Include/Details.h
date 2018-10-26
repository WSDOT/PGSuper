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

#ifndef INCLUDE_DETAILS_H_
#define INCLUDE_DETAILS_H_

#include <PGSuperTypes.h>

#include <PgsExt\GirderPointOfInterest.h>
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
   Float64 CamberEffect;
   Float64 GirderOrientationEffect;
   Float64 ProfileEffect;
   Float64 RequiredHaunchDepth;
   Float64 ElevTopGirder;
   Float64 ActualHaunchDepth;
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

   Float64 fps;       // Stress in strand an nominal resistance

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

   Float64 fpe; // Effective prestress
   Float64 e_initial; // Initial strain in strands

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
   Float64 MrMin;  // Minimum nominal resistance max(MrMin1,MrMin2)
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
   Float64 RealMu; // Actual Mu computed from structural analysis. Same as Mu is MuLimitUsed is false
   Float64 PhiMu; // capacity reduction factor for moment
   bool    MuLimitUsed; // true if Mu set equal to Vu*dv per 2000 version of LRFD
   Float64 Vi; // Vu that goes with Mu
   Float64 Vu; // Maximum Vu at section
   Float64 Vd; // Vdc + Vdw
   Float64 Vp;
   Float64 Phi;
   Float64 dv;
   Float64 bv;
   Float64 fpe;
   Float64 fpc;
   Float64 Es;
   Float64 As;
   Float64 Ep;
   Float64 Aps;
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
   Float64 fpo;
   bool ShearInRange; // If this is true, the applied shear was in range so
                      // shear capacity could be calculated. Otherwise all
                      // values below to Vn1 are not defined.
   Float64 vfc;
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
   Float64 e;   // Eccentricity of prestress strand
   Float64 P;   // Prestress force
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
         pCriticalSection = &CsDv;
      else if ( &other.CsDvt == other.pCriticalSection )
         pCriticalSection = &CsDvt;
      else
         pCriticalSection = NULL;
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

#define TIMESTEP_CREEP      0
#define TIMESTEP_SHRINKAGE  1
#define TIMESTEP_RELAXATION 2

// This struct holds the computation details for a cross section of a concrete part
// for a specific interval for a time step loss analysis
// The concrete part could be a girder segment, closure pour, or deck
struct TIME_STEP_CONCRETE
{
   //
   // TIME STEP ANALYSIS INPUT PARAMETERS
   //

   // Net Section Properties of concrete part
   Float64 An;  // Area
   Float64 Ytn; // Centroid measured from top of girder
   Float64 Ybn; // Centroid measured from bottom of girder
   Float64 In;  // Moment of inertia

   // Creep Strains during this interval due to loads applied in previous intervals
   std::vector<Float64> ec; // = (Change in Creep Strain during interval i (de))*(Yce - Ycs)
   std::vector<Float64> rc; // = (Change in curvature during interval i (dr))*(Yce - Ycs)

   // Unrestrained deformations due to creep and shrinkage in concrete during this interval
   Float64 esi; // shrinkage
   Float64 e; // sum of ec vector
   Float64 r; // sum of rc vector

   // Restraining force (force required to cause deformations that are
   // equal and opposite to the unrestrained deformations)
   Float64 PrCreep; // = -Ea*An*e
   Float64 MrCreep; // = -Ea*In*r
   Float64 PrShrinkage; // = -Ea*An*esi

   //
   // TIME STEP ANALYSIS OUTPUT PARAMETERS
   //

   // Change in strain and curvature of this concrete part during this interval
   // due to externally applied loads and restraining forces.
   // er and rr are the deformation (axial strain and curvature) of the entire
   // cross section due to extrernally applied loads and restraining forces. er and
   // rr are found in the parent structure TIME_STEP_DETAILS. Ytr is the centroid
   // (measured down from the top of the girder) of the transformed section (also found on the TIME_STEP_DETAILS struct)
   Float64 de; // axial strain = er + rr(Yn-Ytr)
   Float64 dr; // curvature = rr

   // Force on this concrete part due to deformations during this interval
   Float64 dP; // = de*Ea*Ag + Pr+Pre
   Float64 dM; // = dr*Ea*In + Mr+Mre

   // Force on this concrete part at the end of this interval
   Float64 P; // = (P in previous interval) + dP;
   Float64 M; // = (M in previous interval) + dM;

   // Change in elastic strains during this interval (used for creep strains in next interval)
   Float64 dee; // = dP/(Ea*An)
   Float64 der; // = dM/(Ea*In)

   // Stress at the end of this interval = stress at end of previous interval + dP/An + dM*y/In 
   // where y is the depth from the top of the concrete part
   // ---- alternatively ----
   // Stress at the end of this interval = stress at end of previous interval + (dee + der*y)*Ea
   Float64 fTop;
   Float64 fBot;

   TIME_STEP_CONCRETE()
   {
      An = 0;
      Ytn = 0;
      Ybn = 0;
      In = 0;

      esi = 0;
      e = 0;
      r = 0;

      PrCreep = 0;
      MrCreep = 0;

      PrShrinkage = 0;

      de = 0;
      dr = 0;

      dP = 0;
      dM = 0;

      P = 0;
      M = 0;

      dee = 0;
      der = 0;

      fTop = 0;
      fBot = 0;
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
   Float64 Ys; // centroid of strand, measured from top of girder

   // Relaxation
   Float64 fr; // prestress loss due to relaxation during this interval
   Float64 er; // apparent deformation due to relaxation (fr/Eps)
   Float64 Pr; // restraining force (= -Eps*Aps*er)

   Float64 Pj;  // jacking force  (includes deductions for anchors set and friction if PT)
   Float64 fpj; // jacking stress (includes deductions for anchors set and friction if PT)

   //
   // TIME STEP ANALYSIS OUTPUT PARAMETERS
   //
   Float64 de; // strain at centroid of strand
   Float64 dP; // change in force in strand due to deformations in this interval
               // dP = de*Eps*Aps + Pr+Pre
   
   Float64 P; // force in strand at end of this interval = (P previous interval + dP)

   // Loss/Gain during this interval
   Float64 dFps; // = dP/Aps

   // Effective prestress
   Float64 fpe; // = fpj + Sum(dFps for all intervals up to and including this interval)
   // also fpe = fpe (previous) + dFps (this interval)

   // sum of losses up to and including this interval
   Float64 loss; // = loss (previous) + dFps

   // Elastic effect of live load on effective prestress
   Float64 dFll;
   Float64 fpeLL; // fpe + dFll;
   Float64 lossLL; // loss + dFll;

   // This value can be checked by (-Pj+P) = fpe*Aps
   TIME_STEP_STRAND()
   {
      tEnd = 0;
      As = 0;
      Ys = 0;
      fr = 0;
      er = 0;
      Pr = 0;

      Pj = 0;
      fpj = 0;

      de = 0;
      dP = 0;
      
      P = 0;

      dFps = 0;
      fpe = 0;
      loss = 0;

      dFll   = 0;
      fpeLL  = 0;
      lossLL = 0;
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
   Float64 Ys; // centroid of rebar, measured from top of girder

   //
   // TIME STEP ANALYSIS OUTPUT PARAMETERS
   //
   Float64 de; // change in strain at centroid of rebar
   Float64 dP; // change in force, dP = de*Eps*Aps

   Float64 P; // force in rebar at end of this interval = (P previous interval + dP)

   TIME_STEP_REBAR()
   {
      As = 0;
      Ys = 0;
      de = 0;
      dP = 0;
      P  = 0;
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
   // Ytr is measured from the top of the girder
   Float64 Atr, Ytr, Itr;

   // Externally applied loads
   Float64 P, M;

   // Time step parameters for girder and slab
   TIME_STEP_CONCRETE Girder;
   TIME_STEP_CONCRETE Slab;

   // Time step parameters for strands and tendons
   TIME_STEP_STRAND Strands[3]; // pgsTypes::StrandType (Straight, Harped, Temporary)
   std::vector<TIME_STEP_STRAND> Tendons; // one per duct

   // Time step parameters for rebar (access array with pgsTypes::DeckRebarMatType)
   TIME_STEP_REBAR DeckRebar[2];

   // Time step parameters for girder rebar
   std::vector<TIME_STEP_REBAR> GirderRebar;

   // Total restraining force in this interval
   Float64 Pr[3], Mr[3];

   // Initial Strains (access array with pgsTypes::Back and pgsTypes::Ahead for left and right side of POI)
   Float64 e[3][2];
   Float64 r[3][2];

   // Section results from analyzing the initial strains
   Float64 Pre[3];
   Float64 Mre[3];

   // Deformation due to externally applied loads and restraining forces in this interval
   Float64 er; // axial strain
   Float64 rr; // curvature

   // Check equilibrium
   Float64 dPext, dPint;
   Float64 dMext, dMint;
   Float64 Pext, Pint;
   Float64 Mext, Mint;

   TIME_STEP_DETAILS()
   {
      intervalIdx = INVALID_INDEX;
      tStart  = 0;
      tMiddle = 0;
      tEnd    = 0;

      Atr = 0;
      Ytr = 0;
      Itr = 0;

      P = 0;
      M = 0;

      for (int i = 0; i < 3; i++)
      {
         Pr[i] = 0;
         Mr[i] = 0;

         e[i][pgsTypes::Back]  = 0;
         e[i][pgsTypes::Ahead] = 0;
         r[i][pgsTypes::Back]  = 0;
         r[i][pgsTypes::Ahead] = 0;

         Pre[i] = 0;
         Mre[i] = 0;
      }

      er = 0;
      rr = 0;

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
         p[i]     = 0;
      }
   }

   // Key
   CGirderKey girderKey;
   DuctIndexType ductIdx;

   // Value
   // Array index is pgsTypes::MemberEndType
   Float64 Lset[2]; // Anchor set zone length
   Float64 dfpAT[2]; // Loss of effect stress at anchorage due to seating
   Float64 p[2]; // Friction loss per unit length

   bool operator<(const ANCHORSETDETAILS& other) const
   {
      if ( girderKey < other.girderKey )
         return true;

      if ( other.girderKey < girderKey )
         return false;

      return ductIdx < other.ductIdx;
   }
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
   LOSSDETAILS() { pLosses = 0; }
   LOSSDETAILS(const LOSSDETAILS& other)
   { MakeCopy(other); }
   LOSSDETAILS& operator=(const LOSSDETAILS& other)
   { return MakeCopy(other); }
   LOSSDETAILS& MakeCopy(const LOSSDETAILS& other)
   {
      LossMethod = other.LossMethod;

      RefinedLosses                     = other.RefinedLosses;
      RefinedLosses2005                 = other.RefinedLosses2005;
      ApproxLosses                      = other.ApproxLosses;
      ApproxLosses2005                  = other.ApproxLosses2005;
      LumpSum                           = other.LumpSum;

      if ( other.pLosses == &other.RefinedLosses2005 )
         pLosses = &RefinedLosses2005;
      else if ( other.pLosses == &other.RefinedLosses )
         pLosses = &RefinedLosses;
      else if ( other.pLosses == &other.ApproxLosses )
         pLosses = &ApproxLosses;
      else if ( other.pLosses == &other.ApproxLosses2005 )
         pLosses = &ApproxLosses2005;
      else if ( other.pLosses == &other.LumpSum )
         pLosses = &LumpSum;

      FrictionLossDetails = other.FrictionLossDetails;

      TimeStepDetails = other.TimeStepDetails;

      return *this;
   }

   // Method for computing prestress losses... the value of this parameter
   // defines which of the loss details given below are applicable
   pgsTypes::LossMethod LossMethod;

   ///////////////////////////////////////////////////////////////////////////////
   // Losses computed by LRFD Approximate or Refined methods or general lump sum
   ///////////////////////////////////////////////////////////////////////////////

   // THESE LOSS OBJECTS NOT VALID WITH TIME_STEP METHOD

   // LRFD Method Losses (details can be extracted from these objects)
   const lrfdLosses* pLosses;                  // pointer to one of the loss objects below
   lrfdLumpSumLosses LumpSum;                  // general lump sum losses
   lrfdRefinedLosses RefinedLosses;            // refined before LRFD 2005
   lrfdApproximateLosses ApproxLosses;         // approximate before LRFD 2005
   lrfdRefinedLosses2005 RefinedLosses2005;    // refined LRFD 2005 and later
   lrfdApproximateLosses2005 ApproxLosses2005; // approximate LRFD 2005 and later


   ///////////////////////////////////////////////////////////////////////////////
   // Losses computed by Time Step Method
   ///////////////////////////////////////////////////////////////////////////////

   // Friction and Anchor Set Losses
   // vector index is the duct index
   std::vector<FRICTIONLOSSDETAILS> FrictionLossDetails;

   // vector index in an interval index
   std::vector<TIME_STEP_DETAILS> TimeStepDetails;
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
    Float64 lt; // transfer length
};

struct REBARDEVLENGTHDETAILS
{
   Float64 Ab;
   Float64 fy;
   Float64 fc;
   Float64 db;
   Float64 factor; // Factor applied if light weight concrete (1.0 if not)

   // two equations for #11 or smaller
   Float64 ldb1;
   Float64 ldb2;
   Float64 ldb; // controlling value
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