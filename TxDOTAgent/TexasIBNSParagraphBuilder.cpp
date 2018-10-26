///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <Reporting\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>

#include "TexasIBNSParagraphBuilder.h"

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\DebondUtil.h>

#include <psgLib\SpecLibraryEntry.h>
#include <psgLib\GirderLibraryEntry.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\MomentCapacity.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\DistributionFactors.h>

#include "TxDOTOptionalDesignUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/// Inline functions
inline bool IsNonStandardStrands(StrandIndexType nperm, bool isHarpedDesign, int npsType)
{
   if (nperm>0)
   {
      return npsType == NPS_DIRECT_SELECTION ||
            (isHarpedDesign && npsType != NPS_TOTAL_NUMBER );
   }
   else
      return false;
}

static void WriteGirderScheduleTable(rptParagraph* p, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits,
                                     const std::vector<SpanGirderHashType>& spanGirders, ColumnIndexType startIdx, ColumnIndexType endIdx,
                                     IStrandGeometry* pStrandGeometry, IGirderData* pGirderData, IPointOfInterest* pPointOfInterest,
                                     const CBridgeDescription* pBridgeDesc, IArtifact* pIArtifact, ILiveLoadDistributionFactors* pDistFact,
                                     IBridgeMaterial* pMaterial, IMomentCapacity* pMomentCapacity,
                                     bool bUnitsSI, bool areAnyTempStrands, bool areAllHarpedStrandsStraight, bool areAnyHarpedStrands);


/****************************************************************************
CLASS	CTexasIBNSParagraphBuilder
****************************************************************************/


class TxDOTIBNSDebondWriter : public TxDOTDebondTool
{
public:
   TxDOTIBNSDebondWriter(SpanIndexType span, GirderIndexType gdr, Float64 girderLength, IStrandGeometry* pStrandGeometry):
   TxDOTDebondTool(span, gdr, girderLength, pStrandGeometry)
   {;}

   void WriteDebondData(rptParagraph* pPara,IEAFDisplayUnits* pDisplayUnits);
};

