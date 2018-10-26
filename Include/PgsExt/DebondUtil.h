///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSEXT_TXDOTDEBONDUTIL_H_
#define INCLUDED_PGSEXT_TXDOTDEBONDUTIL_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#include <PGSuperTypes.h>
#include <IFace\Bridge.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   CDeckDescription

   Utility class for describing the deck.

DESCRIPTION
   Utility class for describing the deck.

COPYRIGHT
   Copyright © 1997-2008
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 06.20.2008 : Created file
*****************************************************************************/

// free function for checking if a set of numbers is evenly divisible by a number
typedef std::set<Float64> FloatSet;
typedef FloatSet::iterator FloatSetIterator;

static bool IsDivisible(FloatSetIterator start, FloatSetIterator end, Float64 divisor)
{
   while(start != end)
   {
      Float64 val = *start;
      Float64 remain = fmod(val,divisor);
      if ( !( IsZero( remain-divisor ) || IsZero( remain ) ) )
         return false;

      start++;
   }

   return true;
}


// local utility class for sorting and writing txdot debond data
/////////////////////////////////////////////////////////////////
class TxDOTDebondTool
{
public:
   TxDOTDebondTool(SpanIndexType span, GirderIndexType gdr, Float64 girderLength, IStrandGeometry* pStrandGeometry):
   m_Span(span ), m_Girder(gdr ), m_pStrandGeometry(pStrandGeometry ), m_GirderLength(girderLength),
   m_OutCome(AllStandard), m_SectionSpacing(0.0)
   {;}

protected:

   SpanIndexType m_Span;
   GirderIndexType m_Girder;
   Float64 m_GirderLength;
   IStrandGeometry* m_pStrandGeometry;

   enum OutComeType {AllStandard, NonStandardSection, SectionMismatch, SectionsNotSymmetrical, TooManySections};
   OutComeType m_OutCome;

   Float64           m_SectionSpacing;
   StrandIndexType   m_NumDebonded;

   // need strands in rows with sections in row
   struct SectionData
   {
      Float64 m_XLoc;
      Int16   m_NumDebonds;

      bool operator==(const SectionData& rOther) const 
      { 
         return ::IsEqual(m_XLoc,rOther.m_XLoc); 
      }

      bool operator<(const SectionData& rOther) const 
      { 
         return m_XLoc < rOther.m_XLoc; 
      }
   };

   typedef std::set< SectionData> SectionList;
   typedef SectionList::iterator SectionListIter;
   typedef SectionList::const_iterator SectionListConstIter;

   struct RowData
   {
      Float64 m_Elevation;
      SectionList m_Sections;

      bool operator==(const RowData& rOther) const
      { 
         return ::IsEqual(m_Elevation,rOther.m_Elevation);
      }

      bool operator<(const RowData& rOther) const 
      { 
         return m_Elevation < rOther.m_Elevation; 
      }
   };

   typedef std::set<RowData> RowList;
   typedef RowList::iterator RowListIter;

   RowList m_Rows;

   void Compute();
   Int16 CountDebondsInRow(const RowData& row) const;
};

