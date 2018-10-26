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

#ifndef INCLUDED_PGSLIB_HORIZONTALINTERFACEZONEDATA_H_
#define INCLUDED_PGSLIB_HORIZONTALINTERFACEZONEDATA_H_

// SYSTEM INCLUDES
//
#include <WBFLCore.h>


// PROJECT INCLUDES
//
#include "psgLibLib.h"

#include <StrData.h>
#include <Material\Rebar.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
   class sysIStructuredLoad;
   class sysIStructuredSave;
// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   CHorizontalInterfaceZoneData

   Utility class for shear zone description data.

DESCRIPTION
   Utility class for shear description data. This class encapsulates 
   the input data a single shear zone and implements the IStructuredLoad 
   and IStructuredSave persistence interfaces.

LOG
   rdp : 12.03.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS CHorizontalInterfaceZoneData
{
public:
   ZoneIndexType ZoneNum;
   Float64 ZoneLength;
   Float64 BarSpacing;
   matRebar::Size BarSize;
   Float64 nBars;

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CHorizontalInterfaceZoneData();

   //------------------------------------------------------------------------
   // Copy constructor
   CHorizontalInterfaceZoneData(const CHorizontalInterfaceZoneData& rOther);

   //------------------------------------------------------------------------
   // Destructor
   ~CHorizontalInterfaceZoneData();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CHorizontalInterfaceZoneData& operator = (const CHorizontalInterfaceZoneData& rOther);
   bool operator == (const CHorizontalInterfaceZoneData& rOther) const;
   bool operator != (const CHorizontalInterfaceZoneData& rOther) const;

   // GROUP: OPERATIONS

	HRESULT Load(sysIStructuredLoad* pStrLoad);
	HRESULT Save(sysIStructuredSave* pStrSave);

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CHorizontalInterfaceZoneData& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CHorizontalInterfaceZoneData& rOther);

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
class PSGLIBCLASS HorizontalInterfaceZoneDataLess
{
public:
   bool operator()(const CHorizontalInterfaceZoneData& a, const CHorizontalInterfaceZoneData& b)
   {
      return a.ZoneNum < b.ZoneNum;
   }
};


#endif // INCLUDED_PGSLIB_HorizontalInterfaceZoneDATA_H_
