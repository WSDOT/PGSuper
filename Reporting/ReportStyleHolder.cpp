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

#include "stdafx.h"
#include <Reporting\ReportStyleHolder.h>
#include <ctype.h>

#include <EAF\EAFUtilities.h>
#include <EAF\EAFApp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsReportStyleHolder
****************************************************************************/


std::_tstring pgsReportStyleHolder::ms_ReportTitleStyle           = _T("ReportTitleStyle");
std::_tstring pgsReportStyleHolder::ms_ReportSubtitleStyle        = _T("ReportSubtitleStyle");
std::_tstring pgsReportStyleHolder::ms_ChapterTitleStyle          = _T("ChapterTitleStyle");
std::_tstring pgsReportStyleHolder::ms_HeadingStyle               = _T("HeadingStyle");
std::_tstring pgsReportStyleHolder::ms_SubheadingStyle            = _T("SubheadingStyle");
std::_tstring pgsReportStyleHolder::ms_TableColumnHeadingStyle    = _T("TableColumnHeadingStyle");
std::_tstring pgsReportStyleHolder::ms_FootnoteStyle              = _T("Footnote");
std::_tstring pgsReportStyleHolder::ms_CopyrightStyle             = _T("Copyright");
std::_tstring pgsReportStyleHolder::ms_TableCellStyle[6]          = {   _T("NB-LJ"),   _T("TB-LJ"),   _T("NB-RJ"),   _T("TB-RJ"),   _T("NB-CJ"),   _T("TB-CJ")};
std::_tstring pgsReportStyleHolder::ms_TableStripeRowCellStyle[6] = {_T("SR-NB-LJ"),_T("SR-TB-LJ"),_T("SR-NB-RJ"),_T("SR-TB-RJ"),_T("SR-NB-CJ"),_T("SR-TB-CJ")};
std::_tstring pgsReportStyleHolder::ms_ReportCoverImage           = _T("");

std::auto_ptr<std::_tstring> pgsReportStyleHolder::ms_pImagePath;

Float64 pgsReportStyleHolder::ms_MaxTableWidth = 7.5; // 7.5" wide tables



////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================


