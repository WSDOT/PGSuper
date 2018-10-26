///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// TimeDependentLossesAtDeckPlacementTable.h : Declaration of the CTimeDependentLossesAtDeckPlacementTable

#ifndef __TIMEDEPENDENTLOSSESATDECKPLACEMENTTABLE_H_
#define __TIMEDEPENDENTLOSSESATDECKPLACEMENTTABLE_H_

#include "resource.h"       // main symbols
#include <Details.h>
#include <EAF\EAFDisplayUnits.h>
#include <PgsExt\ReportStyleHolder.h>

class lrfdLosses;
class CGirderData;

/////////////////////////////////////////////////////////////////////////////
// CTimeDependentLossesAtDeckPlacementTable
class CTimeDependentLossesAtDeckPlacementTable : public rptRcTable
{
public:
	static CTimeDependentLossesAtDeckPlacementTable* PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits,Uint16 level);
	void AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level);

private:
   CTimeDependentLossesAtDeckPlacementTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits);

   DECLARE_UV_PROTOTYPE( rptPointOfInterest,  spanloc );
   DECLARE_UV_PROTOTYPE( rptPointOfInterest,  gdrloc );
   DECLARE_UV_PROTOTYPE( rptLengthUnitValue,  offset );
   DECLARE_UV_PROTOTYPE( rptStressUnitValue,  mod_e );
   DECLARE_UV_PROTOTYPE( rptForceUnitValue,   force );
   DECLARE_UV_PROTOTYPE( rptAreaUnitValue,    area );
   DECLARE_UV_PROTOTYPE( rptLength4UnitValue, mom_inertia );
   DECLARE_UV_PROTOTYPE( rptLengthUnitValue,  ecc );
   DECLARE_UV_PROTOTYPE( rptMomentUnitValue,  moment );
   DECLARE_UV_PROTOTYPE( rptStressUnitValue,  stress );

   const CGirderData* m_pGD;
};

#endif //__TIMEDEPENDENTLOSSESATDECKPLACEMENTTABLE_H_
