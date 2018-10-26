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
#pragma once

#include <PgsExt\PgsExtExp.h>
#include <WBFLCore.h>
#include <StrData.h>

/*****************************************************************************
CLASS 
   CConcreteMaterial

   Utility class for concrete material input parameters.

DESCRIPTION
   Utility class for concrete material input parameters.

LOG
   rab : 06.23.2008 : Created file
*****************************************************************************/

class PGSEXTCLASS CConcreteMaterial
{
public:
   pgsTypes::ConcreteType Type;

   // General Concrete Parameters
   Float64 Fc; // 28-day concrete strength
   Float64 WeightDensity;
   Float64 StrengthDensity;
   Float64 MaxAggregateSize;
   bool    bUserEc; // if true, Ec is a user input value
   Float64 Ec; // 28-day secant modulus

   // Initial concrete strength and modulus
   bool    bHasInitial; //if true the concrete has initial values: Fci, bUserEci, and Eci are applicable, otherwise they are undefined
   Float64 Fci; // strength at application of load (at release for pretension, at jacking for post-tension). Not used for plain reinforced concrete
   bool    bUserEci; // if true, Eci is a user input value
   Float64 Eci;

   // Parameters for the AASHTO LRFD Material Model
   Float64 EcK1;
   Float64 EcK2;
   Float64 CreepK1;
   Float64 CreepK2;
   Float64 ShrinkageK1;
   Float64 ShrinkageK2;
   bool    bHasFct; // if true, the concrete model is defined with Fct
   Float64 Fct;

   // Time Dependent Models - General
   bool bBasePropertiesOnInitialValues; // if true, and if bHasInitial is true, the time dependent
                                            // concrete models are based on Fci and Eci, otherwise they are
                                            // based on Fc and Ec.

   // Parameters for the ACI 209R-92 Model
   // also used for LRFD time-dependent concrete
   bool bACIUserParameters; // if true, A and B are user defined parameters, otherwise look up A and B based on cure method and cement type
   Float64 A, B; // A has system units of Time
   pgsTypes::CureMethod CureMethod;
   pgsTypes::ACI209CementType ACI209CementType;

   // Parameters for the CEB-FIP Model
   bool bCEBFIPUserParameters; // if true, S and BetaSc are used defined parameters, otherwise look up S and BetaSc based on cement type
   Float64 S, BetaSc;
   pgsTypes::CEBFIPCementType CEBFIPCementType;


   //------------------------------------------------------------------------
   // Default constructor
   CConcreteMaterial();

   //------------------------------------------------------------------------
   // Copy constructor
   CConcreteMaterial(const CConcreteMaterial& rOther);

   //------------------------------------------------------------------------
   // Destructor
   ~CConcreteMaterial();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CConcreteMaterial& operator = (const CConcreteMaterial& rOther);

   //------------------------------------------------------------------------
   bool operator==(const CConcreteMaterial& rOther) const;

   //------------------------------------------------------------------------
   bool operator!=(const CConcreteMaterial& rOther) const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

#if defined _DEBUG
   void AssertValid();
#endif

protected:
   //------------------------------------------------------------------------
   void MakeCopy(const CConcreteMaterial& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CConcreteMaterial& rOther);
};
