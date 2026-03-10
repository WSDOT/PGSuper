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

#include <Details.h>
#include <EAF\EAFDisplayUnits.h>
#include <PgsExt/ReportPointOfInterest.h>

class WBFL::LRFD::Losses;
class CStrandData;

class CTimeDependentLossesAtShippingTable : public rptRcTable
{
public:
   static CTimeDependentLossesAtShippingTable* PrepareTable(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,const CSegmentKey& segmentKey,bool bTemporaryStrands,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,Uint16 level);
	void AddRow(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,Uint16 level);

private:
   CTimeDependentLossesAtShippingTable(ColumnIndexType NumColumns, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);
   
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
   rptRcScalar scalar;

   const CStrandData* m_pStrands = nullptr;
   bool m_bTemporaryStrands;
   bool m_bPCIUHPC;
};
