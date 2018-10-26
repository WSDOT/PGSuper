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
#include <Reporting\GirderComparisonChapterBuilder.h>
#include <Reporting\StirrupTable.h>

#include <EAF\EAFDisplayUnits.h>
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


void girders(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,SpanIndexType span);
bool prestressing(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,SpanIndexType span);
void debonding(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,SpanIndexType span);
void material(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,SpanIndexType span);
void stirrups(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,SpanIndexType span);
void handling(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,SpanIndexType span);

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CGirderComparisonChapterBuilder::CGirderComparisonChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
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

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

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

void girders(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,SpanIndexType span)
{
   rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<_T("Girder Types for Span ")<< LABEL_SPAN(span) << rptNewLine;

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(2,_T(""));
   *pPara << p_table<<rptNewLine;

   int col = 0;
   (*p_table)(0,col++) << _T("Girder");
   (*p_table)(0,col++) << _T("Type");


   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

   GirderIndexType nGirders = pSpan->GetGirderCount();
   int row=1;
   for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
   {
      std::_tstring strGirderName = pSpan->GetGirderTypes()->GetGirderName(gdrIdx);

      col = 0;
      (*p_table)(row,col++) << LABEL_GIRDER(gdrIdx);
      (*p_table)(row,col++) << strGirderName;
      row++;
   }
}

