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

#ifndef INCLUDED_PGSEXT_RAILINGSYSTEM_H_
#define INCLUDED_PGSEXT_RAILINGSYSTEM_H_

#include <WBFLCore.h>

#if !defined INCLUDED_MATHEX_H_
#include <MathEx.h>
#endif

#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

class TrafficBarrierEntry;
interface ILibrary;


/*****************************************************************************
CLASS 
   CRailingSystem

   Utility class for describing a railing system.

DESCRIPTION
   Utility class for describing a railing system. A railing system consists
    of an exterior railing, traffic barrier, and interior railing

COPYRIGHT
   Copyright © 1997-2008
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 05.13.2008 : Created file
*****************************************************************************/

class PGSEXTCLASS CRailingSystem
{
public:
   CRailingSystem();
   CRailingSystem(const CRailingSystem& rOther);
   ~CRailingSystem();

   CRailingSystem& operator = (const CRailingSystem& rOther);
   bool operator == (const CRailingSystem& rOther) const;
   bool operator != (const CRailingSystem& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

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
   pgsTypes::ConcreteType ConcreteType;
   Float64 fc;
   Float64 Ec;
   bool bUserEc;
   Float64 StrengthDensity;
   Float64 WeightDensity;
   Float64 EcK1;
   Float64 EcK2;
   Float64 CreepK1;
   Float64 CreepK2;
   Float64 ShrinkageK1;
   Float64 ShrinkageK2;
   Float64 MaxAggSize;
   bool bHasFct;
   Float64 Fct;

protected:
   void MakeCopy(const CRailingSystem& rOther);
   void MakeAssignment(const CRailingSystem& rOther);

   const TrafficBarrierEntry* pExteriorRailing;
   const TrafficBarrierEntry* pInteriorRailing;
};


#endif // INCLUDED_PGSEXT_RAILINGSYSTEM_H_
