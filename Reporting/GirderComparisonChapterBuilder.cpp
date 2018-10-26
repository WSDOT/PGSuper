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
#include <Reporting\GirderComparisonChapterBuilder.h>
#include <Reporting\StirrupTable.h>

#include <IFace\DisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\GirderHandling.h>

#include <Material\PsStrand.h>

#include <PgsExt\BridgeDescription.h>
#include <PgsExt\GirderData.h>

#include <Lrfd\RebarPool.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CGirderComparisonChapterBuilder
****************************************************************************/


void girders(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDisplayUnits,SpanIndexType span);
bool prestressing(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDisplayUnits,SpanIndexType span);
void debonding(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDisplayUnits,SpanIndexType span);
void material(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDisplayUnits,SpanIndexType span);
void stirrups(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDisplayUnits,SpanIndexType span);
void handling(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDisplayUnits,SpanIndexType span);

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CGirderComparisonChapterBuilder::CGirderComparisonChapterBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CGirderComparisonChapterBuilder::GetName() const
{
   return TEXT("Girder Comparison");
}

                                               
rptChapter* CGirderComparisonChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanReportSpecification* pSpec = dynamic_cast<CSpanReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);
   SpanIndexType span = pSpec->GetSpan();

   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   girders(pChapter,pBroker,pDisplayUnits,span);
   if (prestressing(pChapter,pBroker,pDisplayUnits,span))
   {
      debonding(pChapter,pBroker,pDisplayUnits,span);
   }

   material(pChapter,pBroker,pDisplayUnits,span);
   stirrups(pChapter,pBroker,pDisplayUnits,span);
   handling(pChapter,pBroker,pDisplayUnits,span);

   return pChapter;
}