void pgsReportStyleHolder::InitStyles()
{
   // load up the style library
   bool flag;
   rptFontStyleLibrary* psl = rptFontStyleLibrary::Instance();

   // Configure the default style
   rptRiStyle& default_style = psl->GetDefaultStyle();
   default_style.SetFontType(rptRiStyle::SWISS);
   default_style.SetFontSize(9);

   // Report Title
   rptRiStyle rpttitle;
   rpttitle.SetFontSize(16);
   rpttitle.SetBold( true );
   rpttitle.SetFontType(rptRiStyle::SWISS);
   rpttitle.SetAlignment(rptRiStyle::CENTER);
   flag = psl->AddNamedStyle(ms_ReportTitleStyle, rpttitle);
   //CHECK(flag);

   // Report Subtitle
   rptRiStyle rptsubtitle;
   rptsubtitle.SetFontSize(15);
   rptsubtitle.SetItalic( true );
   rptsubtitle.SetFontType(rptRiStyle::SWISS);
   rptsubtitle.SetAlignment(rptRiStyle::CENTER);
   flag = psl->AddNamedStyle(ms_ReportSubtitleStyle, rptsubtitle);
   //CHECK(flag);

   // Chapter Titles
   rptRiStyle  chaptertitle;
   chaptertitle.SetFontType(rptRiStyle::SWISS);
   chaptertitle.SetFontSize(13);
   chaptertitle.SetAlignment(rptRiStyle::LEFT);
   chaptertitle.SetBold( true );
   chaptertitle.SetColor( rptRiStyle::Blue );
   flag = psl->AddNamedStyle(ms_ChapterTitleStyle, chaptertitle);
   //CHECK(flag);
   
   // Headings
   rptRiStyle headings;
   headings.SetFontType(rptRiStyle::SWISS);
   headings.SetFontSize(11);
   headings.SetBold( true );
   flag = psl->AddNamedStyle(ms_HeadingStyle, headings);
   //CHECK(flag);
   
   // SubHeadings
   rptRiStyle subheadings;
   subheadings.SetFontType(rptRiStyle::SWISS);
   subheadings.SetFontSize(9);
   subheadings.SetItalic( true );
   subheadings.SetBold( true );
   flag = psl->AddNamedStyle(ms_SubheadingStyle, subheadings);
   //CHECK(flag);

   // Table Column Headings
   rptRiStyle colheadings;
   colheadings.SetFontSize(9);
   colheadings.SetFontType(rptRiStyle::SWISS);
   colheadings.SetAlignment(rptRiStyle::CENTER);
   colheadings.SetVerticalAlignment( rptRiStyle::TOP );
   colheadings.SetBold( true );
   colheadings.SetBGColor( rptRiStyle::LightSteelBlue );
   flag = psl->AddNamedStyle(ms_TableColumnHeadingStyle, colheadings);
   //CHECK(flag);

   // Setup basic style
   rptRiStyle cell;
   cell.SetFontType(rptRiStyle::SWISS);
   cell.SetFontSize(9);

   // Style for No Border, Left Justified
   cell.SetAlignment(rptRiStyle::LEFT);
   cell.SetVerticalAlignment( rptRiStyle::TOP );
   cell.SetTopBorder(rptRiStyle::NOBORDER);
   cell.SetBottomBorder(rptRiStyle::NOBORDER);
   cell.SetLeftBorder(rptRiStyle::NOBORDER);
   cell.SetRightBorder(rptRiStyle::NOBORDER);
   psl->AddNamedStyle(ms_TableCellStyle[0],cell);

   // Style for Thin Border, Left Justified
   cell.SetTopBorder(rptRiStyle::HAIR_THICK);
   cell.SetBottomBorder(rptRiStyle::HAIR_THICK);
   cell.SetLeftBorder(rptRiStyle::HAIR_THICK);
   cell.SetRightBorder(rptRiStyle::HAIR_THICK);
   psl->AddNamedStyle(ms_TableCellStyle[1],cell);


   // Style for No Border, Right Justified
   cell.SetAlignment(rptRiStyle::RIGHT);
   cell.SetTopBorder(rptRiStyle::NOBORDER);
   cell.SetBottomBorder(rptRiStyle::NOBORDER);
   cell.SetLeftBorder(rptRiStyle::NOBORDER);
   cell.SetRightBorder(rptRiStyle::NOBORDER);
   psl->AddNamedStyle(ms_TableCellStyle[2],cell);

   // Style for Thin Border, Right Justified
   cell.SetTopBorder(rptRiStyle::HAIR_THICK);
   cell.SetBottomBorder(rptRiStyle::HAIR_THICK);
   cell.SetLeftBorder(rptRiStyle::HAIR_THICK);
   cell.SetRightBorder(rptRiStyle::HAIR_THICK);
   psl->AddNamedStyle(ms_TableCellStyle[3],cell);

   // Style for No Border, Center Justified
   cell.SetAlignment(rptRiStyle::CENTER);
   cell.SetTopBorder(rptRiStyle::NOBORDER);
   cell.SetBottomBorder(rptRiStyle::NOBORDER);
   cell.SetLeftBorder(rptRiStyle::NOBORDER);
   cell.SetRightBorder(rptRiStyle::NOBORDER);
   psl->AddNamedStyle(ms_TableCellStyle[4],cell);

   // Style for Thin Border, Right Justified
   cell.SetTopBorder(rptRiStyle::HAIR_THICK);
   cell.SetBottomBorder(rptRiStyle::HAIR_THICK);
   cell.SetLeftBorder(rptRiStyle::HAIR_THICK);
   cell.SetRightBorder(rptRiStyle::HAIR_THICK);
   psl->AddNamedStyle(ms_TableCellStyle[5],cell);

   for ( int i = 0; i < 6; i++ )
   {
      rptRiStyle stripe_row_cell = psl->GetNamedStyle(ms_TableCellStyle[i]);
      stripe_row_cell.SetBGColor( rptRiStyle::AliceBlue );
      psl->AddNamedStyle( ms_TableStripeRowCellStyle[i],stripe_row_cell);
   }

   // Footnote style
   rptRiStyle footnote;
   footnote.SetFontType( rptRiStyle::SWISS );
   footnote.SetFontSize(8);
   footnote.SetAlignment(rptRiStyle::LEFT);
   flag = psl->AddNamedStyle( ms_FootnoteStyle, footnote );

   // Copyright style
   rptRiStyle copyright;
   copyright.SetFontType( rptRiStyle::SWISS );
   copyright.SetFontSize(8);
   copyright.SetAlignment(rptRiStyle::CENTER);
   flag = psl->AddNamedStyle( ms_CopyrightStyle, copyright );
}


