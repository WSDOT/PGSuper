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

#ifndef INCLUDE_DETAILS_H_
#define INCLUDE_DETAILS_H_

#include <PGSuperTypes.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <Lrfd\Lrfd.h>
#include <WBFLRCCapacity.h>

struct PIER_DIAPHRAGM_LOAD_DETAILS
{
   Float64 TribWidth;
   Float64 Height;
   Float64 Width;
   Float64 SkewAngle;
   Float64 Density;
   Float64 P;
   Float64 M;
   Float64 MomentArm;
};

struct SLAB_OFFSET_AT_SECTION
{
   pgsPointOfInterest PointOfInterest;
   Float64 Station;
   Float64 Offset;
   Float64 tSlab;
   Float64 Fillet;
   Float64 ElevAlignment;
   Float64 ElevGirderChord; // elevation of girder chord without camber effects
   Float64 CrownSlope;
   Float64 GirderTopSlope;
   Float64 Wtop;
   Float64 C;
   Float64 D;
   Float64 CamberEffect;
   Float64 GirderOrientationEffect;
   Float64 ProfileEffect;
   Float64 TopFlangeShapeEffect;
   Float64 ElevTopGirder; // elevation of top of girder, including camber, precamber, elevation adjustment, and top flange thickening effects
   Float64 TopSlabToTopGirder;
   Float64 ElevAdjustment; // elevation adjustment at temporary supports
   Float64 RequiredSlabOffsetRaw;  // raw full precision value
};

struct SLABOFFSETDETAILS
{
   std::vector<SLAB_OFFSET_AT_SECTION> SlabOffset;
   Float64 RequiredMaxSlabOffsetRaw; // "A" Dimension - max of raw values
   Float64 RequiredMaxSlabOffsetRounded; // "A" Dimension - max of rounded values
   Float64 HaunchDiff; // maximum difference in haunch thickness
};

struct MOMENTCAPACITYDETAILS
{
   Float64 Mr{ 0.0 };        // Nominal resistance (Mr = Phi*Mn);
   Float64 Mn{ 0.0 };        // Nominal moment capacity
   Float64 Phi{ 0.0 };       // Strength reduction factor
   Float64 PPR{ 0.0 };       // Partial prestress ratio at this section
   Float64 MomentArm{ 0.0 }; // Distance between dc and de
   Float64 c{ 0.0 };         // Distance from extreme compression fiber to the neutral axis
   Float64 dc{ 0.0 };        // Distance from extreme compression fiber to the resultant compressive force
   Float64 de{ 0.0 };        // Distance from extreme compression fiber to the resultant tensile force (used to compute c/de)
   Float64 de_shear{ 0.0 };  // Distance from extreme compression fiber to the resultant tensile force for only those strands in tension (used for shear)
   WBFL::Geometry::Point2d pnt_de; // Location of the resultant tensile force for only those strands in tension (location of de_shear)
   Float64 C{ 0.0 };         // Resultant compressive force
   Float64 T{ 0.0 };         // Resultant tensile force

   int Method{ LRFD_METHOD };        // LRFD_METHOD or WSDOT_METHOD
                      // WSDOT_METHOD = variable phi factor
                      // LRFD_METHOD = over reinforce capacity per C5.7.3.3.1 (removed from spec 2005)

   // WSDOT_METHOD
   Float64 dt{ 0.0 };        // Depth from extreme compression fiber to cg of lowest piece of reinforcement
   Float64 et{ 0.0 };        // Net tensile strain
   Float64 etl{ 0.0 };       // Tension Control Strain Limit
   Float64 ecl{ 0.0 };       // Compression Control Strain Limit

   Float64 fps_avg{ 0.0 };   // Average stress in strands at nominal resistance
   Float64 fpt_avg_segment{ 0.0 };  // Average stress in segment tendons at nominal resistance
   Float64 fpt_avg_girder{ 0.0 };  // Average stress in girder tendons at nominal resistance

