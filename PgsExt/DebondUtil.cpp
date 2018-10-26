///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\DebondUtil.h>
#include <IFace\Bridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

TxDOTDebondTool::TxDOTDebondTool(const CSegmentKey& segmentKey, Float64 girderLength, IStrandGeometry* pStrandGeometry) :
m_SegmentKey(segmentKey), 
m_pStrandGeometry(pStrandGeometry ), 
m_GirderLength(girderLength),
m_OutCome(AllStandard), 
m_SectionSpacing(0.0)
{
}



CDebondSectionCalculator::CDebondSectionCalculator(const std::vector<DEBONDCONFIG>& rDebondInfo, Float64 girderLength)
{
   // set up section locations
   for (std::vector<DEBONDCONFIG>::const_iterator it=rDebondInfo.begin(); it!=rDebondInfo.end(); it++)
   {
      const DEBONDCONFIG& rinfo = *it;

      // Left side
      if(rinfo.DebondLength[pgsTypes::metStart] > 0.0)
      {
         DbSection bogus_sec;
         bogus_sec.m_Location = rinfo.DebondLength[pgsTypes::metStart];

         std::set<DbSection>::iterator sec_it = m_LeftSections.find( bogus_sec );
         if (sec_it == m_LeftSections.end())
         {
            // not found, make a new section
            DbSection section;
            ATLASSERT(girderLength/2.0 > rinfo.DebondLength[pgsTypes::metStart]);
            section.m_Location = rinfo.DebondLength[pgsTypes::metStart];
            section.m_NumDebonds = 1;

            m_LeftSections.insert(section);
         }
         else
         {
            // found, add debond
            DbSection& sec = *sec_it;
            sec.m_NumDebonds++;
         }
      }

      // Right side
      if(rinfo.DebondLength[pgsTypes::metEnd] > 0.0)
      {
         DbSection bogus_sec;
         ATLASSERT(girderLength/2.0 > rinfo.DebondLength[pgsTypes::metEnd]);
         Float64 loc = girderLength - rinfo.DebondLength[pgsTypes::metEnd];
         bogus_sec.m_Location = loc;

         std::set<DbSection>::iterator sec_it = m_RightSections.find( bogus_sec );
         if (sec_it == m_RightSections.end())
         {
            // not found, make a new section
            DbSection section;
            section.m_Location = loc;
            section.m_NumDebonds = 1;

            m_RightSections.insert(section);
         }
         else
         {
            // found, add debond
            DbSection& sec = *sec_it;
            sec.m_NumDebonds++;
         }
      }
   }
}

SectionIndexType CDebondSectionCalculator::GetNumLeftSections()
{
   return m_LeftSections.size();
}

void CDebondSectionCalculator::GetLeftSectionInfo(SectionIndexType idx, Float64* pLocation, IndexType* numStrandsDebonded)
{
   std::set<DbSection>::iterator sec_it = m_LeftSections.begin();
   for(SectionIndexType is=0; is<idx; is++)
      sec_it++;

   DbSection& sec = *sec_it;
   *numStrandsDebonded = sec.m_NumDebonds;
   *pLocation = sec.m_Location;
}

SectionIndexType CDebondSectionCalculator::GetNumRightSections()
{
   return m_RightSections.size();
}

void CDebondSectionCalculator::GetRightSectionInfo(SectionIndexType idx, Float64* pLocation, IndexType* numStrandsDebonded)
{
   std::set<DbSection>::iterator sec_it = m_RightSections.begin();
   for(SectionIndexType is=0; is<idx; is++)
      sec_it++;

   DbSection& sec = *sec_it;
   *numStrandsDebonded = sec.m_NumDebonds;
   *pLocation = sec.m_Location;
}

StrandRowUtil::StrandRowSet StrandRowUtil::GetStrandRowSet(IBroker* pBroker, const pgsPointOfInterest& midPoi)
{
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );

   // Want number of strands in each row location. Count number of straight and harped per row
   StrandRowSet strandrows;

   // Straight
   CComPtr<IPoint2dCollection> ss_points;
   pStrandGeometry->GetStrandPositions(midPoi, pgsTypes::Straight, &ss_points);

   const CSegmentKey& segmentKey = midPoi.GetSegmentKey();

   RowIndexType nrows = pStrandGeometry->GetNumRowsWithStrand(midPoi,pgsTypes::Straight);
   for (RowIndexType rowIdx=0; rowIdx!=nrows; rowIdx++)
   {
      std::vector<StrandIndexType> sstrands = pStrandGeometry->GetStrandsInRow(midPoi, rowIdx, pgsTypes::Straight);
      for (std::vector<StrandIndexType>::iterator sit=sstrands.begin(); sit!=sstrands.end(); sit++)
      {
         StrandIndexType idx = *sit;
         CComPtr<IPoint2d> point;
         ss_points->get_Item(idx,&point);
         Float64 Y;
         point->get_Y(&Y);

         StrandRow srow(Y);
         StrandRowIter srit = strandrows.find(srow);
         if (srit != strandrows.end())
         {
            srit->Count++;
         }
         else
         {
            strandrows.insert(srow);
         }
      }
   }

   // Harped
   CComPtr<IPoint2dCollection> hs_points;
   pStrandGeometry->GetStrandPositions(midPoi, pgsTypes::Harped, &hs_points);

   nrows = pStrandGeometry->GetNumRowsWithStrand(midPoi,pgsTypes::Harped);
   for (RowIndexType rowIdx=0; rowIdx!=nrows; rowIdx++)
   {
      std::vector<StrandIndexType> hstrands = pStrandGeometry->GetStrandsInRow(midPoi, rowIdx, pgsTypes::Harped);
      for (std::vector<StrandIndexType>::iterator sit=hstrands.begin(); sit!=hstrands.end(); sit++)
      {
         StrandIndexType idx = *sit;
         CComPtr<IPoint2d> point;
         hs_points->get_Item(idx,&point);
         Float64 Y;
         point->get_Y(&Y);

         StrandRow srow(Y);
         StrandRowIter srit = strandrows.find(srow);
         if (srit != strandrows.end())
         {
            srit->Count++;
         }
         else
         {
            strandrows.insert(srow);
         }
      }
   }
   
   return strandrows;
}