// return true if debonding exists
bool prestressing(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,SpanIndexType span)
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
   *pHead<<_T("Prestressing Strands for Span ")<< LABEL_SPAN(span) <<rptNewLine;


   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,          pDisplayUnits->GetShearUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,         pDisplayUnits->GetStressUnit(),        false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(bTempStrands ? 12 : 9,_T(""));
   *pPara << p_table << rptNewLine;

   p_table->SetNumberOfHeaderRows(3);

   p_table->SetRowSpan(0,0,3);
   (*p_table)(0,0) << _T("Girder");
   p_table->SetRowSpan(1,0,SKIP_CELL);
   p_table->SetRowSpan(2,0,SKIP_CELL);

   p_table->SetColumnSpan(0,1,8);
   p_table->SetColumnSpan(0,2,SKIP_CELL);
   p_table->SetColumnSpan(0,3,SKIP_CELL);
   p_table->SetColumnSpan(0,4,SKIP_CELL);
   p_table->SetColumnSpan(0,5,SKIP_CELL);
   p_table->SetColumnSpan(0,6,SKIP_CELL);
   p_table->SetColumnSpan(0,7,SKIP_CELL);
   p_table->SetColumnSpan(0,8,SKIP_CELL);
   (*p_table)(0,1) << _T("Permanent Strands");

   p_table->SetRowSpan(1,1,2);
   (*p_table)(1,1) << _T("Material");
   p_table->SetRowSpan(2,1,SKIP_CELL);

   p_table->SetColumnSpan(1,2,2);
   p_table->SetColumnSpan(1,3,SKIP_CELL);
   (*p_table)(1,2) << _T("Straight");
   (*p_table)(2,2) << _T("#");
   (*p_table)(2,3) << COLHDR(Sub2(_T("P"),_T("jack")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());

   p_table->SetColumnSpan(1,4,5);
   p_table->SetColumnSpan(1,5,SKIP_CELL);
   p_table->SetColumnSpan(1,6,SKIP_CELL);
   p_table->SetColumnSpan(1,7,SKIP_CELL);
   p_table->SetColumnSpan(1,8,SKIP_CELL);
   (*p_table)(1,4) << _T("Adjustable Strands");
   (*p_table)(2,4) << _T("Type");
   (*p_table)(2,5) << _T("#");
   (*p_table)(2,6) << COLHDR(Sub2(_T("P"),_T("jack")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
   (*p_table)(2,7) << COLHDR(_T("Girder End")<<rptNewLine<<_T("Offset"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(2,8)<< COLHDR(_T("Harping Pt")<<rptNewLine<<_T("Offset"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   if ( bTempStrands )
   {
      p_table->SetColumnSpan(0,9,3);
      p_table->SetColumnSpan(0,10,SKIP_CELL);
      p_table->SetColumnSpan(0,11,SKIP_CELL);
      (*p_table)(0,9) << _T("Temporary Strands");

      p_table->SetRowSpan(1,9,2);
      (*p_table)(1,9) << _T("Material");
      p_table->SetRowSpan(2,9,SKIP_CELL);

      p_table->SetRowSpan(1,10,2);
      (*p_table)(1,10) << _T("#");
      p_table->SetRowSpan(2,10,SKIP_CELL);

      p_table->SetRowSpan(1,11,2);
      (*p_table)(1,11) << COLHDR(Sub2(_T("P"),_T("jack")),rptForceUnitTag,pDisplayUnits->GetGeneralForceUnit());
      p_table->SetRowSpan(2,11,SKIP_CELL);
   }

   GirderIndexType ngirds = pBridge->GetGirderCount(span);
   RowIndexType row=p_table->GetNumberOfHeaderRows();
   for (GirderIndexType ig=0; ig<ngirds; ig++)
   {
      const CGirderData* pgirderData = pGirderData->GetGirderData(span,ig);

      ColumnIndexType col = 0;
      (*p_table)(row,col++) << LABEL_GIRDER(ig);
      (*p_table)(row,col++) << pgirderData->Material.pStrandMaterial[pgsTypes::Straight]->GetName();
      
      (*p_table)(row,col) << pgirderData->PrestressData.GetNstrands(pgsTypes::Straight);
      StrandIndexType nd = pStrandGeometry->GetNumDebondedStrands(span,ig,pgsTypes::Straight);
      if (0 < nd)
      {
         (*p_table)(row,col) << rptNewLine << nd << _T(" Debonded");
         was_debonding = true;     
      }
      StrandIndexType nExtLeft  = pStrandGeometry->GetNumExtendedStrands(span,ig,pgsTypes::metStart,pgsTypes::Straight);
      StrandIndexType nExtRight = pStrandGeometry->GetNumExtendedStrands(span,ig,pgsTypes::metEnd,pgsTypes::Straight);
      if ( 0 < nExtLeft || 0 < nExtRight )
      {
         (*p_table)(row,col) << rptNewLine << nExtLeft << _T(" Ext. Left");
         (*p_table)(row,col) << rptNewLine << nExtRight << _T(" Ext. Right");
      }

      col++;

      (*p_table)(row,col++) << force.SetValue(pgirderData->PrestressData.Pjack[pgsTypes::Straight]);

      // Right now, straight and harped strands must have the same material
      ATLASSERT(pgirderData->Material.pStrandMaterial[pgsTypes::Straight] == pgirderData->Material.pStrandMaterial[pgsTypes::Harped]);

      (*p_table)(row,col++) << LABEL_HARP_TYPE(pStrandGeometry->GetAreHarpedStrandsForcedStraight(span,ig));
      (*p_table)(row,col++) << pgirderData->PrestressData.GetNstrands(pgsTypes::Harped);
      (*p_table)(row,col++) << force.SetValue(pgirderData->PrestressData.Pjack[pgsTypes::Harped]);

      ConfigStrandFillVector confvec = pStrandGeometry->ComputeStrandFill(span, ig, pgsTypes::Harped, pgirderData->PrestressData.GetNstrands(pgsTypes::Harped));

      // convert to absolute adjustment
      Float64 adjustment = pStrandGeometry->ComputeAbsoluteHarpedOffsetEnd(span, ig, confvec, 
                                                           pgirderData->PrestressData.HsoEndMeasurement, pgirderData->PrestressData.HpOffsetAtEnd);
      (*p_table)(row,col++) << dim.SetValue(adjustment);

      adjustment = pStrandGeometry->ComputeAbsoluteHarpedOffsetHp(span, ig, confvec, 
                                                                  pgirderData->PrestressData.HsoHpMeasurement, pgirderData->PrestressData.HpOffsetAtHp);
      (*p_table)(row,col++) << dim.SetValue(adjustment);

      if ( bTempStrands )
      {
         (*p_table)(row,col++) << pgirderData->Material.pStrandMaterial[pgsTypes::Temporary]->GetName();
         (*p_table)(row,col++) << pgirderData->PrestressData.GetNstrands(pgsTypes::Temporary);
         (*p_table)(row,col++) << force.SetValue(pgirderData->PrestressData.Pjack[pgsTypes::Temporary]);
      }
      row++;
   }

   *pPara<<_T("Girder End Offset - Distance the harped strands at the girder ends are adjusted vertically from their default (library) locations.")<<rptNewLine;
   *pPara<<_T("Harping Point Offset - Distance the harped strands at the harping point are adjusted vertically from their default (library) locations.")<<rptNewLine;
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
      DebondRowDataSet GetDebondRowDataSet(GirderIndexType gdr)
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

void debonding(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,SpanIndexType span)
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
   *pHead<<_T("Debonding of Prestressing Strands for Span ")<< LABEL_SPAN(span) <<rptNewLine;

   if (status==DebondComparison::NonSymmetricDebonding)
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;

      *pPara<<color(Red) << bold(ON) << _T("Debonding in one or more girders is Unsymmetrical - Cannot perform comparison") << bold(OFF) << color(Black)<<rptNewLine;
      return;
   }

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc,            pDisplayUnits->GetSpanLengthUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   CollectionIndexType num_section_locations = debond_comparison.m_SectionLocations.size();
   ColumnIndexType num_cols = 3 + num_section_locations;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(num_cols,_T(""));
   *pPara << p_table<<rptNewLine;

   p_table->SetNumberOfHeaderRows(2);

   ColumnIndexType col1 = 0;
   ColumnIndexType col2 = 0;
   p_table->SetRowSpan(0,col1,2);
   (*p_table)(0,col1++) << _T("Girder");
   p_table->SetRowSpan(1,col2++,SKIP_CELL);

   p_table->SetRowSpan(0,col1,2);
   (*p_table)(0,col1++) << COLHDR(_T("Strand Elev"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   p_table->SetRowSpan(1,col2++,SKIP_CELL);

   p_table->SetRowSpan(0,col1,2);
   (*p_table)(0,col1++) << _T("Total #")<<rptNewLine<<_T("Strands")<<rptNewLine<<_T("Debond");
   p_table->SetRowSpan(1,col2++,SKIP_CELL);

   p_table->SetColumnSpan(0, col1, num_section_locations);
   (*p_table)(0,col1) << COLHDR(_T(" # of Strands Debonded at Distance from Ends of Girder"),rptLengthUnitTag,pDisplayUnits->GetSpanLengthUnit());

   for ( ColumnIndexType i = 1; i < num_section_locations; i++ )
   {
      p_table->SetColumnSpan(0,col1+i,SKIP_CELL);
   }

   // section location header columns
   for (std::set<Float64>::iterator siter=debond_comparison.m_SectionLocations.begin(); siter!=debond_comparison.m_SectionLocations.end(); siter++)
   {
      (*p_table)(1,col1++) << loc.SetValue( *siter );
   }

   // Data rows in table
   RowIndexType row=p_table->GetNumberOfHeaderRows();
   for (GirderIndexType ig=0; ig<ngirds; ig++)
   {
      (*p_table)(row,0) << LABEL_GIRDER(ig);

      DebondRowDataSet row_data = debond_comparison.GetDebondRowDataSet(ig);
      if (row_data.empty())
      {
         (*p_table)(row,1) << RPT_NA; // no debonding in this girder
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

void material(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,SpanIndexType span)
{
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   GET_IFACE2(pBroker,IBridge,pBridge);
   SpanIndexType nspans = pBridge->GetSpanCount();
   CHECK(span<nspans);

   rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<_T("Girder Concrete for Span ") << LABEL_SPAN(span) << rptNewLine;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,            pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,         pDisplayUnits->GetStressUnit(),        false );
   INIT_UV_PROTOTYPE( rptDensityUnitValue, density, pDisplayUnits->GetDensityUnit(),      false );

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(6,_T(""));
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << p_table<<rptNewLine;

   (*p_table)(0,0) << _T("Girder");
   (*p_table)(0,1) << COLHDR(RPT_FC,rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,2) << COLHDR(RPT_FCI,rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*p_table)(0,3) << COLHDR(_T("Density") << rptNewLine << _T("for") << rptNewLine << _T("Strength"),rptDensityUnitTag, pDisplayUnits->GetDensityUnit() );
   (*p_table)(0,4) << COLHDR(_T("Density") << rptNewLine << _T("for") << rptNewLine << _T("Weight"),rptDensityUnitTag, pDisplayUnits->GetDensityUnit() );
   (*p_table)(0,5) << COLHDR(_T("Max") << rptNewLine << _T("Aggregate") << rptNewLine << _T("Size"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

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

void stirrups(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,SpanIndexType span)
{
   GET_IFACE2(pBroker,IBridge,pBridge);

   rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<_T("Transverse Reinforcement Stirrup Zones")<<rptNewLine;

   CStirrupTable stirr_table;

   GirderIndexType ngirds = pBridge->GetGirderCount(span);
   int row=1;
   for (GirderIndexType ig=0; ig<ngirds; ig++)
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara <<bold(ON)<<_T("Span ") << LABEL_SPAN(span) << _T(", Girder ")<<LABEL_GIRDER(ig)<<bold(OFF);
      stirr_table.Build(pChapter,pBroker,span,ig,pDisplayUnits);
   }
}

void handling(rptChapter* pChapter,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits,SpanIndexType span)
{
   GET_IFACE2(pBroker,IGirderLifting,pGirderLifting);
   GET_IFACE2(pBroker,IGirderHauling,pGirderHauling);
   GET_IFACE2(pBroker,IBridge,pBridge);
   SpanIndexType nspans = pBridge->GetSpanCount();
   CHECK(span<nspans);

   rptParagraph* pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter<<pHead;
   *pHead<<_T("Lifting and Shipping Locations for Span ")<< LABEL_SPAN(span)<<rptNewLine;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, loc, pDisplayUnits->GetSpanLengthUnit(), false );

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(5,_T(""));
   *pPara << p_table<<rptNewLine;

   (*p_table)(0,0) << _T("Girder");
   (*p_table)(0,1) << COLHDR(_T("Left Lifting") << rptNewLine << _T("Loop Location"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,2) << COLHDR(_T("Right Lifting") << rptNewLine << _T("Loop Location"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,3) << COLHDR(_T("Leading Truck") << rptNewLine << _T("Support Location"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*p_table)(0,4) << COLHDR(_T("Trailing Truck") << rptNewLine << _T("Support Location"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );


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