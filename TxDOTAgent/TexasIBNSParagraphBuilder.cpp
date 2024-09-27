///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#include <Reporting\SpanGirderReportSpecification.h>

#include "TexasIBNSParagraphBuilder.h"

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\BridgeDescription2.h>

#include <psgLib\SpecLibraryEntry.h>
#include <psgLib\GirderLibraryEntry.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\MomentCapacity.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\DistributionFactors.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/// Inline functions


static void WriteGirderScheduleTable(rptParagraph* p, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits,
                                     const std::vector<CSegmentKey>& segments,  const std::vector<txcwStrandLayoutType>& strandLayoutVec,
                                     ColumnIndexType startIdx, ColumnIndexType endIdx,
                                     IStrandGeometry* pStrandGeometry, ISegmentData* pSegmentData, IPointOfInterest* pPointOfInterest,
                                     const CBridgeDescription2* pBridgeDesc, IArtifact* pIArtifact, ILiveLoadDistributionFactors* pDistFact,
                                     IMaterials* pMaterial, IMomentCapacity* pMomentCapacity,
                                     bool bUnitsSI, bool areAnyTempStrandsInTable, 
                                     bool areAnyHarpedStrandsInTable, bool areAnyDebondingInTable);



/****************************************************************************
CLASS	TxDOTIBNSDebondWriter
****************************************************************************/

