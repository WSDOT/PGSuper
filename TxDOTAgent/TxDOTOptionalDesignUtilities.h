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
//
// 
#ifndef INCLUDED_PGSEXT_TXDOTOPTIONALDESIGNUTILILITIES_H_
#define INCLUDED_PGSEXT_TXDOTOPTIONALDESIGNUTILILITIES_H_

// SYSTEM INCLUDES
//


// PROJECT INCLUDES
//
#include <Material\PsStrand.h>
#include <System\Tokenizer.h>
#include <System\FileStream.h>

#include <PGSuperTypes.h>
#include <IFace\Bridge.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//
// Span and girder for our template model
#define TOGA_SPAN 0
#define TOGA_NUM_GDRS 8
#define TOGA_ORIG_GDR 3
#define TOGA_FABR_GDR 4

/*****************************************************************************
CLASS 

DESCRIPTION
   Utilitity functions and classes

COPYRIGHT
   Copyright © 1997-2010
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 02.19.2010 : Created file
*****************************************************************************/

   // Location of template folders
inline CString GetTOGAFolder()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CString strHelpFolderName =  AfxGetApp()->m_pszHelpFilePath;
   int loc = strHelpFolderName.ReverseFind(_T('\\'));
   CString strWorkgroupFolderName = strHelpFolderName.Left(loc+1) + _T("TogaTemplates");

   return strWorkgroupFolderName;
}


inline CString get_strand_size( matPsStrand::Size size )
{
   CString sz;
   switch( size )
   {
   case matPsStrand::D635:
      sz = _T("1/4\"");
      break;

   case matPsStrand::D794:
      sz = _T("5/16\"");
      break;

   case matPsStrand::D953:
      sz = _T("3/8\"");
      break;

   case matPsStrand::D1111:
      sz = _T("7/16\"");
      break;

   case matPsStrand::D1270:
      sz = _T("1/2\"");
      break;

   case matPsStrand::D1320:
      sz = _T("1/2\" Special (0.52\")");
      break;

   case matPsStrand::D1524:
      sz = _T("0.6\"");
      break;

   case matPsStrand::D1575:
      sz = _T("0.62\"");
      break;

   case matPsStrand::D1778:
      sz = _T("0.7\"");
      break;

   default:
      ATLASSERT(false); // should never get here (unless there is a new strand type)
   }

   return sz;
}

inline BOOL ParseTemplateFile(const LPCTSTR lpszPathName, CString& girderEntry, 
                              CString& leftConnEntry, CString& rightConnEntry,
                              CString& projectCriteriaEntry)
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

   return TRUE;
}


class OptionalDesignHarpedFillUtil
{
public:

   // utility struct to temporarily store and sort rows
   struct StrandRow
   {
      double Elevation;
      std::_tstring fillListString; // "ABC..."

      StrandRow():
         Elevation(0.0)
      {;}

      StrandRow(Float64 elev):
         Elevation(elev)
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

   // Function to retrieve fill letter for X location
   static TCHAR GetFillString(Float64 X)
   {
      ATLASSERT(X>0.0); // should be weeding out negative values

      // This is VERY TxDOT I beam specific. Expect X locations at 2" spacing: 1.0, 3.0, 5.0,...
      Float64 xin = ::ConvertFromSysUnits( X, unitMeasure::Inch );
      int index = (int)Round((xin-1.0)/2.0);
      TCHAR label = (index % 26) + _T('A');

      return label;
   }

   static StrandRowSet GetStrandRowSet(IBroker* pBroker, const pgsPointOfInterest& midPoi);
};

inline OptionalDesignHarpedFillUtil::StrandRowSet OptionalDesignHarpedFillUtil::GetStrandRowSet(IBroker* pBroker, const pgsPointOfInterest& midPoi)
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

// Function to compute columns in table that attempts to group all girders in a span per table
// Returns a list of number of columns per table. Size of list is number of tables to be created
static const int MIN_TBL_COLS=3; // Minimum columns in multi-girder table
static const int MAX_TBL_COLS=8; // Maximum columns in multi-girder table

inline std::list<ColumnIndexType> ComputeTableCols(const std::vector<CGirderKey>& girderKeys)
{
   // Idea here is to break tables at spans, but also try to group if all girders are from different spans
   // First build list of sizes of contiguous blocks of spans
   std::list<ColumnIndexType> contiguous_blocks1;
   GroupIndexType current_group(INVALID_INDEX);
   bool first=false;
   for(std::vector<CGirderKey>::const_iterator it=girderKeys.begin(); it!=girderKeys.end(); it++)
   {
      const CGirderKey& girderKey(*it);

      if (first || current_group!=girderKey.groupIndex)
      {
         first = false;
         current_group = girderKey.groupIndex;
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
      if (MAX_TBL_COLS < ncols)
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




#endif // INCLUDED_PGSEXT_TXDOTOPTIONALDESIGNUTILILITIES_H_

