///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include <WbflTypes.h>
#include <PGSuperTypes.h>

/*****************************************************************************
INTERFACE
   IResistanceFactors

   Interface to access resistance factors defined in the project criteria

DESCRIPTION
   Interface to access resistance factors defined in the project criteria
*****************************************************************************/
// {A6AE2680-831C-4c25-98BD-A6FE7556A726}
DEFINE_GUID(IID_IResistanceFactors, 
0xa6ae2680, 0x831c, 0x4c25, 0x98, 0xbd, 0xa6, 0xfe, 0x75, 0x56, 0xa7, 0x26);
interface IResistanceFactors : IUnknown
{
   virtual void GetFlexureResistanceFactors(pgsTypes::ConcreteType type,Float64* phiTensionPS,Float64* phiTensionRC,Float64* phiTensionSpliced,Float64* phiCompression) = 0;
   virtual void GetFlexuralStrainLimits(matPsStrand::Grade grade,matPsStrand::Type type,Float64* pecl,Float64* petl) = 0;
   virtual void GetFlexuralStrainLimits(matRebar::Grade rebarGrade,Float64* pecl,Float64* petl) = 0;
   virtual Float64 GetShearResistanceFactor(pgsTypes::ConcreteType type) = 0;

   virtual Float64 GetClosureJointFlexureResistanceFactor(pgsTypes::ConcreteType type) = 0;
   virtual Float64 GetClosureJointShearResistanceFactor(pgsTypes::ConcreteType type) = 0;
};