void TxDOTIBNSDebondWriter::WriteDebondData(rptParagraph* pPara,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits, const std::_tstring& optionalName)
{
   *pPara<<rptNewLine; // make some space

   StrandIndexType nss = m_pStrandGeometry->GetStrandCount(m_SegmentKey,pgsTypes::Straight);
   bool is_optional = optionalName.size() > 0;

   // see if we have an error condition - don't build table if so
   if (nss==0 || m_OutCome==SectionMismatch || m_OutCome==TooManySections || m_OutCome==SectionsNotSymmetrical)
   {
      if (is_optional)
      {
         *pPara <<bold(ON)<< _T("Debonding Information for ") << optionalName << bold(OFF) << rptNewLine;
      }
      else
      {
         *pPara <<bold(ON)<< _T("Debonding Information for Span ") << LABEL_SPAN(m_SegmentKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(m_SegmentKey.girderIndex) << bold(OFF) << rptNewLine;
      }

      if(nss==0)
      {
         *pPara<< color(Red) <<_T("Warning: No straight strands in girder. Cannot write debonding information.")<<color(Black)<<rptNewLine;
      }
      else if(m_OutCome==SectionMismatch)
      {
         *pPara<< color(Red) <<_T("Warning: Irregular, Non-standard debonding increments used. Cannot write debonding information.")<<color(Black)<<rptNewLine;
      }
      else if (m_OutCome==TooManySections)
      {
         *pPara<< color(Red) <<_T("Warning: More than ten debonding increments exist. Cannot write debonding information.")<<color(Black)<<rptNewLine;
      }
      else if (m_OutCome==SectionsNotSymmetrical)
      {
         *pPara<< color(Red) <<_T("Warning: Debond sections are not symmetic about girder mid-span. Cannot write debonding information.")<<color(Black)<<rptNewLine;
      }
      else
      {
         ATLASSERT(false); // A new outcome we aren't aware of?
      }
   }
   else
   {
      // All is ok to write table
      INIT_UV_PROTOTYPE( rptLengthUnitValue, uloc, pDisplayUnits->GetSpanLengthUnit(), true);
      INIT_UV_PROTOTYPE( rptLengthUnitValue, ucomp,    pDisplayUnits->GetComponentDimUnit(), true );

      uloc.SetFormat(WBFL::System::NumericFormatTool::Format::Automatic);

      const ColumnIndexType num_cols=13;
      std::_tostringstream os;
      if (is_optional)
      {
         os << _T("NS Direct-Fill Strand Pattern for ") << optionalName;
      }
      else
      {
         os <<_T("Debonded Strand Pattern for Span ")<<LABEL_SPAN(m_SegmentKey.groupIndex)<<_T(" Girder ")<<LABEL_GIRDER(m_SegmentKey.girderIndex);
      }

      rptRcTable* p_table = rptStyleManager::CreateDefaultTable(num_cols,os.str());
      *pPara << p_table;

      // This table has a very special header
      p_table->SetNumberOfHeaderRows(2);

      p_table->SetRowSpan(0,0,2);
      (*p_table)(0,0) << _T("Dist from Bottom");

      p_table->SetColumnSpan(0,1,2);
      (*p_table)(0, 1) << _T("# of Strands");
      (*p_table)(1, 1) << Bold(_T("Total"));
      (*p_table)(1, 2) << Bold(_T("Debonded"));

      p_table->SetColumnSpan(0,3,10);
      (*p_table)(0,3) << _T("# of Strands Debonded At");

      // next row of header
      Int16 loc_inc=1;
      for (ColumnIndexType ic = 3; ic < num_cols; ic++)
      {
         Float64 loc = m_SectionSpacing*loc_inc;
         (*p_table)(1,ic) <<Bold( uloc.SetValue(loc) );
         loc_inc++;
      }

      RowIndexType row = p_table->GetNumberOfHeaderRows();
      if (m_NumDebonded == 0 && !is_optional)
      {
         // no debonded strands, just write one row

         pgsPointOfInterest poi(m_SegmentKey, m_GirderLength/2.0);

         std::vector<StrandIndexType> vss  = m_pStrandGeometry->GetStrandsInRow(poi,0,pgsTypes::Straight);
         ATLASSERT(vss.size()>0);

         // get y of any strand in row
         CComPtr<IPoint2dCollection> coords;
         m_pStrandGeometry->GetStrandPositions(poi, pgsTypes::Straight, &coords);

         GET_IFACE2(pBroker,ISectionProperties,pSectProp);
         GET_IFACE2(pBroker,IIntervals,pIntervals);
         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);
         Float64 Hg = pSectProp->GetHg(releaseIntervalIdx,poi);

         // get elevation of strand
         CComPtr<IPoint2d> point;
         coords->get_Item(vss[0],&point);
         Float64 curr_y;
         point->get_Y(&curr_y);

         (*p_table)(row,0) << ucomp.SetValue(Hg + curr_y);
         (*p_table)(row,1) << (IndexType)vss.size();

         // rest of columns are zeros
         for (ColumnIndexType icol = 2; icol < num_cols; icol++)
         {
            (*p_table)(row,icol) << (IndexType)0;
         }
      }
      else
      {
         // Finished writing Header, now write table, row by row
         ATLASSERT(!m_Rows.empty()); // we have debonds - we gotta have rows?

         RowIndexType nrow = 0;
         RowListIter riter = m_Rows.begin();
         while(riter != m_Rows.end())
         {
            const RowData& rowdata = *riter;

            if( !rowdata.m_Sections.empty() || is_optional) // Only write row if it has debonding, or we are writing optional design
            {
               pgsPointOfInterest poi(m_SegmentKey, m_GirderLength/2.0);

               GET_IFACE2(pBroker,ISectionProperties,pSectProp);
               GET_IFACE2(pBroker,IIntervals,pIntervals);
               IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);
               Float64 Hg = pSectProp->GetHg(releaseIntervalIdx,poi);

               (*p_table)(row,0) << ucomp.SetValue(Hg + rowdata.m_Elevation);

               (*p_table)(row,1) << rowdata.m_NumTotalStrands;;

               Int16 ndbr = CountDebondsInRow(rowdata);
               (*p_table)(row,2) << ndbr;

               // we have 10 columns to write no matter what
               SectionListConstIter scit = rowdata.m_Sections.begin();

               for (ColumnIndexType icol = 3; icol < num_cols; icol++)
               {
                  Int16 db_cnt = 0;

                  if (scit!= rowdata.m_Sections.end())
                  {
                     const SectionData secdata = *scit;
                     Float64 row_loc = (icol-2)*m_SectionSpacing;

                     if (IsEqual(row_loc, secdata.m_XLoc))
                     {
                        db_cnt = secdata.m_NumDebonds;
                        scit++;
                     }
                  }

                  (*p_table)(row,icol) << db_cnt;
               }

               ATLASSERT(scit==rowdata.m_Sections.end()); // we didn't find all of our sections - bug
               row++; // table 
            }


            nrow++;
            riter++;
         }

         // write note about non-standard spacing if applicable
         if (m_OutCome==NonStandardSection)
         {
            *pPara<< color(Red)<<_T("Warning: Non-standard debonding increment of ")<<uloc.SetValue(m_SectionSpacing)<<_T(" used.")<<color(Black)<<rptNewLine;
         }
      }
   }
}

