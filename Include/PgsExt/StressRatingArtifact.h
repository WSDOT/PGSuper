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

#pragma once

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\ReportPointOfInterest.h>

/*****************************************************************************
CLASS 
   pgsStressRatingArtifact

   Artifact for flexural stress load rating analysis


DESCRIPTION
   Artifact for load rating analysis. Holds rating artificts for
   design, legal, and permit load ratings.


COPYRIGHT
   Copyright © 1997-2009
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 12.07.2009 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsStressRatingArtifact
{
public:
   pgsStressRatingArtifact();
   pgsStressRatingArtifact(const pgsStressRatingArtifact& rOther);
   virtual ~pgsStressRatingArtifact();

   pgsStressRatingArtifact& operator = (const pgsStressRatingArtifact& rOther);

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

   void SetDeadLoadStress(Float64 fdc);
   Float64 GetDeadLoadStress() const;

   void SetPrestressStress(Float64 fps);
   Float64 GetPrestressStress() const;

   void SetWearingSurfaceFactor(Float64 gDW);
   Float64 GetWearingSurfaceFactor() const;

   void SetWearingSurfaceStress(Float64 fdw);
   Float64 GetWearingSurfaceStress() const;

   void SetLiveLoadFactor(Float64 gLL);
   Float64 GetLiveLoadFactor() const;

   void SetLiveLoadStress(Float64 fllim);
   Float64 GetLiveLoadStress() const;

   Float64 GetRatingFactor() const;

protected:
   void MakeCopy(const pgsStressRatingArtifact& rOther);
   virtual void MakeAssignment(const pgsStressRatingArtifact& rOther);

   mutable bool m_bRFComputed;
   mutable Float64 m_RF;

   pgsPointOfInterest m_POI;

   pgsTypes::LoadRatingType m_RatingType;

   VehicleIndexType m_VehicleIndex;
   Float64 m_VehicleWeight;
   std::_tstring m_strVehicleName;

   Float64 m_fr;
   Float64 m_gDC;
   Float64 m_gDW;
   Float64 m_gLL;
   Float64 m_fdc;
   Float64 m_fps;
   Float64 m_fdw;
   Float64 m_fllim;
};
