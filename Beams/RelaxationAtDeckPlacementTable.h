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

// RelaxationAtDeckPlacementTable.h : Declaration of the CRelaxationAtDeckPlacementTable

#ifndef __RELAXATIONATDECKPLACEMENTTABLE_H_
#define __RELAXATIONATDECKPLACEMENTTABLE_H_

#include "resource.h"       // main symbols
#include <Details.h>
#include <EAF\EAFDisplayUnits.h>


class lrfdLosses;
class CGirderData;

/////////////////////////////////////////////////////////////////////////////
// CRelaxationAtDeckPlacementTable
class CRelaxationAtDeckPlacementTable : public rptRcTable
{
public:
	static CRelaxationAtDeckPlacementTable* PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level);
	void AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level);

private:
   CRelaxationAtDeckPlacementTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits);

   DECLARE_UV_PROTOTYPE( rptStressUnitValue,  stress );
   DECLARE_UV_PROTOTYPE( rptTimeUnitValue,    time);
   rptRcScalar scalar;

   const CGirderData* m_pGD;
};

#endif //__RELAXATIONATDECKPLACEMENTTABLE_H_
