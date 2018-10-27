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

#include "StdAfx.h"
#include <Reporting\StrandLocations.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>

#include <PgsExt\StrandData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CStrandLocations
****************************************************************************/


CStrandLocations::CStrandLocations()
{
}

CStrandLocations::CStrandLocations(const CStrandLocations& rOther)
{
   MakeCopy(rOther);
}

CStrandLocations::~CStrandLocations()
{
}

CStrandLocations& CStrandLocations::operator= (const CStrandLocations& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }
   return *this;
}

void CStrandLocations::Build(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,
                                IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   GET_IFACE2(pBroker,IBridge,pBridge);

   Float64 Lg = pBridge->GetSegmentLength(segmentKey);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, len, pDisplayUnits->GetSpanLengthUnit(),  false );

   rptParagraph* pHead;
   pHead = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pHead;
   pHead->SetName(_T("Strand Locations"));
   *pHead << pHead->GetName() << rptNewLine;

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   (*pPara) << _T("X = 0 is at CL Girder, Y = 0 is at Top of Girder") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   StrandIndexType Ns = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Straight);
   StrandIndexType Nt = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Temporary);

   rptRcTable* pStraightLayoutTable = nullptr;
   if ( 0 < Ns && 0 < Nt )
   {
      pStraightLayoutTable = rptStyleManager::CreateLayoutTable(2);
      *pPara << pStraightLayoutTable << rptNewLine;
      pPara = &(*pStraightLayoutTable)(0,0);
   }

   // Straight strands
   StrandIndexType nDebonded = pStrandGeometry->GetNumDebondedStrands(segmentKey,pgsTypes::Straight,pgsTypes::dbetEither);
   StrandIndexType nExtendedLeft  = pStrandGeometry->GetNumExtendedStrands(segmentKey,pgsTypes::metStart,pgsTypes::Straight);
   StrandIndexType nExtendedRight = pStrandGeometry->GetNumExtendedStrands(segmentKey,pgsTypes::metEnd,pgsTypes::Straight);
   if (0 < Ns)
   {
      ColumnIndexType nColumns = 5;
      if ( 0 < nDebonded )
      {
         nColumns += 2;
      }

      if ( 0 < nExtendedLeft || 0 < nExtendedRight )
      {
         nColumns += 2;
      }

      rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nColumns,_T("Straight Strands"));
      *pPara << p_table;

      p_table->SetNumberOfHeaderRows(2);

      ColumnIndexType col = 0;

      p_table->SetRowSpan(0, col, 2);
      (*p_table)(0,col++) << _T("Strand");

      p_table->SetColumnSpan(0, col, 2);
      (*p_table)(0, col) << _T("Left End");
      (*p_table)(1,col++) << COLHDR(_T("X"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*p_table)(1,col++) << COLHDR(_T("Y"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

      p_table->SetColumnSpan(0, col, 2);
      (*p_table)(0, col) << _T("Right End");
      (*p_table)(1, col++) << COLHDR(_T("X"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*p_table)(1, col++) << COLHDR(_T("Y"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      if ( 0 < nDebonded )
      {
         p_table->SetRowSpan(0, col, 2);
         (*p_table)(0,col++) << COLHDR(_T("Debonded from") << rptNewLine << _T("Left End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

         p_table->SetRowSpan(0, col, 2);
         (*p_table)(0, col++) << COLHDR(_T("Debonded from") << rptNewLine << _T("Right End"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      }

      if ( 0 < nExtendedLeft || 0 < nExtendedRight )
      {
         p_table->SetRowSpan(0, col, 2);
         p_table->SetColumnStyle(col,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_CENTER));
         p_table->SetStripeRowColumnStyle(col,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_CENTER));
         (*p_table)(0,col++) << _T("Extended") << rptNewLine << _T("Left End");

         p_table->SetRowSpan(0, col, 2);
         p_table->SetColumnStyle(col, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_CENTER));
         p_table->SetStripeRowColumnStyle(col, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_CENTER));
         (*p_table)(0, col++) << _T("Extended") << rptNewLine << _T("Right End");
      }

      GET_IFACE2(pBroker,IPointOfInterest, pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | POI_RELEASED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 2);
      const pgsPointOfInterest& leftPoi(vPoi.front());
      const pgsPointOfInterest& rightPoi(vPoi.back());

      CComPtr<IPoint2dCollection> leftPoints, rightPoints;
      pStrandGeometry->GetStrandPositions(leftPoi, pgsTypes::Straight, &leftPoints);
      pStrandGeometry->GetStrandPositions(rightPoi, pgsTypes::Straight, &rightPoints);
      RowIndexType row = p_table->GetNumberOfHeaderRows();
      for (StrandIndexType is = 0; is < Ns; is++, row++)
      {
         col = 0;
         (*p_table)(row,col++) << is+1;

         CComPtr<IPoint2d> leftPoint;
         leftPoints->get_Item(is, &leftPoint);
         Float64 x,y;
         leftPoint->Location(&x,&y);
         (*p_table)(row,col++) << dim.SetValue(x);
         (*p_table)(row,col++) << dim.SetValue(y);

         CComPtr<IPoint2d> rightPoint;
         rightPoints->get_Item(is, &rightPoint);
         rightPoint->Location(&x, &y);
         (*p_table)(row, col++) << dim.SetValue(x);
         (*p_table)(row, col++) << dim.SetValue(y);

         if ( 0 < nDebonded ) 
         {
            Float64 start,end;
            if ( pStrandGeometry->IsStrandDebonded(segmentKey,is,pgsTypes::Straight,nullptr,&start,&end) )
            {
               (*p_table)(row,col++) << len.SetValue(start);
               (*p_table)(row,col++) << len.SetValue(Lg - end);
            }
            else
            {
               (*p_table)(row,col++) << _T("-");
               (*p_table)(row,col++) << _T("-");
            }
         }

         if ( 0 < nExtendedLeft || 0 < nExtendedRight )
         {
            if ( pStrandGeometry->IsExtendedStrand(segmentKey,pgsTypes::metStart,is,pgsTypes::Straight) )
            {
               (*p_table)(row,col++) << symbol(DOT);
            }
            else
            {
               (*p_table)(row,col++) << _T("");
            }

            if ( pStrandGeometry->IsExtendedStrand(segmentKey,pgsTypes::metEnd,is,pgsTypes::Straight) )
            {
               (*p_table)(row,col++) << symbol(DOT);
            }
            else
            {
               (*p_table)(row,col++) << _T("");
            }
         }
      }
   }
   else
   {
      *pPara <<_T("No Straight Strands in Girder")<<rptNewLine;
   }


   // Temporary strands
   if ( 0 < Nt )
   {
      if ( 0 < Ns )
      {
         pPara = &(*pStraightLayoutTable)(0,1);
      }

      nDebonded = pStrandGeometry->GetNumDebondedStrands(segmentKey,pgsTypes::Temporary,pgsTypes::dbetEither);

      rptRcTable* p_table = rptStyleManager::CreateDefaultTable(5 + (0 < nDebonded ? 2 : 0),_T("Temporary Strand"));
      *pPara << p_table;

      p_table->SetNumberOfHeaderRows(2);

      ColumnIndexType col = 0;

      p_table->SetRowSpan(0, col, 2);
      (*p_table)(0, col++) << _T("Strand");

      p_table->SetColumnSpan(0, col, 2);
      (*p_table)(0, col) << _T("Left End");
      (*p_table)(1, col++) << COLHDR(_T("X"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*p_table)(1, col++) << COLHDR(_T("Y"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      p_table->SetColumnSpan(0, col, 2);
      (*p_table)(0, col) << _T("Right End");
      (*p_table)(1, col++) << COLHDR(_T("X"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*p_table)(1, col++) << COLHDR(_T("Y"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());

      if (0 < nDebonded)
      {
         p_table->SetRowSpan(0, col, 2);
         (*p_table)(0, col++) << COLHDR(_T("Debonded from") << rptNewLine << _T("Left End"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

         p_table->SetRowSpan(0, col, 2);
         (*p_table)(0, col++) << COLHDR(_T("Debonded from") << rptNewLine << _T("Right End"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      }

      GET_IFACE2(pBroker, IPointOfInterest, pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | POI_RELEASED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 2);
      const pgsPointOfInterest& leftPoi(vPoi.front());
      const pgsPointOfInterest& rightPoi(vPoi.back());

      CComPtr<IPoint2dCollection> leftPoints, rightPoints;
      pStrandGeometry->GetStrandPositions(leftPoi, pgsTypes::Temporary, &leftPoints);
      pStrandGeometry->GetStrandPositions(rightPoi, pgsTypes::Temporary, &rightPoints);

      RowIndexType row = p_table->GetNumberOfHeaderRows();
      for (StrandIndexType is = 0; is < Nt; is++, row++)
      {
         col = 0;
         (*p_table)(row,col++) << is+1;

         CComPtr<IPoint2d> leftPoint;
         leftPoints->get_Item(is, &leftPoint);
         Float64 x, y;
         leftPoint->Location(&x, &y);
         (*p_table)(row, col++) << dim.SetValue(x);
         (*p_table)(row, col++) << dim.SetValue(y);

         CComPtr<IPoint2d> rightPoint;
         rightPoints->get_Item(is, &rightPoint);
         rightPoint->Location(&x, &y);
         (*p_table)(row, col++) << dim.SetValue(x);
         (*p_table)(row, col++) << dim.SetValue(y);

         if ( 0 < nDebonded ) 
         {
            Float64 start,end;
            if ( pStrandGeometry->IsStrandDebonded(segmentKey,is,pgsTypes::Temporary,nullptr,&start,&end) )
            {
               (*p_table)(row,col++) << len.SetValue(start);
               (*p_table)(row,col++) << len.SetValue(end);
            }
            else
            {
               (*p_table)(row,col++) << _T("-");
               (*p_table)(row,col++) << _T("-");
            }
         }
      }
   }
   else
   {
      *pPara <<_T("No Temporary Strands in Girder")<<rptNewLine;
   }

   // Harped strands
   GET_IFACE2(pBroker,IGirder,pGirder);
   bool bSymmetric = pGirder->IsSymmetricSegment(segmentKey);

   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   PoiList vPoi;

   bool areHarpedStraight = pStrandGeometry->GetAreHarpedStrandsForcedStraight(segmentKey);
   StrandIndexType Nh = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Harped);
   nDebonded = pStrandGeometry->GetNumDebondedStrands(segmentKey,pgsTypes::Harped,pgsTypes::dbetEither);

   int nStrandGrids;
   if ( areHarpedStraight || Nh==0 )
   {
      if ( bSymmetric )
      {
         nStrandGrids = 1;
         pPoi->GetPointsOfInterest(segmentKey,POI_START_FACE,&vPoi);
         ATLASSERT(vPoi.size() == 1);
      }
      else
      {
         nStrandGrids = 2;
         pPoi->GetPointsOfInterest(segmentKey,POI_START_FACE | POI_END_FACE,&vPoi);
         ATLASSERT(vPoi.size() == 2);
      }
   }
   else
   {
      if ( bSymmetric )
      {
         nStrandGrids = 2;
         pPoi->GetPointsOfInterest(segmentKey, POI_START_FACE, &vPoi);
         ATLASSERT(vPoi.size() == 1);
         PoiList vHpPoi;
         pPoi->GetPointsOfInterest(segmentKey, POI_HARPINGPOINT, &vHpPoi);
         vPoi.push_back(vHpPoi.front());
      }
      else
      {
         nStrandGrids = 4;
         pPoi->GetPointsOfInterest(segmentKey, POI_START_FACE | POI_END_FACE, &vPoi);
         ATLASSERT(vPoi.size() == 2);
         PoiList vHpPoi;
         pPoi->GetPointsOfInterest(segmentKey, POI_HARPINGPOINT, &vHpPoi);
         pPoi->MergePoiLists(vPoi, vHpPoi, &vPoi);
      }
   }

   if (0 < Nh)
   {
      pPara = new rptParagraph;
      *pChapter << pPara;
      rptRcTable* pHarpedLayoutTable = rptStyleManager::CreateLayoutTable(nStrandGrids);
#pragma Reminder("UPDATE: need to use bottom justification for layout table")
   // need to have layout tables support vertical justification so that everything can
   // be bottom justified
   //for ( ColumnIndexType colIdx = 0; colIdx < pHarpedLayoutTable->GetNumberOfColumns(); colIdx++ )
   //{
   //   pHarpedLayoutTable->SetColumnStyle(colIdx,rptStyleManager::GetTableCellStyle(CA_BOTTOM | CJ_LEFT));
   //}

   *pPara << pHarpedLayoutTable << rptNewLine;

      for ( int i = 0; i < nStrandGrids; i++ )
      {
         pPara = &(*pHarpedLayoutTable)(0,i);

         std::_tstring label;
         if ( areHarpedStraight )
         {
            if ( bSymmetric )
            {
               label = _T("Adjustable Straight Strand Locations");
            }
            else
            {
               if ( i == 0 )
               {
                  label = _T("Adjustable Straight Strand Locations at Left End");
               }
               else
               {
                  label = _T("Adjustable Straight Strand Locations at Right End");
               }
            }
         }
         else
         {
            if ( bSymmetric )
            {
               if ( i == 0 )
               {
                  label = _T("Harped Strand Locations at Ends of Girder");
               }
               else
               {
                  label = _T("Harped Strand Locations at Harping Points");
               }
            }
            else
            {
               if ( i == 0 )
               {
                  label = _T("Harped Strand Locations at Left End of Girder");
               }
               else if ( i == 1 )
               {
                  label = _T("Harped Strand Locations at Left Harp Point");
               }
               else if ( i == 2 )
               {
                  label = _T("Harped Strand Locations at Right Harp Point");
               }
               else
               {
                  label = _T("Harped Strand Locations at Right End of Girder");
               }
            }
         }

         rptRcTable* p_table = rptStyleManager::CreateDefaultTable(3 + (0 < nDebonded ? 2 : 0),label.c_str());
         *pPara << p_table;

         (*p_table)(0,0) << _T("Strand");
         (*p_table)(0,1) << COLHDR(_T("X"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         (*p_table)(0,2) << COLHDR(_T("Y"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

         if ( 0 < nDebonded )
         {
            (*p_table)(0,3) << COLHDR(_T("Left End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0,4) << COLHDR(_T("Right End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         }

         CComPtr<IPoint2dCollection> spts;
         pStrandGeometry->GetStrandPositions(vPoi[i], pgsTypes::Harped, &spts);

         RowIndexType row = p_table->GetNumberOfHeaderRows();
         StrandIndexType is;
         for (is = 0; is < Nh; is++,row++)
         {
            (*p_table)(row, 0) << is + 1;
            CComPtr<IPoint2d> spt;
            spts->get_Item(is, &spt);
            Float64 x,y;
            spt->Location(&x,&y);
            (*p_table)(row,1) << dim.SetValue(x);
            (*p_table)(row,2) << dim.SetValue(y);

            if ( 0 < nDebonded ) 
            {
               Float64 start,end;
               if ( pStrandGeometry->IsStrandDebonded(segmentKey,is,pgsTypes::Harped,nullptr,&start,&end) )
               {
                  (*p_table)(row,3) << len.SetValue(start);
                  (*p_table)(row,4) << len.SetValue(end);
               }
               else
               {
                  (*p_table)(row,3) << _T("-");
                  (*p_table)(row,4) << _T("-");
               }
            }
         }
      }
   }
   else
   {
      *pPara <<_T("No Harped Strands in Girder")<<rptNewLine;
   }
}

void CStrandLocations::MakeCopy(const CStrandLocations& rOther)
{
   // Add copy code here...
}

void CStrandLocations::MakeAssignment(const CStrandLocations& rOther)
{
   MakeCopy( rOther );
}