   // LRFD_METHOD 
   // For C5.7.3.3.1... Capacity of over reinforced section  (removed from spec 2005)
   bool    bOverReinforced{ false }; // True if section is over reinforced
   bool    bRectSection{ true };    // True if rectangular section behavior
   Float64 Beta1Slab{ 0.0 };       // Beta1 for slab only... B1 and f'c of slab are used in these calcs
   Float64 FcSlab{ 0.0 };
   Float64 hf{ 0.0 };
   Float64 b{ 0.0 };
   Float64 bw{ 0.0 };
   Float64 MnMin{ 0.0 };           // Minimum nominal capacity of a over reinforced section (Eqn C5.7.3.3.1-1 or 2)

   // solution object provides the full equilibrium state of the moment
   // capacity solution
   IndexType girderShapeIndex{ INVALID_INDEX }; // Index of the girder shape in the general section model
   IndexType deckShapeIndex{ INVALID_INDEX }; // Index of the deck shape in the general section model
   CComPtr<IGeneralSection> Section; // this is the section that is analyzed
   CComPtr<IMomentCapacitySolution> ConcreteCrushingSolution; // deck compressive strain at -0.003 (or bottom of girder at concrete strain limit for negative moment)
   CComPtr<IMomentCapacitySolution> UHPCGirderCrushingSolution; // compressive strain in UHPC girder concrete at it's crushing limit
   CComPtr<IMomentCapacitySolution> UHPCCrackLocalizationSolution; // tension strain in UHPC girder is at crack localization strain
   CComPtr<IMomentCapacitySolution> ReinforcementFractureSolution; // reinforcement is at it's fraction strain limit
   CComPtr<IMomentCapacitySolution> ReinforcementStressLimitStateSolution; // stress in the strand or reinforcement is at its service limit state (0.8fpy or 0.8fy, respectively). This is used for UHPC capacity reduction factor.
   
   enum class ControllingType 
   { 
      ConcreteCrushing, // ConcreteCrushingSolution is controlling
      GirderConcreteCrushing, // UHPCGirderCrushingSolution is controlling
      GirderConcreteLocalization, // UHPCCrackLocalizationSolution is controlling
      ReinforcementFracture // ReinforcementFractureSolution is controlling
   };
   ControllingType Controlling{ ControllingType::ConcreteCrushing };
   void GetControllingSolution(IMomentCapacitySolution** ppSolution)
   {
      switch (Controlling)
      {
      case MOMENTCAPACITYDETAILS::ControllingType::ConcreteCrushing:
         ConcreteCrushingSolution.CopyTo(ppSolution);
         break;

      case MOMENTCAPACITYDETAILS::ControllingType::GirderConcreteCrushing:
         UHPCGirderCrushingSolution.CopyTo(ppSolution);
         break;

      case MOMENTCAPACITYDETAILS::ControllingType::GirderConcreteLocalization:
         UHPCCrackLocalizationSolution.CopyTo(ppSolution);
         break;

      case MOMENTCAPACITYDETAILS::ControllingType::ReinforcementFracture:
         ReinforcementFractureSolution.CopyTo(ppSolution);
         break;

      default:
         ASSERT(false); // is there a new controlling type?
         break;
      }
   }


   bool bDevelopmentLengthReducedStress{ false };// when true, the section was analyzed with development length reduced stress capability of the strands
};
inline constexpr auto operator+(MOMENTCAPACITYDETAILS::ControllingType t) noexcept { return std::underlying_type<MOMENTCAPACITYDETAILS::ControllingType>::type(t); }

struct CRACKINGMOMENTDETAILS
{
   Float64 Mcr;  // Cracking moment
   Float64 Mdnc; // Dead load moment on non-composite girder
   Float64 fr;   // Rupture stress
   Float64 fcpe; // Stress at bottom of non-composite girder due to prestress
   Float64 Sb;   // Bottom section modulus of non-composite girder
   Float64 Sbc;  // Bottom section modulus of composite girder
   Float64 McrLimit; // Limiting cracking moment ... per 2nd Edition + 2003 interims (changed in 2005 interims)

