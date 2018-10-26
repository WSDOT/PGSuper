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
#include <Reporting\SectPropTable2.h>
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
   CSectionPropertiesTable2
****************************************************************************/

CSectionPropertiesTable2::CSectionPropertiesTable2()
{
}

CSectionPropertiesTable2::~CSectionPropertiesTable2()
{
}

rptRcTable* CSectionPropertiesTable2::Build(IBroker* pBroker,
                                            const CSegmentKey& segmentKey,
                                            IntervalIndexType intervalIdx,
                                            IEAFDisplayUnits* pDisplayUnits) const
{
   USES_CONVERSION;
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IBridge,pBridge);

   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(segmentKey);
   IntervalIndexType lastTendonStressingIntervalIdx = pIntervals->GetLastTendonStressingInterval(segmentKey);

   std::_tostringstream os;
   os << "Interval " << LABEL_INTERVAL(intervalIdx) << _T(" : ") <<  pIntervals->GetDescription(segmentKey,intervalIdx);

   bool bIsCompositeDeck = pBridge->IsCompositeDeck();

   ColumnIndexType nCol;
   if ( intervalIdx < compositeDeckIntervalIdx )
      nCol = 12;
   else if ( pBridge->GetDeckType() != pgsTypes::sdtNone && bIsCompositeDeck && compositeDeckIntervalIdx <= intervalIdx)
      nCol = 17;
   else if ( intervalIdx == compositeDeckIntervalIdx )
      nCol = 13; // BS2 and noncomposite deck
   else
      nCol = 10;

   rptRcTable* xs_table = pgsReportStyleHolder::CreateDefaultTable(nCol,os.str().c_str());

   if ( segmentKey.groupIndex == ALL_GROUPS )
   {
      xs_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      xs_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup column headers
   ColumnIndexType col = 0;
   (*xs_table)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*xs_table)(0,col++) << COLHDR(_T("Area"),           rptAreaUnitTag,    pDisplayUnits->GetAreaUnit() );
   (*xs_table)(0,col++) << COLHDR(_T("Depth"),          rptLengthUnitTag,  pDisplayUnits->GetComponentDimUnit() );
   (*xs_table)(0,col++) << COLHDR(RPT_IX,               rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
   (*xs_table)(0,col++) << COLHDR(RPT_IY,               rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );

   if ( intervalIdx < compositeDeckIntervalIdx )
   {
      (*xs_table)(0,col++) << COLHDR(RPT_YTOP_GIRDER, rptLengthUnitTag,  pDisplayUnits->GetComponentDimUnit() );
      (*xs_table)(0,col++) << COLHDR(RPT_YBOT_GIRDER, rptLengthUnitTag,  pDisplayUnits->GetComponentDimUnit() );
      (*xs_table)(0,col++) << COLHDR(RPT_STOP_GIRDER, rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
      (*xs_table)(0,col++) << COLHDR(RPT_SBOT_GIRDER, rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
   }
   else
   {
      (*xs_table)(0,col++) << COLHDR(RPT_YTOP_GIRDER, rptLengthUnitTag,  pDisplayUnits->GetComponentDimUnit() );
      (*xs_table)(0,col++) << COLHDR(RPT_YBOT_GIRDER, rptLengthUnitTag,  pDisplayUnits->GetComponentDimUnit() );

      if ( bIsCompositeDeck )
      {
         (*xs_table)(0,col++) << COLHDR(RPT_YTOP_DECK, rptLengthUnitTag,  pDisplayUnits->GetComponentDimUnit() );
         (*xs_table)(0,col++) << COLHDR(RPT_YBOT_DECK, rptLengthUnitTag,  pDisplayUnits->GetComponentDimUnit() );
      }

      (*xs_table)(0,col++) << COLHDR(RPT_STOP_GIRDER, rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
      (*xs_table)(0,col++) << COLHDR(RPT_SBOT_GIRDER, rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );

      if ( bIsCompositeDeck )
      {
         (*xs_table)(0,col++) << COLHDR(RPT_STOP_DECK, rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
         (*xs_table)(0,col++) << COLHDR(RPT_SBOT_DECK, rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
      }
   }

   if ( intervalIdx < compositeDeckIntervalIdx ||
       (lastTendonStressingIntervalIdx != INVALID_INDEX && compositeDeckIntervalIdx <= lastTendonStressingIntervalIdx)
       )
   {
      (*xs_table)(0,col++) << Sub2(_T("k"),_T("t")) << _T(" (") << rptLengthUnitTag( &pDisplayUnits->GetComponentDimUnit().UnitOfMeasure ) <<_T(")") << rptNewLine << _T("(Top") << rptNewLine << _T("kern") << rptNewLine << _T("point)");
      (*xs_table)(0,col++) << Sub2(_T("k"),_T("b")) << _T(" (") << rptLengthUnitTag( &pDisplayUnits->GetComponentDimUnit().UnitOfMeasure ) <<_T(")") << rptNewLine << _T("(Bottom") << rptNewLine << _T("kern") << rptNewLine << _T("point)");
   }

   if ( compositeDeckIntervalIdx <= intervalIdx && bIsCompositeDeck )
   {
      (*xs_table)(0,col++) << COLHDR(Sub2(_T("Q"),_T("deck")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
      (*xs_table)(0,col++) << COLHDR(_T("Effective") << rptNewLine << _T("Flange") << rptNewLine << _T("Width"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }
   else if ( intervalIdx <= compositeDeckIntervalIdx )
   {
      (*xs_table)(0,col++) << COLHDR(_T("Perimeter"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),      false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,        l1,       pDisplayUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,          l2,       pDisplayUnits->GetAreaUnit(),            false );
   INIT_UV_PROTOTYPE( rptLength3UnitValue,       l3,       pDisplayUnits->GetSectModulusUnit(),     false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue,       l4,       pDisplayUnits->GetMomentOfInertiaUnit(), false );

   location.IncludeSpanAndGirder(segmentKey.groupIndex == ALL_GROUPS || segmentKey.segmentIndex == ALL_SEGMENTS);

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

      Float64 end_size = pBridge->GetSegmentStartEndDistance(segKey);
      if ( intervalIdx < compositeDeckIntervalIdx )
         end_size = 0;

      Float64 depth = pSectProp->GetHg(intervalIdx,poi);

      (*xs_table)(row,col++) << location.SetValue( POI_RELEASED_SEGMENT, poi, end_size );
      (*xs_table)(row,col++) << l2.SetValue(pSectProp->GetAg(intervalIdx,poi));
      (*xs_table)(row,col++) << l1.SetValue(depth);
      (*xs_table)(row,col++) << l4.SetValue(pSectProp->GetIx(intervalIdx,poi));
      (*xs_table)(row,col++) << l4.SetValue(pSectProp->GetIy(intervalIdx,poi));

      if ( intervalIdx < compositeDeckIntervalIdx )
      {
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetY(intervalIdx,poi,pgsTypes::TopGirder));
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetY(intervalIdx,poi,pgsTypes::BottomGirder));
         (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetS(intervalIdx,poi,pgsTypes::TopGirder));
         (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetS(intervalIdx,poi,pgsTypes::BottomGirder));
      }
      else
      {
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetY(intervalIdx,poi,pgsTypes::TopGirder));
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetY(intervalIdx,poi,pgsTypes::BottomGirder));
         if ( bIsCompositeDeck )
         {
            (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetY(intervalIdx,poi,pgsTypes::TopDeck));
            (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetY(intervalIdx,poi,pgsTypes::BottomDeck));
         }

         (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetS(intervalIdx,poi,pgsTypes::TopGirder));
         (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetS(intervalIdx,poi,pgsTypes::BottomGirder));
         if ( bIsCompositeDeck )
         {
            (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetS(intervalIdx,poi,pgsTypes::TopDeck));
            (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetS(intervalIdx,poi,pgsTypes::BottomDeck));
         }
      }

      if ( (intervalIdx < compositeDeckIntervalIdx) ||
           (lastTendonStressingIntervalIdx != INVALID_INDEX && compositeDeckIntervalIdx <= lastTendonStressingIntervalIdx)
         )
      {
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetKt(intervalIdx,poi));
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetKb(intervalIdx,poi));
      }

      if ( compositeDeckIntervalIdx <= intervalIdx && bIsCompositeDeck )
      {
         (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetQSlab(poi));
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetEffectiveFlangeWidth(poi));
      }
      else if ( intervalIdx <= compositeDeckIntervalIdx )
      {
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetPerimeter(poi));
      }
   }

   return xs_table;
}
