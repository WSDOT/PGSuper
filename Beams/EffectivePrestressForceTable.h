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

// EffectivePrestressForceTable.h : Declaration of the CEffectivePrestressForceTable

#pragma once

#include "resource.h"       // main symbols
#include <Details.h>
#include <EAF\EAFDisplayUnits.h>


class WBFL::LRFD::Losses;
class CGirderData;

/////////////////////////////////////////////////////////////////////////////
// CEffectivePrestressForceTable
class CEffectivePrestressForceTable : public rptRcTable
{
public:
	static CEffectivePrestressForceTable* PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level);
	void AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,IEAFDisplayUnits* pDisplayUnits,Uint16 level);

private:
   CEffectivePrestressForceTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits);

   DECLARE_UV_PROTOTYPE( rptForceUnitValue,   force );
   DECLARE_UV_PROTOTYPE( rptAreaUnitValue,    area );
   DECLARE_UV_PROTOTYPE( rptStressUnitValue,  stress);
   rptRcScalar scalar;

   IntervalIndexType m_LiveLoadIntervalIdx;
};
