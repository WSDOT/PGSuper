///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
#include <PgsExt\PointOfInterest.h>

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

   void SetAllowableStress(Float64 fr);
   Float64 GetAllowableStress() const;

   void SetDeadLoadFactor(Float64 gDC);
   Float64 GetDeadLoadFactor() const;

   void SetDeadLoadMoment(Float64 Mdc);
   Float64 GetDeadLoadMoment() const;

   void SetWearingSurfaceFactor(Float64 gDW);
   Float64 GetWearingSurfaceFactor() const;

   void SetWearingSurfaceMoment(Float64 Mdw);
   Float64 GetWearingSurfaceMoment() const;

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

   void SetReinforcementDepth(Float64 dps);
   Float64 GetReinforcementDepth() const;

   void SetEffectivePrestress(Float64 fpe);
   Float64 GetEffectivePrestress() const;

   void SetEs(Float64 Es);
   Float64 GetEs() const;

   void SetEg(Float64 Eg);
   Float64 GetEg() const;

   Float64 GetExcessMoment() const;
   Float64 GetCrackingStressIncrement() const;
   Float64 GetStrandStress() const;
   Float64 GetStressRatio() const;

protected:
   void MakeCopy(const pgsYieldStressRatioArtifact& rOther);
   void MakeAssignment(const pgsYieldStressRatioArtifact& rOther);

   mutable bool m_bRFComputed;
   mutable Float64 m_RF;
   mutable Float64 m_fcr;
   mutable Float64 m_fs;

   pgsPointOfInterest m_POI;

   pgsTypes::LoadRatingType m_RatingType;

   VehicleIndexType m_VehicleIndex;
   Float64 m_VehicleWeight;
   std::_tstring m_strVehicleName;

   Float64 m_fr;
   Float64 m_fpe;
   Float64 m_Mdc;
   Float64 m_Mdw;
   Float64 m_Mllim;
   Float64 m_Mcr;
   Float64 m_Icr;
   Float64 m_c;
   Float64 m_dps;
   Float64 m_Es;
   Float64 m_Eg;
   Float64 m_gDC;
   Float64 m_gDW;
   Float64 m_gLL;
};
