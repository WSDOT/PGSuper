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


// MultiGirderSelectGrid.h : header file
//
#include <PGSuperAll.h>
#include <PsgLib\Keys.h>

#if defined _NOGRID
#include <NoGrid.h>
#else
#include "SharedCTrls\MultiSelectGrid.h" 
#endif

// Container for spans and girders on/off settings
typedef std::vector<bool> GirderOnVector;
typedef std::vector<GirderOnVector> GroupGirderOnCollection; // groups containing girders
typedef GroupGirderOnCollection::iterator GroupGirderOnIterator;

/////////////////////////////////////////////////////////////////////////////
// CMultiGirderSelectGrid window

class CMultiGirderSelectGrid : public CMultiSelectGrid
{



// Construction
public:
	CMultiGirderSelectGrid();


public:
   // custom stuff for grid
   void CustomInit(const GroupGirderOnCollection& groupGirderOnCollection, std::_tstring(*pGetGirderLabel)(GirderIndexType));

   // Vector of items turned on (checked)
   std::vector<CGirderKey> GetData();

};

