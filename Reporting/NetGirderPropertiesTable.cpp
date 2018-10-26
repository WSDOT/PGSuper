///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include "StdAfx.h"
#include <Reporting\NetGirderPropertiesTable.h>
#include <Reporting\ReportNotes.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#include <PgsExt\GirderPointOfInterest.h>
#include <PgsExt\BridgeDescription2.h>


#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CNetGirderPropertiesTable
****************************************************************************/

CNetGirderPropertiesTable::CNetGirderPropertiesTable()
{
}

CNetGirderPropertiesTable::~CNetGirderPropertiesTable()
{
}

rptRcTable* CNetGirderPropertiesTable::Build(IBroker* pBroker,
                                            const CSegmentKey& segmentKey,
                                            IntervalIndexType intervalIdx,
                                            IEAFDisplayUnits* pDisplayUnits) const
{
   USES_CONVERSION;
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   std::_tostringstream os;
   os << "Interval " << LABEL_INTERVAL(intervalIdx) << _T(" : ") <<  pIntervals->GetDescription(intervalIdx);

   rptRcTable* xs_table = pgsReportStyleHolder::CreateDefaultTable(9,os.str().c_str());
   xs_table->SetNumberOfHeaderRows(2);

   if ( segmentKey.groupIndex == ALL_GROUPS )
   {
      xs_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      xs_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      xs_table->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      xs_table->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup column headers
   ColumnIndexType col = 0;
   xs_table->SetRowSpan(0,col,2);
   xs_table->SetRowSpan(1,col,SKIP_CELL);
   (*xs_table)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );

   xs_table->SetColumnSpan(0,col,4);
   xs_table->SetColumnSpan(0,col+1,SKIP_CELL);
   xs_table->SetColumnSpan(0,col+2,SKIP_CELL);
   xs_table->SetColumnSpan(0,col+3,SKIP_CELL);
   (*xs_table)(0,col) << _T("Girder");
   (*xs_table)(1,col++) << COLHDR(_T("Area"),           rptAreaUnitTag,    pDisplayUnits->GetAreaUnit() );
   (*xs_table)(1,col++) << COLHDR(RPT_IX,               rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
   (*xs_table)(1,col++) << COLHDR(RPT_YTOP_GIRDER,             rptLengthUnitTag,  pDisplayUnits->GetComponentDimUnit() );
   (*xs_table)(1,col++) << COLHDR(RPT_YBOT_GIRDER, rptLengthUnitTag,  pDisplayUnits->GetComponentDimUnit() );

   xs_table->SetColumnSpan(0,col,4);
   xs_table->SetColumnSpan(0,col+1,SKIP_CELL);
   xs_table->SetColumnSpan(0,col+2,SKIP_CELL);
   xs_table->SetColumnSpan(0,col+3,SKIP_CELL);
   (*xs_table)(0,col) << _T("Deck");
   (*xs_table)(1,col++) << COLHDR(_T("Area"),           rptAreaUnitTag,    pDisplayUnits->GetAreaUnit() );
   (*xs_table)(1,col++) << COLHDR(RPT_IX,               rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
   (*xs_table)(1,col++) << COLHDR(RPT_YTOP_DECK,             rptLengthUnitTag,  pDisplayUnits->GetComponentDimUnit() );
   (*xs_table)(1,col++) << COLHDR(RPT_YBOT_DECK, rptLengthUnitTag,  pDisplayUnits->GetComponentDimUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),      false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,        l1,       pDisplayUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,          l2,       pDisplayUnits->GetAreaUnit(),            false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue,       l4,       pDisplayUnits->GetMomentOfInertiaUnit(), false );

   location.IncludeSpanAndGirder(segmentKey.groupIndex == ALL_SPANS);

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);

   // Get all the tabular poi's for flexure and shear
   // Merge the two vectors to form one vector to report on.
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(segmentKey) );

   RowIndexType row = xs_table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; i != end; i++, row++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = *i;

      const CSegmentKey& segKey = poi.GetSegmentKey();

      //Float64 end_size = pBridge->GetSegmentStartEndDistance(segKey);
      //if ( stage < castDeckStageIdx )
      //   end_size = 0;
      Float64 end_size = 0;

#pragma Reminder("UPDATE: poi location reference")
      (*xs_table)(row,col++) << location.SetValue( POI_RELEASED_SEGMENT, poi, end_size );

      (*xs_table)(row,col++) << l2.SetValue(pSectProp->GetNetAg(intervalIdx,poi));
      (*xs_table)(row,col++) << l4.SetValue(pSectProp->GetNetIg(intervalIdx,poi));
      (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetNetYtg(intervalIdx,poi));
      (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetNetYbg(intervalIdx,poi));

      (*xs_table)(row,col++) << l2.SetValue(pSectProp->GetNetAd(intervalIdx,poi));
      (*xs_table)(row,col++) << l4.SetValue(pSectProp->GetNetId(intervalIdx,poi));
      (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetNetYtd(intervalIdx,poi));
      (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetNetYbd(intervalIdx,poi));
   }

   return xs_table;
}