const std::_tstring& pgsReportStyleHolder::GetReportTitleStyle()
{
return ms_ReportTitleStyle;
}

const std::_tstring& pgsReportStyleHolder::GetReportSubtitleStyle()
{
return ms_ReportSubtitleStyle;
}

const std::_tstring& pgsReportStyleHolder::GetChapterTitleStyle()
{
return ms_ChapterTitleStyle;
}

const std::_tstring& pgsReportStyleHolder::GetHeadingStyle()
{
return ms_HeadingStyle;
}

const std::_tstring& pgsReportStyleHolder::GetSubheadingStyle()
{
return ms_SubheadingStyle;
}

const std::_tstring& pgsReportStyleHolder::GetTableColumnHeadingStyle()
{
   return ms_TableColumnHeadingStyle;
}

const std::_tstring& pgsReportStyleHolder::GetTableCellStyle(Uint32 style)
{
   // Bit 1 = Border Style
   // Bit 2 = Justification
   //
   // 0 = No Border, Left Justified
   // 1 = Thin Border, Left Justified
   // 2 = No Border, Right Justified
   // 3 = Thin Border, Right Justified
   // 4 = No Border, Center Justified
   // 5 = Thin Border, Center Justified

   Int16 index = 0;

   if ( style & CB_NONE )
      index |= 0x0000;
   else if ( style & CB_THIN )
      index |= 0x0001;
   
   if ( style & CJ_LEFT )
      index |= 0x0000;
   else if ( style & CJ_RIGHT )
      index |= 0x0002;
   else if ( style & CJ_CENTER )
      index |= 0x0004;

   CHECK( 0 <= index && index <= 5 );

   return ms_TableCellStyle[index];
}

const std::_tstring& pgsReportStyleHolder::GetTableStripeRowCellStyle(Uint32 style)
{
   // Bit 1 = Border Style
   // Bit 2 = Justification
   //
   // 0 = No Border, Left Justified
   // 1 = Thin Border, Left Justified
   // 2 = No Border, Right Justified
   // 3 = Thin Border, Right Justified
   // 4 = No Border, Center Justified
   // 5 = Thin Border, Center Justified

   Int16 index = 0;

   if ( style & CB_NONE )
      index |= 0x0000;
   else if ( style & CB_THIN )
      index |= 0x0001;
   
   if ( style & CJ_LEFT )
      index |= 0x0000;
   else if ( style & CJ_RIGHT )
      index |= 0x0002;
   else if ( style & CJ_CENTER )
      index |= 0x0004;

   CHECK( 0 <= index && index <= 5 );

   return ms_TableStripeRowCellStyle[index];
}

const std::_tstring& pgsReportStyleHolder::GetFootnoteStyle()
{
   return ms_FootnoteStyle;
}

const std::_tstring& pgsReportStyleHolder::GetCopyrightStyle()
{
   return ms_CopyrightStyle;
}

Float64 pgsReportStyleHolder::GetMaxTableWidth()
{
   return ms_MaxTableWidth;
}

