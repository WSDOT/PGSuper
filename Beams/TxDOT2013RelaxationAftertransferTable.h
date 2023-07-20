///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// TxDOT2013RelaxationAfterTransferTable.h : Declaration of the CTxDOT2013RelaxationAfterTransferTable

#ifndef __TxDOT2013RelaxationAfterTransferTable_H_
#define __TxDOT2013RelaxationAfterTransferTable_H_

#include "resource.h"       // main symbols
#include <Details.h>
#include <EAF\EAFDisplayUnits.h>


class WBFL::LRFD::Losses;

/////////////////////////////////////////////////////////////////////////////
// CTxDOT2013RelaxationAfterTransferTable
class CTxDOT2013RelaxationAfterTransferTable : public rptRcTable
{
public:
	static CTxDOT2013RelaxationAfterTransferTable* PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level);
	void AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level);

private:
   CTxDOT2013RelaxationAfterTransferTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits);

   DECLARE_UV_PROTOTYPE( rptStressUnitValue,  stress );
};

#endif //__TxDOT2013RelaxationAfterTransferTable_H_
