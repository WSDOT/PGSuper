///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

// ElasticGainDueToDeckPlacementTable.h : Declaration of the CElasticGainDueToDeckPlacementTable

#ifndef __ELASTICGAINDUETODECKPLACEMENTTABLE_H_
#define __ELASTICGAINDUETODECKPLACEMENTTABLE_H_

#include "resource.h"       // main symbols
#include <Details.h>
#include <EAF\EAFDisplayUnits.h>

#include <IFace\AnalysisResults.h>

class WBFL::LRFD::Losses;

/////////////////////////////////////////////////////////////////////////////
// CElasticGainDueToDeckPlacementTable
class CElasticGainDueToDeckPlacementTable : public rptRcTable
{
public:
	static CElasticGainDueToDeckPlacementTable* PrepareTable(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,IEAFDisplayUnits* pDisplayUnits,Uint16 level);
   void AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,const LOSSDETAILS* pDetails,IEAFDisplayUnits* pDisplayUnits,Uint16 level);

private:
   CElasticGainDueToDeckPlacementTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits);

   pgsTypes::BridgeAnalysisType m_BAT;
   bool m_bHasDeckLoads;
   bool m_bHasDeckPanel;
   bool m_bHasUserLoads;
   bool m_bHasLongitudinalJoints;

   DECLARE_UV_PROTOTYPE( rptPointOfInterest,  spanloc );
   DECLARE_UV_PROTOTYPE( rptPointOfInterest,  gdrloc );
   DECLARE_UV_PROTOTYPE( rptLengthUnitValue,  cg );
   DECLARE_UV_PROTOTYPE( rptStressUnitValue,  mod_e );
   DECLARE_UV_PROTOTYPE( rptForceUnitValue,   force );
   DECLARE_UV_PROTOTYPE( rptAreaUnitValue,    area );
   DECLARE_UV_PROTOTYPE( rptLength4UnitValue, mom_inertia );
   DECLARE_UV_PROTOTYPE( rptLengthUnitValue,  ecc );
   DECLARE_UV_PROTOTYPE( rptMomentUnitValue,  moment );
   DECLARE_UV_PROTOTYPE( rptStressUnitValue,  stress );

   rptRcScalar scalar;
   bool m_bIsPrismatic;
   bool m_bIsAsymmetric;
};

#endif //__ELASTICGAINDUETODECKPLACEMENTTABLE_H_
