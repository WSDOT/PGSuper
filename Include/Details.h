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

#ifndef INCLUDE_DETAILS_H_
#define INCLUDE_DETAILS_H_

#include <PGSuperTypes.h>

#include <PgsExt\PointOfInterest.h>
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
   pgsPointOfInterest Poi;
   Float64            Dv;
   bool               InRange; // false if theta calculation couln't be done
   Float64            Theta;
   Float64            CotanThetaDv05;
};

// Critical Section for shear details
struct CRITSECTDETAILS
{
   // if there is an uplift reaction, the critical section is at the face of support
   bool bAtLeftFaceOfSupport, bAtRightFaceOfSupport;
   pgsPointOfInterest poiLeftFaceOfSupport, poiRightFaceOfSupport;

   // intersection locations
   CRITSECTIONDETAILSATPOI LeftCsDv;  // section locations from dv calculation
   CRITSECTIONDETAILSATPOI RightCsDv;
   CRITSECTIONDETAILSATPOI LeftCsDvt;  // section locations from .5*dv*cot(theta) calculation
   CRITSECTIONDETAILSATPOI RightCsDvt;

   // data at all pois
   std::list<CRITSECTIONDETAILSATPOI> PoiData;
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

struct LOSSDETAILS
{
   LOSSDETAILS() { pLosses = 0; }
   LOSSDETAILS(const LOSSDETAILS& other)
   { MakeCopy(other); }
   LOSSDETAILS& operator=(const LOSSDETAILS& other)
   { return MakeCopy(other); }
   LOSSDETAILS& MakeCopy(const LOSSDETAILS& other)
   {
      Method = other.Method;

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

      return *this;
   }

   Uint32 Method; // Loss method (a LOSSES_xxx constant)

   // LRFD Method Losses (details can be extracted from these objects)
   const lrfdLosses* pLosses;

   lrfdLumpSumLosses LumpSum;

   // before LRFD 2005
   lrfdRefinedLosses RefinedLosses;
   lrfdApproximateLosses ApproxLosses;

   // LRFD 2005 and later
   lrfdRefinedLosses2005 RefinedLosses2005;
   lrfdApproximateLosses2005 ApproxLosses2005;
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