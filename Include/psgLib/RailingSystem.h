///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#include "PsgLibLib.h"
#include <MathEx.h>
#include <PsgLib\ConcreteMaterial.h>

class TrafficBarrierEntry;
class ILibrary;


/*****************************************************************************
CLASS 
   CRailingSystem

   Utility class for describing a railing system.

DESCRIPTION
   Utility class for describing a railing system. A railing system consists
    of an exterior railing, traffic barrier, and interior railing

LOG
   rab : 05.13.2008 : Created file
*****************************************************************************/

class PSGLIBCLASS CRailingSystem
{
public:
   CRailingSystem();
   CRailingSystem(const CRailingSystem& rOther) = default;
   ~CRailingSystem() = default;

   CRailingSystem& operator = (const CRailingSystem& rOther) = default;
   bool operator == (const CRailingSystem& rOther) const;
   bool operator != (const CRailingSystem& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress);
   HRESULT Save(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress);

   std::_tstring strExteriorRailing; // name of the exterior railing from the library
   std::_tstring strInteriorRailing; // name of the interior railing from the library

   const TrafficBarrierEntry* GetExteriorRailing() const;
   void SetExteriorRailing(const TrafficBarrierEntry* pRailing);
   const TrafficBarrierEntry* GetInteriorRailing() const;
   void SetInteriorRailing(const TrafficBarrierEntry* pRailing);

   bool bUseSidewalk; // true if a sidewalk is used
   bool bUseInteriorRailing; // true if an interior railing is used (sidewalk must also be used)

   // sidewalk dimensions
   Float64 Width;
   Float64 LeftDepth;
   Float64 RightDepth;

   bool bBarriersOnTopOfSidewalk;
   bool bSidewalkStructurallyContinuous;

   // material properties
   CConcreteMaterial Concrete;

protected:
   const TrafficBarrierEntry* pExteriorRailing;
   const TrafficBarrierEntry* pInteriorRailing;
};
