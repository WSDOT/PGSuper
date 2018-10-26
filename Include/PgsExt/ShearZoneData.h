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

#ifndef INCLUDED_PGSEXT_SHEARZONEDATA_H_
#define INCLUDED_PGSEXT_SHEARZONEDATA_H_

// SYSTEM INCLUDES
//
#include <WBFLCore.h>

#if !defined INCLUDED_MATHEX_H_
#include <MathEx.h>
#endif

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <StrData.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   CShearZoneData

   Utility class for shear zone description data.

DESCRIPTION
   Utility class for shear description data. This class encapsulates 
   the input data a single shear zone and implements the IStructuredLoad 
   and IStructuredSave persistence interfaces.

COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 12.03.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS CShearZoneData
{
public:
   Uint32  ZoneNum;
   BarSizeType  VertBarSize, HorzBarSize;
   Float64 BarSpacing;
   Float64 ZoneLength;
   Uint32 nVertBars, nHorzBars;

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CShearZoneData();

   //------------------------------------------------------------------------
   // Constructor
   CShearZoneData(Uint32 zoneNum,BarSizeType barSize,Float64 barSpacing,Float64 zoneLength,Uint32 nbrLegs);

   //------------------------------------------------------------------------
   // Copy constructor
   CShearZoneData(const CShearZoneData& rOther);

   //------------------------------------------------------------------------
   // Destructor
   ~CShearZoneData();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CShearZoneData& operator = (const CShearZoneData& rOther);
   bool operator == (const CShearZoneData& rOther) const;
   bool operator != (const CShearZoneData& rOther) const;

   // GROUP: OPERATIONS

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CShearZoneData& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CShearZoneData& rOther);

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
class PGSEXTCLASS ShearZoneDataLess
{
public:
   bool operator()(const CShearZoneData& a, const CShearZoneData& b)
   {
      return a.ZoneNum < b.ZoneNum;
   }
};


#endif // INCLUDED_PGSEXT_SHEARZONEDATA_H_