   Float64 g1,g2,g3; // gamma factors from LRFD 5.6.3.3 (pre2017: 5.7.3.3.2) (starting LRFD 6th Edition, 2012)
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

struct PRINCIPALTENSIONWEBSECTIONDETAILS
{
   PRINCIPALTENSIONWEBSECTIONDETAILS(LPCTSTR lpszLocation, Float64 YwebSection, Float64 fTop, Float64 fBot, Float64 fpcx, Float64 Vu,Float64 Qnc, Float64 Qc, Float64 bw, bool bAdjusted, Float64 t, Float64 fmax) :
      strLocation(lpszLocation), fTop(fTop), fBot(fBot), fpcx(fpcx), YwebSection(YwebSection),Vu(Vu), Qnc(Qnc), Qc(Qc), bw(bw), bIsShearWidthAdjustedForTendon(bAdjusted), t(t), fmax(fmax)
   {
   }

   std::_tstring strLocation; // description of web section
   Float64 YwebSection; // elevation of web section in girder section coordinates
   Float64 fTop; // Service III stress at top of non-composite girder, including prestress, for computing maximum fpcx
   Float64 fBot; // Service III stress at bottom of non-composite girder, including prestress, for computing maximum fpcx
   Float64 fpcx; // maximum axial stress at evaluation point
   Float64 Vu;  // Service III shear on composite section if NCHRP method, Vu for LRFD method
   Float64 Qnc; // first moment of area of the non-composite section about the evaluation point
   Float64 Qc; // first moment of area of the composite section about the evaluation point
   Float64 bw; // shear width at the evaluation point
   bool bIsShearWidthAdjustedForTendon;
   Float64 t; // shear stress at the evaluation point
   Float64 fmax; // maximum principal stress at the evaluation point
};

struct PRINCIPALSTRESSINWEBDETAILS
{
   Float64 Hg; // height of non-composite girder
   Float64 Vp; // Vertical component of prestress

   Float64 Vnc; // Service III shear on non-composite section
   Float64 Inc; // moment of inertia of non-composite section
   Float64 Ic; // moment of inertia of composite section
   std::vector<PRINCIPALTENSIONWEBSECTIONDETAILS> WebSections; // points along the web height where principal tension is evaluted
};

struct SHEARCAPACITYDETAILS
{
   SHEARCAPACITYDETAILS()
   {
      memset((void*)this,0,sizeof(SHEARCAPACITYDETAILS));
   };

   // [IN]
   pgsTypes::ShearCapacityMethod Method; // General or Simplified per LRFD 5.8.3.4.3 (Vci/Vcw - added to LRFD in 2007) (removed from LRFD in 2017)
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
   Float64 VptSegment; // vertical component of prestress due to segment tendons
   Float64 VptGirder; // vertical component of prestress due to girder tendons
   Float64 Phi;
   Float64 bv;
   Float64 dv;
   Float64 dv_uhpc; // dv portion of UHPC beams (see GS 1.7.3.3)
   Float64 controlling_uhpc_dv; // controlling value of dv per GS 1.7.2.8 (used for Vuhpc and shear stress)
   Float64 fpeps;
   Float64 fpeptSegment; // average effective prestress in segment tendons
   Float64 fpeptGirder; // average effective prestress in girder tendons
   Float64 fpc;
   Float64 Es;
   Float64 As;
   Float64 Eps;
   Float64 Aps;
   Float64 EptSegment;
   Float64 AptSegment;
   Float64 EptGirder;
   Float64 AptGirder;
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
   bool bLimitNetTensionStrainToPositiveValues;
   bool bIgnoreMiniumStirrupRequirementForBeta;