void TxDOTIBNSDebondWriter::WriteDebondData(rptParagraph* pPara,IEAFDisplayUnits* pDisplayUnits)
{
   *pPara<<rptNewLine; // make some space

   // build data structures
   Compute();

   StrandIndexType nss = m_pStrandGeometry->GetNumStrands(m_Span,m_Girder,pgsTypes::Straight);

   // see if we have an error condition - don't build table if so
   if (nss==0 || m_OutCome==SectionMismatch || m_OutCome==TooManySections || m_OutCome==SectionsNotSymmetrical)
   {
      *pPara <<bold(ON)<< _T("Debonding Information for Span ") << LABEL_SPAN(m_Span) << _T(" Girder ") << LABEL_GIRDER(m_Girder) << bold(OFF) << rptNewLine;

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
         ATLASSERT(0); // A new outcome we aren't aware of?
      }
   }
   else
   {
      // All is ok to write table
      INIT_UV_PROTOTYPE( rptLengthUnitValue, uloc, pDisplayUnits->GetSpanLengthUnit(), true);
      INIT_UV_PROTOTYPE( rptLengthUnitValue, ucomp,    pDisplayUnits->GetComponentDimUnit(), true );

      uloc.SetFormat(sysNumericFormatTool::Automatic);

      const ColumnIndexType num_cols=13;
      std::_tostringstream os;
      os <<_T("Debonded Strand Pattern for Span ")<<LABEL_SPAN(m_Span)<<_T(" Girder ")<<LABEL_GIRDER(m_Girder);

      rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(num_cols,os.str());
      *pPara << p_table;

      // This table has a very special header
      p_table->SetNumberOfHeaderRows(2);

      RowIndexType row = 0;

      p_table->SetRowSpan(row,0,2);
      (*p_table)(row,0) << _T("Dist from Bottom");

      p_table->SetColumnSpan(row,1,2);
      (*p_table)(row,1) << _T("No. Strands");

      p_table->SetColumnSpan(row,2,10);
      (*p_table)(row,2) << _T("Number of Strands Debonded To");

      // null remaining cells in this row
      ColumnIndexType ic;
      for (ic = 3; ic < num_cols; ic++)
      {
         p_table->SetColumnSpan(row,ic,SKIP_CELL);
      }

      // next row of header
      p_table->SetColumnSpan(++row,0,SKIP_CELL); 

      (*p_table)(row,1) << Bold(_T("Total"));
      (*p_table)(row,2) << Bold(_T("Debonded"));

      Int16 loc_inc=1;
      for (ic = 3; ic < num_cols; ic++)
      {
         Float64 loc = m_SectionSpacing*loc_inc;
         (*p_table)(row,ic) <<Bold( uloc.SetValue(loc) );
         loc_inc++;
      }

      if (m_NumDebonded == 0)
      {
         // no debonded strands, just write one row
         row++;

         std::vector<StrandIndexType> vss  = m_pStrandGeometry->GetStrandsInRow(m_Span,m_Girder,0,pgsTypes::Straight);
         ATLASSERT(vss.size()>0);

         // get y of any strand in row
         pgsPointOfInterest poi(m_Span,m_Girder, m_GirderLength/2.0);
         CComPtr<IPoint2dCollection> coords;
         m_pStrandGeometry->GetStrandPositions(poi, pgsTypes::Straight, &coords);

         // get elevation of strand
         CComPtr<IPoint2d> point;
         coords->get_Item(vss[0],&point);
         Float64 curr_y;
         point->get_Y(&curr_y);

         (*p_table)(row,0) << ucomp.SetValue(curr_y);
         (*p_table)(row,1) << (long)vss.size();

         // rest of colums are zeros
         for (ColumnIndexType icol = 2; icol < num_cols; icol++)
         {
            (*p_table)(row,icol) << (long)0;
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

            if( !rowdata.m_Sections.empty()) // Only write row if it has debonding
            {
               row++; // table 

               (*p_table)(row,0) << ucomp.SetValue(rowdata.m_Elevation);

               StrandIndexType nsrow = m_pStrandGeometry->GetNumStrandInRow(m_Span,m_Girder,nrow,pgsTypes::Straight);
               (*p_table)(row,1) << nsrow;

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

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTexasIBNSParagraphBuilder::CTexasIBNSParagraphBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================

/*--------------------------------------------------------------------*/
rptParagraph* CTexasIBNSParagraphBuilder::Build(IBroker*	pBroker, const std::vector<SpanGirderHashType>& spanGirders,
                                                IEAFDisplayUnits* pDisplayUnits, Uint16	level) const
{
   rptParagraph* p = new rptParagraph;

   bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   GET_IFACE2(pBroker, IGirderData, pGirderData);
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );
   GET_IFACE2(pBroker, IBridgeMaterial, pMaterial);
   GET_IFACE2(pBroker,IMomentCapacity,pMomentCapacity);
   GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest );
   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SpanIndexType span;
   GirderIndexType girder;

   // First thing - check if we can generate the girder schedule table at all.
   bool areAnyHarpedStrands(false), areAnyBentHarpedStrands(false);
   bool areAnyTempStrands(false), areAnyDebonding(false);
   std::vector<SpanGirderHashType>::const_iterator itsg_end(spanGirders.end());
   std::vector<SpanGirderHashType>::const_iterator itsg(spanGirders.begin());
   while(itsg != itsg_end)
   {
      UnhashSpanGirder(*itsg,&span,&girder);

      StrandIndexType nh = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Harped);
      if (nh>0)
      {
         areAnyHarpedStrands = true;

         // check that eccentricity is same at ends and mid-girder
         pgsPointOfInterest pois(span,girder,0.0);
         std::vector<pgsPointOfInterest> pmid = pPointOfInterest->GetPointsOfInterest(span, girder,pgsTypes::BridgeSite1, POI_MIDSPAN);
         CHECK(pmid.size()==1);

         Float64 nEff;
         Float64 hs_ecc_end = pStrandGeometry->GetHsEccentricity(pois, &nEff);
         Float64 hs_ecc_mid = pStrandGeometry->GetHsEccentricity(pmid[0], &nEff);
         if (! IsEqual(hs_ecc_end, hs_ecc_mid) )
         {
            areAnyBentHarpedStrands = true;
         }
      }

      areAnyTempStrands   |= 0 < pStrandGeometry->GetMaxStrands(span,girder,pgsTypes::Temporary);
      areAnyDebonding     |= 0 < pStrandGeometry->GetNumDebondedStrands(span,girder,pgsTypes::Straight);

      itsg++;
   }

   bool areAllHarpedStrandsStraight = areAnyHarpedStrands && !areAnyBentHarpedStrands;

   // Cannot have Harped and Debonded strands in same report
   if (areAnyHarpedStrands && areAnyBentHarpedStrands && areAnyDebonding)
   {
      *p << color(Red) << bold(ON) <<_T("Note: The TxDOT Girder Schedule report cannot be generated with mixed harped and debonded designs.")
         << _T("Please select other (similar) girders and try again.") << bold(OFF) << color(Black) << rptNewLine;
   }
   else
   {
      // First notes, if non-standard design for any girders
      bool wasNS(false);
      itsg = spanGirders.begin();
      while(itsg != itsg_end)
      {
         UnhashSpanGirder(*itsg,&span,&girder);

         StrandIndexType ns = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Straight);
         StrandIndexType nh = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Harped);

         bool isHarpedDesign = 0 < pStrandGeometry->GetMaxStrands(span, girder, pgsTypes::Harped);
         const CGirderData* pgirderData = pGirderData->GetGirderData(span, girder);

         int npsType = pgirderData->PrestressData.GetNumPermStrandsType();

         if ( IsNonStandardStrands( ns+nh, isHarpedDesign, npsType) )
         {
            *p << color(Red) << _T("Note: A Non-Standard Strand Fill Was Used For Span ")
               << LABEL_SPAN(span) << _T(" Girder ") << LABEL_GIRDER(girder) << color(Black) << rptNewLine;

            wasNS = true;
         }

         itsg++;
      }

      if (wasNS)
      {
         *p << rptNewLine; // add a separator for cleaness
      }

      // Main Girder Schedule Table(s)
      // Compute a list of tables to be created. Each item in the list is the number of columns for
      // each table
      std::list<ColumnIndexType> table_list = ComputeTableCols(spanGirders);

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

         WriteGirderScheduleTable(p, pBroker, pDisplayUnits, spanGirders, start_idx, end_idx,
                                     pStrandGeometry, pGirderData, pPointOfInterest,
                                     pBridgeDesc, pIArtifact, pDistFact, pMaterial, pMomentCapacity,
                                     bUnitsSI, areAnyTempStrands, areAllHarpedStrandsStraight, areAnyHarpedStrands);
      }

      // Write debond table(s)
      if (!areAnyHarpedStrands)
      {
         itsg = spanGirders.begin();
         while(itsg != itsg_end)
         {
            UnhashSpanGirder(*itsg,&span,&girder);

            WriteDebondTable(p, pBroker, span, girder, pDisplayUnits);

            itsg++;
         }
      }

      // Write non-standard designs tables
      itsg = spanGirders.begin();
      while(itsg != itsg_end)
      {
         UnhashSpanGirder(*itsg,&span,&girder);

         StrandIndexType ns = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Straight);
         StrandIndexType nh = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Harped);

         bool isHarpedDesign = 0 < pStrandGeometry->GetMaxStrands(span, girder, pgsTypes::Harped);
         const CGirderData* pgirderData = pGirderData->GetGirderData(span, girder);

         int npsType = pgirderData->PrestressData.GetNumPermStrandsType();

         if ( IsNonStandardStrands( ns+nh, isHarpedDesign, npsType) )
         {
            // Nonstandard strands table
            std::vector<pgsPointOfInterest> pmid = pPointOfInterest->GetPointsOfInterest(span, girder,pgsTypes::BridgeSite1, POI_MIDSPAN);
            ATLASSERT(pmid.size()==1);

            OptionalDesignHarpedFillUtil::StrandRowSet strandrows = OptionalDesignHarpedFillUtil::GetStrandRowSet(pBroker, pmid[0]);

            std::_tostringstream os;
            os <<_T("Non-Standard Strand Pattern for Span ")<<LABEL_SPAN(span)<<_T(" Girder ")<<LABEL_GIRDER(girder);

            rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(2, os.str());
            p_table->SetColumnWidth(0,1.0);
            p_table->SetColumnWidth(1,1.8);
            *p << rptNewLine << p_table;

            RowIndexType row = 0;
            (*p_table)(row,0) << _T("Row")<<rptNewLine<<_T("(in)"); // TxDOT dosn't do metric and we need special formatting below
            (*p_table)(row++,1) << _T("Strands");

            for (OptionalDesignHarpedFillUtil::StrandRowIter srit=strandrows.begin(); srit!=strandrows.end(); srit++)
            {
               const OptionalDesignHarpedFillUtil::StrandRow& srow = *srit;
               Float64 elev_in = RoundOff(::ConvertFromSysUnits( srow.Elevation, unitMeasure::Inch ),0.001);

               (*p_table)(row,0) << elev_in; 
               (*p_table)(row++,1) << srow.fillListString << _T( " (") << srow.fillListString.size()*2 << _T(")");
            }
         }

         itsg++;
      }
   }

   return p;
}