CChapterBuilder* CGirderComparisonChapterBuilder::Clone() const
{
   return new CGirderComparisonChapterBuilder;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

void girders(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDisplayUnits,SpanIndexType span)
{
   rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<"Girder Types for Span "<< LABEL_SPAN(span) << rptNewLine;

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(2,"");
   *pPara << p_table<<rptNewLine;

   int col = 0;
   (*p_table)(0,col++) << "Girder";
   (*p_table)(0,col++) << "Type";


   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

   GirderIndexType nGirders = pSpan->GetGirderCount();
   int row=1;
   for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
   {
      std::string strGirderName = pSpan->GetGirderTypes()->GetGirderName(gdrIdx);

      col = 0;
      (*p_table)(row,col++) << LABEL_GIRDER(gdrIdx);
      (*p_table)(row,col++) << strGirderName;
      row++;
   }
}

// return true if debonding exists
bool prestressing(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDisplayUnits,SpanIndexType span)
{
   bool was_debonding = false;

   GET_IFACE2(pBroker,IGirderData,pGirderData);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   SpanIndexType nspans = pBridge->GetSpanCount();
   CHECK(span<nspans);

   bool bTempStrands = (0 < pStrandGeometry->GetMaxStrands(span,0,pgsTypes::Temporary) ? true : false);


   rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<"Prestressing Strands for Span "<< LABEL_SPAN(span) <<rptNewLine;


   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,          pDisplayUnits->GetShearUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,         pDisplayUnits->GetStressUnit(),        false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(bTempStrands ? 10 : 8,"");
   *pPara << p_table<<rptNewLine;

   p_table->SetNumberOfHeaderRows(2);

   ColumnIndexType col1 = 0;
   ColumnIndexType col2 = 0;
   p_table->SetRowSpan(0,col1,2);
   (*p_table)(0,col1++) << "Girder";
   p_table->SetRowSpan(1,col2++,-1);

   p_table->SetRowSpan(0,col1,2);
   (*p_table)(0,col1++) << "Material";
   p_table->SetRowSpan(1,col2++,-1);

   ColumnIndexType nSkipCols = 1;
   p_table->SetColumnSpan(0,col1,2);
   (*p_table)(0,col1++) << "Straight Strands";
   (*p_table)(1,col2++) << "# Total"<<rptNewLine<<"(Debonded)";
   (*p_table)(1,col2++) << COLHDR(Sub2("P","jack"),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());

   p_table->SetColumnSpan(0,col1,2);
   (*p_table)(0,col1++) << "Harped Strands";
   (*p_table)(1,col2++) << "#";
   (*p_table)(1,col2++) << COLHDR(Sub2("P","jack"),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
   nSkipCols++;

   if ( bTempStrands )
   {
      p_table->SetColumnSpan(0,col1,2);
      (*p_table)(0,col1++) << "Temporary Strands";
      (*p_table)(1,col2++) << "#";
      (*p_table)(1,col2++) << COLHDR(Sub2("P","jack"),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
      nSkipCols++;
   }

   for ( ColumnIndexType i = 0; i < nSkipCols; i++ )
   {
      p_table->SetColumnSpan(0,col1+i,-1);
   }
   col1 += nSkipCols;


   p_table->SetRowSpan(0,col1,2);
   (*p_table)(0,col1++) << COLHDR("Girder End"<<rptNewLine<<"Offset",rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   p_table->SetRowSpan(1,col2++,-1);

   p_table->SetRowSpan(0,col1,2);
   (*p_table)(0,col1++)<< COLHDR("Harping Pt"<<rptNewLine<<"Offset",rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   p_table->SetRowSpan(1,col2++,-1);


   GirderIndexType ngirds = pBridge->GetGirderCount(span);
   int row=p_table->GetNumberOfHeaderRows();
   for (GirderIndexType ig=0; ig<ngirds; ig++)
   {
      CGirderData girderData = pGirderData->GetGirderData(span,ig);

      ColumnIndexType col = 0;
      (*p_table)(row,col++) << LABEL_GIRDER(ig);
      (*p_table)(row,col++) << girderData.Material.pStrandMaterial->GetName();
      (*p_table)(row,col) << girderData.Nstrands[pgsTypes::Straight];

      long nd = pStrandGeometry->GetNumDebondedStrands(span,ig,pgsTypes::Straight);
      if (nd>0)
      {
         (*p_table)(row,col) << " ("<<nd<<")";
         was_debonding = true;     
      }
      col++;

      (*p_table)(row,col++) << force.SetValue(girderData.Pjack[pgsTypes::Straight]);
      (*p_table)(row,col++) << girderData.Nstrands[pgsTypes::Harped];
      (*p_table)(row,col++) << force.SetValue(girderData.Pjack[pgsTypes::Harped]);
      if ( bTempStrands )
      {
         (*p_table)(row,col++) << girderData.Nstrands[pgsTypes::Temporary];
         (*p_table)(row,col++) << force.SetValue(girderData.Pjack[pgsTypes::Temporary]);
      }

      // convert to absolute adjustment
      double adjustment = pStrandGeometry->ComputeAbsoluteHarpedOffsetEnd(span, ig, girderData.Nstrands[pgsTypes::Harped], girderData.HsoEndMeasurement, girderData.HpOffsetAtEnd);
      (*p_table)(row,col++) << dim.SetValue(adjustment);

      adjustment = pStrandGeometry->ComputeAbsoluteHarpedOffsetHp(span, ig, girderData.Nstrands[pgsTypes::Harped], girderData.HsoHpMeasurement, girderData.HpOffsetAtHp);
      (*p_table)(row,col++) << dim.SetValue(adjustment);
      row++;
   }

   *pPara<<"Girder End Offset - Distance the HS bundle at the girder ends is adjusted vertically from original library location."<<rptNewLine;
   *pPara<<"Harping Point Offset - Distance the HS bundle at the harping point is adjusted  vertically from original library location."<<rptNewLine;
   return was_debonding;
}

   // Utility classes for storing and retreiving debond section information
   struct DebondSectionData
   {
      Float64 m_EndDistance;
      Uint32  m_DebondCount;

      DebondSectionData(Float64 endDist):
      m_EndDistance(endDist), m_DebondCount(1) 
      {;}

      bool operator==(const DebondSectionData& rOther) const
      { 
         return ::IsEqual(m_EndDistance,rOther.m_EndDistance);
      }

      bool operator<(const DebondSectionData& rOther) const 
      { 
         return m_EndDistance < rOther.m_EndDistance; 
      }

   private:
      DebondSectionData()
      {;}
   };

   typedef std::set<DebondSectionData> DebondSectionSet;
   typedef DebondSectionSet::iterator DebondSectionSetIter;


   struct DebondRowData
   {
      Float64 m_Elevation;
      DebondSectionSet m_DebondSections;

      DebondRowData(Float64 elevation):
      m_Elevation(elevation)
      {;}

      Uint32 GetNumDebonds()
      {
         Uint32 num=0;
         for (DebondSectionSetIter it=m_DebondSections.begin(); it!=m_DebondSections.end(); it++)
         {
            num += it->m_DebondCount;
         }

         return num;
      }

      bool operator==(const DebondRowData& rOther) const
      { 
         return ::IsEqual(m_Elevation,rOther.m_Elevation);
      }

      bool operator<(const DebondRowData& rOther) const 
      { 
         return m_Elevation < rOther.m_Elevation; 
      }
   };

   typedef std::set<DebondRowData> DebondRowDataSet;
   typedef DebondRowDataSet::iterator DebondRowDataSetIter;

   class DebondComparison
   {
   public:
      // status of debonding
      enum DebondStatus {IsDebonding, NonSymmetricDebonding, NoDebonding};

      DebondStatus Init(SpanIndexType span, GirderIndexType ngirds, IStrandGeometry* pStrandGeometry);

      // get debond row data for a given girder
      DebondRowDataSet GetDebondRowDataSet(Uint32 gdr)
      {
         assert(gdr<m_DebondRowData.size());
         return m_DebondRowData[gdr];
      }

      std::set<Float64> m_SectionLocations;

   private:
      std::vector<DebondRowDataSet> m_DebondRowData;
   };

DebondComparison::DebondStatus DebondComparison::Init(SpanIndexType span, GirderIndexType ngirds, IStrandGeometry* pStrandGeometry)
{
   assert(m_DebondRowData.empty());

   int total_num_debonded=0;

   for (GirderIndexType gdr=0; gdr<ngirds; gdr++)
   {
      if (!pStrandGeometry->IsDebondingSymmetric(span,gdr))
      {
         // can't do for non-symetrical strands
         return NonSymmetricDebonding;
      }

      m_DebondRowData.push_back(DebondRowDataSet());
      DebondRowDataSet& row_data_set = m_DebondRowData.back();

      CComPtr<IPoint2dCollection> strand_coords;
      pStrandGeometry->GetStrandPositions(pgsPointOfInterest(span,gdr,0.0), pgsTypes::Straight, &strand_coords);

      CollectionIndexType num_strands;
      strand_coords->get_Count(&num_strands);

      for (StrandIndexType istrand=0; istrand<num_strands; istrand++)
      {
         Float64 dist_start, dist_end;
         if (pStrandGeometry->IsStrandDebonded(span, gdr, istrand, pgsTypes::Straight, &dist_start, &dist_end))
         {
            total_num_debonded++;

            // blindly insert section location
            m_SectionLocations.insert(dist_start);

            // Get to building row data
            Float64 elev;
            CComPtr<IPoint2d> point;
            strand_coords->get_Item(istrand, &point);
            point->get_Y(&elev);

            DebondRowDataSetIter row_iter = row_data_set.find( DebondRowData(elev) );
            if (row_iter != row_data_set.end())
            {
               // found row, now look for end distance
               DebondSectionData section_data(dist_start);
               DebondSectionSetIter section_iter = row_iter->m_DebondSections.find(section_data);
               if (section_iter!=row_iter->m_DebondSections.end())
               {
                  // existing section/row with debonds - increment count
                  section_iter->m_DebondCount++;
               }
               else
               {
                  // new section location in row
                  row_iter->m_DebondSections.insert(section_data);
               }
            }
            else
            {
               // create a new row
               DebondRowData row_data(elev);
               row_data.m_DebondSections.insert(DebondSectionData(dist_start));
               row_data_set.insert(row_data);
            }
         }
      }
   }

   if (total_num_debonded==0)
      return NoDebonding;
   else
      return IsDebonding;
}

void debonding(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDisplayUnits,SpanIndexType span)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   // First need to build data structure with all debond elevations/locations
   GirderIndexType ngirds = pBridge->GetGirderCount(span);

   DebondComparison debond_comparison;
   DebondComparison::DebondStatus status = debond_comparison.Init(span, ngirds, pStrandGeometry);

   // First deal with odd cases
   if (status==DebondComparison::NoDebonding)
   {
      return; // nothing to do
   }

   rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<"Debonding of Prestressing Strands for Span "<< LABEL_SPAN(span) <<rptNewLine;

   if (status==DebondComparison::NonSymmetricDebonding)
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;

      *pPara<<color(Red) << bold(ON) << "Debonding in one or more girders is Unsymmetrical - Cannot perform comparison" << bold(OFF) << color(Black)<<rptNewLine;
      return;
   }

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc,            pDisplayUnits->GetSpanLengthUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   Int32 num_section_locations = debond_comparison.m_SectionLocations.size();
   Int32 num_cols = 3 + num_section_locations;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(num_cols,"");
   *pPara << p_table<<rptNewLine;

   p_table->SetNumberOfHeaderRows(2);

   ColumnIndexType col1 = 0;
   ColumnIndexType col2 = 0;
   p_table->SetRowSpan(0,col1,2);
   (*p_table)(0,col1++) << "Girder";
   p_table->SetRowSpan(1,col2++,-1);

   p_table->SetRowSpan(0,col1,2);
   (*p_table)(0,col1++) << COLHDR("Strand Elev",rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   p_table->SetRowSpan(1,col2++,-1);

   p_table->SetRowSpan(0,col1,2);
   (*p_table)(0,col1++) << "Total #"<<rptNewLine<<"Strands"<<rptNewLine<<"Debond";
   p_table->SetRowSpan(1,col2++,-1);

   p_table->SetColumnSpan(0, col1, num_section_locations);
   (*p_table)(0,col1) << COLHDR(" # of Strands Debonded at Distance from Ends of Girder",rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());

   for ( ColumnIndexType i = 1; i < num_section_locations; i++ )
   {
      p_table->SetColumnSpan(0,col1+i,-1);
   }

   // section location header columns
   for (std::set<Float64>::iterator siter=debond_comparison.m_SectionLocations.begin(); siter!=debond_comparison.m_SectionLocations.end(); siter++)
   {
      (*p_table)(1,col1++) << loc.SetValue( *siter );
   }

   // Data rows in table
   int row=p_table->GetNumberOfHeaderRows();
   for (GirderIndexType ig=0; ig<ngirds; ig++)
   {
      (*p_table)(row,0) << LABEL_GIRDER(ig);

      DebondRowDataSet row_data = debond_comparison.GetDebondRowDataSet(ig);
      if (row_data.empty())
      {
         (*p_table)(row,1) << "N/A"; // no debonding in this girder
      }
      else
      {
         bool first=true;
         for (DebondRowDataSetIter riter=row_data.begin(); riter!=row_data.end(); riter++)
         {
            Int32 col = 1;

            if (!first)(*p_table)(row,col) << rptNewLine;
            (*p_table)(row,col++) << dim.SetValue( riter->m_Elevation );

            if (!first)(*p_table)(row,col) << rptNewLine;
            (*p_table)(row,col++) << riter->GetNumDebonds();

            // cycle through each section and see if we have debonds there
            for (std::set<Float64>::iterator siter=debond_comparison.m_SectionLocations.begin(); siter!=debond_comparison.m_SectionLocations.end(); siter++)
            {
               Float64 section_location = *siter;
               Int32 num_debonds = 0;

               DebondSectionData section_data(section_location);
               DebondSectionSetIter section_iter = riter->m_DebondSections.find(section_data);
               if (section_iter!=riter->m_DebondSections.end())
               {
                  num_debonds = section_iter->m_DebondCount;
               }

               if (!first)(*p_table)(row,col) << rptNewLine;
               (*p_table)(row,col++) << num_debonds;
            }

            if (first)
               first = false;
         }
      }

      row++;
   }

}

void material(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDisplayUnits,SpanIndexType span)
{
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   GET_IFACE2(pBroker,IBridge,pBridge);
   SpanIndexType nspans = pBridge->GetSpanCount();
   CHECK(span<nspans);

   rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<"Girder Concrete for Span " << LABEL_SPAN(span) << rptNewLine;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,         pDisplayUnits->GetStressUnit(),        false );
   INIT_UV_PROTOTYPE( rptDensityUnitValue, density, pDisplayUnits->GetDensityUnit(),      false );

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(6,"");
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << p_table<<rptNewLine;

   (*p_table)(0,0) << "Girder";
   (*p_table)(0,1) << COLHDR(RPT_FC,rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,2) << COLHDR(RPT_FCI,rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,3) << COLHDR("Density" << rptNewLine << "for" << rptNewLine << "Strength",rptDensityUnitTag, pDisplayUnits->GetDensityUnit() );
   (*p_table)(0,4) << COLHDR("Density" << rptNewLine << "for" << rptNewLine << "Weight",rptDensityUnitTag, pDisplayUnits->GetDensityUnit() );
   (*p_table)(0,5) << COLHDR("Max" << rptNewLine << "Aggregate" << rptNewLine << "Size",rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   GirderIndexType ngirds = pBridge->GetGirderCount(span);
   int row=1;
   for (GirderIndexType ig=0; ig<ngirds; ig++)
   {
      const CGirderMaterial* pGirderMaterial = pGirderData->GetGirderMaterial(span,ig);
      CHECK(pGirderMaterial!=0);

      (*p_table)(row,0) << LABEL_GIRDER(ig);
      (*p_table)(row,1) << stress.SetValue(pGirderMaterial->Fc);
      (*p_table)(row,2) << stress.SetValue(pGirderMaterial->Fci);
      (*p_table)(row,3) << density.SetValue(pGirderMaterial->StrengthDensity);
      (*p_table)(row,4) << density.SetValue(pGirderMaterial->WeightDensity);
      (*p_table)(row,5) << dim.SetValue(pGirderMaterial->MaxAggregateSize);
      row++;
   }

}

void stirrups(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDisplayUnits,SpanIndexType span)
{
   GET_IFACE2(pBroker,IBridge,pBridge);

   rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<"Transverse Reinforcement Stirrup Zones"<<rptNewLine;

   CStirrupTable stirr_table;

   GirderIndexType ngirds = pBridge->GetGirderCount(span);
   int row=1;
   for (GirderIndexType ig=0; ig<ngirds; ig++)
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara <<bold(ON)<<"Span " << LABEL_SPAN(span) << ", Girder "<<LABEL_GIRDER(ig)<<bold(OFF);
      stirr_table.Build(pChapter,pBroker,span,ig,pDisplayUnits);
   }
}

void handling(rptChapter* pChapter,IBroker* pBroker,IDisplayUnits* pDisplayUnits,SpanIndexType span)
{
   GET_IFACE2(pBroker,IGirderLifting,pGirderLifting);
   GET_IFACE2(pBroker,IGirderHauling,pGirderHauling);
   GET_IFACE2(pBroker,IBridge,pBridge);
   SpanIndexType nspans = pBridge->GetSpanCount();
   CHECK(span<nspans);

   rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<"Lifting and Shipping Locations for Span "<< LABEL_SPAN(span)<<rptNewLine;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc, pDisplayUnits->GetSpanLengthUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(5,"");
   *pPara << p_table<<rptNewLine;

   (*p_table)(0,0) << "Girder";
   (*p_table)(0,1) << COLHDR("Left Lifting" << rptNewLine << "Loop Location",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,2) << COLHDR("Right Lifting" << rptNewLine << "Loop Location",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,3) << COLHDR("Leading Truck" << rptNewLine << "Support Location",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,4) << COLHDR("Trailing Truck" << rptNewLine << "Support Location",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );


   GirderIndexType ngirds = pBridge->GetGirderCount(span);
   int row=1;
   for (GirderIndexType ig=0; ig<ngirds; ig++)
   {
      (*p_table)(row,0) << LABEL_GIRDER(ig);
      (*p_table)(row,1) << loc.SetValue(pGirderLifting->GetLeftLiftingLoopLocation(span,ig));
      (*p_table)(row,2) << loc.SetValue(pGirderLifting->GetRightLiftingLoopLocation(span,ig));
      (*p_table)(row,3) << loc.SetValue(pGirderHauling->GetLeadingOverhang(span,ig));
      (*p_table)(row,4) << loc.SetValue(pGirderHauling->GetTrailingOverhang(span,ig));
      row++;
   }
}