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
#include <Reporting\ConstructabilityCheckTable.h>

#include <IFace\Artifact.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Constructability.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\HoldDownForceArtifact.h>
#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// return true if more than a single span report so we can add columns for span/girder
bool ConstrNeedSpanCols(const std::vector<CGirderKey>& girderList, IBridge* pBridge)
{
   bool doNeed = 1 < girderList.size();
   if (!doNeed)
   {
      for (const auto& girderKey : girderList)
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
   GET_IFACE2(pBroker,IBridge,pBridge);

   if (pBridge->GetDeckType() == pgsTypes::sdtNone)
   {
      return;
   }

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
   bool areAsDifferent(false);
   for (const auto& girderKey : girderList)
   {
      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);
      const pgsConstructabilityArtifact* pConstrArtifact = pGdrArtifact->GetConstructabilityArtifact();

      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         const auto& artifact = pConstrArtifact->GetSegmentArtifact(segIdx);

         if (!artifact.AreSlabOffsetsSameAtEnds())
         {
            areAsDifferent = true;
         }

         if (areAsDifferent)
         {
            break; // no need to go further
         }
      }

      if (bMultiGirderReport)
      {
         pProgress->Increment();
      }
   }

   if (areAsDifferent)
   {
      BuildMultiSlabOffsetTable(pChapter, pBroker, girderList, pDisplayUnits);
   }
   else
   {
      BuildMonoSlabOffsetTable(pChapter, pBroker, girderList, pDisplayUnits);
   }
}

