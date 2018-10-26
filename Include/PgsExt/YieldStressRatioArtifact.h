///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <PgsExt\ReportPointOfInterest.h>

/*****************************************************************************
CLASS 
   pgsYieldStressRatioArtifact

   Artifact for permit rating yield stress ratio evaluation


DESCRIPTION
   Artifact for permit rating yield stress ratio evaluation. See LRFR 6A.5.4.2.2b


COPYRIGHT
   Copyright © 1997-2010
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 01.07.2010 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsYieldStressRatioArtifact
{
public:
   pgsYieldStressRatioArtifact();
   pgsYieldStressRatioArtifact(const pgsYieldStressRatioArtifact& rOther);
   virtual ~pgsYieldStressRatioArtifact();

   pgsYieldStressRatioArtifact& operator = (const pgsYieldStressRatioArtifact& rOther);

   void SetPointOfInterest(const pgsPointOfInterest& poi);
   const pgsPointOfInterest& GetPointOfInterest() const;

   void SetRatingType(pgsTypes::LoadRatingType ratingType);
   pgsTypes::LoadRatingType GetLoadRatingType() const;

   void SetVehicleIndex(VehicleIndexType vehicleIdx);
   VehicleIndexType GetVehicleIndex() const;

   void SetVehicleWeight(Float64 W);
   Float64 GetVehicleWeight() const;

   void SetVehicleName(LPCTSTR str);
   std::_tstring GetVehicleName() const;

   void SetAllowableStressRatio(Float64 K);
   Float64 GetAllowableStressRatio() const;

   void SetDeadLoadFactor(Float64 gDC);
   Float64 GetDeadLoadFactor() const;

   void SetDeadLoadMoment(Float64 Mdc);
   Float64 GetDeadLoadMoment() const;

   void SetWearingSurfaceFactor(Float64 gDW);
   Float64 GetWearingSurfaceFactor() const;

   void SetWearingSurfaceMoment(Float64 Mdw);
   Float64 GetWearingSurfaceMoment() const;

   void SetCreepFactor(Float64 gCR);
   Float64 GetCreepFactor() const;

   void SetCreepMoment(Float64 Mcr);
   Float64 GetCreepMoment() const;

   void SetShrinkageFactor(Float64 gSH);
   Float64 GetShrinkageFactor() const;

   void SetShrinkageMoment(Float64 Msh);
   Float64 GetShrinkageMoment() const;

   void SetRelaxationFactor(Float64 gRE);
   Float64 GetRelaxationFactor() const;

   void SetRelaxationMoment(Float64 Mre);
   Float64 GetRelaxationMoment() const;

   void SetSecondaryEffectsFactor(Float64 gPS);
   Float64 GetSecondaryEffectsFactor() const;

   void SetSecondaryEffectsMoment(Float64 Mps);
   Float64 GetSecondaryEffectsMoment() const;
   
   void SetLiveLoadDistributionFactor(Float64 gM);
   Float64 GetLiveLoadDistributionFactor() const;

   void SetLiveLoadFactor(Float64 gLL);
   Float64 GetLiveLoadFactor() const;

   void SetLiveLoadMoment(Float64 Mllim);
   Float64 GetLiveLoadMoment() const;

   void SetCrackingMoment(Float64 Mcr);
   Float64 GetCrackingMoment() const;

   void SetIcr(Float64 Icr);
   Float64 GetIcr() const;

   void SetCrackDepth(Float64 c);
   Float64 GetCrackDepth() const;

   void SetRebar(Float64 db,Float64 fb,Float64 fyb,Float64 Eb);
   bool GetRebar(Float64* pdb, Float64* pfb,Float64* pfyb,Float64* pEb) const;

   void SetStrand(Float64 dps,Float64 fps,Float64 fyps,Float64 Eps);
   bool GetStrand(Float64* pdps,Float64* pfps,Float64* pfyps,Float64* pEps) const;

   void SetTendon(Float64 dpt,Float64 fpt,Float64 fypt,Float64 Ept);
   bool GetTendon(Float64* pdpt,Float64* pfpt,Float64* pfypt,Float64* pEpt);

   void SetEg(Float64 Eg);
   Float64 GetEg() const;

   Float64 GetExcessMoment() const;

   Float64 GetRebarCrackingStressIncrement() const;
   Float64 GetRebarStress() const;
   Float64 GetRebarStressRatio() const;
   Float64 GetRebarAllowableStress() const;

   Float64 GetStrandCrackingStressIncrement() const;
   Float64 GetStrandStress() const;
   Float64 GetStrandStressRatio() const;
   Float64 GetStrandAllowableStress() const;

   Float64 GetTendonCrackingStressIncrement() const;
   Float64 GetTendonStress() const;
   Float64 GetTendonStressRatio() const;
   Float64 GetTendonAllowableStress() const;

   Float64 GetStressRatio() const;

protected:
   void MakeCopy(const pgsYieldStressRatioArtifact& rOther);
   virtual void MakeAssignment(const pgsYieldStressRatioArtifact& rOther);
   void ComputeStressRatios() const;
   void ComputeStressRatio(Float64 d,Float64 E,Float64 fbcr,Float64 fy,Float64* pfcr,Float64* pfs,Float64* pRF) const;

   mutable bool m_bRFComputed;

   mutable Float64 m_RebarRF;
   mutable Float64 m_StrandRF;
   mutable Float64 m_TendonRF;

   mutable Float64 m_fcrRebar;
   mutable Float64 m_fcrStrand;
   mutable Float64 m_fcrTendon;
   
   mutable Float64 m_fsRebar;
   mutable Float64 m_fsStrand;
   mutable Float64 m_fsTendon;

   pgsPointOfInterest m_POI;

   pgsTypes::LoadRatingType m_RatingType;

   VehicleIndexType m_VehicleIndex;
   Float64 m_VehicleWeight;
   std::_tstring m_strVehicleName;

   Float64 m_AllowableStressRatio; // stress in reinforcement should not exceed this times the yield strength
   Float64 m_Mdc;
   Float64 m_Mdw;
   Float64 m_Mcr;
   Float64 m_Msh;
   Float64 m_Mre;
   Float64 m_Mps;
   Float64 m_Mllim; // includes LLDF
   Float64 m_Mcrack;
   Float64 m_Icrack;
   Float64 m_c;
   Float64 m_Eg;
   Float64 m_gDC;
   Float64 m_gDW;
   Float64 m_gCR;
   Float64 m_gSH;
   Float64 m_gRE;
   Float64 m_gPS;
   Float64 m_gLL;
   Float64 m_gM;   // LLDF used.. in Mllim... just holding on to it here for reporting

   bool m_bRebar;
   Float64 m_db;  // depth to reinforcement from extreme compression face
   Float64 m_fb;  // stress in reinforcement beforc cracking
   Float64 m_fyb; // yield strength
   Float64 m_Eb;  // mod E.

   bool m_bStrand;
   Float64 m_dps;  // depth to reinforcement from extreme compression face
   Float64 m_fps;  // stress in reinforcement beforc cracking
   Float64 m_fyps; // yield strength
   Float64 m_Eps;  // mod E.

   bool m_bTendon;
   Float64 m_dpt;  // depth to reinforcement from extreme compression face
   Float64 m_fpt;  // stress in reinforcement beforc cracking
   Float64 m_fypt; // yield strength
   Float64 m_Ept;  // mod E.
};