   // [OUT]
   Float64 fpops; // fpo for strand
   Float64 fpoptSegment; // fpo for segment tendons
   Float64 fpoptGirder; // fpo for girder tendons
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
   Float64 fv; // stress in stirrups (this parameter is for UHPC, but is set equal to fy for other concrete types so calculation of Vs works more generically)
   Float64 e2; // UHPC
   Float64 ev; // UHPC
   Float64 etcr; // UHPC
   Float64 et_loc; // UHPC
   Float64 ft_loc; // UHPC
   Float64 gamma_u; // UHPC
   Float64 rho; // UHPC
   Float64 FiberStress;// coefficient for compute the contribution of fibers in PCI UHPC to the shear capacity (taken as 0.75 ksi for now)
                       // also taken as gamma_u*ft,loc for UHPC
   Float64 Vuhpc; // Value of Vuhpc per GS Eq. 1.7.3.3-3
   Float64 Vc; // Shear strength of concrete (= Vcf for PCI UHPC, Vuhpc for UHPC)
   Float64 Vs;
   Float64 Vn1;  // [Eqn 5.8.3.3-1]
   Float64 Vn2;  // [Eqn 5.8.3.3-2]
   Float64 Vn;   // Nominal shear resistance
   Float64 pVn;  // Factored nominal shear resistance
   Float64 VuLimit; // Limiting Vu where stirrups are required [Eqn 5.8.2.4-1]
   bool bStirrupsReqd; // If true, stirrups and/or Vf from fibers is required LRFD 5.7.2.3-1
   Int16 Equation; // Equation used to compute ex (Only applicable after LRFD 1999)
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

   // LRFD 9th Edition
   Float64 bw; // web width without any deductions
   Float64 duct_diameter; // diameter of largest duct in section
   Float64 delta; // duct diameter correction factor
   Float64 lambda_duct; // shear strength reduction factor

};

// fpc - strand stress for shear capacity calculation
struct FPCDETAILS
{
   Float64 eps; // Eccentricity of prestress strand
   Float64 Pps; // Prestress force
   Float64 eptSegment; // Eccentricity of segment post-tension strand
   Float64 PptSegment; // Segment Post-tension force
   Float64 eptGirder; // Eccentricity of girder post-tension strand
   Float64 PptGirder; // Girder Post-tension force
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
      pCriticalSection = nullptr;
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
         pCriticalSection = nullptr;
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
   //Float64 kc; // this is stored in ktd because it is actually a time-development factor

   // 2005 and later
   Float64 kvs;
   Float64 khc;
   //Float64 kf; // using the kf above
   Float64 ktd;

   Float64 kl; // time factor

   Float64 K1, K2; // 2005 and later, from NCHRP Report 496
};

struct INCREMENTALCREEPDETAILS
{
   // idealy this should be unique pointers, but I haven't been able to figure
   // out all of the details to make this work in a complex data structure
   //std::unique_ptr<WBFL::Materials::ConcreteBaseCreepDetails> pStartDetails;
   //std::unique_ptr<WBFL::Materials::ConcreteBaseCreepDetails> pEndDetails;
   std::shared_ptr<WBFL::Materials::ConcreteBaseCreepDetails> pStartDetails;
   std::shared_ptr<WBFL::Materials::ConcreteBaseCreepDetails> pEndDetails;
};

struct INCREMENTALSHRINKAGEDETAILS
{
   // idealy this should be unique pointers, but I haven't been able to figure
   // out all of the details to make this work in a complex data structure
   //std::unique_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> pStartDetails;
   //std::unique_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> pEndDetails;
   std::shared_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> pStartDetails;
   std::shared_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> pEndDetails;
   Float64 esi{ 0.0 };
};

// Details of incremental relaxation computation used for time-step analysis
struct INCREMENTALRELAXATIONDETAILS
{
   // common parameters
   Float64 fpi{ 0 }; // effective prestress at the start of the interval
   Float64 fpy{ 0 };
   Float64 fpu{ 0 };
   Float64 tStart{ 0 };
   Float64 tEnd{ 0 };
   Float64 epoxyFactor{ 1.0 }; // this factor is multiplied to the relaxation.
                        // see PCI "Guidelines for the use of Epoxy-Coated Strand", PCI Journal, July-August 1993
                        // relaxation is doubled, so this factor is 2.0, for expoxy coated strands