rptRcTable* pgsReportStyleHolder::CreateDefaultTable(ColumnIndexType numColumns, LPCTSTR lpszLabel)
{
   rptRcTable* pTable = new rptRcTable( numColumns, 0.0/*pgsReportStyleHolder::GetMaxTableWidth()*/ );
   if ( lpszLabel != NULL)
      pTable->TableLabel() << lpszLabel;

   pgsReportStyleHolder::ConfigureTable(pTable);

   return pTable;
}

rptRcTable* pgsReportStyleHolder::CreateTableNoHeading(ColumnIndexType numColumns, LPCTSTR lpszLabel)
{
   rptRcTable* pTable = CreateDefaultTable(numColumns,lpszLabel);

   pTable->SetTableHeaderStyle( pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT) );
   pTable->SetNumberOfHeaderRows(0);

   for ( Uint16 i = 0; i < numColumns; i++ )
   {
      pTable->SetColumnStyle(i, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT ) );
      pTable->SetStripeRowColumnStyle( i, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT) );
   }

   return pTable;
}

void pgsReportStyleHolder::ConfigureTable(rptRcTable* pTable)
{
   pTable->SetStyleName( pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT) );
   pTable->TableLabel().SetStyleName( pgsReportStyleHolder::GetHeadingStyle() );
   pTable->SetTableHeaderStyle( pgsReportStyleHolder::GetTableColumnHeadingStyle() );
   pTable->SetOutsideBorderStyle( rptRiStyle::HAIR_THICK );
   pTable->SetInsideBorderStyle( rptRiStyle::NOBORDER );
   pTable->SetCellPad( 0.03125 );

   pTable->EnableRowStriping(true);

   ColumnIndexType numColumns = pTable->GetNumberOfColumns();

   for ( ColumnIndexType i = 0; i < numColumns; i++ )
   {
      pTable->SetColumnStyle( i, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT) );
      pTable->SetStripeRowColumnStyle( i, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT) );
   }
}

void make_upper( std::_tstring::iterator begin,std::_tstring::iterator end)
{
   while ( begin != end )
   {
      *begin = toupper(*begin);
      begin++;
   }
}

const std::_tstring& pgsReportStyleHolder::GetImagePath()
{
   if ( ms_pImagePath.get() == 0 )
   {
      TCHAR szBuff[_MAX_PATH];
      ::GetModuleFileName(::GetModuleHandle(NULL), szBuff, _MAX_PATH);
      std::_tstring filename(szBuff);
      make_upper( filename.begin(), filename.end() );

      CEAFApp* pApp = EAFGetApp();
      // find first occurance of the application name
      std::_tstring strAppName(pApp->m_pszAppName);
      make_upper( strAppName.begin(), strAppName.end() );
      std::_tstring::size_type loc = filename.find(strAppName);
      if ( loc != std::_tstring::npos )
      {
         loc += strAppName.length();
      }
      else
      {
         // something is wrong... that find should have succeeded
         // hard code the default install location so that there is a remote chance of success
         TCHAR szNativeProgramFilesFolder[MAX_PATH];
         ExpandEnvironmentStrings(_T("%ProgramW6432%"),szNativeProgramFilesFolder,ARRAYSIZE(szNativeProgramFilesFolder));
         filename = _T("\\") + std::_tstring(szNativeProgramFilesFolder) + _T("\\WSDOT\\") + strAppName;
         loc = filename.length();
      }

      filename.replace(filename.begin()+loc,filename.end(),_T("\\IMAGES\\"));
      ms_pImagePath = std::auto_ptr<std::_tstring>(new std::_tstring(filename));
   }

   return *ms_pImagePath;
}

void pgsReportStyleHolder::SetReportCoverImage(LPCTSTR strImagePath)
{
   ms_ReportCoverImage = strImagePath;
   make_upper(ms_ReportCoverImage.begin(),ms_ReportCoverImage.end());
}

const std::_tstring& pgsReportStyleHolder::GetReportCoverImage()
{
   if ( ms_ReportCoverImage == _T("") )
   {
      SetReportCoverImage(std::_tstring(GetImagePath() + _T("title_page_art.gif")).c_str());
   }
   return ms_ReportCoverImage;
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

