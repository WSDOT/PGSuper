///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSEXT_GIRDERMATERIAL_H_
#define INCLUDED_PGSEXT_GIRDERMATERIAL_H_

// SYSTEM INCLUDES
//
#include <WBFLCore.h>

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#if !defined INCLUDED_STRDATA_H_
#include <StrData.h>
#endif

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class matPsStrand;

// MISCELLANEOUS
//


/*****************************************************************************
CLASS 
   CGirderMaterial

   Utility class for girder materials.

DESCRIPTION
   Utility class for girder materials.


COPYRIGHT
   Copyright © 1997-2008
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 06.23.2008 : Created file
*****************************************************************************/

class PGSEXTCLASS CGirderMaterial
{
public:
   // girder concrete material properties
   pgsTypes::ConcreteType Type;
   Float64 Fci;             // f'c at release
   Float64 Fc;
   Float64 WeightDensity;
   Float64 StrengthDensity;
   Float64 MaxAggregateSize;
   Float64 EcK1;
   Float64 EcK2;
   Float64 CreepK1;
   Float64 CreepK2;
   Float64 ShrinkageK1;
   Float64 ShrinkageK2;
   Float64 Eci;
   bool    bUserEci;
   Float64 Ec;
   bool    bUserEc;
   bool    bHasFct;
   Float64 Fct;

   // strand
   const matPsStrand* pStrandMaterial;


   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CGirderMaterial();

   //------------------------------------------------------------------------
   // Copy constructor
   CGirderMaterial(const CGirderMaterial& rOther);

   //------------------------------------------------------------------------
   // Destructor
   ~CGirderMaterial();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CGirderMaterial& operator = (const CGirderMaterial& rOther);

   //------------------------------------------------------------------------
   bool operator==(const CGirderMaterial& rOther) const;

   //------------------------------------------------------------------------
   bool operator!=(const CGirderMaterial& rOther) const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CGirderMaterial& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CGirderMaterial& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//


#endif // INCLUDED_PGSEXT_GIRDERMATERIAL_H_

