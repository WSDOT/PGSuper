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

#include "StdAfx.h"
#include <Reporting\SectPropTable2.h>
#include <Reporting\ReportNotes.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#include <PgsExt\ReportPointOfInterest.h>
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
                                            pgsTypes::SectionPropertyType spType,
                                            const CSegmentKey& segmentKey,
                                            IntervalIndexType intervalIdx,
                                            IEAFDisplayUnits* pDisplayUnits) const
{
   USES_CONVERSION;
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);

   IntervalIndexType erectionIntervalIdx            = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType firstCompositeDeckIntervalIdx  = pIntervals->GetFirstCompositeDeckInterval();
   IntervalIndexType lastCompositeDeckIntervalIdx   = pIntervals->GetLastCompositeDeckInterval();
   IntervalIndexType lastTendonStressingIntervalIdx = pIntervals->GetLastGirderTendonStressingInterval(segmentKey);

   std::_tostringstream os;
   os << "Interval " << LABEL_INTERVAL(intervalIdx) << _T(" : ") << pIntervals->GetDescription(intervalIdx);

   if ( spType == pgsTypes::sptTransformedNoncomposite )
   {
      os << _T(" - Transformed non-composite properties");
   }
   else if ( spType == pgsTypes::sptTransformed )
   {
      if ( intervalIdx < firstCompositeDeckIntervalIdx )
      {
         os << _T(" - Transformed non-composite properties");
      }
      else
      {
         os << _T(" - Transformed composite properties");
      }
   }

   os << _T("; for ") << pgsGirderLabel::GetSegmentLabel(segmentKey);

   bool bIsCompositeDeck = pBridge->IsCompositeDeck();
   bool bAsymmetricGirders = pBridge->HasAsymmetricGirders();
   pgsTypes::HaunchAnalysisSectionPropertiesType haunchAType = pSectProp->GetHaunchAnalysisSectionPropertiesType();


   ColumnIndexType nCol;
   if ( intervalIdx < erectionIntervalIdx)
   {
      nCol = 14;
      if (bAsymmetricGirders)
      {
         nCol++;
      }
   }
   else if ( pBridge->GetDeckType() != pgsTypes::sdtNone && bIsCompositeDeck && lastCompositeDeckIntervalIdx <= intervalIdx)
   {
      if (spType == pgsTypes::sptGrossNoncomposite || spType == pgsTypes::sptTransformedNoncomposite)
      {
         nCol = (firstCompositeDeckIntervalIdx == intervalIdx ? 13 : 12);
      }
      else
      {
         if (lastTendonStressingIntervalIdx != INVALID_INDEX && lastCompositeDeckIntervalIdx <= lastTendonStressingIntervalIdx)
         {
            nCol = 18;
         }
         else
         {
            nCol = 16;
         }
      }

      if (pgsTypes::hspZeroHaunch != haunchAType && (spType == pgsTypes::sptGross || spType == pgsTypes::sptTransformed))
      {
         // assumed haunch depth
         nCol++;
      }
   }
   else
   {
      nCol = 13;
   }

   rptRcTable* xs_table = rptStyleManager::CreateDefaultTable(nCol,os.str().c_str());

   if ( segmentKey.groupIndex == ALL_GROUPS )
   {
      xs_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      xs_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Setup column headers
   ColumnIndexType col = 0;
   if ( intervalIdx < erectionIntervalIdx )
   {
      (*xs_table)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   }
   else
   {
      (*xs_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   }
   (*xs_table)(0,col++) << COLHDR(_T("Area"),           rptAreaUnitTag,    pDisplayUnits->GetAreaUnit() );
   (*xs_table)(0,col++) << COLHDR(_T("Depth"),          rptLengthUnitTag,  pDisplayUnits->GetComponentDimUnit() );
   (*xs_table)(0,col++) << COLHDR(RPT_IX,               rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );

   if (intervalIdx < erectionIntervalIdx)
   {
      (*xs_table)(0, col++) << COLHDR(RPT_IY, rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());

      if (bAsymmetricGirders)
      {
         (*xs_table)(0, col++) << COLHDR(RPT_IXY, rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
      }
   }


   (*xs_table)(0, col++) << COLHDR(RPT_XLEFT_GIRDER, rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*xs_table)(0, col++) << COLHDR(RPT_XRIGHT_GIRDER, rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

   if ( intervalIdx < firstCompositeDeckIntervalIdx  || (spType == pgsTypes::sptGrossNoncomposite || spType == pgsTypes::sptTransformedNoncomposite) )
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

   if ( intervalIdx < firstCompositeDeckIntervalIdx ||
       (spType == pgsTypes::sptGrossNoncomposite || spType == pgsTypes::sptTransformedNoncomposite) ||
       (lastTendonStressingIntervalIdx != INVALID_INDEX && lastCompositeDeckIntervalIdx <= lastTendonStressingIntervalIdx)
       )
   {
      (*xs_table)(0,col++) << Sub2(_T("k"),_T("t")) << _T(" (") << rptLengthUnitTag( &pDisplayUnits->GetComponentDimUnit().UnitOfMeasure ) <<_T(")") << rptNewLine << _T("(Top") << rptNewLine << _T("kern") << rptNewLine << _T("point)");
      (*xs_table)(0,col++) << Sub2(_T("k"),_T("b")) << _T(" (") << rptLengthUnitTag( &pDisplayUnits->GetComponentDimUnit().UnitOfMeasure ) <<_T(")") << rptNewLine << _T("(Bottom") << rptNewLine << _T("kern") << rptNewLine << _T("point)");
   }

   if ( lastCompositeDeckIntervalIdx <= intervalIdx && bIsCompositeDeck && (spType == pgsTypes::sptGross || spType == pgsTypes::sptTransformed))
   {
      (*xs_table)(0,col++) << COLHDR(Sub2(_T("Q"),_T("deck")), rptLength3UnitTag, pDisplayUnits->GetSectModulusUnit() );
      (*xs_table)(0,col++) << COLHDR(_T("Effective") << rptNewLine << _T("Flange") << rptNewLine << _T("Width"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

      if (pgsTypes::hspZeroHaunch != haunchAType)
      {
         (*xs_table)(0,col++) << COLHDR(_T("Haunch") << rptNewLine << _T("Depth"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      }
   }
   else if ( intervalIdx <= firstCompositeDeckIntervalIdx )
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

   // Get all the tabular poi's for flexure and shear
   // Merge the two vectors to form one vector to report on.
   PoiAttributeType poiRefAttribute = (intervalIdx < erectionIntervalIdx ? POI_RELEASED_SEGMENT : POI_ERECTED_SEGMENT);
   PoiList vPoi;
   pIPoi->GetPointsOfInterest(segmentKey, poiRefAttribute, &vPoi);
   PoiList vPoiSpecial;
   pIPoi->GetPointsOfInterest(segmentKey, POI_SPECIAL | POI_START_FACE | POI_END_FACE | POI_SECTCHANGE, &vPoiSpecial);
   pIPoi->MergePoiLists(vPoi, vPoiSpecial, &vPoi);

   RowIndexType row = xs_table->GetNumberOfHeaderRows();

   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      Float64 depth = pSectProp->GetHg(spType,intervalIdx,poi);

      (*xs_table)(row,col++) << location.SetValue( poiRefAttribute, poi );
      (*xs_table)(row,col++) << l2.SetValue(pSectProp->GetAg(spType,intervalIdx,poi));
      (*xs_table)(row,col++) << l1.SetValue(depth);
      (*xs_table)(row,col++) << l4.SetValue(pSectProp->GetIxx(spType,intervalIdx,poi));
      if (intervalIdx < erectionIntervalIdx)
      {
         (*xs_table)(row, col++) << l4.SetValue(pSectProp->GetIyy(spType, intervalIdx, poi));
         if (bAsymmetricGirders)
         {
            (*xs_table)(row, col++) << l4.SetValue(pSectProp->GetIxy(spType, intervalIdx, poi));
         }
      }

      (*xs_table)(row, col++) << l1.SetValue(pSectProp->GetXleft(spType, intervalIdx, poi));
      (*xs_table)(row, col++) << l1.SetValue(pSectProp->GetXright(spType, intervalIdx, poi));

      if ( intervalIdx < firstCompositeDeckIntervalIdx  || (spType == pgsTypes::sptGrossNoncomposite || spType == pgsTypes::sptTransformedNoncomposite) )
      {
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetY(spType,intervalIdx,poi,pgsTypes::TopGirder));
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetY(spType,intervalIdx,poi,pgsTypes::BottomGirder));
         (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetS(spType,intervalIdx,poi,pgsTypes::TopGirder));
         (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetS(spType,intervalIdx,poi,pgsTypes::BottomGirder));
      }
      else
      {
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetY(spType,intervalIdx,poi,pgsTypes::TopGirder));
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetY(spType,intervalIdx,poi,pgsTypes::BottomGirder));
         if ( bIsCompositeDeck )
         {
            (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetY(spType,intervalIdx,poi,pgsTypes::TopDeck));
            (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetY(spType,intervalIdx,poi,pgsTypes::BottomDeck));
         }

         (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetS(spType,intervalIdx,poi,pgsTypes::TopGirder));
         (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetS(spType,intervalIdx,poi,pgsTypes::BottomGirder));
         if ( bIsCompositeDeck )
         {
            (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetS(spType,intervalIdx,poi,pgsTypes::TopDeck));
            (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetS(spType,intervalIdx,poi,pgsTypes::BottomDeck));
         }
      }

      if ( (intervalIdx < firstCompositeDeckIntervalIdx) ||
           (spType == pgsTypes::sptGrossNoncomposite || spType == pgsTypes::sptTransformedNoncomposite) ||
           (lastTendonStressingIntervalIdx != INVALID_INDEX && lastCompositeDeckIntervalIdx <= lastTendonStressingIntervalIdx)
         )
      {
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetKt(spType,intervalIdx,poi));
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetKb(spType,intervalIdx,poi));
      }

      if ( firstCompositeDeckIntervalIdx <= intervalIdx && bIsCompositeDeck  && (spType == pgsTypes::sptGross || spType == pgsTypes::sptTransformed) )
      {
         (*xs_table)(row,col++) << l3.SetValue(pSectProp->GetQSlab(intervalIdx,poi));
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetEffectiveFlangeWidth(poi));
      }
      else if ( intervalIdx <= firstCompositeDeckIntervalIdx )
      {
         (*xs_table)(row,col++) << l1.SetValue(pSectProp->GetPerimeter(poi));
      }

      if (lastCompositeDeckIntervalIdx <= intervalIdx && bIsCompositeDeck && pgsTypes::hspZeroHaunch != haunchAType && (spType == pgsTypes::sptGross || spType == pgsTypes::sptTransformed))
      {
         (*xs_table)(row, col++) << l1.SetValue(pSectProp->GetStructuralHaunchDepth(poi, haunchAType));
      }

      row++;
   }

   return xs_table;
}