void CTexasIBNSParagraphBuilder::WriteDebondTable(rptParagraph* pPara, IBroker* pBroker, SpanIndexType span,GirderIndexType girder, IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );

   bool bCanDebond = pStrandGeometry->CanDebondStrands(span,girder,pgsTypes::Straight);
   bCanDebond     |= pStrandGeometry->CanDebondStrands(span,girder,pgsTypes::Harped);
   bCanDebond     |= pStrandGeometry->CanDebondStrands(span,girder,pgsTypes::Temporary);

   if ( !bCanDebond )
      return;

   Float64 girder_length = pBridge->GetGirderLength(span, girder);

   // Need compute tool to decipher debond data
   TxDOTIBNSDebondWriter tx_writer(span, girder, girder_length, pStrandGeometry);

   tx_writer.WriteDebondData(pPara,pDisplayUnits);


}

void WriteGirderScheduleTable(rptParagraph* p, IBroker* pBroker, IEAFDisplayUnits* pDisplayUnits,
                              const std::vector<SpanGirderHashType>& spanGirders, ColumnIndexType startIdx, ColumnIndexType endIdx,
                              IStrandGeometry* pStrandGeometry, IGirderData* pGirderData, IPointOfInterest* pPointOfInterest,
                              const CBridgeDescription* pBridgeDesc, IArtifact* pIArtifact, ILiveLoadDistributionFactors* pDistFact,
                              IBridgeMaterial* pMaterial, IMomentCapacity* pMomentCapacity,
                              bool bUnitsSI, bool areAnyTempStrands, bool areAllHarpedStrandsStraight, bool areAnyHarpedStrands)
{
   CollectionIndexType ng = endIdx-startIdx+1;
   rptRcTable* p_table = pgsReportStyleHolder::CreateTableNoHeading(ng+1,_T("TxDOT Girder Schedule"));

   *p << p_table;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, ecc,    pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dia,    pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(),       true );
   rptRcScalar df;
   df.SetFormat(sysNumericFormatTool::Fixed);
   df.SetWidth(8);
   df.SetPrecision(5);

   bool bFirst(true);
   ColumnIndexType col = 1;
   for (ColumnIndexType gdr_idx=startIdx; gdr_idx<=endIdx; gdr_idx++)
   {
      SpanIndexType span;
      GirderIndexType girder;

      SpanGirderHashType hash = spanGirders[gdr_idx];
      UnhashSpanGirder(hash,&span,&girder);

      StrandIndexType ns = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Straight);
      StrandIndexType nh = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Harped);

      const CGirderData* pgirderData = pGirderData->GetGirderData(span, girder);
      const matPsStrand* pstrand = pGirderData->GetStrandMaterial(span,girder,pgsTypes::Permanent);

      // create pois at the start of girder and mid-span
      pgsPointOfInterest pois(span,girder,0.0);
      std::vector<pgsPointOfInterest> pmid = pPointOfInterest->GetPointsOfInterest(span, girder,pgsTypes::BridgeSite1, POI_MIDSPAN);
      CHECK(pmid.size()==1);

      RowIndexType row = 0;
      if(bFirst)
         (*p_table)(row,0) << Bold(_T("Span"));

      (*p_table)(row++,col) << Bold(LABEL_SPAN(span));

      if(bFirst)
         (*p_table)(row,0) << Bold(_T("Girder"));

      (*p_table)(row++,col) << Bold(LABEL_GIRDER(girder));

      if(bFirst)
         (*p_table)(row,0) << _T("Girder Type");

      (*p_table)(row++,col) << pBridgeDesc->GetSpan(span)->GetGirderTypes()->GetGirderName(girder);

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

      // Determine if harped strands are straight by comparing harped eccentricity at end/mid
      bool are_harped_straight(false);
      if (nh>0)
      {
         Float64 nEff;
         Float64 hs_ecc_end = pStrandGeometry->GetHsEccentricity(pois, &nEff);
         Float64 hs_ecc_mid = pStrandGeometry->GetHsEccentricity(pmid[0], &nEff);
         are_harped_straight = IsEqual(hs_ecc_end, hs_ecc_mid);
      }

      if(bFirst)
         (*p_table)(row,0) << _T("NO. (N") << Sub(_T("h")) << _T(" + N") << Sub(_T("s")) << _T(")");

      (*p_table)(row++,col) << Int16(nh + ns);

      if(bFirst)
         (*p_table)(row,0) << _T("Size");

      (*p_table)(row++,col) << dia.SetValue(pstrand->GetNominalDiameter()) << _T(" Dia.");

      if(bFirst)
         (*p_table)(row,0) << _T("Strength");

      std::_tstring strData;
      std::_tstring strGrade;
      if ( bUnitsSI )
      {
         strGrade = (pstrand->GetGrade() == matPsStrand::Gr1725 ? _T("1725") : _T("1860"));
         strData = _T("Grade ") + strGrade;
         strData += _T(" ");
         strData += (pstrand->GetType() == matPsStrand::LowRelaxation ? _T("Low Relaxation") : _T("Stress Relieved"));
      }
      else
      {
         strGrade = (pstrand->GetGrade() == matPsStrand::Gr1725 ? _T("250") : _T("270"));
         strData = _T("Grade ") + strGrade;
         strData += _T(" ");
         strData += (pstrand->GetType() == matPsStrand::LowRelaxation ? _T("Low Relaxation") : _T("Stress Relieved"));
      }

      (*p_table)(row++,col) << strData;


      if(bFirst)
      {
         (*p_table)(row,0) << _T("Eccentricity @ CL");
         if ( areAnyTempStrands )
            (*p_table)(row,0) << _T(" (w/o Temporary Strands)");
      }

      double nEff;
      (*p_table)(row++,col) << ecc.SetValue( pStrandGeometry->GetEccentricity( pmid[0], false, &nEff ) );

      if(bFirst)
      {
         (*p_table)(row,0) << _T("Eccentricity @ End");
         if ( areAnyTempStrands )
            (*p_table)(row,0) << _T(" (w/o Temporary Strands)");
      }

      (*p_table)(row++,col) << ecc.SetValue( pStrandGeometry->GetEccentricity( pois, false, &nEff ) );

      StrandIndexType ndb = pStrandGeometry->GetNumDebondedStrands(span,girder,pgsTypes::Straight);

      if(bFirst)
      {
         (*p_table)(row,0) << Bold(_T("Prestressing Strands"));
      }

      if (areAnyHarpedStrands)
      {
         if (areAllHarpedStrandsStraight)
         {
            (*p_table)(row++,col) << Bold(_T("Straight"));

            if (bFirst)
               (*p_table)(row,0) << _T("NO. (# of Harped Strands)");

            (*p_table)(row++,col) << 0; // by definition
         }
         else
         {
            (*p_table)(row++,col) << Bold(_T("Depressed"));

            if (bFirst)
               (*p_table)(row,0) << _T("NO. (# of Harped Strands)");

            (*p_table)(row++,col) << nh;

            if (bFirst)
               (*p_table)(row,0) << _T("Y")<<Sub(_T("b"))<<_T(" of Topmost Depressed Strand(s) @ End");

            double TO;
            pStrandGeometry->GetHighestHarpedStrandLocation(span,girder,&TO);
            (*p_table)(row++,col) << ecc.SetValue(TO);
         }
      }
      else
      {
         // no harped strands, assume debond design
         (*p_table)(row++,col) << Bold(_T("Debonded"));

         if (bFirst)
            (*p_table)(row,0) << _T("NO. (# of Debonded Strands)");

         (*p_table)(row++,col) << ndb;
      }

      if (bFirst)
         (*p_table)(row,0) << Bold(_T("Concrete"));

      (*p_table)(row++,col) << Bold(_T(""));

      if (bFirst)
         (*p_table)(row,0) << _T("Release Strength ")<<RPT_FCI;

      (*p_table)(row++,col) << stress.SetValue(pMaterial->GetFciGdr(span,girder));

      if (bFirst)
         (*p_table)(row,0) << _T("Minimum 28 day compressive strength ")<<RPT_FC;

      (*p_table)(row++,col) << stress.SetValue(pMaterial->GetFcGdr(span,girder));

      if (bFirst)
         (*p_table)(row,0) << Bold(_T("Optional Design"));

      (*p_table)(row++,col) << Bold(_T(""));

      const pgsFlexuralStressArtifact* pArtifact;
      double fcTop = 0.0, fcBot = 0.0, ftTop = 0.0, ftBot = 0.0;


      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetArtifact(span,girder);
      pArtifact = pGdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite3,
                                                           pgsTypes::ServiceI,pgsTypes::Compression,pmid[0].GetDistFromStart()) );
      pArtifact->GetExternalEffects( &fcTop, &fcBot );

      if (bFirst)
         (*p_table)(row,0) << _T("Design Load Compressive Stress (Top CL)");

      (*p_table)(row++,col) << stress.SetValue(-fcTop);

      pArtifact = pGdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension,pmid[0].GetDistFromStart()) );
      pArtifact->GetExternalEffects( &ftTop, &ftBot );

      if (bFirst)
         (*p_table)(row,0) << _T("Design Load Tensile Stress (Bottom CL)");

      (*p_table)(row++,col) << stress.SetValue(-ftBot);

      //const pgsFlexuralCapacityArtifact* pFlexureArtifact = pGdrArtifact->GetFlexuralCapacityArtifact( pgsFlexuralCapacityArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,pmid[0].GetDistFromStart()) );
      MINMOMENTCAPDETAILS mmcd;
      pMomentCapacity->GetMinMomentCapacityDetails(pgsTypes::BridgeSite3,pmid[0],true,&mmcd);

      if (bFirst)
         (*p_table)(row,0) << _T("Required minimum ultimate moment capacity ");

      (*p_table)(row++,col) << moment.SetValue( _cpp_max(mmcd.Mu,mmcd.MrMin) );

      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      {
         if (bFirst)
            (*p_table)(row,0) << _T("Live Load Distribution Factor for Moment");

         (*p_table)(row++,col) << df.SetValue(pDistFact->GetMomentDistFactor(span,girder,pgsTypes::StrengthI));

         if (bFirst)
            (*p_table)(row,0) << _T("Live Load Distribution Factor for Shear");

         (*p_table)(row++,col) << df.SetValue(pDistFact->GetShearDistFactor(span,girder,pgsTypes::StrengthI));
      }
      else
      {
         if (bFirst)
            (*p_table)(row,0) << _T("Live Load Distribution Factor for Moment (Strength and Service Limit States)");

         (*p_table)(row++,col) << df.SetValue(pDistFact->GetMomentDistFactor(span,girder,pgsTypes::StrengthI));

         if (bFirst)
            (*p_table)(row,0) << _T("Live Load Distribution Factor for Shear (Strength and Service Limit States)");

         (*p_table)(row++,col) << df.SetValue(pDistFact->GetShearDistFactor(span,girder,pgsTypes::StrengthI));

         if (bFirst)
            (*p_table)(row,0) << _T("Live Load Distribution Factor for Moment (Fatigue Limit States)");

         (*p_table)(row++,col) << df.SetValue(pDistFact->GetMomentDistFactor(span,girder,pgsTypes::FatigueI));
      }

      bFirst = false;
      col++;
   }

   (*p) << rptNewLine;
   (*p) << color(Red) <<_T("NOTE: Stresses show in the above table reflect the following sign convention:") << rptNewLine 
        << _T("Compressive Stress is positive. Tensile Stress is negative") << color(Black) << rptNewLine;
}

