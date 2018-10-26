///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#ifndef INCLUDED_REPORTSTYLEHOLDER_H_
#define INCLUDED_REPORTSTYLEHOLDER_H_

#include <Reporting\ReportingExp.h>
#include <Reporter\Reporter.h>

#include <MathEx.h>

#define DECLARE_UV_PROTOTYPE( type, proto ) type proto
#define DEFINE_UV_PROTOTYPE( proto, im, _bShowTag ) \
   proto.SetUnitOfMeasure(&im.UnitOfMeasure ); \
   proto.ShowUnitTag(_bShowTag); \
   proto.SetZeroTolerance( im.Tol ); \
   proto.SetFormat( im.Format );\
   proto.SetWidth( im.Width ); \
   proto.SetPrecision( im.Precision )

#define RPT_OFFSET(_value_,_um_) \
_um_.SetValue(fabs(_value_)) << (::Sign(_value_) < 0 ? " L" : (::Sign(_value_) > 0 ? " R" : ""))

#define RPT_BEARING(_value_) rptRcString(_value_,true)
#define RPT_ANGLE(_value_) rptRcString(_value_,true)

#define CB_NONE    0x0001
#define CB_THIN    0x0002
#define CJ_LEFT    0x0004
#define CJ_RIGHT   0x0008

/*****************************************************************************
CLASS 
   pgsReportStyleHolder

   A convenient place to hold common text styles for the Pgsuper application


DESCRIPTION
   A purely static class used for maintaining consistent text style
   information

COPYRIGHT
   Copyright (c) 1997
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 09.26.1997 : Created file
*****************************************************************************/

class REPORTINGCLASS pgsReportStyleHolder
{
public:
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Initialize the styles in the style library
   static void InitStyles();

   //------------------------------------------------------------------------
   // Return the Style for the Report Title.
   static const std::string& GetReportTitleStyle();

   //------------------------------------------------------------------------
   // Return the Style for the Report Subtitle.
   static const std::string& GetReportSubtitleStyle();

   //------------------------------------------------------------------------
   // Return the Style for the Chapter Titles.
   static const std::string& GetChapterTitleStyle();

   //------------------------------------------------------------------------
   // Return the style for Headings
   static const std::string& GetHeadingStyle();

   //------------------------------------------------------------------------
   // Return the style for Subheadings
   static const std::string& GetSubheadingStyle(); 

   //------------------------------------------------------------------------
   // Return the style for table column headings
   static const std::string& GetTableColumnHeadingStyle();

   //------------------------------------------------------------------------
   // Returns the style for table cells.  Use the CS_XXXX and CJ_XXXX constants
   // to specify a style.
   static const std::string& GetTableCellStyle(Uint32 style);

   //------------------------------------------------------------------------
   // Returns the style for table cells.  Use the CS_XXXX and CJ_XXXX constants
   // to specify a style.
   static const std::string& GetTableStripeRowCellStyle(Uint32 style);

   //------------------------------------------------------------------------
   static const std::string& GetFootnoteStyle();

   //------------------------------------------------------------------------
   static const std::string& GetCopyrightStyle();

   //------------------------------------------------------------------------
   // Returns the maximum table width to be used in any given chapter
   static double GetMaxTableWidth();

   //------------------------------------------------------------------------ 
   // Returns a pointer to a dynamically allocated defaultly configured table 
   // with 0.75" wide columns
   // If bLoadingColumn is true, column 1 is 1.5" wide
   static rptRcTable* CreateDefaultTable(ColumnIndexType numColumns, std::string label);

   //------------------------------------------------------------------------ 
   // Returns a pointer to a dynamically allocated defaultly configured table 
   // with 0.75" wide columns
   // This table does not have a heading row.
   static rptRcTable* CreateTableNoHeading(ColumnIndexType numColumns, std::string label);

   static void ConfigureTable(rptRcTable* pTable);

   //------------------------------------------------------------------------ 
   // Returns the path to where the images are stored.
   static const std::string& GetImagePath();

   static void SetReportCoverImage(const char* strImagePath);
   static const std::string& GetReportCoverImage();

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   static std::string ms_ReportTitleStyle;
   static std::string ms_ReportSubtitleStyle;
   static std::string ms_ChapterTitleStyle;
   static std::string ms_HeadingStyle;
   static std::string ms_SubheadingStyle;
   static std::string ms_TableColumnHeadingStyle;
   static std::string ms_FootnoteStyle;
   static std::string ms_CopyrightStyle;
   static std::string ms_TableCellStyle[4];
   static std::string ms_TableStripeRowCellStyle[4];
   static std::auto_ptr<std::string> ms_pImagePath;
   static double ms_MaxTableWidth;
   static std::string ms_ReportCoverImage;

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsReportStyleHolder();
   // Prevent accidental copying and assignment
   pgsReportStyleHolder(const pgsReportStyleHolder&);
   pgsReportStyleHolder& operator=(const pgsReportStyleHolder&);

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_REPORTSTYLEHOLDER_H_

