///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <Reporting\ConstructabilityCheckTable.h>

#include <IFace\Artifact.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Constructability.h>
#include <IFace\Intervals.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\HoldDownForceArtifact.h>
#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// return true if more than a single span report so we can add columns for span/girder
inline bool ConstrNeedSpanCols(const std::vector<CGirderKey>& girderList, IBridge* pBridge)
{
   bool doNeed = 1 < girderList.size();
   if (!doNeed)
   {
      BOOST_FOREACH(const CGirderKey& girderKey, girderList)
      {
         SpanIndexType startSpanIdx, endSpanIdx;
         pBridge->GetGirderGroupSpans(girderKey.groupIndex,&startSpanIdx,&endSpanIdx);

         if (0 < (endSpanIdx-startSpanIdx) )
         {
            doNeed = true;
            break;
         }
      }
   }
   return doNeed;
}

/****************************************************************************
CLASS
   CConstructabilityCheckTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CConstructabilityCheckTable::CConstructabilityCheckTable()
{
}

CConstructabilityCheckTable::CConstructabilityCheckTable(const CConstructabilityCheckTable& rOther)
{
   MakeCopy(rOther);
}

CConstructabilityCheckTable::~CConstructabilityCheckTable()
{
}

//======================== OPERATORS  =======================================
CConstructabilityCheckTable& CConstructabilityCheckTable::operator= (const CConstructabilityCheckTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}


//======================== OPERATIONS =======================================
void CConstructabilityCheckTable::BuildSlabOffsetTable(rptChapter* pChapter,IBroker* pBroker,const std::vector<CGirderKey>& girderList,IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   // Give progress window a progress meter if needed
   bool bMultiGirderReport = (1 < girderList.size() ? true : false);

   GET_IFACE2(pBroker,IProgress,pProgress);
   DWORD mask = bMultiGirderReport ? PW_ALL|PW_NOCANCEL : PW_ALL|PW_NOGAUGE|PW_NOCANCEL;

   CEAFAutoProgress ap(pProgress,0,mask); 

   if (bMultiGirderReport)
   {
      pProgress->Init(0,(short)girderList.size(),1);  // and for multi-girders, a gauge.
   }

   // First thing we must to is determine if all girders have the same "A" at start and ends. If not, we need a special table
   bool doTable(false);
   bool areAsDifferent(false);
   BOOST_FOREACH(const CGirderKey& girderKey, girderList)
   {
      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);
      const pgsConstructabilityArtifact* pConstrArtifact = pGdrArtifact->GetConstructabilityArtifact();

      SpanIndexType startSpanIdx, endSpanIdx;
      pConstrArtifact->GetSpans(&startSpanIdx,&endSpanIdx);

      for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++)
      {
         const pgsSpanConstructabilityArtifact* pArtifact = pConstrArtifact->GetSpanArtifact(spanIdx);

         if (pArtifact->SlabOffsetStatus() != pgsSpanConstructabilityArtifact::NA)
         {
            doTable = true;
         }

         if (!pArtifact->AreSlabOffsetsSameAtEnds())
         {
            areAsDifferent = true;
         }

         if (doTable && areAsDifferent)
         {
            break; // no need to go further
         }
      }

      if (bMultiGirderReport)
      {
         pProgress->Increment();
      }
   }

   if (doTable)
   {
      if (areAsDifferent)
      {
         BuildMultiSlabOffsetTable(pChapter, pBroker, girderList, pDisplayUnits);
      }
      else
      {
         BuildMonoSlabOffsetTable(pChapter, pBroker, girderList, pDisplayUnits);
      }
   }
}
void CConstructabilityCheckTable::BuildMonoSlabOffsetTable(rptChapter* pChapter,IBroker* pBroker,const std::vector<CGirderKey>& girderList,IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2_NOCHECK(pBroker,IGirderHaunch,pGdrHaunch);
   GET_IFACE2_NOCHECK(pBroker,IBridge,pBridge);

   // if there is only one span/girder, don't need to print span info
   bool needSpanCols = true; //ConstrNeedSpanCols(girderList, pBridge);

   // Create table - delete it later if we don't need it
   ColumnIndexType nCols = needSpanCols ? 6 : 4; // put span/girder in table if multi girder
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols,_T(""));

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim2, pDisplayUnits->GetComponentDimUnit(), true );

   pTable->SetColumnStyle(nCols-1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(nCols-1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   if (needSpanCols)
   {
      (*pTable)(0,col++) << _T("Span");
      (*pTable)(0,col++) << _T("Girder");
   }

   (*pTable)(0,col++) << COLHDR(_T("Provided"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Required"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << _T("Status");
   (*pTable)(0,col++) << _T("Notes");

   RowIndexType row = 0;
   BOOST_FOREACH(const CGirderKey& girderKey, girderList)
   {
      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);
      const pgsConstructabilityArtifact* pConstrArtifact = pGdrArtifact->GetConstructabilityArtifact();

      SpanIndexType startSpanIdx, endSpanIdx;
      pConstrArtifact->GetSpans(&startSpanIdx,&endSpanIdx);

      for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++)
      {
         const pgsSpanConstructabilityArtifact* pArtifact = pConstrArtifact->GetSpanArtifact(spanIdx);

         if (pArtifact->SlabOffsetStatus() != pgsSpanConstructabilityArtifact::NA)
         {
            row++;
            col = 0;
            bool wasExcessive(false);

            if (needSpanCols)
            {
               GirderIndexType girder = girderKey.girderIndex;
               (*pTable)(row, col++) << LABEL_SPAN(spanIdx);
               (*pTable)(row, col++) << LABEL_GIRDER(girder);
            }

            Float64 endA, startA;
            pArtifact->GetProvidedSlabOffset(&startA, &endA); // both values are same because of what function we are in

            (*pTable)(row, col++) << dim.SetValue(startA);
            (*pTable)(row, col++) << dim.SetValue(pArtifact->GetRequiredSlabOffset());

            switch( pArtifact->SlabOffsetStatus() )
            {
               case pgsSpanConstructabilityArtifact::Pass:
                  (*pTable)(row, col++) << RPT_PASS;
                  break;

               case pgsSpanConstructabilityArtifact::Fail:
                  (*pTable)(row, col++) << RPT_FAIL;
                  break;

               case pgsSpanConstructabilityArtifact::Excessive:
                  (*pTable)(row, col++) << color(Blue) << _T("Excessive") << color(Black);
                  wasExcessive = true;
                  break;

               default:
                  ATLASSERT(false);
                  break;
            }

            bool didNote(false);
            // NOTE: Don't increment the column counter in the (*pTable)(row,col) below. All notes go into the same column. If we do (*pTable)(row,col++)
            // and there are multiple notes, the column index advances and we are then writing beyond the end of the table for each subsequent note.
            if ( pArtifact->CheckStirrupLength() )
            {
               didNote = true;
               CSpanKey spanKey(spanIdx, girderKey.girderIndex);
               HAUNCHDETAILS haunch_details;
               pGdrHaunch->GetHaunchDetails(spanKey,&haunch_details);
               (*pTable)(row, col) << color(Red) << _T("The difference betwen the minimum and maximum CL haunch depths along the girder is ") << dim2.SetValue(haunch_details.HaunchDiff) 
                                                 << _T(". This exceeds one half of the slab depth. Check stirrup lengths to ensure they engage the deck in all locations.");
                                                 
               if(pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP)
               {
                  (*pTable)(row, col) << _T(" Also carefully check deck panel leveling.");
               }

               (*pTable)(row, col) << _T(" Refer to the Haunch Details chapter in the Details report for more information.") << color(Black) << rptNewLine;
            }

            if (wasExcessive)
            {
               didNote = true;
               (*pTable)(row, col) << _T("Provided Slab Offset exceeded Required by allowable tolerance of ") << dim2.SetValue(pArtifact->GetExcessSlabOffsetWarningTolerance()) << rptNewLine;
            }

            if (!didNote)
            {
               (*pTable)(row, col) << _T(""); // otherwise table will be rendered funkily
            }

            col++; // done with notes column.... advance the index
         }
      } // next girder
   } // span

   // Only return a table if it has content
   if (0 < row)
   {
      rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
      *pChapter << pTitle;
      *pTitle << _T("Slab Offset (\"A\" Dimension)");
      rptParagraph* pBody = new rptParagraph;
      *pChapter << pBody;
      *pBody << _T("This table compares the input slab offset to the computed slab offset required to have the least haunch depth be equal to the Fillet dimension. A failed status indicates that the top of the girder will encroach into the deck slab and the Slab Offset dimension should be refined.") << rptNewLine;
      *pBody << pTable << rptNewLine;
   }
   else
   {
      delete pTable;
   }
}

void CConstructabilityCheckTable::BuildMultiSlabOffsetTable(rptChapter* pChapter,IBroker* pBroker,const std::vector<CGirderKey>& girderList,IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2_NOCHECK(pBroker,IGirderHaunch,pGdrHaunch);
   GET_IFACE2(pBroker,IBridge,pBridge);

   // if there is only one span/girder, don't need to print span info
   bool needSpanCols = true; // ConstrNeedSpanCols(girderList, pBridge);

   // Create table - delete it later if we don't need it
   ColumnIndexType nCols = needSpanCols ? 9 : 7; // put span/girder in table if multi girder
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols,_T(""));

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, spandim, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim2, pDisplayUnits->GetComponentDimUnit(), true );

   pTable->SetColumnStyle(nCols-1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(nCols-1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   if (needSpanCols)
   {
      (*pTable)(0,col++) << _T("Span");
      (*pTable)(0,col++) << _T("Girder");
   }

   (*pTable)(0,col++) << COLHDR(_T("Slab Offset") << rptNewLine << _T("at Left End") << rptNewLine << _T("CL Brg"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Slab Offset") << rptNewLine << _T("at Right End") << rptNewLine << _T("CL Brg"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Location of") << rptNewLine << _T("Least Haunch") << rptNewLine << _T("Depth From") << rptNewLine << _T("Left CL Brg"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Least Haunch") << rptNewLine << _T("Depth"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Fillet"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << _T("Status");
   (*pTable)(0,col++) << _T("Notes");

   RowIndexType row=0;
   BOOST_FOREACH(const CGirderKey& girderKey, girderList)
   {
      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);
      const pgsConstructabilityArtifact* pConstrArtifact = pGdrArtifact->GetConstructabilityArtifact();

      SpanIndexType startSpanIdx, endSpanIdx;
      pConstrArtifact->GetSpans(&startSpanIdx,&endSpanIdx);

      for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++)
      {
         const pgsSpanConstructabilityArtifact* pArtifact = pConstrArtifact->GetSpanArtifact(spanIdx);

         if (pArtifact->SlabOffsetStatus() != pgsSpanConstructabilityArtifact::NA)
         {
            row++;
            col = 0;
            bool wasExcessive(false);

            if (needSpanCols)
            {
               GirderIndexType girder = girderKey.girderIndex;
               (*pTable)(row, col++) << LABEL_SPAN(spanIdx);
               (*pTable)(row, col++) << LABEL_GIRDER(girder);
            }

            Float64 endA, startA;
            pArtifact->GetProvidedSlabOffset(&startA, &endA);

            (*pTable)(row, col++) << dim.SetValue(startA);
            (*pTable)(row, col++) << dim.SetValue(endA);

            Float64 lloc, lval;
            pArtifact->GetLeastHaunchDepth(&lloc,&lval);

            Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));
            lloc -= end_size;
            (*pTable)(row, col++) << spandim.SetValue(lloc);
            (*pTable)(row, col++) << dim.SetValue(lval);

            (*pTable)(row, col++) << dim.SetValue(pArtifact->GetProvidedFillet());

            switch( pArtifact->SlabOffsetStatus() )
            {
               case pgsSpanConstructabilityArtifact::Pass:
                  (*pTable)(row, col++) << RPT_PASS;
                  break;

               case pgsSpanConstructabilityArtifact::Fail:
                  (*pTable)(row, col++) << RPT_FAIL;
                  break;

               case pgsSpanConstructabilityArtifact::Excessive:
                  (*pTable)(row, col++) << color(Blue) << _T("Excessive") << color(Black);
                  wasExcessive = true;
                  break;

               default:
                  ATLASSERT(false);
                  break;
            }

            bool didNote(false);
            // NOTE: Don't increment the column counter in the (*pTable)(row,col) below. All notes go into the same column. If we do (*pTable)(row,col++)
            // and there are multiple notes, the column index advances and we are then writing beyond the end of the table for each subsequent note.
            if ( pArtifact->CheckStirrupLength() )
            {
               didNote = true;
               CSpanKey spanKey(spanIdx, girderKey.girderIndex);
               HAUNCHDETAILS haunch_details;
               pGdrHaunch->GetHaunchDetails(spanKey,&haunch_details);
               (*pTable)(row, col) << color(Red) << _T("The difference betwen the minimum and maximum CL haunch depths along the girder is ") << dim2.SetValue(haunch_details.HaunchDiff) 
                                                 << _T(". This exceeds one half of the slab depth. Check stirrup lengths to ensure they engage the deck in all locations.");
                                                 
               if(pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP)
               {
                  (*pTable)(row, col)  << _T(" Also carefully check deck panel leveling.");
               }

               (*pTable)(row, col) << _T(" Refer to the Haunch Details chapter in the Details report for more information.") << color(Black) << rptNewLine;
            }

            if (wasExcessive)
            {
               didNote = true;
               (*pTable)(row, col) << _T("Provided least haunch depth exceeded required by allowable tolerance of ") << dim2.SetValue(pArtifact->GetExcessSlabOffsetWarningTolerance()) << rptNewLine;
            }

            if (!didNote)
            {
               (*pTable)(row, col) << _T(""); // otherwise table will be rendered funkily
            }

            col++; // done with notes column.... advance the index
         }
      } // next girder
   } // span

   // Only return a table if it has content
   if (0 < row)
   {
      rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
      *pChapter << pTitle;
      *pTitle << _T("Slab Offset (\"A\" Dimension)");
      rptParagraph* pBody = new rptParagraph;
      *pChapter << pBody;
      *pBody << _T("This table compares the least haunch depth with input Fillet dimension to determine if the Slab Offsets are adequate. The least haunch depth typically occurs at the tip of the top flange. A failed status indicates that the top of the girder will encroach into the deck slab, and the Slab Offset and/or Fillet dimension(s) should be increased. An excessive status indicates that the haunch is deeper than required for this girder.") << rptNewLine;
      *pBody << pTable << rptNewLine;
   }
   else
   {
      delete pTable;
   }
}

void CConstructabilityCheckTable::BuildMinimumHaunchCLCheck(rptChapter* pChapter,IBroker* pBroker, const std::vector<CGirderKey>& girderList, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2_NOCHECK(pBroker,IBridge,pBridge);

   // Create table - delete it later if we don't need it
   bool needSpanCols = true; // ConstrNeedSpanCols(girderList, pBridge);

   ColumnIndexType nCols = needSpanCols ? 5 : 3; // put span/girder in table if multi girder
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols,_T(""));

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim2, pDisplayUnits->GetComponentDimUnit(), true );

   pTable->SetColumnStyle(nCols-1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(nCols-1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   if (needSpanCols)
   {
      (*pTable)(0,col++) << _T("Span");
      (*pTable)(0,col++) << _T("Girder");
   }

   (*pTable)(0,col++) << COLHDR(_T("Provided") << rptNewLine << _T("Haunch Depth"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Required") << rptNewLine << _T("Haunch Depth"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << _T("Status");

   RowIndexType row=0;
   BOOST_FOREACH(const CGirderKey& girderKey, girderList)
   {
      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);
      const pgsConstructabilityArtifact* pConstrArtifact = pGdrArtifact->GetConstructabilityArtifact();

      SpanIndexType startSpanIdx, endSpanIdx;
      pConstrArtifact->GetSpans(&startSpanIdx,&endSpanIdx);

      for (SpanIndexType spanIdx=startSpanIdx; spanIdx<=endSpanIdx; spanIdx++)
      {
         const pgsSpanConstructabilityArtifact* pArtifact = pConstrArtifact->GetSpanArtifact(spanIdx);
      
         if (pArtifact->IsHaunchAtBearingCLsApplicable())
         {
            row++;
            col = 0;

            if (needSpanCols)
            {
               GirderIndexType girder = girderKey.girderIndex;
               (*pTable)(row, col++) << LABEL_SPAN(spanIdx);
               (*pTable)(row, col++) << LABEL_GIRDER(girder);
            }

            (*pTable)(row, col++) << dim.SetValue(pArtifact->GetProvidedHaunchAtBearingCLs());
            (*pTable)(row, col++) << dim.SetValue(pArtifact->GetRequiredHaunchAtBearingCLs());

            if( pArtifact->HaunchAtBearingCLsPassed() )
            {
               (*pTable)(row, col++) << RPT_PASS;
            }
            else
            {
               (*pTable)(row, col++) << RPT_FAIL;
            }
         }
      } // next girder
   } // span

   // Only add table if it has content
   if (0 < row)
   {
      rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
      *pChapter << pTitle;
      *pTitle << _T("Minimum Haunch Depth at Bearing Centerlines");
      rptParagraph* pBody = new rptParagraph;
      *pChapter << pBody;
      *pBody << pTable;
   }
   else
   {
      delete pTable;
   }
}

void CConstructabilityCheckTable::BuildMinimumFilletCheck(rptChapter* pChapter,IBroker* pBroker, const std::vector<CGirderKey>& girderList, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2_NOCHECK(pBroker,IArtifact,pIArtifact);
   GET_IFACE2_NOCHECK(pBroker,IBridge,pBridge);

   // This table is not applicable if girders are adjacently spaced.
   // There is no need to report something that cannot happen
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();
   if ( IsAdjacentSpacing(spacingType) )
   {
      return;
   }

   // if there is only one span/girder, don't need to print span info
   bool needSpanCols = true; // ConstrNeedSpanCols(girderList, pBridge);

   ColumnIndexType nCols = needSpanCols ? 5 : 3; // put span/girder in table if multi girder
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols,_T(""));

   rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Minimum Fillet Depth");
   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;
   *pBody << _T("This table compares the provided Fillet dimension to the minimum Fillet dimension specified in the girder library. A failed status indicates that the Fillet dimension is too small.") << rptNewLine;
   *pBody << pTable << rptNewLine;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim2, pDisplayUnits->GetComponentDimUnit(), true );

   pTable->SetColumnStyle(nCols-1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(nCols-1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   if (needSpanCols)
   {
      (*pTable)(0,col++) << _T("Span");
      (*pTable)(0,col++) << _T("Girder");
   }

   (*pTable)(0,col++) << COLHDR(_T("Provided"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Required"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << _T("Status");

   RowIndexType row = 0;
   BOOST_FOREACH(const CGirderKey& girderKey, girderList)
   {
      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);
      const pgsConstructabilityArtifact* pConstrArtifact = pGdrArtifact->GetConstructabilityArtifact();

      SpanIndexType startSpanIdx, endSpanIdx;
      pConstrArtifact->GetSpans(&startSpanIdx,&endSpanIdx);

      for (SpanIndexType spanIdx=startSpanIdx; spanIdx<=endSpanIdx; spanIdx++)
      {
         const pgsSpanConstructabilityArtifact* pArtifact = pConstrArtifact->GetSpanArtifact(spanIdx);

         row++;
         col = 0;

         if (needSpanCols)
         {
            GirderIndexType girder = girderKey.girderIndex;
            (*pTable)(row, col++) << LABEL_SPAN(spanIdx);
            (*pTable)(row, col++) << LABEL_GIRDER(girder);
         }

         (*pTable)(row, col++) << dim.SetValue(pArtifact->GetProvidedFillet());
         (*pTable)(row, col++) << dim.SetValue(pArtifact->GetRequiredMinimumFillet());

         if( pArtifact->MinimumFilletPassed() )
         {
            (*pTable)(row, col++) << RPT_PASS;
         }
         else
         {
            (*pTable)(row, col++) << RPT_FAIL;
         }
      }
   }
}

void CConstructabilityCheckTable::BuildHaunchGeometryComplianceCheck(rptChapter* pChapter,IBroker* pBroker, const std::vector<CGirderKey>& girderList, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2_NOCHECK(pBroker,IBridge,pBridge);

   // if there is only one span/girder, don't need to print span info
   bool needSpanCols = true; // ConstrNeedSpanCols(girderList, pBridge);

   ColumnIndexType nCols = needSpanCols ? 8 : 6; // put span/girder in table if multi girder
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols,_T(""));

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );

   pTable->SetColumnStyle(nCols-1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(nCols-1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   if (needSpanCols)
   {
      (*pTable)(0,col++) << _T("Span");
      (*pTable)(0,col++) << _T("Girder");
   }

   (*pTable)(0,col++) << COLHDR(_T("Computed")<<rptNewLine<<_T("Fillet") << Super(_T("*")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Fillet"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Difference"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Allowable")<<rptNewLine<<_T("Difference"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << _T("Status");
   (*pTable)(0,col++) << _T("Notes");

   RowIndexType row = 0;
   BOOST_FOREACH(const CGirderKey& girderKey, girderList)
   {
      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);
      const pgsConstructabilityArtifact* pConstrArtifact = pGdrArtifact->GetConstructabilityArtifact();

      SpanIndexType startSpanIdx, endSpanIdx;
      pConstrArtifact->GetSpans(&startSpanIdx,&endSpanIdx);

      for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++)
      {
         const pgsSpanConstructabilityArtifact* pArtifact = pConstrArtifact->GetSpanArtifact(spanIdx);

         if (pArtifact->IsHaunchGeometryCheckApplicable())
         {
            row++;
            col = 0;

            if (needSpanCols)
            {
               GirderIndexType girder = girderKey.girderIndex;
               (*pTable)(row, col++) << LABEL_SPAN(spanIdx);
               (*pTable)(row, col++) << LABEL_GIRDER(girder);
            }

            (*pTable)(row, col++) << dim.SetValue(pArtifact->GetComputedFillet());
            (*pTable)(row, col++) << dim.SetValue(pArtifact->GetProvidedFillet());
            (*pTable)(row, col++) << dim.SetValue(pArtifact->GetComputedFillet()-pArtifact->GetProvidedFillet());
            (*pTable)(row, col++) << dim.SetValue(pArtifact->GetHaunchGeometryTolerance());

            pgsSpanConstructabilityArtifact::HaunchGeometryStatusType status = pArtifact->HaunchGeometryStatus();
            if(pgsSpanConstructabilityArtifact::hgPass == status)
            {
               (*pTable)(row, col++) << RPT_PASS;
            }
            else
            {
               (*pTable)(row, col++) << RPT_FAIL;
            }

            if(pgsSpanConstructabilityArtifact::hgPass == status)
            {
               (*pTable)(row, col++) << _T("Haunch load is within tolerance");
            }
            else if(pgsSpanConstructabilityArtifact::hgInsufficient == status)
            {
               (*pTable)(row, col++) << _T("Haunch load is under-predicted");
            }
            else if(pgsSpanConstructabilityArtifact::hgExcessive == status)
            {
               (*pTable)(row, col++) << _T("Haunch load is over-predicted");
            }
         }
      }
   }

   // Only add table if it has content
   if (0 < row)
   {
      rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
      *pChapter << pTitle;
      *pTitle << _T("Haunch Geometry Compliance at Mid-Span");
      rptParagraph* pBody = new rptParagraph;
      *pChapter << pBody;
      *pBody << _T("This table compares the assumed geometry of the slab haunch, as defined by Fillet and Slab Offset, with the computed haunch geometry based on the calculated excess camber and roadway geometry. A failed status indicates that the haunch dead load is estimated inaccurately.");
      *pBody << pTable << rptNewLine;
      *pBody << Super(_T("*")) << _T("Computed Fillet is mid-span haunch depth at the tip of top flange accounting for the computed excess camber. See the Haunch Details chapter in Details Report for more information.");
   }
   else
   {
      delete pTable;
   }
}

void CConstructabilityCheckTable::BuildCamberCheck(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      BuildTimeStepCamberCheck(pChapter,pBroker,girderKey,pDisplayUnits);
   }
   else
   {
      BuildRegularCamberCheck(pChapter,pBroker,girderKey,pDisplayUnits);
   }
}

void CConstructabilityCheckTable::BuildGlobalGirderStabilityCheck(rptChapter* pChapter,IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   bool bIsApplicable = false;
   SegmentIndexType nSegments = pBridge->GetSegmentCount(pGirderArtifact->GetGirderKey());
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsSegmentStabilityArtifact* pArtifact = pSegmentArtifact->GetSegmentStabilityArtifact();
      
      if ( pArtifact->IsGlobalGirderStabilityApplicable() )
      {
         bIsApplicable = true;
         break;
      }
   }

   if ( !bIsApplicable )
   {
      return;
   }

   rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Global Stability of Girder");

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   *pBody << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("GlobalGirderStability.gif"));

   rptRcScalar slope;
   slope.SetFormat(pDisplayUnits->GetScalarFormat().Format);
   slope.SetWidth(pDisplayUnits->GetScalarFormat().Width);
   slope.SetPrecision(pDisplayUnits->GetScalarFormat().Precision);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsSegmentStabilityArtifact* pArtifact = pSegmentArtifact->GetSegmentStabilityArtifact();

      CString strTitle;
      if ( 1 < nSegments )
      {
         strTitle.Format(_T("Segment %d"), LABEL_SEGMENT(segIdx));
      }
      else
      {
         strTitle = _T("");
      }

      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(5,strTitle);
      std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

      (*pTable)(0,0) << COLHDR(Sub2(_T("W"),_T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*pTable)(0,1) << COLHDR(Sub2(_T("Y"),_T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*pTable)(0,2) << _T("Incline from Vertical (") << Sub2(symbol(theta),_T("max")) << _T(")") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
      (*pTable)(0,3) << _T("Max Incline") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
      (*pTable)(0,4) << _T("Status");

      Float64 Wb, Yb, Orientation;
      pArtifact->GetGlobalGirderStabilityParameters(&Wb,&Yb,&Orientation);
      Float64 maxIncline = pArtifact->GetMaxGirderIncline();

      (*pTable)(1,0) << dim.SetValue(Wb);
      (*pTable)(1,1) << dim.SetValue(Yb);
      (*pTable)(1,2) << slope.SetValue(Orientation);
      (*pTable)(1,3) << slope.SetValue(maxIncline);

      if ( pArtifact->Passed() )
      {
         (*pTable)(1,4) << RPT_PASS;
      }
      else
      {
         (*pTable)(1,4) << RPT_FAIL << rptNewLine << _T("Reaction falls outside of middle third of bottom width of girder");
      }
      
      *pBody << pTable;
   } // next segment
}

void CConstructabilityCheckTable::BuildBottomFlangeClearanceCheck(rptChapter* pChapter,IBroker* pBroker, const std::vector<CGirderKey>& girderList, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2_NOCHECK(pBroker,IBridge,pBridge);

   // if there is only one span/girder, don't need to print span info
   bool needSpanCols = true; // ConstrNeedSpanCols(girderList, pBridge);

   // Create table - delete it later if we don't need it
   ColumnIndexType nCols = needSpanCols ? 5 : 3; // put span/girder in table if multi girder
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols,_T(""));

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetSpanLengthUnit(), false );
   std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   pTable->SetColumnStyle(nCols-1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(nCols-1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   if (needSpanCols)
   {
      (*pTable)(0,col++) << _T("Span");
      (*pTable)(0,col++) << _T("Girder");
   }

   (*pTable)(0,col++) << COLHDR(_T("Clearance"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Min. Clearance"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*pTable)(0,col++) << _T("Status");

   RowIndexType row=0;
   BOOST_FOREACH(const CGirderKey& girderKey, girderList)
   {
      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);
      const pgsConstructabilityArtifact* pConstrArtifact = pGdrArtifact->GetConstructabilityArtifact();

      SpanIndexType startSpanIdx, endSpanIdx;
      pConstrArtifact->GetSpans(&startSpanIdx,&endSpanIdx);

      for (SpanIndexType spanIdx=startSpanIdx; spanIdx<=endSpanIdx; spanIdx++)
      {
         const pgsSpanConstructabilityArtifact* pArtifact = pConstrArtifact->GetSpanArtifact(spanIdx);

         if (pArtifact->IsBottomFlangeClearanceApplicable())
         {
            row++;
            col = 0;

            if (needSpanCols)
            {
               GirderIndexType girder = girderKey.girderIndex;
               (*pTable)(row, col++) << LABEL_SPAN(spanIdx);
               (*pTable)(row, col++) << LABEL_GIRDER(girder);
            }

            Float64 C, Cmin;
            pArtifact->GetBottomFlangeClearanceParameters(&C,&Cmin);

            (*pTable)(row, col++) << dim.SetValue(C);
            (*pTable)(row, col++) << dim.SetValue(Cmin);

            if ( pArtifact->BottomFlangeClearancePassed() )
            {
               (*pTable)(row, col++) << RPT_PASS;
            }
            else
            {
               (*pTable)(row, col++) << RPT_FAIL;
            }
         }
      }
   }

   // Only add table if it has content
   if (0 < row)
   {
      rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
      *pChapter << pTitle;
      *pTitle << _T("Bottom Flange Clearance");

      rptParagraph* pBody = new rptParagraph;
      *pChapter << pBody;

      *pBody << pTable;
   }
   else
   {
      delete pTable;
   }
}

void CConstructabilityCheckTable::BuildRegularCamberCheck(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,ICamber,pCamber);

   GET_IFACE2( pBroker, ILibrary, pLib );
   GET_IFACE2( pBroker, ISpecification, pSpec );
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
   Float64 min_days =  ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Min(), unitMeasure::Day);
   Float64 max_days =  ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Max(), unitMeasure::Day);

   rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Camber");

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(2,_T(""));

   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   *pBody << pTable << rptNewLine;

   (*pTable)(0,0) << _T("");
   (*pTable)(0,1) << COLHDR(_T("Camber"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest );

   GET_IFACE2(pBroker, IBridge, pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   SpanIndexType startSpanIdx, endSpanIdx;
   pBridge->GetGirderGroupSpans(girderKey.groupIndex,&startSpanIdx,&endSpanIdx);
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      CSpanKey spanKey(spanIdx,girderKey.girderIndex);
      std::vector<pgsPointOfInterest> vPoi = pPointOfInterest->GetPointsOfInterest(spanKey,POI_SPAN | POI_5L);
      ATLASSERT(vPoi.size()==1);
      pgsPointOfInterest poiMidSpan(vPoi.front());

      Float64 C = 0;
      if ( deckType != pgsTypes::sdtNone )
      {
         C = pCamber->GetScreedCamber( poiMidSpan ) ;
         (*pTable)(row,  0) << _T("Screed Camber, C");
         (*pTable)(row++,1) << dim.SetValue(C);
      }
   
      // get # of days for creep
      Float64 Dmax_UpperBound, Dmax_Average, Dmax_LowerBound;
      Float64 Dmin_UpperBound, Dmin_Average, Dmin_LowerBound;
      Float64 Cfactor = pCamber->GetLowerBoundCamberVariabilityFactor();
      Dmin_UpperBound = pCamber->GetDCamberForGirderSchedule( poiMidSpan, CREEP_MINTIME);
      Dmax_UpperBound = pCamber->GetDCamberForGirderSchedule( poiMidSpan, CREEP_MAXTIME);
      
      Dmin_LowerBound = Cfactor*Dmin_UpperBound;
      Dmin_Average    = (1+Cfactor)/2*Dmin_UpperBound;
      
      Dmax_LowerBound = Cfactor*Dmax_UpperBound;
      Dmax_Average    = (1+Cfactor)/2*Dmax_UpperBound;
   
   
      if ( IsEqual(min_days,max_days) )
      {
         // Min/Max timing cambers will be the same, only report them once
         ATLASSERT(IsEqual(Dmin_UpperBound,Dmax_UpperBound));
         ATLASSERT(IsEqual(Dmin_Average,   Dmax_Average));
         ATLASSERT(IsEqual(Dmin_LowerBound,Dmax_LowerBound));
         if ( IsZero(1-Cfactor) )
         {
            // Upper,average, and lower bound cambers will all be the same... report them once
            ATLASSERT(IsEqual(Dmin_UpperBound,Dmin_LowerBound));
            ATLASSERT(IsEqual(Dmin_UpperBound,Dmin_Average));
   
            (*pTable)(row,0) << _T("Camber at ") << min_days << _T(" days, D") << Sub(min_days);
            if ( Dmin_UpperBound < 0 )
            {
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmin_UpperBound) << color(Black);
            }
            else
            {
               (*pTable)(row++,1) << dim.SetValue(Dmin_UpperBound);
            }
         }
         else
         {
            (*pTable)(row,0) << _T("Lower Bound Camber at ") << min_days << _T(" days, ")<<Cfactor*100<<_T("% of D") <<Sub(min_days);
            if ( Dmin_LowerBound < 0 )
            {
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmin_LowerBound) << color(Black);
            }
            else
            {
               (*pTable)(row++,1) << dim.SetValue(Dmin_LowerBound);
            }
   
            (*pTable)(row,0) << _T("Average Camber at ") << min_days << _T(" days, ")<<(1+Cfactor)/2*100<<_T("% of D") <<Sub(min_days);
            if ( Dmin_Average < 0 )
            {
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmin_Average) << color(Black);
            }
            else
            {
               (*pTable)(row++,1) << dim.SetValue(Dmin_Average);
            }
   
            (*pTable)(row,0) << _T("Upper Bound Camber at ") << min_days << _T(" days, D") << Sub(min_days);
            if ( Dmin_UpperBound < 0 )
            {
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmin_UpperBound) << color(Black);
            }
            else
            {
               (*pTable)(row++,1) << dim.SetValue(Dmin_UpperBound);
            }
         }
      }
      else
      {
         if ( IsZero(1-Cfactor) )
         {
            (*pTable)(row,0) << _T("Camber at ") << min_days << _T(" days, D") << Sub(min_days);
            if ( Dmin_UpperBound < 0 )
            {
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmin_UpperBound) << color(Black);
            }
            else
            {
               (*pTable)(row++,1) << dim.SetValue(Dmin_UpperBound);
            }
   
            (*pTable)(row,0) << _T("Camber at ") << max_days << _T(" days, D") << Sub(max_days);
            if ( Dmax_UpperBound < 0 )
            {
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmax_UpperBound) << color(Black);
            }
            else
            {
               (*pTable)(row++,1) << dim.SetValue(Dmax_UpperBound);
            }
         }
         else
         {
            (*pTable)(row,0) << _T("Lower bound camber at ")<< min_days<<_T(" days, ")<<Cfactor*100<<_T("% of D") <<Sub(min_days);
            if ( Dmin_LowerBound < 0 )
            {
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmin_LowerBound) << color(Black);
            }
            else
            {
               (*pTable)(row++,1) << dim.SetValue(Dmin_LowerBound);
            }
   
            (*pTable)(row,0) << _T("Average camber at ")<< min_days<<_T(" days, ")<<(1+Cfactor)/2*100<<_T("% of D") <<Sub(min_days);
            if ( Dmin_Average < 0 )
            {
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmin_Average) << color(Black);
            }
            else
            {
               (*pTable)(row++,1) << dim.SetValue(Dmin_Average);
            }
   
            (*pTable)(row,0) << _T("Upper bound camber at ")<< min_days<<_T(" days, D") << Sub(min_days);
            if ( Dmin_UpperBound < 0 )
            {
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmin_UpperBound) << color(Black);
            }
            else
            {
               (*pTable)(row++,1) << dim.SetValue(Dmin_UpperBound);
            }
   
            (*pTable)(row,0) << _T("Lower bound camber at ")<< max_days<<_T(" days, ")<<Cfactor*100<<_T("% of D") <<Sub(max_days);
            if ( Dmax_LowerBound < 0 )
            {
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmax_LowerBound) << color(Black);
            }
            else
            {
               (*pTable)(row++,1) << dim.SetValue(Dmax_LowerBound);
            }
   
            (*pTable)(row,0) << _T("Average camber at ")<< max_days<<_T(" days, ")<<(1+Cfactor)/2*100<<_T("% of D") <<Sub(max_days);
            if ( Dmax_Average < 0 )
            {
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmax_Average) << color(Black);
            }
            else
            {
               (*pTable)(row++,1) << dim.SetValue(Dmax_Average);
            }
   
            (*pTable)(row,0) << _T("Upper bound camber at ")<< max_days<<_T(" days, D") << Sub(max_days);
            if ( Dmax_UpperBound < 0 )
            {
               (*pTable)(row++,1) << color(Red) << dim.SetValue(Dmax_UpperBound) << color(Black);
            }
            else
            {
               (*pTable)(row++,1) << dim.SetValue(Dmax_UpperBound);
            }
         }
      }
   
      if ( pSpecEntry->CheckGirderSag() && deckType != pgsTypes::sdtNone )
      {
         std::_tstring camberType;
         Float64 D = 0;
   
         switch(pSpecEntry->GetSagCamberType())
         {
         case pgsTypes::LowerBoundCamber:
            D = Dmin_LowerBound;
            camberType = _T("lower bound");
            break;
         case pgsTypes::AverageCamber:
            D = Dmin_Average;
            camberType = _T("average");
            break;
         case pgsTypes::UpperBoundCamber:
            D = Dmin_UpperBound;
            camberType = _T("upper bound");
            break;
         }
   
         if ( D < C )
         {
            rptParagraph* p = new rptParagraph;
            *pChapter << p;
   
            *p << color(Red) << _T("WARNING: Screed Camber, C, is greater than the ") << camberType.c_str() << _T(" camber at time of deck casting, D. The girder may end up with a sag.") << color(Black) << rptNewLine;
         }
         else if ( IsEqual(C,D,::ConvertToSysUnits(0.25,unitMeasure::Inch)) )
         {
            rptParagraph* p = new rptParagraph;
            *pChapter << p;
   
            *p << color(Red) << _T("WARNING: Screed Camber, C, is nearly equal to the ") << camberType.c_str() << _T(" camber at time of deck casting, D. The girder may end up with a sag.") << color(Black) << rptNewLine;
         }
   
         if ( Dmin_LowerBound < C && pSpecEntry->GetSagCamberType() != pgsTypes::LowerBoundCamber )
         {
            rptParagraph* p = new rptParagraph;
            *pChapter << p;
   
            *p << _T("Screed Camber (C) is greater than the lower bound camber at time of deck casting (") << Cfactor*100 << _T("% of D") << Sub(min_days) << _T("). The girder may end up with a sag if the deck is placed at day ") << min_days << _T(" and the actual camber is a lower bound value.") << rptNewLine;
         }
      }
   }
}

void CConstructabilityCheckTable::BuildTimeStepCamberCheck(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IProductForces, pProductForces);
   GET_IFACE2(pBroker,ILimitStateForces,pLSForces);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2( pBroker, ILibrary, pLib );
   GET_IFACE2( pBroker, ISpecification, pSpec );

   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   Float64 deckCastingTime = pIntervals->GetTime(castDeckIntervalIdx,pgsTypes::Start);

   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);


   rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Camber");

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(3,_T(""));

   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   pTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   *pBody << pTable << rptNewLine;

   (*pTable)(0,0) << _T("Span");
   (*pTable)(0,1) << _T("Description");
   (*pTable)(0,2) << COLHDR(_T("Camber"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest );

   GET_IFACE2(pBroker, IBridge, pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   SpanIndexType startSpanIdx, endSpanIdx;
   pBridge->GetGirderGroupSpans(girderKey.groupIndex,&startSpanIdx,&endSpanIdx);
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      CSpanKey spanKey(spanIdx,girderKey.girderIndex);
      std::vector<pgsPointOfInterest> vPoi = pPointOfInterest->GetPointsOfInterest(spanKey,POI_SPAN | POI_5L);
      ATLASSERT(vPoi.size()==1);
      pgsPointOfInterest poiMidSpan(vPoi.front());

      pTable->SetRowSpan(row,0,2);
      pTable->SetRowSpan(row+1,0,SKIP_CELL);
      (*pTable)(row,0) << LABEL_SPAN(spanIdx);

      Float64 C = 0;
      if ( deckType != pgsTypes::sdtNone )
      {
         C = pCamber->GetScreedCamber( poiMidSpan ) ;
         (*pTable)(row,  1) << _T("Screed Camber, C");
         (*pTable)(row++,2) << dim.SetValue(C);
      }

      Float64 Dmin,Dmax;
      pLSForces->GetDeflection(castDeckIntervalIdx-1,pgsTypes::ServiceI,poiMidSpan,bat,true/*include prestress*/,false/*no liveload*/,true /*include elevation adjustment*/,&Dmin,&Dmax);
      ATLASSERT(IsEqual(Dmin,Dmax));
      (*pTable)(row,1) << _T("Camber at time of deck casting, at ") << deckCastingTime << _T(" days, D");
      if ( Dmin < 0 )
      {
         (*pTable)(row++,2) << color(Red) << dim.SetValue(Dmin) << color(Black);
      }
      else
      {
         (*pTable)(row++,2) << dim.SetValue(Dmin);
      }

      if ( pSpecEntry->CheckGirderSag() && deckType != pgsTypes::sdtNone )
      {
         std::_tstring camberType;
   
         if ( Dmin < C )
         {
            rptParagraph* p = new rptParagraph;
            *pChapter << p;
   
            *p << color(Red) << _T("WARNING: Screed Camber, C, is greater than the camber at time of deck casting, D. The girder may end up with a sag.") << color(Black) << rptNewLine;
         }
         else if ( IsEqual(C,Dmin,::ConvertToSysUnits(0.25,unitMeasure::Inch)) )
         {
            rptParagraph* p = new rptParagraph;
            *pChapter << p;
   
            *p << color(Red) << _T("WARNING: Screed Camber, C, is nearly equal to the camber at time of deck casting, D. The girder may end up with a sag.") << color(Black) << rptNewLine;
         }
      }
   }  // next span
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CConstructabilityCheckTable::MakeCopy(const CConstructabilityCheckTable& rOther)
{
   // Add copy code here...
}

void CConstructabilityCheckTable::MakeAssignment(const CConstructabilityCheckTable& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool CConstructabilityCheckTable::AssertValid() const
{
   return true;
}

void CConstructabilityCheckTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CConstructabilityCheckTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CConstructabilityCheckTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CConstructabilityCheckTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CConstructabilityCheckTable");

   TESTME_EPILOG("CConstructabilityCheckTable");
}
#endif // _UNITTEST