inline void TxDOTDebondTool::Compute()
{
   m_NumDebonded = m_pStrandGeometry->GetNumDebondedStrands(m_Span,m_Girder,pgsTypes::Straight);

   // standard debond increment
   Float64 three_feet = ::ConvertToSysUnits( 3.0,unitMeasure::Feet);

   // no debonds, make a single row with no debond sections
   if (m_NumDebonded<=0)
   {
      m_OutCome = AllStandard;
      m_SectionSpacing = three_feet;
      return;
   }
   else
   {
      StrandIndexType nss = m_pStrandGeometry->GetNumStrands(m_Span,m_Girder,pgsTypes::Straight);

      pgsPointOfInterest poi(m_Span,m_Girder, m_GirderLength/2.0);
      CComPtr<IPoint2dCollection> coords;
      m_pStrandGeometry->GetStrandPositions(poi, pgsTypes::Straight, &coords);

      // We also want to see if there is a common debond increment, and if by chance, it is 3 feet
      FloatSet section_spacings;

      CollectionIndexType size;
      coords->get_Count(&size);
      ATLASSERT(nss==size);
      // loop over all strands and put debond strands in rows and sections
      for (CollectionIndexType idx = 0; idx < size; idx++)
      {
         Float64 startLoc, endLoc;
         if( m_pStrandGeometry->IsStrandDebonded(m_Span,m_Girder,idx, pgsTypes::Straight, &startLoc, &endLoc) )
         {
            if (!IsEqual(startLoc,m_GirderLength-endLoc))
            {
               // TxDOT expects even debonding all around
               m_OutCome = SectionsNotSymmetrical;
               return;
            }
            else
            {
               // get elevation of strand
               CComPtr<IPoint2d> point;
               coords->get_Item(idx,&point);
               Float64 curr_y;
               point->get_Y(&curr_y);
            
               // fill row with current strands
               RowData bogus_row;
               bogus_row.m_Elevation = curr_y;
               RowListIter curr_row_it = m_Rows.find( bogus_row );
               if (curr_row_it == m_Rows.end())
               {
                  // not found, make a new row
                  SectionData section;
                  section.m_XLoc = startLoc;
                  section.m_NumDebonds = 1;

                  RowData row;
                  row.m_Elevation = curr_y;
                  row.m_Sections.insert(section);

                  m_Rows.insert(row);

                  section_spacings.insert(startLoc);
               }
               else
               {
                  // found a row, try to find sectio in row
                  SectionData bogus_section;
                  bogus_section.m_XLoc = startLoc;
                  RowData& row = *curr_row_it;
                  SectionListIter sit = row.m_Sections.find(bogus_section);
                  if (sit==row.m_Sections.end())
                  {
                     SectionData section;
                     section.m_XLoc = startLoc;
                     section.m_NumDebonds = 1;

                     row.m_Sections.insert(section);

                     section_spacings.insert(startLoc);
                  }
                  else
                  {
                     SectionData& rsect = *sit;
                     rsect.m_NumDebonds++;
                  }
               }
            }
         }
      }

      // now we have our data structures set up. See if our increment is 3'
      if ( IsDivisible(section_spacings.begin(), section_spacings.end(), three_feet) )
      {
         m_OutCome = AllStandard;
         m_SectionSpacing = three_feet;

         // make sure we don't have too many sections to write to
         FloatSetIterator fsit = section_spacings.end();
         fsit--;
         Float64 largest_debond_dist = *fsit;

         Int16 num_sects = (Int16)ceil( (largest_debond_dist-1.0e-05)/m_SectionSpacing );
         if (num_sects>10)
         {
            m_OutCome = TooManySections;
         }
      }
      else
      {
         // See if we have an even spacing between sections. First find smallest distance between sections
         Float64 smallest = Float64_Max;
         Float64 last = 0.0;
         FloatSetIterator fsit;
         for (fsit=section_spacings.begin();fsit!=section_spacings.end(); fsit++)
         {
            Float64 curr = *fsit;

            smallest = min(smallest, curr-last);
            last = curr;
         }

         // see if smallest distance is an even denominator, if not then try 1/2, and 1/3 before giving up
         if (IsDivisible(section_spacings.begin(), section_spacings.end(), smallest))
         {
            m_OutCome = NonStandardSection;
            m_SectionSpacing = smallest;
         }
         else if (IsDivisible(section_spacings.begin(), section_spacings.end(), smallest/2.0))
         {
            m_OutCome = NonStandardSection;
            m_SectionSpacing = smallest/2.0;
         }
         else if (IsDivisible(section_spacings.begin(), section_spacings.end(), smallest/3.0))
         {
            m_OutCome = NonStandardSection;
            m_SectionSpacing = smallest/3.0;
         }
         else
         {
            m_OutCome=SectionMismatch;
         }

         // make sure we have room to lay out our sections
         if (m_OutCome == NonStandardSection)
         {
            fsit = section_spacings.end();
            fsit--;
            Float64 largest_debond_dist = *fsit;

            Int16 num_sects = (Int16)ceil( (largest_debond_dist-1.0e-05)/m_SectionSpacing );
            if (num_sects>10)
            {
               // This is too many sections at non-standard spacing. Call it a mismatch 
               m_OutCome = SectionMismatch;
            }
         }
      }
   }
}

inline Int16 TxDOTDebondTool::CountDebondsInRow(const RowData& row) const
{
   Int16 cnt=0;
   for(SectionListConstIter scit = row.m_Sections.begin(); scit != row.m_Sections.end(); scit++)
   {
      SectionData rdata = *scit;
      cnt += rdata.m_NumDebonds;
   }

   ATLASSERT(cnt>0);
   return cnt;
}


// local utility class for sorting and collection debond sections
/////////////////////////////////////////////////////////////////
class DebondSectionComputer
{
public:
   DebondSectionComputer(const std::vector<DEBONDCONFIG>& rDebondInfo, Float64 girderLength);

   CollectionIndexType GetNumLeftSections();
   void GetLeftSectionInfo(CollectionIndexType idx, Float64* location, StrandIndexType* numStrandsDebonded);

   CollectionIndexType GetNumRightSections();
   void GetRightSectionInfo(CollectionIndexType idx, Float64* location, StrandIndexType* numStrandsDebonded);

private:
   struct DbSection
   {
      Float64 m_Location;
      StrandIndexType   m_NumDebonds;

      bool operator==(const DbSection& rOther) const
      { 
         return ::IsEqual(m_Location,rOther.m_Location);
      }

