///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// TxDOTOptionalDesignUtilities.cpp
#include "stdafx.h"

#include "TxDOTOptionalDesignUtilities.h"

#include <PgsExt\BridgeDescription2.h>

#include <PsgLib\GirderLibraryEntry.h>

BOOL DoParseTemplateFile(const LPCTSTR lpszPathName, CString& girderEntry, 
                              CString& leftConnEntry, CString& rightConnEntry,
                              CString& projectCriteriaEntry, CString& folderName)
{
   // Read girder type, connection types, and pgsuper template file name
   std::_tifstream ifile(lpszPathName);
   if ( !ifile )
   {
      CString msg;
      msg.Format(_T("Error opening template file: %s - File not found?"),lpszPathName);
      AfxMessageBox(msg );
      ASSERT( 0 ); // this should never happen
      return FALSE;
   }

   TCHAR line[1024];
   ifile.getline(line,1024);

   // comma delimited file in format of:
   // GirderEntryName, EndConnection, StartConnection, ProjectCriteria
   sysTokenizer tokenizer(_T(","));
   tokenizer.push_back(line);

   sysTokenizer::size_type nitems = tokenizer.size();
   if (nitems<4) // Don't limit to allow new items to be added
   {
      CString msg;
      msg.Format(_T("Error reading template file: %s - Invalid Format"),lpszPathName);
      AfxMessageBox(msg );
      return FALSE;
   }

   // set our data values
   girderEntry = tokenizer[0].c_str();
   leftConnEntry = tokenizer[1].c_str();
   rightConnEntry = tokenizer[2].c_str();
   projectCriteriaEntry = tokenizer[3].c_str();

   if (nitems==5)
   {
      folderName= tokenizer[4].c_str();
   }
   else
   {
      // Folder name not spec'd in template file. Give it a default
      folderName = _T("Unnamed");
   }

   return TRUE;
}

OptionalDesignHarpedFillUtil::StrandRowSet OptionalDesignHarpedFillUtil::GetStrandRowSet(IBroker* pBroker, const pgsPointOfInterest& midPoi)
{
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry );

   // Want to list filled strands in each row location. Loop over and build fill string
   StrandRowSet strandrows;

   // Harped
   CComPtr<IPoint2dCollection> hs_points;
   pStrandGeometry->GetStrandPositions(midPoi, pgsTypes::Harped, &hs_points);

   RowIndexType nrows = pStrandGeometry->GetNumRowsWithStrand(midPoi.GetSegmentKey(),pgsTypes::Harped);
   for (RowIndexType rowIdx=0; rowIdx!=nrows; rowIdx++)
   {
      std::vector<StrandIndexType> hstrands = pStrandGeometry->GetStrandsInRow(midPoi.GetSegmentKey(), rowIdx, pgsTypes::Harped);
      for (std::vector<StrandIndexType>::iterator sit=hstrands.begin(); sit!=hstrands.end(); sit++)
      {
         StrandIndexType idx = *sit;
         CComPtr<IPoint2d> point;
         hs_points->get_Item(idx,&point);
         Float64 Y;
         point->get_Y(&Y);

         Float64 X;
         point->get_X(&X);

         if (X>0.0)
         {
            TCHAR fill_char = GetFillString(X);

            StrandRow srow(Y);
            StrandRowIter srit = strandrows.find(srow);
            if (srit != strandrows.end())
            {
               srit->fillListString += fill_char;
            }
            else
            {
               srow.fillListString += fill_char;
               strandrows.insert(srow);
            }
         }
      }
   }


   // Straight
   CComPtr<IPoint2dCollection> ss_points;
   pStrandGeometry->GetStrandPositions(midPoi, pgsTypes::Straight, &ss_points);

   nrows = pStrandGeometry->GetNumRowsWithStrand(midPoi.GetSegmentKey(),pgsTypes::Straight);
   for (RowIndexType rowIdx=0; rowIdx!=nrows; rowIdx++)
   {
      std::vector<StrandIndexType> sstrands = pStrandGeometry->GetStrandsInRow(midPoi.GetSegmentKey(), rowIdx, pgsTypes::Straight);
      for (std::vector<StrandIndexType>::iterator sit=sstrands.begin(); sit!=sstrands.end(); sit++)
      {
         StrandIndexType idx = *sit;
         CComPtr<IPoint2d> point;
         ss_points->get_Item(idx,&point);
         Float64 Y;
         point->get_Y(&Y);

         Float64 X;
         point->get_X(&X);

         if (X>0.0)
         {
            TCHAR fill_char = GetFillString(X);

            StrandRow srow(Y);
            StrandRowIter srit = strandrows.find(srow);
            if (srit != strandrows.end())
            {
               srit->fillListString += fill_char;
            }
            else
            {
               srow.fillListString += fill_char;
               strandrows.insert(srow);
            }
         }
      }
   }

   return strandrows;
}

// class to help sort strands
class StrandLoc
{
public:
   StrandLoc(Float64 x, Float64 y, StrandIndexType idx):
   X(x), Y(y), Idx(idx)
   {;}

