///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <PgsExt\PgsExtExp.h>

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

// free function for checking if a set of numbers is divisible by a number
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
class PGSEXTCLASS TxDOTDebondTool
{
public:
   TxDOTDebondTool(const CSegmentKey& segmentKey, Float64 girderLength, IStrandGeometry* pStrandGeometry);

protected:
   CSegmentKey m_SegmentKey;
   Float64 m_GirderLength;
   IStrandGeometry* m_pStrandGeometry;

   enum OutComeType {AllStandard, NonStandardSection, SectionMismatch, SectionsNotSymmetrical, TooManySections};
   OutComeType m_OutCome;

   Float64   m_SectionSpacing;
   AxleIndexType   m_NumDebonded;

   // need strands in rows with sections in row
   struct SectionData
   {
      Float64 m_XLoc;
      Int16   m_NumDebonds;

      SectionData():
      m_XLoc(0.0), m_NumDebonds(0)
      {;}

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
      StrandIndexType m_NumTotalStrands;
      SectionList m_Sections;

      RowData():
      m_Elevation(0.0), m_NumTotalStrands(0)
      {;}

      bool operator==(const RowData& rOther) const
      { 
         return ::IsEqual(m_Elevation,rOther.m_Elevation);
      }

      bool operator<(const RowData& rOther) const 
      { 
         return ::IsLT(m_Elevation, rOther.m_Elevation); 
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
   m_NumDebonded = m_pStrandGeometry->GetNumDebondedStrands(m_SegmentKey,pgsTypes::Straight);

   // standard debond increment
   Float64 three_feet = ::ConvertToSysUnits( 3.0,unitMeasure::Feet);

   StrandIndexType nss = m_pStrandGeometry->GetStrandCount(m_SegmentKey,pgsTypes::Straight);

   pgsPointOfInterest poi(m_SegmentKey, m_GirderLength/2.0);
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
      // get elevation of strand
      CComPtr<IPoint2d> point;
      coords->get_Item(idx,&point);
      Float64 curr_x, curr_y;
      point->get_X(&curr_x);
      point->get_Y(&curr_y);
   
      // create or find row with current strand elevation
      RowData bogus_row;
      bogus_row.m_Elevation = curr_y;
      RowListIter curr_row_it = m_Rows.find( bogus_row );
      if (curr_row_it == m_Rows.end())
      {
         // Create row because it doesn't exist
         RowData row;
         row.m_Elevation = curr_y;
         std::pair<RowListIter,bool> new_iter = m_Rows.insert(row);
         ATLASSERT(new_iter.second);

         curr_row_it = new_iter.first;
      }

      curr_row_it->m_NumTotalStrands++; // add our strand to row

      Float64 startLoc, endLoc;
      if( m_pStrandGeometry->IsStrandDebonded(m_SegmentKey,idx, pgsTypes::Straight, &startLoc, &endLoc) )
      {
         if (!IsEqual(startLoc,m_GirderLength-endLoc))
         {
            // TxDOT expects even debonding all around
            m_OutCome = SectionsNotSymmetrical;
            return;
         }
         else
         {
            // fill row's debond information
            // try to find section in row
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

   // now we have our data structures set up. See if our increment is 3'
   if (m_NumDebonded>0)
   {
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

            smallest = Min(smallest, curr-last);
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

   return cnt;
}


// local utility class for sorting and collection debond sections
/////////////////////////////////////////////////////////////////
class PGSEXTCLASS CDebondSectionCalculator
{
public:
   CDebondSectionCalculator(const std::vector<DEBONDCONFIG>& rDebondInfo, Float64 girderLength);

   SectionIndexType GetNumLeftSections();
   void GetLeftSectionInfo(SectionIndexType idx, Float64* location, IndexType* numStrandsDebonded);

   SectionIndexType GetNumRightSections();
   void GetRightSectionInfo(SectionIndexType idx, Float64* location, IndexType* numStrandsDebonded);

private:
   struct DbSection
   {
      Float64 m_Location;
      IndexType m_NumDebonds;

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

class PGSEXTCLASS StrandRowUtil
{
public:

   // utility struct to temporarily store and sort rows
   struct StrandRow
   {
      Float64 Elevation;
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

#endif // INCLUDED_PGSEXT_TXDOTDEBONDUTIL_H_