      bool operator<(const DbSection& other) const
      {
         return m_Location < other.m_Location;
      }
   };

   std::set<DbSection> m_LeftSections;
   std::set<DbSection> m_RightSections;
};

inline DebondSectionComputer::DebondSectionComputer(const std::vector<DEBONDCONFIG>& rDebondInfo, Float64 girderLength)
{
   // set up section locations
   for (DebondConfigConstIterator it=rDebondInfo.begin(); it!=rDebondInfo.end(); it++)
   {
      const DEBONDCONFIG& rinfo = *it;

      // Left side
      if(rinfo.LeftDebondLength > 0.0)
      {
         DbSection bogus_sec;
         bogus_sec.m_Location = rinfo.LeftDebondLength;

         std::set<DbSection>::iterator sec_it = m_LeftSections.find( bogus_sec );
         if (sec_it == m_LeftSections.end())
         {
            // not found, make a new section
            DbSection section;
            ATLASSERT(girderLength/2.0 > rinfo.LeftDebondLength);
            section.m_Location = rinfo.LeftDebondLength;
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
      if(rinfo.RightDebondLength > 0.0)
      {
         DbSection bogus_sec;
         ATLASSERT(girderLength/2.0 > rinfo.RightDebondLength);
         Float64 loc = girderLength - rinfo.RightDebondLength;
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

inline CollectionIndexType DebondSectionComputer::GetNumLeftSections()
{
   return m_LeftSections.size();
}

inline void DebondSectionComputer::GetLeftSectionInfo(CollectionIndexType idx, Float64* pLocation, StrandIndexType* numStrandsDebonded)
{
   std::set<DbSection>::iterator sec_it = m_LeftSections.begin();
   for(CollectionIndexType is=0; is<idx; is++)
      sec_it++;

   DbSection& sec = *sec_it;
   *numStrandsDebonded = sec.m_NumDebonds;
   *pLocation = sec.m_Location;
}

inline CollectionIndexType DebondSectionComputer::GetNumRightSections()
{
   return m_RightSections.size();
}

inline void DebondSectionComputer::GetRightSectionInfo(CollectionIndexType idx, Float64* pLocation, StrandIndexType* numStrandsDebonded)
{
   std::set<DbSection>::iterator sec_it = m_RightSections.begin();
   for(CollectionIndexType is=0; is<idx; is++)
      sec_it++;

   DbSection& sec = *sec_it;
   *numStrandsDebonded = sec.m_NumDebonds;
   *pLocation = sec.m_Location;
}

class StrandRowUtil
{
public:

   // utility struct to temporarily store and sort rows
   struct StrandRow
   {
      double Elevation;
      StrandIndexType Count;

      StrandRow():
         Elevation(0.0),
         Count(1)
         {;}

      StrandRow(Float64 elev):
         Elevation(elev),
         Count(1)
         {;}

      bool operator==(const StrandRow& rOther) const 
      { 
         return ::IsEqual(Elevation,rOther.Elevation); 
      }

      bool operator<(const StrandRow& rOther) const 
      { 
         return Elevation < rOther.Elevation; 
      }
   };
   typedef std::set<StrandRow> StrandRowSet;
   typedef StrandRowSet::iterator StrandRowIter;

   static StrandRowSet GetStrandRowSet(IBroker* pBroker, const pgsPointOfInterest& midPoi);
};

inline StrandRowUtil::StrandRowSet StrandRowUtil::GetStrandRowSet(IBroker* pBroker, const pgsPointOfInterest& midPoi)
{
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );

   // Want number of strands in each row location. Count number of straight and harped per row
   StrandRowSet strandrows;

   // Straight
   CComPtr<IPoint2dCollection> ss_points;
   pStrandGeometry->GetStrandPositions(midPoi, pgsTypes::Straight, &ss_points);

   RowIndexType nrows = pStrandGeometry->GetNumRowsWithStrand(midPoi.GetSpan(),midPoi.GetGirder(),pgsTypes::Straight);
   for (RowIndexType rowIdx=0; rowIdx!=nrows; rowIdx++)
   {
      std::vector<StrandIndexType> sstrands = pStrandGeometry->GetStrandsInRow(midPoi.GetSpan(), midPoi.GetGirder(), rowIdx, pgsTypes::Straight);
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

   nrows = pStrandGeometry->GetNumRowsWithStrand(midPoi.GetSpan(),midPoi.GetGirder(),pgsTypes::Harped);
   for (RowIndexType rowIdx=0; rowIdx!=nrows; rowIdx++)
   {
      std::vector<StrandIndexType> hstrands = pStrandGeometry->GetStrandsInRow(midPoi.GetSpan(), midPoi.GetGirder(), rowIdx, pgsTypes::Harped);
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

#endif // INCLUDED_PGSEXT_TXDOTDEBONDUTIL_H_