   // These parameters are for AASHTO and ACI209 models
   Float64 K{ 0 };

   // These parameters are for CEB-FEP model
   Float64 p{ 0 };
   Float64 k{ 0 };

   // Incremental relaxation
   Float64 fr{ 0 };
};

#define TIMESTEP_CR  0
#define TIMESTEP_SH  1
#define TIMESTEP_RE  2

// the number of product forces that we need to consider from the pgsTypes::ProductForceType enum.
// all of the product forces count except for the special cases at the end of the enum.
constexpr auto pftTimeStepSize = ((int)pgsTypes::pftProductForceTypeCount) - 1; // remove pgsTypes::pftOverlayRating from the count as it is a special case and don't apply here

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
   Float64 E;   // Modulus of Elasticity
   Float64 Ea;  // Age adjusted Modulus of Elasticity (used for computing transformed section properties)

   // Creep Strains during this interval due to loads applied in previous intervals
   struct CREEP_STRAIN
   {
      Float64 P{ 0 };
      Float64 E{ 0 }; // modulus of elasticity used to compute creep strain (Not age adjusted)
      Float64 A{ 0 };
      Float64 Cs{ 0 }; // C(i+1/2,j)
      Float64 Ce{ 0 }; // C(i-1/2,j)
      Float64 Xs{ 1 }; // concrete Aging coefficient X(i+1/2,j)
      Float64 Xe{ 1 }; // concrete Aging coefficient X(i-1/2,j)
      Float64 e{ 0 };
   };

   struct CREEP_CURVATURE
   {
      Float64 M{ 0 };
      Float64 E{ 0 }; // modulus of elasticity used to compute creep curvature (Not age adjusted)
      Float64 I{ 0 };
      Float64 Cs{ 0 }; // C(i+1/2,j)
      Float64 Ce{ 0 }; // C(i-1/2,j)
      Float64 Xs{ 1 }; // concrete Aging coefficient X(i+1/2,j)
      Float64 Xe{ 1 }; // concrete Aging coefficient X(i-1/2,j)
      Float64 r{ 0 };
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

   std::array<Float64,pftTimeStepSize> dei;
   Float64 de;

   std::array<Float64, pftTimeStepSize> ei;
   Float64 e;

   std::array<Float64, pftTimeStepSize> dri;
   Float64 dr;

   std::array<Float64, pftTimeStepSize> ri;
   Float64 r;

   // Force on this concrete part due to elastic effects during this interval
   std::array<Float64, pftTimeStepSize> dPi; // index is one of the pgsTypes::ProductForceType enum values
   std::array<Float64, pftTimeStepSize> dMi;
   Float64 dP, dM; // summation of dPi and dMi

   // Force on this concrete part at the end of this interval
   std::array<Float64, pftTimeStepSize> Pi; // = (P in previous interval) + dP;
   std::array<Float64, pftTimeStepSize> Mi; // = (M in previous interval) + dM;
   Float64 P, M; // summation of Pi and Mi

   // Stress at the end of this interval = stress at end of previous interval + dP/An + dM*y/In 
   // where y is the depth from the top of the concrete part
   Float64 f[2][pftTimeStepSize][2]; // first index is one of the pgsTypes::FaceType enums, second index is one of the pgsTypes::ProductForceType enum values
                        // third index is one of the ResultsType enum values

   //// Stress in this due to live load
   //Float64 fLLMin[2]; // index is pgsTypes::FaceType
   //Float64 fLLMax[2];

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

      for (int i = 0; i < pftTimeStepSize; i++)
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

      //fLLMin[pgsTypes::TopFace]    = 0;
      //fLLMin[pgsTypes::BottomFace] = 0;
      //fLLMax[pgsTypes::TopFace]    = 0;
      //fLLMax[pgsTypes::BottomFace] = 0;
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
   std::array<Float64, pftTimeStepSize> dei; // change in strain in strand due to deformations in this interval
   Float64 de; // summation of dei

   std::array<Float64, pftTimeStepSize> dPi; // change in force in strand due to deformations in this interval
   Float64 dP; // summation of dPi