   bool operator < (const StrandLoc& rOther) const
   {
      if (Y < rOther.Y)
         return true;
      else if (Y > rOther.Y)
         return false;
      else if (X < rOther.X)
         return true;
      else if (X > rOther.X)
         return false;
      else if (Idx < rOther.Idx)
         return true;
      else
         return false;
   }

   Float64 X, Y;
   StrandIndexType Idx;
};

void MakeHarpedCloneStraight(const GirderLibraryEntry* pGdrEntry, GirderLibraryEntry* pGdrClone)
{
   pGdrClone->ClearAllStrands();

   // Cycle through all permanent strands. Change to straight in same global order
   StrandIndexType np = pGdrEntry->GetPermanentStrandGridSize();
   for (StrandIndexType ip=0; ip<np; ip++)
   {
      GirderLibraryEntry::psStrandType sType;
      GridIndexType gridIdx;

      pGdrEntry->GetGridPositionFromPermStrandGrid(ip, &sType, &gridIdx);

      StrandIndexType istr;
      if (sType==pgsTypes::Straight)
      {
         // Copy straight strands and make all debondable
         Float64 Xstart, Ystart, Xend, Yend;
         bool canDebond;
         pGdrEntry->GetStraightStrandCoordinates(gridIdx, &Xstart, &Ystart, &Xend, &Yend, &canDebond);

         istr = pGdrClone->AddStraightStrandCoordinates(Xstart, Ystart, Xstart, Ystart, true);
      }
      else if (sType==pgsTypes::Harped)
      {
         Float64 Xstart, Ystart, Xhp, Yhp, Xend, Yend;
         pGdrEntry->GetHarpedStrandCoordinates(gridIdx, &Xstart, &Ystart, &Xhp, &Yhp, &Xend, &Yend);

         istr = pGdrClone->AddStraightStrandCoordinates(Xstart, Ystart, Xstart, Ystart, false);
      }
      else
      {
         ATLASSERT(false); // TxDOT girders should not have temp strands
         break;
      }

      // make permanent strand order same as original
      pGdrClone->AddStrandToPermStrandGrid(GirderLibraryEntry::stStraight, istr-1);
   }
}

std::list<ColumnIndexType> ComputeTableCols(const std::vector<CGirderKey>& girderKeys)
{
   // Idea here is to break tables at spans. 
   // First build list of sizes of contiguous blocks of spans
   std::list<ColumnIndexType> contiguous_blocks1;
   SpanIndexType curr_span(INVALID_INDEX);
   bool first=false;
   std::vector<CGirderKey>::const_iterator it(girderKeys.begin());
   std::vector<CGirderKey>::const_iterator itEnd(girderKeys.end());
   for( ; it != itEnd; it++)
   {
      const CGirderKey& girderKey = *it;
      SpanIndexType new_span = girderKey.groupIndex;
      GirderIndexType new_gdr = girderKey.girderIndex;

      if (first || curr_span!=new_span)
      {
         first = false;
         curr_span = new_span;
         contiguous_blocks1.push_back(1);
      }
      else
      {
         contiguous_blocks1.back()++;
      }
   }

   // Next break blocks into list of table-sized chunks 
   std::list<ColumnIndexType> contiguous_blocks2;
   for(std::list<ColumnIndexType>::const_iterator it=contiguous_blocks1.begin(); it!=contiguous_blocks1.end(); it++)
   {
      ColumnIndexType ncols = *it;
      if (ncols > MAX_TBL_COLS)
      {
         ColumnIndexType num_big_chunks = ncols / MAX_TBL_COLS;
         ColumnIndexType rmdr = ncols % MAX_TBL_COLS;

         for (ColumnIndexType ich=0; ich<num_big_chunks; ich++)
         {
            contiguous_blocks2.push_back(MAX_TBL_COLS);
         }

         if(rmdr != 0)
         {
            contiguous_blocks2.push_back(rmdr);
         }
      }
      else
      {
         contiguous_blocks2.push_back(ncols);
      }
   }

   // Now we have a "right-sized" columns, but we could have a list of one-column tables, which
   // would be ugly. If all num colums are LE than min, combine into a wider, but not pretty table
   bool is_ugly = true;
   for(std::list<ColumnIndexType>::const_iterator it=contiguous_blocks2.begin(); it!=contiguous_blocks2.end(); it++)
   {
      ColumnIndexType ncols = *it;
      if (ncols > MIN_TBL_COLS)
      {
         is_ugly = false; // we have at least one table of minimum width - we're not ugly.
         break;
      }
   }

   std::list<ColumnIndexType> final_blocks;
   if (!is_ugly)
   {
      final_blocks = contiguous_blocks2;
   }
   else
   {
      // work to combine blocks
      std::list<ColumnIndexType>::const_iterator it=contiguous_blocks2.begin();
      while(it!=contiguous_blocks2.end())
      {
         ColumnIndexType ncols = *it;
         while (ncols<=MAX_TBL_COLS)
         {
            it++;
            if (it==contiguous_blocks2.end() || ncols+(*it) > MAX_TBL_COLS)
            {
               final_blocks.push_back(ncols);
               break;
            }
            else
            {
               ncols+= (*it);
            }
         }
      }
   }

   return final_blocks;
}