/****************************************************************************
CLASS	CTexasIBNSParagraphBuilder
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTexasIBNSParagraphBuilder::CTexasIBNSParagraphBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================

/*--------------------------------------------------------------------*/
rptParagraph* CTexasIBNSParagraphBuilder::Build(IBroker*	pBroker, const std::vector<CSegmentKey>& segmentKeys,
                                                IEAFDisplayUnits* pDisplayUnits, Uint16 level, bool& rbEjectPage) const
{
   rbEjectPage = true; // we can just fit this and the geometry table on a page if there is no additional data


   rptParagraph* p = new rptParagraph;

   bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   GET_IFACE2_NOCHECK(pBroker, ISegmentData, pSegmentData);
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );
   GET_IFACE2_NOCHECK(pBroker, IMaterials, pMaterial);
   GET_IFACE2_NOCHECK(pBroker,IMomentCapacity,pMomentCapacity);
   GET_IFACE2_NOCHECK(pBroker, IPointOfInterest, pPointOfInterest );
   GET_IFACE2_NOCHECK(pBroker,ILiveLoadDistributionFactors,pDistFact);
   GET_IFACE2_NOCHECK(pBroker,IArtifact,pIArtifact);
   GET_IFACE2_NOCHECK(pBroker,IIntervals,pIntervals);
   GET_IFACE2_NOCHECK(pBroker,ISectionProperties,pSectProp);

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // Round up data common to all tables
   bool areAnyHarpedStrandsInTable(false);
   bool areAnyTempStrandsInTable(false), areAnyDebondingInTable(false);
   bool areAnyRaisedStraightStrandsInTable(false);
   std::vector<txcwStrandLayoutType> strand_layout_vec; // store for next loops instead of recomputing
   std::vector<CSegmentKey>::const_iterator itsg(segmentKeys.begin());
   std::vector<CSegmentKey>::const_iterator itsg_end(segmentKeys.end());
   for (; itsg != itsg_end; itsg++ )
   {
      const CSegmentKey& segmentKey(*itsg);

      txcwStrandLayoutType strand_layout = GetStrandLayoutType(pBroker, segmentKey);
      strand_layout_vec.push_back(strand_layout); 

      areAnyRaisedStraightStrandsInTable |= strand_layout==tslRaisedStraight;
      areAnyHarpedStrandsInTable   |= strand_layout==tslHarped           || strand_layout==tslMixedHarpedRaised  || strand_layout==tslMixedHarpedDebonded;
      areAnyDebondingInTable |= strand_layout==tslDebondedStraight || strand_layout==tslMixedHarpedDebonded;

      areAnyTempStrandsInTable   |= 0 < pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Temporary);
   }

   // First thing - check if we can generate the girder schedule table at all.
   // Cannot have Harped and Debonded strands in same report
   if (areAnyHarpedStrandsInTable && areAnyDebondingInTable)
   {
      *p << color(Red) << bold(ON) <<_T("Note: The TxDOT Girder Schedule report cannot be generated with mixed harped and debonded designs.")
         << _T("  Please select other girders (with similar strand layouts) and try again.") << bold(OFF) << color(Black) << rptNewLine;
   }
   //else if (areAnyHarpedStrandsInTable && areAnyRaisedStraightStrandsInTable)
   //{
   //   *p << color(Red) << bold(ON) <<_T("Note: The TxDOT Girder Schedule report cannot be generated with mixed harped and raised straight strand designs.")
   //      << _T("  Please select other (similar) girders and try again.") << bold(OFF) << color(Black) << rptNewLine;
   //}
   else
   {
      // First notes, if non-standard design for any girders
      bool wasNS(false);
      std::vector<txcwStrandLayoutType>::iterator slit = strand_layout_vec.begin();
      itsg = segmentKeys.begin();
      while(itsg != itsg_end)
      {
         const CSegmentKey& segmentKey(*itsg);
         const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
         txcwStrandLayoutType strand_layout = *slit;

         if ( !IsTxDOTStandardStrands( strand_layout, pStrands->GetStrandDefinitionType(), segmentKey, pBroker ))
         {
            SpanIndexType spanIdx = segmentKey.groupIndex;
            GirderIndexType gdrIdx = segmentKey.girderIndex;

            *p << color(Red) << _T("Note: A Non-Standard Strand Fill Was Used For Span ")
               << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << color(Black) << rptNewLine;

            wasNS = true;
         }

         slit++;
         itsg++;
      }

      if (wasNS)
      {
         *p << rptNewLine; // add a separator for cleaness
      }

      // Main Girder Schedule Table(s)
      // Compute a list of tables to be created. Each item in the list is the number of columns for
      // each table
      std::vector<CGirderKey> girderKeys;
      girderKeys.insert(girderKeys.begin(),segmentKeys.begin(),segmentKeys.end());
      std::list<ColumnIndexType> table_list = ComputeTableCols(girderKeys);

      bool tbfirst = true;
      ColumnIndexType start_idx, end_idx;
      for (std::list<ColumnIndexType>::iterator itcol = table_list.begin(); itcol!=table_list.end(); itcol++)
      {
         if (tbfirst)
         {
            start_idx = 0;
            end_idx = *itcol-1;
            tbfirst = false;
         }
         else
         {
            start_idx = end_idx+1;
            end_idx += *itcol;
            ATLASSERT(end_idx < table_list.size());
         }

         WriteGirderScheduleTable(p, pBroker, pDisplayUnits, segmentKeys, strand_layout_vec, start_idx, end_idx,
                                     pStrandGeometry, pSegmentData, pPointOfInterest,
                                     pBridgeDesc, pIArtifact, pDistFact, pMaterial, pMomentCapacity,
                                     bUnitsSI, areAnyTempStrandsInTable, areAnyHarpedStrandsInTable, areAnyDebondingInTable);
      }

      // Write debond table(s)
      if (areAnyDebondingInTable)
      {
         rbEjectPage = false;
         slit = strand_layout_vec.begin();
         itsg = segmentKeys.begin();
         while(itsg != itsg_end)
         {
            const CSegmentKey& segmentKey(*itsg);
            txcwStrandLayoutType strand_layout = *slit;

            if (strand_layout == tslDebondedStraight)
            {
               WriteDebondTable(p, pBroker, segmentKey, pDisplayUnits);
            }

            itsg++;
            slit++;
         }
      }

      // Write non-standard designs tables
      slit = strand_layout_vec.begin();
      itsg = segmentKeys.begin();
      while(itsg != itsg_end)
      {
         const CSegmentKey& segmentKey(*itsg);
         const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

         txcwStrandLayoutType strand_layout = *slit;

         if ( !IsTxDOTStandardStrands( strand_layout, pStrands->GetStrandDefinitionType(), segmentKey, pBroker) )
         {
            rbEjectPage = false;
            // Nonstandard strands table
            PoiList vPoi;
            pPointOfInterest->GetPointsOfInterest(segmentKey, POI_5L | POI_SPAN, &vPoi);
            ATLASSERT(vPoi.size()==1);
            const pgsPointOfInterest& pmid(vPoi.front());

            OptionalDesignHarpedFillUtil::StrandRowSet strandrows = OptionalDesignHarpedFillUtil::GetStrandRowSet(pBroker, pmid);

            SpanIndexType spanIdx = segmentKey.groupIndex;
            GirderIndexType gdrIdx = segmentKey.girderIndex;

            std::_tostringstream os;
            os <<_T("Non-Standard Strand Pattern for Span ")<<LABEL_SPAN(spanIdx)<<_T(" Girder ")<<LABEL_GIRDER(gdrIdx);

            if (strandrows.empty())
            {
               rptRcTable* p_table = rptStyleManager::CreateDefaultTable(1, os.str().c_str());
               *p << rptNewLine << p_table;
               (*p_table)(0, 0) << _T("The section contains zero strands");
            }
            else
            {
               rptRcTable* p_table = rptStyleManager::CreateDefaultTable(2, os.str().c_str());
               p_table->SetColumnWidth(0, 1.0);
               p_table->SetColumnWidth(1, 1.8);
               *p << rptNewLine << p_table;

               RowIndexType row = 0;
               (*p_table)(row, 0) << _T("Row") << rptNewLine << _T("From Bottom") << rptNewLine << _T("(in)"); // TxDOT dosn't do metric and we need special formatting below
               (*p_table)(row++, 1) << _T("Strands");

               for (OptionalDesignHarpedFillUtil::StrandRowIter srit = strandrows.begin(); srit != strandrows.end(); srit++)
               {
                  const OptionalDesignHarpedFillUtil::StrandRow& srow = *srit;
                  Float64 elev_in = RoundOff(WBFL::Units::ConvertFromSysUnits(srow.Elevation, WBFL::Units::Measure::Inch), 0.001);

                  (*p_table)(row, 0) << elev_in;
                  (*p_table)(row++, 1) << srow.fillListString << _T(" (") << srow.fillListString.size() * 2 << _T(")");
               }
            }
         }

         itsg++;
         slit++;
      }
   }

   return p;
}