   std::array<Float64, pftTimeStepSize> ei; // strain in strand at end of this interval = (e previous interval + de)
   Float64 e; // summation of ei
   
   std::array<Float64, pftTimeStepSize> Pi; // force in strand at end of this interval = (P previous interval + dP)
   Float64 P; // summation of Pi

   // Loss/Gain during this interval (change in effective prestress this interval)
   std::array<Float64, pftTimeStepSize> dfpei; // = dP/Aps
   std::array<Float64, pftTimeStepSize> fpei; // = fpei[load] from previous interval plus dfpei[load] from this interval
   Float64 dfpe; // summation of dfpei

   // Effective prestress
   Float64 fpe; // = fpj + Sum(dfpe for all intervals up to and including this interval)
   // also fpe = fpe (previous interval) + dfpe (this interval)

   // sum of losses up to and including this interval
   // this is an effective loss that includes time-dependent and elastic effects
   Float64 loss; // = Sum(dfpe for all intervals up to and including this interval)
   // also loss = loss (previous interval) + dfpe (this interval)

   //// Elastic effect of live load on effective prestress
   //Float64 dFllMin;
   //Float64 fpeLLMin; // fpe + dFll;
   //Float64 lossLLMin; // loss - dFll;

   //Float64 dFllMax;
   //Float64 fpeLLMax; // fpe + dFll;
   //Float64 lossLLMax; // loss - dFll;

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

      for ( int i = 0; i < pftTimeStepSize; i++ )
      {
         dei[i] = 0;
         ei[i] = 0;

         dPi[i] = 0;
         Pi[i]  = 0;

         dfpei[i] = 0;
         fpei[i] = 0;
      }

      dfpe = 0;
      fpe  = 0;
      loss = 0;

      //dFllMin   = 0;
      //fpeLLMin  = 0;
      //lossLLMin = 0;

      //dFllMax   = 0;
      //fpeLLMax  = 0;
      //lossLLMax = 0;
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
   std::array<Float64, pftTimeStepSize> dei; // change in strain in bar due to deformations in this interval
   Float64 de; // summation of dei

   std::array<Float64, pftTimeStepSize> dPi; // change in force in bar during this interval
   Float64 dP; // summation of dPi

   std::array<Float64, pftTimeStepSize> ei; // strain in bar at end of this interval = (e previous interval + de)
   Float64 e; // summation of ei

   std::array<Float64, pftTimeStepSize> Pi; // force in rebar at end of this interval = (P previous interval + dP)
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

      for (int i = 0; i < pftTimeStepSize; i++)
      {
         dei[i] = 0;
         ei[i] = 0;

         dPi[i] = 0;
         Pi[i]  = 0;
      }
   }
};

// Principal stress in web variables for time step analyses
struct TIME_STEP_PRINCIPALTENSIONWEBSECTIONDETAILS
{
   TIME_STEP_PRINCIPALTENSIONWEBSECTIONDETAILS(LPCTSTR lpszLocation, Float64 YwebSection, Float64 Qc, Float64 bw, bool bAdjusted, Float64 fpcx, Float64 fpcxs, Float64 t, Float64 ts) :
      strLocation(lpszLocation), YwebSection(YwebSection), Qc(Qc), bw(bw), bIsShearWidthAdjustedForTendon(bAdjusted),fpcx(fpcx), fpcx_s(fpcxs), tau(t),tau_s(ts)
   {
   }

   std::_tstring strLocation; // description of web section
   Float64 YwebSection; // elevation of web section in girder section coordinates
   Float64 Qc; // first moment of area of the section about the evaluation point
   Float64 bw; // shear width at the evaluation point
   bool bIsShearWidthAdjustedForTendon;
   Float64 fpcx; // Maximum axial stress at evaluation point - increment at this interval
   Float64 fpcx_s; // Maximum axial stress at evaluation point - sum of all previous intervals plus fpcx
   Float64 tau; // shear stress at the evaluation point - increment at this interval
   Float64 tau_s; // shear stress at the evaluation point - sum of all previous intervals plus t
};

