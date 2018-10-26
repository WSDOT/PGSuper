///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#include <IFace\DisplayUnits.h>
#include <IFace\MomentCapacity.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\DistributionFactors.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS	CTexasIBNSParagraphBuilder
****************************************************************************/


class TxDOTIBNSDebondWriter : public TxDOTDebondTool
{
public:
   TxDOTIBNSDebondWriter(int span, int gdr, Float64 girderLength, IStrandGeometry* pStrandGeometry):
   TxDOTDebondTool(span, gdr, girderLength, pStrandGeometry)
   {;}

   void WriteDebondData(rptParagraph* pPara,IDisplayUnits* pDisplayUnits);
};

void TxDOTIBNSDebondWriter::WriteDebondData(rptParagraph* pPara,IDisplayUnits* pDisplayUnits)
{
   *pPara<<rptNewLine; // make some space

   // build data structures
   Compute();

   StrandIndexType nss = m_pStrandGeometry->GetNumStrands(m_Span,m_Girder,pgsTypes::Straight);


   // see if we have an error condition - don't build table if so
   if (nss==0 || m_OutCome==SectionMismatch || m_OutCome==TooManySections || m_OutCome==SectionsNotSymmetrical)
   {
      *pPara <<Bold("Debonding Information")<<rptNewLine;

      if(nss==0)
      {
         *pPara<< color(Red) <<"Warning: No straight strands in girder. Cannot write debonding information."<<color(Black)<<rptNewLine;
      }
      else if(m_OutCome==SectionMismatch)
      {
         *pPara<< color(Red) <<"Warning: Irregular, Non-standard debonding increments used. Cannot write debonding information."<<color(Black)<<rptNewLine;
      }
      else if (m_OutCome==TooManySections)
      {
         *pPara<< color(Red) <<"Warning: More than ten debonding increments exist. Cannot write debonding information."<<color(Black)<<rptNewLine;
      }
      else if (m_OutCome==SectionsNotSymmetrical)
      {
         *pPara<< color(Red) <<"Warning: Debond sections are not symmetic about girder mid-span. Cannot write debonding information."<<color(Black)<<rptNewLine;
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
      rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(num_cols,"Debonded Strand Pattern");
      *pPara << p_table;

      // This table has a very special header
      p_table->SetNumberOfHeaderRows(2);

      RowIndexType row = 0;

      p_table->SetRowSpan(row,0,2);
      (*p_table)(row,0) << "Dist from Bottom";

      p_table->SetColumnSpan(row,1,2);
      (*p_table)(row,1) << "No. Strands";

      p_table->SetColumnSpan(row,2,10);
      (*p_table)(row,2) << "Number of Strands Debonded To";

      // null remaining cells in this row
      ColumnIndexType ic;
      for (ic = 3; ic < num_cols; ic++)
      {
         p_table->SetColumnSpan(row,ic,-1);
      }

      // next row of header
      p_table->SetColumnSpan(++row,0,-1); 

      (*p_table)(row,1) << Bold("Total");
      (*p_table)(row,2) << Bold("Debonded");

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
         m_pStrandGeometry->GetStrandPositionsEx(poi, nss, pgsTypes::Straight, &coords);

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

            riter++;
            nrow++;
         }

         // write note about non-standard spacing if applicable
         if (m_OutCome==NonStandardSection)
         {
            *pPara<< color(Red)<<"Warning: Non-standard debonding increment of "<<uloc.SetValue(m_SectionSpacing)<<" used."<<color(Black)<<rptNewLine;
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
rptParagraph* CTexasIBNSParagraphBuilder::Build(IBroker*	pBroker, SpanIndexType	span,GirderIndexType girder, 
                                                IDisplayUnits* pDisplayUnits, Uint16	level) const
{
   rptParagraph* p = new rptParagraph;

   bool bUnitsSI = (pDisplayUnits->GetUnitDisplayMode() == pgsTypes::umSI);

	/* For broker passed in, get interface information */
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetArtifact(span,girder);

   GET_IFACE2(pBroker, IGirderData, pGirderData);
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );
   GET_IFACE2(pBroker, IBridgeMaterial, pMaterial);
   GET_IFACE2(pBroker, ISectProp2, pSectProp2);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   bool bTempStrands = (0 < pStrandGeometry->GetMaxStrands(span,girder,pgsTypes::Temporary) ? true : false);

   CGirderData girderData = pGirderData->GetGirderData(span, girder);
   bool is_nonstandard = girderData.NumPermStrandsType != NPS_TOTAL_NUMBER;

   if (is_nonstandard)
   {
      *p << color(Red) <<"Note: A Non-Standard Strand Fill Was Used For This Design" << color(Black) << rptNewLine;
   }

   const matPsStrand* pstrand = pGirderData->GetStrandMaterial(span,girder);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, ecc,    pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dia,    pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(),       true );


   // create pois at the start of girder and mid-span
   pgsPointOfInterest pois(span,girder,0.0);
   GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest );
   std::vector<pgsPointOfInterest> pmid = pPointOfInterest->GetPointsOfInterest(pgsTypes::BridgeSite1, span, girder,POI_MIDSPAN);
   CHECK(pmid.size()==1);

   rptRcTable* p_table = pgsReportStyleHolder::CreateTableNoHeading(2,"TxDOT Girder Schedule");
   p_table->SetColumnWidth(0,3.5);
   p_table->SetColumnWidth(1,2.5);
   *p << p_table;

   RowIndexType row = 0;
   (*p_table)(row,0) << "Span";
   (*p_table)(row,1) << LABEL_SPAN(span);

   (*p_table)(++row,0) << "Girder";
   (*p_table)(row  ,1) << LABEL_GIRDER(girder);

   (*p_table)(++row,0) << "Girder Type";
   (*p_table)(row,1) << pBridgeDesc->GetSpan(span)->GetGirderTypes()->GetGirderName(girder);

   (*p_table)(++row,0) << Bold("Prestressing Strands");
   (*p_table)(row,1) << Bold("Total");

   StrandIndexType ns = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Straight);
   StrandIndexType nh = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Harped);

   (*p_table)(++row,0) << "NO. (N" << Sub("h") << " + N" << Sub("s") << ")";
   (*p_table)(row  ,1) << Int16(nh + ns);

   (*p_table)(++row,0) << "Size";
   (*p_table)(row,1) << dia.SetValue(pstrand->GetNominalDiameter()) << " Dia.";

   (*p_table)(++row,0) << "Strength";
   std::string strData;
   std::string strGrade;
   if ( bUnitsSI )
   {
      strGrade = (pstrand->GetGrade() == matPsStrand::Gr1725 ? "1725" : "1860");
      strData = "Grade " + strGrade;
      strData += " ";
      strData += (pstrand->GetType() == matPsStrand::LowRelaxation ? "Low Relaxation" : "Stress Relieved");

      (*p_table)(row,1) << strData;
   }
   else
   {
      strGrade = (pstrand->GetGrade() == matPsStrand::Gr1725 ? "250" : "270");
      strData = "Grade " + strGrade;
      strData += " ";
      strData += (pstrand->GetType() == matPsStrand::LowRelaxation ? "Low Relaxation" : "Stress Relieved");

      (*p_table)(row,1) << strData;
   }

   (*p_table)(++row,0) << "Eccentricity @ CL";
   if ( bTempStrands )
      (*p_table)(row,0) << " (w/o Temporary Strands)";

   double nEff;
   (*p_table)(row,1) << ecc.SetValue( pStrandGeometry->GetEccentricity( pmid[0], false, &nEff ) );

   (*p_table)(++row,0) << "Eccentricity @ End";
   if ( bTempStrands )
      (*p_table)(row,0) << " (w/o Temporary Strands)";

   (*p_table)(row,1) << ecc.SetValue( pStrandGeometry->GetEccentricity( pois, false, &nEff ) );

   StrandIndexType ndb = pStrandGeometry->GetNumDebondedStrands(span,girder,pgsTypes::Straight);

   (*p_table)(++row,0) << Bold("Prestressing Strands");
   if (nh>0)
   {
      (*p_table)(row,1) << Bold("Depressed");

      (*p_table)(++row,0) << "NO. (# of Harped Strands)";
      (*p_table)(row  ,1) << nh;

      double TO;
      pStrandGeometry->GetHighestHarpedStrandLocation(span,girder,&TO);

      (*p_table)(++row,0) << "Y"<<Sub("b")<<" of Topmost Depressed Strand(s) @ End";
      (*p_table)(row  ,1) << ecc.SetValue(TO);
   }
   else
   {
      // no harped strands, assume debond design
      (*p_table)(row,1) << Bold("Debonded");

      (*p_table)(++row,0) << "NO. (# of Debonded Strands)";
      (*p_table)(row  ,1) << ndb;
   }

   (*p_table)(++row,0) << Bold("Concrete");
   (*p_table)(row,1) << Bold("");

   (*p_table)(++row,0) << "Release Strength f'"<<Sub("ci");
   (*p_table)(row  ,1) << stress.SetValue(pMaterial->GetFciGdr(span,girder));

   (*p_table)(++row,0) << "Minimum 28 day compressive strength f'"<<Sub("c");
   (*p_table)(row  ,1) << stress.SetValue(pMaterial->GetFcGdr(span,girder));

   (*p_table)(++row,0) << Bold("Optional Design");
   (*p_table)(row,1) << Bold("");

   const pgsFlexuralStressArtifact* pArtifact;
   double fcTop = 0.0, fcBot = 0.0, ftTop = 0.0, ftBot = 0.0;


   pArtifact = pGdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite3,
	   pgsTypes::ServiceI,pgsTypes::Compression,pmid[0].GetDistFromStart()) );
   pArtifact->GetExternalEffects( &fcTop, &fcBot );

   (*p_table)(++row,0) << "Design Load Compressive Stress (Top CL)";
   (*p_table)(row  ,1) << stress.SetValue(-fcTop);

   pArtifact = pGdrArtifact->GetFlexuralStressArtifact( pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension,pmid[0].GetDistFromStart()) );
   pArtifact->GetExternalEffects( &ftTop, &ftBot );

   (*p_table)(++row,0) << "Design Load Tensile Stress (Bottom CL)";
   (*p_table)(row  ,1) << stress.SetValue(-ftBot);

   //const pgsFlexuralCapacityArtifact* pFlexureArtifact = pGdrArtifact->GetFlexuralCapacityArtifact( pgsFlexuralCapacityArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,pmid[0].GetDistFromStart()) );
   GET_IFACE2(pBroker,IMomentCapacity,pMomentCapacity);
   MINMOMENTCAPDETAILS mmcd;
   pMomentCapacity->GetMinMomentCapacityDetails(pgsTypes::BridgeSite3,pmid[0],true,&mmcd);

   (*p_table)(++row,0) << "Required minimum ultimate moment capacity ";