void CTexasIBNSParagraphBuilder::WriteDebondTable(rptParagraph* pPara, IBroker* pBroker, const CSegmentKey& segmentKey, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );

   bool bCanDebond = pStrandGeometry->CanDebondStrands(segmentKey,pgsTypes::Straight);
   bCanDebond     |= pStrandGeometry->CanDebondStrands(segmentKey,pgsTypes::Harped);
   bCanDebond     |= pStrandGeometry->CanDebondStrands(segmentKey,pgsTypes::Temporary);

   if ( !bCanDebond )
      return;

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);

   // Need compute tool to decipher debond data
   TxDOTIBNSDebondWriter tx_writer(segmentKey, segment_length, pStrandGeometry);

   tx_writer.WriteDebondData(pPara, pBroker, pDisplayUnits, std::_tstring());
}

void WriteGirderScheduleTable(rptParagraph* p, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits,
                              const std::vector<CSegmentKey>& segmentKeys, const std::vector<txcwStrandLayoutType>& strandLayoutVec,
                              ColumnIndexType startIdx, ColumnIndexType endIdx,
                              IStrandGeometry* pStrandGeometry, ISegmentData* pSegmentData, IPointOfInterest* pPointOfInterest,
                              const CBridgeDescription2* pBridgeDesc, IArtifact* pIArtifact, ILiveLoadDistributionFactors* pDistFact,
                              IMaterials* pMaterial, IMomentCapacity* pMomentCapacity,
                              bool bUnitsSI, bool areAnyTempStrandsInTable, 
                              bool areAnyHarpedStrandsInTable, bool areAnyDebondingInTable)
{
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2_NOCHECK(pBroker,ISectionProperties,pSectProp);

#if defined _DEBUG
   GET_IFACE2(pBroker,IGirder,pGirder);
#endif

   IndexType ng = endIdx-startIdx+1;
   rptRcTable* p_table = rptStyleManager::CreateTableNoHeading(ng+1,_T("TxDOT Girder Schedule"));

   *p << p_table;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, ecc,    pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dia,    pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(),       true );
   rptRcScalar df;
   df.SetFormat(WBFL::System::NumericFormatTool::Format::Fixed);
   df.SetWidth(8);
   df.SetPrecision(5);

   bool bFirst(true);
   ColumnIndexType col = 1;
   for (ColumnIndexType gdr_idx=startIdx; gdr_idx<=endIdx; gdr_idx++)
   {
      const CSegmentKey& segmentKey(segmentKeys[gdr_idx]);
      txcwStrandLayoutType strand_layout(strandLayoutVec[gdr_idx]);

      ATLASSERT(pGirder->IsSymmetricSegment(segmentKey)); // this report assumes girders don't taper in depth

      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

      StrandIndexType ns = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Straight);
      StrandIndexType nh = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Harped);

      const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
      const auto* pstrand = pStrands->GetStrandMaterial(pgsTypes::Straight);

      // create pois at the start of girder and mid-span
      PoiList vPoiRel, vPoiEre;
      pPointOfInterest->GetPointsOfInterest(segmentKey, POI_START_FACE, &vPoiRel);
      ATLASSERT(vPoiRel.size() == 1);
      const pgsPointOfInterest& pois(vPoiRel.front());
      vPoiRel.clear();
      pPointOfInterest->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT, &vPoiRel);
      ATLASSERT(vPoiRel.size() == 1);
      const pgsPointOfInterest& pmidrel(vPoiRel.front());


      pPointOfInterest->GetPointsOfInterest(segmentKey, POI_5L | POI_SPAN, &vPoiEre);
      ATLASSERT(vPoiEre.size()==1);
      const pgsPointOfInterest& pmidere(vPoiEre.back());

      SpanIndexType spanIdx = segmentKey.groupIndex;
      GirderIndexType gdrIdx = segmentKey.girderIndex;
      CSpanKey spanKey(spanIdx,gdrIdx);

      RowIndexType row = 0;
      if(bFirst)
         (*p_table)(row,0) << Bold(_T("Span"));

      (*p_table)(row++,col) << Bold(LABEL_SPAN(spanIdx));

      if(bFirst)
         (*p_table)(row,0) << Bold(_T("Girder"));

      (*p_table)(row++,col) << Bold(LABEL_GIRDER(gdrIdx));

      if(bFirst)
         (*p_table)(row,0) << _T("Girder Type");

      (*p_table)(row++,col) << pBridgeDesc->GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetGirderName();

      if(bFirst)
      {
         (*p_table)(row,0) << Bold(_T("Prestressing Strands"));

         if (ng==1)
            (*p_table)(row, 1) << Bold(_T("Total"));
         else
            (*p_table)(row, 1) << _T("");
      }
      else
      {
         (*p_table)(row,col) << _T("");
      }

      row++;

      if(bFirst)
         (*p_table)(row,0) << _T("NO. (N") << Sub(_T("h")) << _T(" + N") << Sub(_T("s")) << _T(")");

      (*p_table)(row++,col) << Int16(nh + ns);

      if(bFirst)
         (*p_table)(row,0) << _T("Size");

      (*p_table)(row++,col) << dia.SetValue(pstrand->GetNominalDiameter()) << _T(" Dia.");

      if(bFirst)
         (*p_table)(row,0) << _T("Strength");

      std::_tstring strData = pstrand->GetName();

      (*p_table)(row++,col) << strData;


      if(bFirst)
      {
         (*p_table)(row,0) << _T("Eccentricity @ CL");
         if ( areAnyTempStrandsInTable )
            (*p_table)(row,0) << _T(" (w/o Temporary Strands)");
      }

      (*p_table)(row++,col) << ecc.SetValue( pStrandGeometry->GetEccentricity( releaseIntervalIdx, pmidrel, false).Y());

      if(bFirst)
      {
         (*p_table)(row,0) << _T("Eccentricity @ End");
         if ( areAnyTempStrandsInTable )
            (*p_table)(row,0) << _T(" (w/o Temporary Strands)");
      }

      (*p_table)(row++,col) << ecc.SetValue( pStrandGeometry->GetEccentricity( releaseIntervalIdx, pois, false).Y());

      if(bFirst)
      {
         (*p_table)(row,0) << Bold(_T("Prestressing Strands"));
      }

      bool isHarped = strand_layout == tslHarped || strand_layout == tslMixedHarpedRaised || strand_layout == tslMixedHarpedDebonded;

      if (isHarped)
      {
         (*p_table)(row++,col) << Bold(_T("Depressed"));
      }
      else
      {
         (*p_table)(row++,col) << Bold(_T("Straight"));
      }

      if (areAnyHarpedStrandsInTable)
      {
         if (bFirst)
            (*p_table)(row, 0) << _T("NO. (# of Depressed Strands)");

         (*p_table)(row++, col) << (isHarped ? nh : 0);

         if (bFirst)
         {
            (*p_table)(row, 0) << _T("Y") << Sub(_T("b")) << _T(" of Topmost Depressed Strand(s) @ End");
            (*p_table)(row + 1, 0) << _T("Y") << Sub(_T("b")) << _T(" of Topmost Depressed Strand(s) @ CL");
         }

         if (!isHarped)
         {
            // straight strand in harped table. Leave topmost strand locs blank
            (*p_table)(row++, col) << symbol(NBSP);
            (*p_table)(row++, col) << symbol(NBSP);
         }
         else
         {
            Float64 Hg = pSectProp->GetHg(releaseIntervalIdx, pois);

            Float64 TO;
            pStrandGeometry->GetHighestHarpedStrandLocationEnds(segmentKey, &TO);

            // value is measured down from top of girder... we want it measured up from the bottom
            TO += Hg;

            (*p_table)(row++, col) << ecc.SetValue(TO);

            // Yb for both harped and adj str are reported. headings are from if blocks above
            Float64 HSLCL;
            pStrandGeometry->GetHighestHarpedStrandLocationHPs(segmentKey, &HSLCL);
            // value is measured down from top of girder... we want it measured up from the bottom
            HSLCL += Hg;
            (*p_table)(row++, col) << ecc.SetValue(HSLCL);
         }
      }

      if ( areAnyDebondingInTable )
      {
         if (bFirst)
            (*p_table)(row,0) << _T("NO. (# of Debonded Strands)");

         StrandIndexType ndb = pStrandGeometry->GetNumDebondedStrands(segmentKey,pgsTypes::Straight,pgsTypes::dbetEither);

         (*p_table)(row++,col) << ndb;
      }

      if (bFirst)
         (*p_table)(row,0) << Bold(_T("Concrete"));

      (*p_table)(row++,col) << Bold(_T(""));

      if (bFirst)
         (*p_table)(row,0) << _T("Release Strength ")<<RPT_FCI;

      (*p_table)(row++,col) << stress.SetValue(pMaterial->GetSegmentFc(segmentKey,releaseIntervalIdx));

      if (bFirst)
         (*p_table)(row,0) << _T("Minimum 28 day compressive strength ")<<RPT_FC;

      (*p_table)(row++,col) << stress.SetValue(pMaterial->GetSegmentFc28(segmentKey));

      if (bFirst)
         (*p_table)(row,0) << Bold(_T("Optional Design"));

      (*p_table)(row++,col) << Bold(_T(""));

      const pgsFlexuralStressArtifact* pArtifact;
      Float64 fcTop = 0.0, fcBot = 0.0, ftTop = 0.0, ftBot = 0.0;

      StressCheckTask task;
      task.intervalIdx = lastIntervalIdx;
      task.limitState = pgsTypes::ServiceI;
      task.stressType = pgsTypes::Compression;
      task.bIncludeLiveLoad = true;

      const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi( task,pmidere.GetID() );
      fcTop = pArtifact->GetExternalEffects(pgsTypes::TopGirder);
      fcBot = pArtifact->GetExternalEffects(pgsTypes::BottomGirder);

      if (bFirst)
         (*p_table)(row,0) << _T("Design Load Compressive Stress (Top CL)");

      (*p_table)(row++,col) << stress.SetValue(-fcTop);

      task.intervalIdx = lastIntervalIdx;
      task.limitState = pgsTypes::ServiceIII;
      task.stressType = pgsTypes::Tension;
      task.bIncludeLiveLoad = true;

      pArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi( task,pmidere.GetID() );
      ftTop = pArtifact->GetExternalEffects(pgsTypes::TopGirder);
      ftBot = pArtifact->GetExternalEffects(pgsTypes::BottomGirder);

      if (bFirst)
         (*p_table)(row,0) << _T("Design Load Tensile Stress (Bottom CL)");

      (*p_table)(row++,col) << stress.SetValue(-ftBot);

      const MINMOMENTCAPDETAILS* pmmcd = pMomentCapacity->GetMinMomentCapacityDetails(lastIntervalIdx,pmidere,true);

      if (bFirst)
         (*p_table)(row,0) << _T("Required minimum ultimate moment capacity ");

      (*p_table)(row++,col) << moment.SetValue( Max(pmmcd->Mu,pmmcd->MrMin) );

      if ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims )
      {
         if (bFirst)
            (*p_table)(row,0) << _T("Live Load Distribution Factor for Moment");

         (*p_table)(row++,col) << df.SetValue(pDistFact->GetMomentDistFactor(spanKey,pgsTypes::StrengthI));

         if (bFirst)
            (*p_table)(row,0) << _T("Live Load Distribution Factor for Shear");

         (*p_table)(row++,col) << df.SetValue(pDistFact->GetShearDistFactor(spanKey,pgsTypes::StrengthI));
      }
      else
      {
         if (bFirst)
            (*p_table)(row,0) << _T("Live Load Distribution Factor for Moment (Strength and Service Limit States)");

         (*p_table)(row++,col) << df.SetValue(pDistFact->GetMomentDistFactor(spanKey,pgsTypes::StrengthI));

         if (bFirst)
            (*p_table)(row,0) << _T("Live Load Distribution Factor for Shear (Strength and Service Limit States)");

         (*p_table)(row++,col) << df.SetValue(pDistFact->GetShearDistFactor(spanKey,pgsTypes::StrengthI));

         if (bFirst)
            (*p_table)(row,0) << _T("Live Load Distribution Factor for Moment (Fatigue Limit States)");

         (*p_table)(row++,col) << df.SetValue(pDistFact->GetMomentDistFactor(spanKey,pgsTypes::FatigueI));
      }

      bFirst = false;
      col++;
   }

   (*p) << rptNewLine;
   (*p) << color(Red) <<_T("NOTE: Stresses show in the above table reflect the following sign convention:") << rptNewLine 
        << _T("Compressive Stress is positive. Tensile Stress is negative") << color(Black) << rptNewLine;
}