struct TIME_STEP_PRINCIPALSTRESSINWEBDETAILS
{
   Float64 Hg; // height of non-composite girder
   Float64 Vu; // Shear on section
   Float64 I; // moment of inertia  section
   Float64 fTop; // Stress at top of non-composite girder for computing maximum fpcx
   Float64 fBot; // Stress at bottom of non-composite girder for computing maximum fpcx
   std::vector<TIME_STEP_PRINCIPALTENSIONWEBSECTIONDETAILS> WebSections; // points along the web height where principal tension is evaluted
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

   Float64 Ea; // modulus used to transform properties into an equivalent material

   // Change in total loading on the section due to externally applied loads during this interval
   // Array index is one of the pgsTypes::ProductForceType enum values
   // upto and including pgsTypes::pftRelaxation
   std::array<Float64,pftTimeStepSize> dPi, dMi, dVi;

   // total change in loading on the section (summation of dPi, dMi, dVi)
   Float64 dP, dM, dV;

   // Total loading on the section due to externally applied loads in all intervals upto
   // and including this interval. Array index is one of the pgsTypes::ProductForceType enum values
   // upto and including pgsTypes::pftRelaxation
   std::array<Float64, pftTimeStepSize> Pi, Mi, Vi;

   // total change in loading on the section (summation of Pi, Mi, Vi)
   Float64 P, M, V;

   // Time step parameters for girder and deck
   TIME_STEP_CONCRETE Girder;
   TIME_STEP_CONCRETE Deck;

   // Time step parameters for strands and tendons
#if defined LUMP_STRANDS
   std::array<TIME_STEP_STRAND,3> Strands; // pgsTypes::StrandType (Straight, Harped, Temporary)
#else
   std::vector<TIME_STEP_STRAND> Strands[3]; // pgsTypes::StrandType (Straight, Harped, Temporary)
#endif
   std::vector<TIME_STEP_STRAND> GirderTendons; // one per duct
   std::vector<TIME_STEP_STRAND> SegmentTendons; // one per duct

   // Time step parameters for rebar
   // access first array with pgsTypes::DeckRebarMatType and second with pgsTypes::DeckRebarBarType
   // e.g DeckRebar[pgsTypes::drmTop][pgsTypes::drbIndividual]
   // Don't use pgsTypes::drbAll
   TIME_STEP_REBAR DeckRebar[2][2];

   // Time step parameters for girder rebar
   std::vector<TIME_STEP_REBAR> GirderRebar;

   // Forces required to totally restrain the cross section for initial strains occuring during this interval
   std::array<Float64,3> Pr, Mr; // index is one of the TIMESTEP_XXX constants

   // Initial Strains 
   std::array<Float64,3> e; // index is one of the TIMESTEP_XXX constants
   std::array<Float64,3> r;

   // Deformation due to externally applied loads and restraining forces in this interval
   std::array<Float64, pftTimeStepSize> der; // axial strain
   std::array<Float64, pftTimeStepSize> drr; // curvature

   // Total deformation due to externally applied loads and restraining forces
   std::array<Float64, pftTimeStepSize> er; // axial strain
   std::array<Float64, pftTimeStepSize> rr; // curvature

   // Principal web stress details for each loading. 
   std::array<TIME_STEP_PRINCIPALSTRESSINWEBDETAILS, pftTimeStepSize> PrincipalStressDetails;

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
      Ea  = 0;