//   (*p_table)(row,1) << moment.SetValue( pFlexureArtifact->GetDemand() );
   (*p_table)(row,1) << moment.SetValue( _cpp_max(mmcd.Mu,mmcd.MrMin) );

   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);
   rptRcScalar df;
   df.SetFormat(sysNumericFormatTool::Fixed);
   df.SetWidth(8);
   df.SetPrecision(5);

   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      (*p_table)(++row,0) << "Live Load Distribution Factor for Moment";
      (*p_table)(row  ,1) << df.SetValue(pDistFact->GetMomentDistFactor(span,girder,pgsTypes::StrengthI));
   }
   else
   {
      (*p_table)(++row,0) << "Live Load Distribution Factor for Moment (Strength and Service Limit States)";
      (*p_table)(row  ,1) << df.SetValue(pDistFact->GetMomentDistFactor(span,girder,pgsTypes::StrengthI));

      (*p_table)(++row,0) << "Live Load Distribution Factor for Moment (Fatigue Limit States)";
      (*p_table)(row  ,1) << df.SetValue(pDistFact->GetMomentDistFactor(span,girder,pgsTypes::FatigueI));
   }

   (*p) << rptNewLine;
   (*p) << color(Red) <<"NOTE: Stresses show in the above table reflect the following sign convention:" << rptNewLine 
        << "Compressive Stress is positive. Tensile Stress is negative" << color(Black) << rptNewLine;

   // write debond table
   WriteDebondTable(p, pBroker, span, girder, pDisplayUnits);

   if (is_nonstandard)
   {
      // Nonstandard strands table
      StrandRowUtil::StrandRowSet strandrows = StrandRowUtil::GetStrandRowSet(pBroker, pmid[0]);

      p_table = pgsReportStyleHolder::CreateDefaultTable(2,"Non-Standard Strand Pattern");
      p_table->SetColumnWidth(0,1.3);
      p_table->SetColumnWidth(1,1.3);
      *p << p_table;

      RowIndexType row = 0;
      (*p_table)(row,0) << "Row"<<rptNewLine<<"(in)"; // TxDOT dosn't do metric and we need special formatting below
      (*p_table)(row++,1) << "# of"<<rptNewLine<<"Strands";

      for (StrandRowUtil::StrandRowIter srit=strandrows.begin(); srit!=strandrows.end(); srit++)
      {
         const StrandRowUtil::StrandRow& srow = *srit;
         Float64 elev_in = RoundOff(::ConvertFromSysUnits( srow.Elevation, unitMeasure::Inch ),0.001);

         (*p_table)(row,0) << elev_in; 
         (*p_table)(row++,1) << srow.Count;
      }
   }


   return p;
}

void CTexasIBNSParagraphBuilder::WriteDebondTable(rptParagraph* pPara, IBroker* pBroker, SpanIndexType span,GirderIndexType girder, IDisplayUnits* pDisplayUnits) const
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