void CConstructabilityCheckTable::BuildMonoSlabOffsetTable(rptChapter* pChapter, IBroker* pBroker, const std::vector<CGirderKey>& girderList, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker, IArtifact, pIArtifact);
   GET_IFACE2_NOCHECK(pBroker, IGirderHaunch, pGdrHaunch);
   GET_IFACE2_NOCHECK(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IDocumentType, pDocType);

   // if there is only one span/girder, don't need to print span info
   bool needSpanCols = true; //ConstrNeedSpanCols(girderList, pBridge);

   // Create table - delete it later if we don't need it
   ColumnIndexType nCols = needSpanCols ? 6 : 4; // put span/girder in table if multi girder
   if (pDocType->IsPGSpliceDocument())
   {
      nCols++; // add one column for segment
   }
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols, _T(""));

   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim2, pDisplayUnits->GetComponentDimUnit(), true);

   pTable->SetColumnStyle(nCols - 1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(nCols - 1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   if (needSpanCols)
   {
      if (pDocType->IsPGSpliceDocument())
      {
         (*pTable)(0, col++) << _T("Group");
      }
      else
      {
         (*pTable)(0, col++) << _T("Span");
      }
      (*pTable)(0, col++) << _T("Girder");
   }

   if (pDocType->IsPGSpliceDocument())
   {
      (*pTable)(0, col++) << _T("Segment");
   }
      
   (*pTable)(0,col++) << COLHDR(_T("Provided"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Required"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << _T("Status");
   (*pTable)(0,col++) << _T("Notes");

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   for (const auto& girderKey : girderList)
   {
      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);
      const pgsConstructabilityArtifact* pConstrArtifact = pGdrArtifact->GetConstructabilityArtifact();

      Float64 Aprovided;
      Float64 Areqd = 0;
      Float64 MaxHaunchDiff = 0;
      bool bCheckStirrupLengths = false;
      pgsSegmentConstructabilityArtifact::SlabOffsetStatusType slabOffsetStatus;

      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         CSegmentKey segmentKey(girderKey, segIdx);
         const auto& artifact = pConstrArtifact->GetSegmentArtifact(segIdx);
         const auto& haunch_details = pGdrHaunch->GetSlabOffsetDetails(segmentKey);

         Float64 endA, startA;
         artifact.GetProvidedSlabOffset(&startA, &endA); // both values are same because of what function we are in
         ATLASSERT(IsEqual(endA, startA));
         Aprovided = startA;
         Float64 Arequired = artifact.GetRequiredSlabOffset();
         if (Areqd < Arequired)
         {
            Areqd = Arequired;
            slabOffsetStatus = artifact.SlabOffsetStatus();
         }

         MaxHaunchDiff = max(MaxHaunchDiff, haunch_details.HaunchDiff);

         if (artifact.CheckStirrupLength())
         {
            bCheckStirrupLengths = true;
         }

         col = 0;

         if (needSpanCols)
         {
            (*pTable)(row, col++) << LABEL_SPAN(segmentKey.groupIndex);
            (*pTable)(row, col++) << LABEL_GIRDER(segmentKey.girderIndex);
         }

         if (pDocType->IsPGSpliceDocument())
         {
            (*pTable)(row, col++) << LABEL_SEGMENT(segmentKey.segmentIndex);
         }

         (*pTable)(row, col++) << dim.SetValue(Aprovided);
         (*pTable)(row, col++) << dim.SetValue(Areqd);

         switch(slabOffsetStatus)
         {
         case pgsSegmentConstructabilityArtifact::Pass:
            (*pTable)(row, col++) << RPT_PASS;
            break;

         case pgsSegmentConstructabilityArtifact::Fail:
            (*pTable)(row, col++) << RPT_FAIL;
            break;

         case pgsSegmentConstructabilityArtifact::Excessive:
            (*pTable)(row, col++) << color(Blue) << _T("Excessive") << color(Black);
            break;

         case pgsSegmentConstructabilityArtifact::NA:
            (*pTable)(row, col++) << RPT_NA;
            break;

         default:
            ATLASSERT(false);
            break;
         }

         bool didNote(false);
         // NOTE: Don't increment the column counter in the (*pTable)(row,col) below. All notes go into the same column. If we do (*pTable)(row,col++)
         // and there are multiple notes, the column index advances and we are then writing beyond the end of the table for each subsequent note.
         if ( bCheckStirrupLengths )
         {
            didNote = true;
            (*pTable)(row, col) << color(Red) << _T("The difference betwen the minimum and maximum CL haunch depths along the girder is ") << dim2.SetValue(MaxHaunchDiff) 
                                                << _T(". This exceeds one half of the slab depth. Check stirrup lengths to ensure they engage the deck in all locations.");
                                                 
            if(pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP)
            {
               (*pTable)(row, col) << _T(" Also carefully check deck panel leveling.");
            }

            (*pTable)(row, col) << _T(" Refer to the Haunch Details chapter in the Details report for more information.") << color(Black) << rptNewLine;
         }

         if (slabOffsetStatus == pgsSegmentConstructabilityArtifact::Excessive)
         {
            didNote = true;

            GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
            const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
            const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
            const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
            const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();

            (*pTable)(row, col) << _T("Provided Slab Offset exceeded Required by allowable tolerance of ") << dim2.SetValue(pGirderEntry->GetExcessiveSlabOffsetWarningTolerance()) << rptNewLine;
         }

         if (!didNote)
         {
            (*pTable)(row, col) << _T(""); // otherwise table will be rendered funkily
         }

         row++;
      } // next segment
   } // next girder

   // Only return a table if it has content
   if (0 < row)
   {
      rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
      *pChapter << pTitle;
      *pTitle << _T("Slab Offset (\"A\" Dimension)");
      rptParagraph* pBody = new rptParagraph;
      *pChapter << pBody;
      *pBody << _T("This table compares the input slab offset to the rounded computed slab offset required to have the least haunch depth be equal to the Fillet dimension. A failed status indicates that the top of the girder will encroach into the deck slab and the Slab Offset dimension should be refined.") << rptNewLine;
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
   GET_IFACE2(pBroker, IDocumentType, pDocType);

   // if there is only one span/girder, don't need to print span info
   bool needSpanCols = true; // ConstrNeedSpanCols(girderList, pBridge);

   // Create table - delete it later if we don't need it
   ColumnIndexType nCols = needSpanCols ? 9 : 7; // put span/girder in table if multi girder

   if (pDocType->IsPGSpliceDocument())
   {
      nCols++; // add one column for segments
   }
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols,_T(""));

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, spandim, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim2, pDisplayUnits->GetComponentDimUnit(), true );

   pTable->SetColumnStyle(nCols-1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(nCols-1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   if (needSpanCols)
   {
      if (pDocType->IsPGSpliceDocument())
      {
         (*pTable)(0, col++) << _T("Group");
      }
      else
      {
         (*pTable)(0, col++) << _T("Span");
      }
      (*pTable)(0,col++) << _T("Girder");
   }

   if (pDocType->IsPGSpliceDocument())
   {
      (*pTable)(0, col++) << _T("Segment");
   }

   (*pTable)(0,col++) << COLHDR(_T("Slab Offset") << rptNewLine << _T("at Left End") << rptNewLine << _T("CL Brg"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Slab Offset") << rptNewLine << _T("at Right End") << rptNewLine << _T("CL Brg"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Location of") << rptNewLine << _T("Least Haunch") << rptNewLine << _T("Depth From") << rptNewLine << _T("Left CL Brg"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Least Haunch") << rptNewLine << _T("Depth"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Fillet"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << _T("Status");
   (*pTable)(0,col++) << _T("Notes");

   RowIndexType row = 0;
   for (const auto& girderKey : girderList)
   {
      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);
      const pgsConstructabilityArtifact* pConstrArtifact = pGdrArtifact->GetConstructabilityArtifact();

      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         const auto& artifact = pConstrArtifact->GetSegmentArtifact(segIdx);

         CSegmentKey segmentKey(girderKey, segIdx);

         row++;
         col = 0;
         bool wasExcessive(false);

         if (needSpanCols)
         {
            (*pTable)(row, col++) << LABEL_SPAN(segmentKey.groupIndex);
            (*pTable)(row, col++) << LABEL_GIRDER(segmentKey.girderIndex);
         }

         if (pDocType->IsPGSpliceDocument())
         {
            (*pTable)(row, col++) << LABEL_SEGMENT(segmentKey.segmentIndex);
         }

         Float64 endA, startA;
         artifact.GetProvidedSlabOffset(&startA, &endA);

         (*pTable)(row, col++) << dim.SetValue(startA);
         (*pTable)(row, col++) << dim.SetValue(endA);

         Float64 lloc, lval;
         artifact.GetLeastHaunchDepth(&lloc,&lval);

         Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);
         lloc -= end_size;
         (*pTable)(row, col++) << spandim.SetValue(lloc);
         (*pTable)(row, col++) << dim.SetValue(lval);

         (*pTable)(row, col++) << dim.SetValue(artifact.GetProvidedFillet());

         switch (artifact.SlabOffsetStatus())
         {
            case pgsSegmentConstructabilityArtifact::Pass:
               (*pTable)(row, col++) << RPT_PASS;
               break;

            case pgsSegmentConstructabilityArtifact::Fail:
               (*pTable)(row, col++) << RPT_FAIL;
               break;

            case pgsSegmentConstructabilityArtifact::Excessive:
               (*pTable)(row, col++) << color(Blue) << _T("Excessive") << color(Black);
               wasExcessive = true;
               break;

         case pgsSegmentConstructabilityArtifact::NA:
            (*pTable)(row, col++) << RPT_NA;
            break;

               default:
                  ATLASSERT(false);
                  break;
            }

            bool didNote(false);
            // NOTE: Don't increment the column counter in the (*pTable)(row,col) below. All notes go into the same column. If we do (*pTable)(row,col++)
            // and there are multiple notes, the column index advances and we are then writing beyond the end of the table for each subsequent note.
            if (artifact.CheckStirrupLength() )
            {
               didNote = true;
               const auto& haunch_details = pGdrHaunch->GetSlabOffsetDetails(segmentKey);
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
               (*pTable)(row, col) << _T("Provided least haunch depth exceeded required by allowable tolerance of ") << dim2.SetValue(artifact.GetExcessSlabOffsetWarningTolerance()) << rptNewLine;
            }

            if (!didNote)
            {
               (*pTable)(row, col) << _T(""); // otherwise table will be rendered funkily
            }

            col++; // done with notes column.... advance the index
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
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   bool isPGSuper = pDocType->IsPGSuperDocument();

   // Create table - delete it later if we don't need it
   bool needSpanCols = true; // ConstrNeedSpanCols(girderList, pBridge);

   ColumnIndexType nCols = needSpanCols ? 6 : 4; // put span/girder in table if multi girder
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols,_T(""));

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim2, pDisplayUnits->GetComponentDimUnit(), true );

   pTable->SetColumnStyle(nCols-1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(nCols-1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   if (needSpanCols)
   {
      (*pTable)(0,col++) << (isPGSuper ? _T("Span") : _T("Group"));
      (*pTable)(0,col++) << _T("Girder");
   }

   (*pTable)(0,col++) << COLHDR(_T("Provided") << rptNewLine << _T("Haunch Depth") << rptNewLine << _T("CL Start Brg"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0,col++) << COLHDR(_T("Provided") << rptNewLine << _T("Haunch Depth") << rptNewLine << _T("CL End Brg"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Required") << rptNewLine << _T("Haunch Depth"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << _T("Status");

   RowIndexType row = 0;
   for (const auto& girderKey : girderList)
   {
      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);
      const pgsConstructabilityArtifact* pConstrArtifact = pGdrArtifact->GetConstructabilityArtifact();
      
      pgsConstructabilityArtifact::HaunchBearingCLApplicabilityType appt = pConstrArtifact->GetHaunchBearingCLApplicability();
      if (pgsConstructabilityArtifact::hbcAppNA != appt)
      {
         row++;
         col = 0;

         if (needSpanCols)
         {
            GroupIndexType group = girderKey.groupIndex;
            GirderIndexType girder = girderKey.girderIndex;
            if (isPGSuper)
            {
               (*pTable)(row,col++) << LABEL_SPAN(group);
            }
            else
            {
               (*pTable)(row,col++) << LABEL_GROUP(group);
            }
            (*pTable)(row,col++) << LABEL_GIRDER(girder);
         }

         Float64 startH,endH;
         pConstrArtifact->GetProvidedHaunchAtBearingCLs(&startH,&endH);
         (*pTable)(row,col++) << dim.SetValue(startH);
         (*pTable)(row,col++) << dim.SetValue(endH);
         (*pTable)(row,col++) << dim.SetValue(pConstrArtifact->GetRequiredHaunchAtBearingCLs());

         if (pgsConstructabilityArtifact::hbcAppNAPrintOnly == appt)
         {
            (*pTable)(row,col++) << RPT_NA;
         }
         else if (pConstrArtifact->HaunchAtBearingCLsPassed())
         {
            (*pTable)(row,col++) << RPT_PASS;
         }
         else
         {
            (*pTable)(row,col++) << RPT_FAIL;
         }
      }
   } // next girder

   // Only add table if it has content
   if (0 < row)
   {
      rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
      *pChapter << pTitle;
      if (isPGSuper)
      {
         *pTitle << _T("Minimum Haunch Depth at Bearing Centerlines");
      }
      else
      {
         *pTitle << _T("Minimum Haunch Depth at Bearing Centerlines at Terminal Ends of Group");
      }
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

   GET_IFACE2(pBroker,IDocumentType, pDocType);
   
   // if there is only one span/girder, don't need to print span info
   bool needSpanCols = true; // ConstrNeedSpanCols(girderList, pBridge);

   ColumnIndexType nCols = needSpanCols ? 5 : 3; // put span/girder in table if multi girder
   if (pDocType->IsPGSpliceDocument())
   {
      nCols++;
   }
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols,_T(""));

   rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Minimum Provided Fillet Depth");
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
      if (pDocType->IsPGSpliceDocument())
      {
         (*pTable)(0, col++) << _T("Group");
      }
      else
      {
         (*pTable)(0, col++) << _T("Span");
      }
      (*pTable)(0,col++) << _T("Girder");
   }

   if (pDocType->IsPGSpliceDocument())
   {
      (*pTable)(0, col++) << _T("Segment");
   }

   (*pTable)(0,col++) << COLHDR(_T("Provided"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Required"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << _T("Status");

   RowIndexType row = 0;
   for (const auto& girderKey : girderList)
   {
      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);
      const pgsConstructabilityArtifact* pConstrArtifact = pGdrArtifact->GetConstructabilityArtifact();

      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         const auto& artifact = pConstrArtifact->GetSegmentArtifact(segIdx);

         row++;
         col = 0;

         if (needSpanCols)
         {
            GroupIndexType group = girderKey.groupIndex;
            GirderIndexType girder = girderKey.girderIndex;
            (*pTable)(row, col++) << LABEL_SPAN(group);
            (*pTable)(row, col++) << LABEL_GIRDER(girder);
         }

         if (pDocType->IsPGSpliceDocument())
         {
            (*pTable)(row, col++) << LABEL_SEGMENT(segIdx);
         }

         (*pTable)(row, col++) << dim.SetValue(artifact.GetProvidedFillet());
         (*pTable)(row, col++) << dim.SetValue(artifact.GetRequiredMinimumFillet());

         if (!artifact.IsSlabOffsetApplicable() && !artifact.GetFinishedElevationApplicability())
         {
            (*pTable)(row, col++) << RPT_NA;
         }
         else if(artifact.MinimumFilletPassed() )
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

   (*pTable)(0,col++) << COLHDR(_T("Computed")<<rptNewLine<<_T("Excess")<<rptNewLine<<_T("Camber"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Assumed") <<rptNewLine<<_T("Excess")<<rptNewLine<<_T("Camber"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Difference"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Allowable")<<rptNewLine<<_T("Difference"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << _T("Status");
   (*pTable)(0,col++) << _T("Notes");

   RowIndexType row = 0;
   for (const auto& girderKey : girderList)
   {
      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);
      const pgsConstructabilityArtifact* pConstrArtifact = pGdrArtifact->GetConstructabilityArtifact();

      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         const auto& artifact = pConstrArtifact->GetSegmentArtifact(segIdx);

         if (artifact.IsHaunchGeometryCheckApplicable())
         {
            row++;
            col = 0;

            if (needSpanCols)
            {
               GroupIndexType group = girderKey.groupIndex;
               GirderIndexType girder = girderKey.girderIndex;
               (*pTable)(row, col++) << LABEL_SPAN(group);
               (*pTable)(row, col++) << LABEL_GIRDER(girder);
            }

            Float64 asscamber = artifact.GetAssumedExcessCamber();
            Float64 cmpcamber = artifact.GetComputedExcessCamber();
            Float64 cambertol = artifact.GetHaunchGeometryTolerance();
            Float64 difference = cmpcamber - asscamber;

            //ATLASSERT(IsEqual(pArtifact->GetAssumedMinimumHaunchDepth(), difference)); // why does this assert?

            (*pTable)(row, col++) << dim.SetValue(cmpcamber);
            (*pTable)(row, col++) << dim.SetValue(asscamber);
            (*pTable)(row, col++) << dim.SetValue(difference);
            (*pTable)(row, col++) << symbol(PLUS_MINUS) << dim.SetValue(cambertol);

            pgsSegmentConstructabilityArtifact::HaunchGeometryStatusType status = artifact.HaunchGeometryStatus();
            if (pgsSegmentConstructabilityArtifact::hgNAPrintOnly == status)
            {
               (*pTable)(row, col++) << RPT_NA;
            }
            else if(pgsSegmentConstructabilityArtifact::hgPass == status)
            {
               (*pTable)(row, col++) << RPT_PASS;
            }
            else
            {
               (*pTable)(row, col++) << RPT_FAIL;
            }

            if(pgsSegmentConstructabilityArtifact::hgPass == status)
            {
               (*pTable)(row, col) << _T("Assumed Excess Camber is within tolerance");
            }
            else if(pgsSegmentConstructabilityArtifact::hgInsufficient == status)
            {
               (*pTable)(row, col) << _T("Assumed Excess Camber is under-predicted");
            }
            else if(pgsSegmentConstructabilityArtifact::hgExcessive == status)
            {
               (*pTable)(row, col) << _T("Assumed Excess Camber is over-predicted");
            }

            if (pgsSegmentConstructabilityArtifact::hgNAPrintOnly == status)
            {
               if (!IsZero(asscamber - cmpcamber, cambertol))
               {
                  (*pTable)(row, col) << rptNewLine <<color(Red) << _T("WARNING: Warning: Assumed Excess Camber is out of tolerance. Effects due the haunch are inaccurate.") << color(Black);
               }
            }

            if (artifact.GetAssumedMinimumHaunchDepth() < -cambertol /*0.0*/)
            {
               (*pTable)(row, col) << rptNewLine <<color(Red) << _T("WARNING: Assumed excess camber causes a negative haunch depth at mid span meaning that the girder intrudes into the bottom of the slab. Negative haunch depths will be taken as zero for purposes of computing haunch dead load and composite properties and capacities.") << color(Black);
            }
         }
      }
   }

   // Only add table if it has content
   if (0 < row)
   {
      GET_IFACE2(pBroker,ISpecification,pSpec);

      rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
      *pChapter << pTitle;
      *pTitle << _T("Excess Camber Check");
      rptParagraph* pBody = new rptParagraph;
      *pChapter << pBody;
      if (pSpec->IsAssumedExcessCamberForLoad())
      {
         *pBody << _T("Haunch dead load is affected by variable haunch depth along the girder. ");
      }

      if (pSpec->IsAssumedExcessCamberForSectProps())
      {
         *pBody << _T("Composite section properties are affected by haunch depth variation along the girder. ");
      }

      *pBody << _T("Haunch depth along a girder is defined by the roadway geometry, slab offset (\"A\"), and the parabolic girder camber defined by the user input Assumed Excess Camber at mid-span.");
      *pBody << _T(" The table below compares the Assumed Excess Camber with the Computed Excess Camber. A failed status indicates the assumed value is not within tolerance of the computed value - meaning that results dependent on haunch dead load may be inaccurate. See the Haunch Details and Loading Details chapters in Details Report for more information.");
      *pBody << pTable << rptNewLine;
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
   Float64 FSmax;
   SegmentIndexType nSegments = pBridge->GetSegmentCount(pGirderArtifact->GetGirderKey());
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsSegmentStabilityArtifact* pArtifact = pSegmentArtifact->GetSegmentStabilityArtifact();

      FSmax = pArtifact->GetTargetFactorOfSafety();
      
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
   *pTitle << _T("Girder Inclination Check");

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   *pBody << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("GlobalGirderStability.png")) << rptNewLine;

   rptRcScalar slope;
   slope.SetFormat(pDisplayUnits->GetScalarFormat().Format);
   slope.SetWidth(pDisplayUnits->GetScalarFormat().Width);
   slope.SetPrecision(pDisplayUnits->GetScalarFormat().Precision);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsSegmentStabilityArtifact* pArtifact = pSegmentArtifact->GetSegmentStabilityArtifact();

      ATLASSERT(IsEqual(FSmax, pArtifact->GetTargetFactorOfSafety()));

      CString strTitle;
      if ( 1 < nSegments )
      {
         strTitle.Format(_T("Segment %d"), LABEL_SEGMENT(segIdx));
      }
      else
      {
         strTitle = _T("");
      }

      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(7,strTitle);
      std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

      ColumnIndexType col = 0;
      (*pTable)(0, col++) << COLHDR(_T("Brg Width"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable)(0, col++) << COLHDR(Sub2(_T("Y"), _T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable)(0, col++) << COLHDR(Sub2(_T("z"), _T("o")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*pTable)(0, col++) << _T("Inclination (") << symbol(theta) << _T(")") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
      (*pTable)(0, col++) << Sub2(symbol(theta),_T("max")) << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
      (*pTable)(0, col++) << _T("FS");
      (*pTable)(0, col++) << _T("Status");

      Float64 BrgPadWidth, Yb, Orientation, zo;
      pArtifact->GetGlobalGirderStabilityParameters(&BrgPadWidth,&Yb,&Orientation,&zo);
      Float64 ThetaMax = pArtifact->GetMaxGirderIncline();
      Float64 FS = pArtifact->GetFactorOfSafety();

      col = 0;
      (*pTable)(1, col++) << dim.SetValue(BrgPadWidth);
      (*pTable)(1, col++) << dim.SetValue(Yb);
      (*pTable)(1, col++) << dim.SetValue(zo);
      (*pTable)(1, col++) << slope.SetValue(Orientation);
      (*pTable)(1, col++) << slope.SetValue(ThetaMax);
      if (10 <= FS)
      {
         (*pTable)(1, col++) << _T("10+");
      }
      else
      {
         (*pTable)(1, col++) << scalar.SetValue(FS);
      }

      if ( pArtifact->Passed() )
      {
         (*pTable)(1,col++) << RPT_PASS;
      }
      else
      {
         (*pTable)(1,col++) << RPT_FAIL;
      }
      
      *pBody << _T("Minimum Factor of Safety = ") << FSmax << rptNewLine;
      *pBody << pTable;
   } // next segment
}

void CConstructabilityCheckTable::BuildPrecamberCheck(rptChapter* pChapter, IBroker* pBroker, const std::vector<CGirderKey>& girderList, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker, IArtifact, pIArtifact);
   GET_IFACE2_NOCHECK(pBroker, IBridge, pBridge);

   // if there is only one span/girder, don't need to print span info
   bool needSpanCols = true; // ConstrNeedSpanCols(girderList, pBridge);

                             // Create table - delete it later if we don't need it
   ColumnIndexType nCols = needSpanCols ? 5 : 3; // put span/girder in table if multi girder
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols, _T(""));

   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false);

   pTable->SetColumnStyle(nCols - 1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(nCols - 1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   if (needSpanCols)
   {
      (*pTable)(0, col++) << _T("Span");
      (*pTable)(0, col++) << _T("Girder");
   }

   (*pTable)(0, col++) << COLHDR(_T("Precamber"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(_T("Precamber Limit"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << _T("Status");

   bool bReportPrecamber = false;
   RowIndexType row = 0;
   for (const auto& girderKey : girderList)
   {
      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);
      const pgsConstructabilityArtifact* pConstrArtifact = pGdrArtifact->GetConstructabilityArtifact();

      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         const auto& artifact = pConstrArtifact->GetSegmentArtifact(segIdx);

         if (artifact.IsPrecamberApplicable())
         {
            row++;
            col = 0;

            if (needSpanCols)
            {
               GroupIndexType group = girderKey.groupIndex;
               GirderIndexType girder = girderKey.girderIndex;
               (*pTable)(row, col++) << LABEL_SPAN(group);
               (*pTable)(row, col++) << LABEL_GIRDER(girder);
            }

            Float64 PrecamberLimit, Precamber;
            CSegmentKey segmentKey(girderKey, 0);
            artifact.GetPrecamber(segmentKey,&PrecamberLimit, &Precamber);

            if (!IsZero(Precamber))
            {
               // precamber is applicable and we have at least on case where precamber was input
               bReportPrecamber = true;
            }

            (*pTable)(row, col++) << dim.SetValue(Precamber);
            (*pTable)(row, col++) << symbol(PLUS_MINUS) << dim.SetValue(PrecamberLimit);

            if (artifact.PrecamberPassed())
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
   if (bReportPrecamber)
   {
      rptParagraph* pTitle = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pTitle;
      *pTitle << _T("Precamber Limits");

      rptParagraph* pBody = new rptParagraph;
      *pChapter << pBody;

      *pBody << pTable;
   }
   else
   {
      delete pTable;
   }
}

void CConstructabilityCheckTable::BuildBottomFlangeClearanceCheck(rptChapter* pChapter,IBroker* pBroker, const std::vector<CGirderKey>& girderList, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker, ILibrary, pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(spec_name.c_str());
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   pgsTypes::SupportedBeamSpacing spacingType = pIBridgeDesc->GetGirderSpacingType();
   if (pSpecEntry->CheckBottomFlangeClearance() && ::IsJointSpacing(spacingType))
   {
      rptParagraph* pTitle = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pTitle;
      *pTitle << _T("Bottom Flange Clearance");

      rptParagraph* pBody = new rptParagraph;
      *pChapter << pBody;
      *pBody << _T("Bottom flange clearance is not checked for girder framing based on joint spacing") << rptNewLine;
      return;
   }

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2_NOCHECK(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker, IDocumentType, pDocType);

   // if there is only one span/girder, don't need to print span info
   bool needSpanCols = true; // ConstrNeedSpanCols(girderList, pBridge);

   // Create table - delete it later if we don't need it
   ColumnIndexType nCols = needSpanCols ? 5 : 3; // put span/girder in table if multi girder
   if (pDocType->IsPGSpliceDocument())
   {
      nCols++; // add one column for segments
   }
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols,_T(""));

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetSpanLengthUnit(), false );
   std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   pTable->SetColumnStyle(nCols-1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(nCols-1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   if (needSpanCols)
   {
      if (pDocType->IsPGSpliceDocument())
      {
         (*pTable)(0, col++) << _T("Group");
      }
      else
      {
         (*pTable)(0, col++) << _T("Span");
      }
      (*pTable)(0, col++) << _T("Girder");
   }

   if (pDocType->IsPGSpliceDocument())
   {
      (*pTable)(0, col++) << _T("Segment");
   }

   (*pTable)(0,col++) << COLHDR(_T("Clearance"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Min. Clearance"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*pTable)(0,col++) << _T("Status");

   RowIndexType row = 0;
   for (const auto& girderKey : girderList)
   {
      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);
      const pgsConstructabilityArtifact* pConstrArtifact = pGdrArtifact->GetConstructabilityArtifact();

      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         const auto& artifact = pConstrArtifact->GetSegmentArtifact(segIdx);

         if (artifact.IsBottomFlangeClearanceApplicable())
         {
            row++;
            col = 0;

            if (needSpanCols)
            {
               (*pTable)(row, col++) << LABEL_SPAN(girderKey.groupIndex);
               (*pTable)(row, col++) << LABEL_GIRDER(girderKey.girderIndex);
            }

            if (pDocType->IsPGSpliceDocument())
            {
               (*pTable)(row, col++) << LABEL_SEGMENT(segIdx);
            }


            Float64 C, Cmin;
            artifact.GetBottomFlangeClearanceParameters(&C,&Cmin);

            (*pTable)(row, col++) << dim.SetValue(C);
            (*pTable)(row, col++) << dim.SetValue(Cmin);

            if (artifact.BottomFlangeClearancePassed() )
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

void CConstructabilityCheckTable::BuildFinishedElevationCheck(rptChapter* pChapter, IBroker* pBroker, const std::vector<CGirderKey>& girderList, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker, IArtifact, pIArtifact);
   GET_IFACE2_NOCHECK(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   bool isPGSuper = pDocType->IsPGSuperDocument();

   ColumnIndexType nCols = 9;

   // Only need to present controlling interval if more than one spec check interval is defined
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   std::vector<IntervalIndexType> checkIntervals = pIntervals->GetSpecCheckGeometryControlIntervals();
   bool bPrintInterval = checkIntervals.size() > 1;
   if (bPrintInterval)
   {
      nCols += 1;
   }
                             // Create table - delete it later if we don't need it
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols, _T(""));

   INIT_UV_PROTOTYPE(rptLengthUnitValue,dim,pDisplayUnits->GetSpanLengthUnit(),false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, cmpdim, pDisplayUnits->GetComponentDimUnit(), false);
   std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   pTable->SetColumnStyle(nCols - 1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(nCols - 1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << (isPGSuper ? _T("Span") : _T("Segment"));
   (*pTable)(0, col++) << _T("Girder");
   (*pTable)(0, col++) << COLHDR(_T("Station"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0, col++) << COLHDR(_T("Offset"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   if (bPrintInterval)
   {
      (*pTable)(0,col++) << _T("Controlling") << rptNewLine << _T("Interval");
   }

   (*pTable)(0, col++) << COLHDR(_T("Design Elevation"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0, col++) << COLHDR(_T("Finished Elevation"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0, col++) << COLHDR(_T("Difference"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0,col++) << COLHDR(_T("Difference"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << _T("Status");

   RowIndexType row = 0;
   for (const auto& girderKey : girderList)
   {
      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);
      const pgsConstructabilityArtifact* pConstrArtifact = pGdrArtifact->GetConstructabilityArtifact();

      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         const auto& artifact = pConstrArtifact->GetSegmentArtifact(segIdx);

         if (artifact.GetFinishedElevationApplicability())
         {
            row++;
            col = 0;

            if (isPGSuper)
            {
               (*pTable)(row,col++) << LABEL_SPAN(segIdx);
            }
            else
            {
               (*pTable)(row,col++) << LABEL_SEGMENT(segIdx);
            }

            GirderIndexType girder = girderKey.girderIndex;
            (*pTable)(row,col++) << LABEL_GIRDER(girder);

            Float64 station, offset, designElev, finishedElev;
            pgsPointOfInterest poi;
            artifact.GetMaxFinishedElevation(&station, &offset, &poi, &designElev, &finishedElev);

            Float64 diff = finishedElev - designElev;

            (*pTable)(row, col++) << rptRcStation(station, &pDisplayUnits->GetStationFormat());
            (*pTable)(row, col++) << RPT_OFFSET(offset,dim);
            if (bPrintInterval)
            {
               IntervalIndexType ctrlInterval = artifact.GetFinishedElevationControllingInterval();
               (*pTable)(row,col++) << LABEL_INTERVAL(ctrlInterval) << _T(": ") << pIntervals->GetDescription(ctrlInterval);
            }

            (*pTable)(row, col++) << dim.SetValue(designElev);
            (*pTable)(row, col++) << dim.SetValue(finishedElev);
            (*pTable)(row,col++) << dim.SetValue(diff);
            (*pTable)(row, col++) << cmpdim.SetValue(diff);

            if (artifact.FinishedElevationPassed())
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
      rptParagraph* pBody = new rptParagraph;
      *pChapter << pBody;

      GET_IFACE2(pBroker,ILibrary, pLib);
      GET_IFACE2(pBroker,ISpecification, pISpec);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pISpec->GetSpecification().c_str());
      Float64 tolerance = pSpecEntry->GetFinishedElevationTolerance();

      INIT_UV_PROTOTYPE(rptLengthSectionValue, dim1, pDisplayUnits->GetSpanLengthUnit(), true);
      INIT_UV_PROTOTYPE(rptLengthSectionValue, dim2, pDisplayUnits->GetComponentDimUnit(), true);
      *pBody << _T("Tolerance: ") << symbol(PLUS_MINUS) << dim1.SetValue(tolerance) << _T(" (") << symbol(PLUS_MINUS) << dim2.SetValue(tolerance) << _T(")") << rptNewLine;
      *pBody << _T("Design Elevation = elevation defined by the alignment, profile, and superelevations") << rptNewLine;
      *pBody << _T("Finished Elevation = top of finished girder, or overlay if applicable, including the effects of camber") << rptNewLine;
      *pBody << _T("Difference = greatest difference between Design Elevation and Finished Elevation which occurs at Station and Offset.") << rptNewLine;
      
      *pBody << pTable;
   }
   else
   {
      delete pTable;
   }
}

void CConstructabilityCheckTable::BuildMinimumHaunchCheck(rptChapter* pChapter,IBroker* pBroker,const std::vector<CGirderKey>& girderList,IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2_NOCHECK(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   bool isPGSuper = pDocType->IsPGSuperDocument();

   ColumnIndexType nCols = 6;

   // Only need to present controlling interval if more than one spec check interval is defined
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   std::vector<IntervalIndexType> checkIntervals = pIntervals->GetSpecCheckGeometryControlIntervals();
   bool bPrintInterval = checkIntervals.size() > 1;
   if (bPrintInterval)
   {
      nCols += 1;
   }

   // Create table - delete it later if we don't need it
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols,_T(""));

   INIT_UV_PROTOTYPE(rptLengthUnitValue,dim,pDisplayUnits->GetSpanLengthUnit(),false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue,cmpdim,pDisplayUnits->GetComponentDimUnit(),false);

   pTable->SetColumnStyle(nCols - 1,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(nCols - 1,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << (isPGSuper ? _T("Span") : _T("Segment"));
   (*pTable)(0,col++) << _T("Girder");
   (*pTable)(0,col++) << COLHDR(_T("Station"),rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());
   (*pTable)(0,col++) << COLHDR(_T("Offset"),rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());
   if (bPrintInterval)
   {
      (*pTable)(0,col++) << _T("Controlling") << rptNewLine << _T("Interval");
   }

   (*pTable)(0,col++) << COLHDR(_T("Minimum Haunch Depth"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0,col++) << _T("Status");

   // Get minimum haunch depth along all girders/segments. Check if it may be excessive
   Float64 minOfAllHaunches(Float64_Max);
   Float64 excessiveWarnTol(0.0);

   RowIndexType row = 0;
   for (const auto& girderKey : girderList)
   {
      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);
      const pgsConstructabilityArtifact* pConstrArtifact = pGdrArtifact->GetConstructabilityArtifact();

      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         const auto& artifact = pConstrArtifact->GetSegmentArtifact(segIdx);

         if (artifact.GetFinishedElevationApplicability())
         {
            row++;
            col = 0;

            if (isPGSuper)
            {
               (*pTable)(row,col++) << LABEL_SPAN(segIdx);
            }
            else
            {
               (*pTable)(row,col++) << LABEL_SEGMENT(segIdx);
            }

            GirderIndexType girder = girderKey.girderIndex;
            (*pTable)(row,col++) << LABEL_GIRDER(girder);

            Float64 station,offset,minHaunch;
            pgsPointOfInterest poi;
            artifact.GetMinimumHaunchDepth(&station,&offset,&poi,&minHaunch);

            minOfAllHaunches = min(minOfAllHaunches,minHaunch);
            excessiveWarnTol = artifact.GetExcessSlabOffsetWarningTolerance();

            (*pTable)(row,col++) << rptRcStation(station,&pDisplayUnits->GetStationFormat());
            (*pTable)(row,col++) << RPT_OFFSET(offset,dim);
            if (bPrintInterval)
            {
               IntervalIndexType ctrlInterval = artifact.GetMinimumHaunchDepthControllingInterval();
               (*pTable)(row,col++) << LABEL_INTERVAL(ctrlInterval) << _T(": ") << pIntervals->GetDescription(ctrlInterval);
            }

            (*pTable)(row,col++) << cmpdim.SetValue(minHaunch);

            if (artifact.MinimumHaunchDepthPassed())
            {
               (*pTable)(row,col++) << RPT_PASS;
            }
            else
            {
               (*pTable)(row,col++) << RPT_FAIL;
            }
         }
      }
   }

   // Only add table if it has content
   if (0 < row)
   {
      rptParagraph* pTitle = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pTitle;
      *pTitle << _T("Minimum Haunch Depth Check");

      rptParagraph* pBody = new rptParagraph;
      *pChapter << pBody;

      Float64 fillet = pBridge->GetFillet();

      INIT_UV_PROTOTYPE(rptLengthSectionValue,dimc,pDisplayUnits->GetComponentDimUnit(),true);
      *pBody << _T("Minimum haunch depth is taken along entire girder length and is compared with a fillet value of ") << dimc.SetValue(fillet) << rptNewLine;
      *pBody << pTable;

      Float64 excessive = excessiveWarnTol + fillet;
      if (minOfAllHaunches > excessive)
      {
         *pBody << Bold(_T("Warning:")) << _T(" The smallest haunch depth along the entire girder is ") << dimc.SetValue(minOfAllHaunches) << _T(". This is larger than the fillet value + the allowable tolerance for this girder = ")
            << dimc.SetValue(excessive)  << Bold(_T(". Hence, the haunch depth is excessive for this girder.")) << _T(" Consider reducing the haunch depth along the girder to reduce waste material.") << rptNewLine << rptNewLine;
      }
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
   Float64 min_days =  WBFL::Units::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Min(), WBFL::Units::Measure::Day);
   Float64 max_days =  WBFL::Units::ConvertFromSysUnits(pSpecEntry->GetCreepDuration2Max(), WBFL::Units::Measure::Day);

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

   GET_IFACE2(pBroker, IPointOfInterest, pPoi );

   GET_IFACE2(pBroker, IBridge, pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   SpanIndexType startSpanIdx, endSpanIdx;
   pBridge->GetGirderGroupSpans(girderKey.groupIndex,&startSpanIdx,&endSpanIdx);
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      CSpanKey spanKey(spanIdx,girderKey.girderIndex);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(spanKey, POI_5L | POI_SPAN, &vPoi);
      ATLASSERT(vPoi.size()==1);
      const pgsPointOfInterest& poiMidSpan(vPoi.front());

      Float64 C;
      if (IsNonstructuralDeck(deckType))
      {
         C = pCamber->GetExcessCamber(poiMidSpan, CREEP_MAXTIME);
         (*pTable)(row, 0) << _T("Final camber, ") << Sub2(_T("C"),_T("F"));
         (*pTable)(row++, 1) << dim.SetValue(C);
      }
      else
      {
         C = pCamber->GetScreedCamber(poiMidSpan, CREEP_MAXTIME);
         (*pTable)(row, 0) << _T("Screed Camber, C at mid-span");
         (*pTable)(row++, 1) << dim.SetValue(C);
      }

      Float64 Dmax_UpperBound, Dmax_Average, Dmax_LowerBound;
      Float64 Dmin_UpperBound, Dmin_Average, Dmin_LowerBound;
      pCamber->GetDCamberForGirderScheduleEx(poiMidSpan, CREEP_MAXTIME, &Dmax_UpperBound, &Dmax_Average, &Dmax_LowerBound);
      pCamber->GetDCamberForGirderScheduleEx(poiMidSpan, CREEP_MINTIME, &Dmin_UpperBound, &Dmin_Average, &Dmin_LowerBound);
   
      Float64 Cfactor = pCamber->GetLowerBoundCamberVariabilityFactor();
   
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
   
      if ( pSpecEntry->CheckGirderSag() )
      {
         if (IsNonstructuralDeck(deckType))
         {
            if (C < 0)
            {
               rptParagraph* p = new rptParagraph;
               *pChapter << p;
               *p << color(Red) << _T("WARNING: Final camber (") << Sub2(_T("C"),_T("F")) << _T(") is downward. The girder may end up with a sag.") << color(Black) << rptNewLine;
            }
         }
         else
         {
            std::_tstring camberType;
            Float64 D = 0;

            switch (pSpecEntry->GetSagCamberType())
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

            if (D < C)
            {
               rptParagraph* p = new rptParagraph;
               *pChapter << p;

               *p << color(Red) << _T("WARNING: Screed camber (C) is greater than the ") << camberType.c_str() << _T(" camber at time of deck casting, D. The girder may end up with a sag.") << color(Black) << rptNewLine;
            }
            else if (IsEqual(C, D, WBFL::Units::ConvertToSysUnits(0.25, WBFL::Units::Measure::Inch)))
            {
               rptParagraph* p = new rptParagraph;
               *pChapter << p;

               *p << color(Red) << _T("WARNING: Screed camber (C) is nearly equal to the ") << camberType.c_str() << _T(" camber at time of deck casting, D. The girder may end up with a sag.") << color(Black) << rptNewLine;
            }

            if (Dmin_LowerBound < C && pSpecEntry->GetSagCamberType() != pgsTypes::LowerBoundCamber)
            {
               rptParagraph* p = new rptParagraph;
               *pChapter << p;

               *p << _T("Screed camber (C) is greater than the lower bound camber at time of deck casting (") << Cfactor * 100 << _T("% of D") << Sub(min_days) << _T("). The girder may end up with a sag if the deck is placed at day ") << min_days << _T(" and the actual camber is a lower bound value.") << rptNewLine;
            }
         }
      }
   }
}

void CConstructabilityCheckTable::BuildTimeStepCamberCheck(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IProductForces, pProductForces);
   GET_IFACE2(pBroker,ILimitStateForces,pLSForces);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2( pBroker, ILibrary, pLib );
   GET_IFACE2( pBroker, ISpecification, pSpec );

   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   IntervalIndexType firstCastDeckIntervalIdx = pIntervals->GetFirstCastDeckInterval();
   Float64 deckCastingTime = pIntervals->GetTime(firstCastDeckIntervalIdx,pgsTypes::Start);

   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;
   Float64 lastIntervalTime = pIntervals->GetTime(lastIntervalIdx, pgsTypes::End);

   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);


   rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Camber");

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(3,_T(""));

   pTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
   pTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

   *pBody << pTable << rptNewLine;

   GET_IFACE2(pBroker, IDocumentType, pDocType);
   if (pDocType->IsPGSpliceDocument())
   {
      (*pTable)(0, 0) << _T("Segment");
   }
   else
   {
      (*pTable)(0, 0) << _T("Span");
   }
   (*pTable)(0,1) << _T("Description");
   (*pTable)(0,2) << COLHDR(_T("Camber"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker, IPointOfInterest, pPoi );

   GET_IFACE2(pBroker, IBridge, pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();


   if (pDocType->IsPGSpliceDocument())
   {
      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         CSegmentKey segmentKey(girderKey, segIdx);
         PoiList vPoi;
         pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT | POI_5L, &vPoi);
         ATLASSERT(vPoi.size() == 1);
         const pgsPointOfInterest& poiMidSpan(vPoi.front());

         pTable->SetRowSpan(row, 0, 2);
         (*pTable)(row, 0) << LABEL_SEGMENT(segIdx);

         GET_IFACE2(pBroker, ICamber, pCamber);
         Float64 C = pCamber->GetScreedCamber(poiMidSpan, CREEP_MAXTIME);
         (*pTable)(row, 1) << _T("Screed Camber, C at mid-segment");
         (*pTable)(row++, 2) << dim.SetValue(C);

         Float64 Dmin, Dmax;
         pLSForces->GetDeflection(firstCastDeckIntervalIdx - 1, pgsTypes::ServiceI, poiMidSpan, bat, true/*include prestress*/, false/*no liveload*/, true /*include elevation adjustment*/, true /*include precamber*/, true /* include unrecoverable */, &Dmin, &Dmax);
         ATLASSERT(IsEqual(Dmin, Dmax));
         (*pTable)(row, 1) << _T("Mid-segment camber at time of deck casting, at ") << deckCastingTime << _T(" days, D");

         if (Dmin < 0)
         {
            (*pTable)(row++, 2) << color(Red) << dim.SetValue(Dmin) << color(Black);
         }
         else
         {
            (*pTable)(row++, 2) << dim.SetValue(Dmin);
         }

         if (pSpecEntry->CheckGirderSag() && IsStructuralDeck(deckType))
         {
            std::_tstring camberType;

            if (Dmin < C)
            {
               rptParagraph* p = new rptParagraph;
               *pChapter << p;

               *p << color(Red) << _T("WARNING: Screed Camber, C, is greater than the camber at time of deck casting, D. The girder may end up with a sag.") << color(Black) << rptNewLine;
            }
            else if (IsEqual(C, Dmin, WBFL::Units::ConvertToSysUnits(0.25, WBFL::Units::Measure::Inch)))
            {
               rptParagraph* p = new rptParagraph;
               *pChapter << p;

               *p << color(Red) << _T("WARNING: Screed Camber, C, is nearly equal to the camber at time of deck casting, D. The girder may end up with a sag.") << color(Black) << rptNewLine;
            }
         }
      }  // next segment 
   }
   else
   {
      SpanIndexType startSpanIdx, endSpanIdx;
      pBridge->GetGirderGroupSpans(girderKey.groupIndex, &startSpanIdx, &endSpanIdx);
      for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++)
      {
         CSpanKey spanKey(spanIdx, girderKey.girderIndex);
         PoiList vPoi;
         pPoi->GetPointsOfInterest(spanKey, POI_5L | POI_SPAN, &vPoi);
         ATLASSERT(vPoi.size() == 1);
         const pgsPointOfInterest& poiMidSpan(vPoi.front());

         pTable->SetRowSpan(row, 0, 2);
         (*pTable)(row, 0) << LABEL_SPAN(spanIdx);

         GET_IFACE2(pBroker, ICamber, pCamber);
         Float64 C = pCamber->GetScreedCamber(poiMidSpan, CREEP_MAXTIME);
         (*pTable)(row, 1) << _T("Screed Camber, C at mid-span");
         (*pTable)(row++, 2) << dim.SetValue(C);

         Float64 Dmin, Dmax;
         pLSForces->GetDeflection(firstCastDeckIntervalIdx - 1, pgsTypes::ServiceI, poiMidSpan, bat, true/*include prestress*/, false/*no liveload*/, true /*include elevation adjustment*/, true /*include precamber*/,true /* include unrecoverable */, &Dmin, &Dmax);
         ATLASSERT(IsEqual(Dmin, Dmax));
         (*pTable)(row, 1) << _T("Mid-span camber at time of deck casting, at ") << deckCastingTime << _T(" days, D");

         if (Dmin < 0)
         {
            (*pTable)(row++, 2) << color(Red) << dim.SetValue(Dmin) << color(Black);
         }
         else
         {
            (*pTable)(row++, 2) << dim.SetValue(Dmin);
         }

         if (pSpecEntry->CheckGirderSag() && IsStructuralDeck(deckType))
         {
            std::_tstring camberType;

            if (Dmin < C)
            {
               rptParagraph* p = new rptParagraph;
               *pChapter << p;

               *p << color(Red) << _T("WARNING: Screed Camber, C, is greater than the camber at time of deck casting, D. The girder may end up with a sag.") << color(Black) << rptNewLine;
            }
            else if (IsEqual(C, Dmin, WBFL::Units::ConvertToSysUnits(0.25, WBFL::Units::Measure::Inch)))
            {
               rptParagraph* p = new rptParagraph;
               *pChapter << p;

               *p << color(Red) << _T("WARNING: Screed Camber, C, is nearly equal to the camber at time of deck casting, D. The girder may end up with a sag.") << color(Black) << rptNewLine;
            }
         }
      }  // next span 
   }
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

void CConstructabilityCheckTable::Dump(WBFL::Debug::LogContext& os) const
{
   os << _T("Dump for CConstructabilityCheckTable") << WBFL::Debug::endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CConstructabilityCheckTable::TestMe(WBFL::Debug::Log& rlog)
{
   TESTME_PROLOGUE("CConstructabilityCheckTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CConstructabilityCheckTable");

   TESTME_EPILOG("CConstructabilityCheckTable");
}
#endif // _UNITTEST