      for (int i = 0; i < pftTimeStepSize; i++)
      {
         dPi[i] = 0;
         dMi[i] = 0;
         dVi[i] = 0;

         Pi[i] = 0;
         Mi[i] = 0;
         Vi[i] = 0;

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
      dV = 0;

      P = 0;
      M = 0;
      V = 0;

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
   ANCHORSETDETAILS() :
      Lset{ 0,0 }, dfpAT{ 0,0 }, dfpS{ 0,0 }
   { 
      segmentKey = CSegmentKey(INVALID_INDEX,INVALID_INDEX,INVALID_INDEX);
      ductIdx = INVALID_INDEX;
   }

   // Key
   CSegmentKey segmentKey; // if segmentIndex == ALL_SEGMENTS, then the key represents a girder key
   DuctIndexType ductIdx;

   // Value
   // Array index is pgsTypes::MemberEndType
   std::array<Float64, 2> Lset; // Anchor set zone length
   std::array<Float64, 2> dfpAT; // Loss of effective stress at anchorage due to seating
   std::array<Float64, 2> dfpS;  // Loss of effective stress at end of anchor set zone length due to seating
                     // This is typically zero except when the anchor set zone is longer than the tendon
};

// This struct holds the computation details for friction and anchor set
// losses at a POI
struct FRICTIONLOSSDETAILS
{
   Float64 alpha{ 0 }; // total angular change from jacking end to this POI
   Float64 X{ 0 };     // distance from start of tendon to this POI
   Float64 dfpF{ 0 };  // friction loss at this POI
   Float64 dfpA{ 0 };  // anchor set loss at this POI
};

// This struct holds the computation details for prestress losses
// at a POI
struct LOSSDETAILS
{
   LOSSDETAILS() {;}

   // Method for computing prestress losses... the value of this parameter
   // defines which of the loss details given below are applicable
   pgsTypes::LossMethod LossMethod;

   ///////////////////////////////////////////////////////////////////////////////
   // Losses computed by LRFD Approximate or Refined methods or general lump sum
   ///////////////////////////////////////////////////////////////////////////////

   // THESE LOSS OBJECTS NOT VALID WITH TIME_STEP METHOD

   // LRFD Method Losses
   // Base class can be casted to derived class to get details. You know who you are!
   std::shared_ptr<const lrfdLosses> pLosses;


   ///////////////////////////////////////////////////////////////////////////////
   // Losses computed by Time Step Method
   ///////////////////////////////////////////////////////////////////////////////

   // Friction and Anchor Set Losses
   // vector index is the duct index
   std::vector<FRICTIONLOSSDETAILS> GirderFrictionLossDetails;
   std::vector<FRICTIONLOSSDETAILS> SegmentFrictionLossDetails;

   // vector index in an interval index
   std::vector<TIME_STEP_DETAILS> TimeStepDetails;

#if defined _DEBUG
   pgsPointOfInterest POI; // this is the POI that this loss details applies to
#endif
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
   std::array<Float64, 4> Fci;
   std::array<Float64,4> L;

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

struct TEMPORARYSUPPORTELEVATIONDETAILS
{
   GirderIndexType girderIdx;
   SegmentIndexType segmentIdx;
   pgsTypes::MemberEndType endType;

   bool bContinuous; // if true, segments are continuous over this temporary support and endType is not applicable

   Float64 Station;
   Float64 Offset;
   Float64 DesignGradeElevation; // Design (target) roadway elevation at Station and Offset (top of overlay if overlay built with bridge, top of deck for no overlay or future overlay)
   Float64 FinishedGradeElevation; // Computed roadway elevation at bearing. For "A" dim input this is always same as the design elevation. For spliced girders this may vary from design
   Float64 OverlayDepth;
   Float64 ProfileGrade;
   Float64 GirderGrade;
   Float64 GirderOrientation;
   Float64 SlabOffset;
   Float64 HaunchDepth;
   Float64 Hg;
   Float64 ElevationAdjustment; // elevation adjustment from temporary support tower
   Float64 Elevation; // elevation at bottom of girder

   bool operator<(const TEMPORARYSUPPORTELEVATIONDETAILS& other) const
   {
      if (girderIdx < other.girderIdx) return true;
      if (other.girderIdx < girderIdx) return false;

      if (segmentIdx < other.segmentIdx) return true;
      if (other.segmentIdx < segmentIdx) return false;

      if (endType < other.endType) return true;
      if (other.endType < endType) return false;

      return false;
   }
};

#endif // INCLUDE_DETAILS_H_