///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

// DeckRegionGrid.cpp : implementation file
//

#include "stdafx.h"
#include "DeckRegionGrid.h"
#include "CastDeckDlg.h"
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CDeckRegionGrid, CS_DBLCLKS, 0, 0, 0);

const ROWCOL colSpan       = 1;
const ROWCOL colPier       = 2;
const ROWCOL colXback      = 3;
const ROWCOL colXbackUnit  = 4;
const ROWCOL colXahead     = 5;
const ROWCOL colXaheadUnit = 6;
const ROWCOL colSequence   = 7;

class CDeckRegionType : public CGXAbstractUserAttribute
{
public:
   CDeckRegionType(CCastingRegion::RegionType type, IndexType idx)
   {
      m_Type = type;
      m_Index = idx;
   }

   virtual CGXAbstractUserAttribute* Clone() const
   {
      return new CDeckRegionType(m_Type, m_Index);
   }

   CCastingRegion::RegionType m_Type;
   IndexType m_Index; // pier or span index, depending on m_Type
};


/////////////////////////////////////////////////////////////////////////////
// CDeckRegionGrid

CDeckRegionGrid::CDeckRegionGrid()
{
}

CDeckRegionGrid::~CDeckRegionGrid()
{
}

BEGIN_MESSAGE_MAP(CDeckRegionGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CDeckRegionGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CDeckRegionGrid::CustomInit()
{
   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
	Initialize( );
   
   GetParam( )->EnableUndo(FALSE);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   // get all the piers that have continuity
   GET_IFACE2(pBroker, IBridge, pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   PierIndexType nPiers = pBridge->GetPierCount();

   const int num_cols = 7;
   SetRowCount(0);
	SetColCount(num_cols);

   // Turn off selecting whole columns and rows when clicking on a header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELROW & ~GX_SELTABLE));

   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   // no row moving
   GetParam()->EnableMoveRows(FALSE);

  // Set column headers
   SetStyleRange(CGXRange(0,0), CGXStyle()
      .SetWrapText(TRUE)
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_VCENTER)
      .SetEnabled(FALSE)          // disables usage as current cell
      .SetValue("Region")
   );

   SetStyleRange(CGXRange(0, colSpan), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetEnabled(FALSE)          // disables usage as current cell
         .SetValue("Span")
	);

   SetStyleRange(CGXRange(0, colPier), CGXStyle()
      .SetWrapText(TRUE)
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_VCENTER)
      .SetEnabled(FALSE)          // disables usage as current cell
      .SetValue("Pier")
   );
   
   SetStyleRange(CGXRange(0,colXback,0,colXbackUnit), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
       	.SetEnabled(FALSE)          // disables usage as current cell
         .SetValue(_T("Back"))
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		);

   SetStyleRange(CGXRange(0, colXahead, 0, colXaheadUnit), CGXStyle()
      .SetWrapText(TRUE)
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_VCENTER)
      .SetEnabled(FALSE)          // disables usage as current cell
      .SetValue(_T("Ahead"))
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
   );

   SetStyleRange(CGXRange(0, colSequence), CGXStyle()
      .SetWrapText(TRUE)
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_VCENTER)
      .SetEnabled(FALSE)          // disables usage as current cell
      .SetValue("Sequence")
   );

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();
	SetFocus();

   // when CCastDeckActivity is new, or if it is currently set to continuous casting, there may
   // not be any deck casting regions defined. initialize the grid with default data
   // when SetData is called, the grid will be filled with the actual data, if it exists
   // this code creates placeholders and defaults for those values
   IndexType sequenceIdx = 0;
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
   {
      BOOL bUseBack, bUseAhead;
      GetPierUsage(pierIdx, pBridge, &bUseBack, &bUseAhead);

      if (bUseBack || bUseAhead)
      {
         ROWCOL row = GetRowCount()+1;
         InsertRows(row, 1);
         SetStyleRange(CGXRange(row, 0), CGXStyle()
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
            .SetUserAttribute(0, CDeckRegionType(CCastingRegion::Pier, pierIdx))
         );

         Float64 Xback(-0.1), Xahead(-0.1);
         bool bContinuousBack, bContinuousAhead;
         pBridge->IsContinuousAtPier(pierIdx, &bContinuousBack, &bContinuousAhead);

         if (pierIdx == 0 && bContinuousBack)
         {
            // if the first pier is used and it's continuous
            // the back measure is always 100%
            Xback = -1;
         }

         if (pierIdx == nPiers - 1 && bContinuousAhead)
         {
            // if the last pier is used and it's continuous
            // the ahead measure is always 100%
            Xahead = -1;
         }

         SetPierData(pierIdx, bUseBack, Xback, bUseAhead, Xahead, sequenceIdx, pDisplayUnits);

         if (pierIdx == 0 && bContinuousBack)
         {
            // back measure cannot be changed
            SetStyleRange(CGXRange(row, colXback, row, colXbackUnit), CGXStyle().SetEnabled(FALSE).SetInterior(::GetSysColor(COLOR_BTNFACE)));
         }

         if (pierIdx == nPiers - 1 && bContinuousAhead)
         {
            // ahead measure cannot be changed
            SetStyleRange(CGXRange(row, colXahead, row, colXaheadUnit), CGXStyle().SetEnabled(FALSE).SetInterior(::GetSysColor(COLOR_BTNFACE)));
         }
      }

      if (pierIdx != nPiers - 1)
      {
         ROWCOL row = GetRowCount()+1;
         InsertRows(row, 1);
         SpanIndexType spanIdx = (SpanIndexType)pierIdx;
         SetStyleRange(CGXRange(row, 0), CGXStyle()
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
            .SetUserAttribute(0, CDeckRegionType(CCastingRegion::Span, spanIdx))
         );

         Float64 L = pBridge->GetSpanLength(spanIdx);
         SetSpanData(spanIdx, L, sequenceIdx, pDisplayUnits);
      }
   }

   // make it so that text fits correctly in header row
   ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));
   //ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));
	GetParam( )->EnableUndo(TRUE);
}

void CDeckRegionGrid::GetPierUsage(PierIndexType pierIdx, IBridge* pBridge, BOOL* pbUseBack, BOOL* pbUseAhead)
{
   // determies if a pier can be used as a reference point for deck casting region boundary
   PierIndexType nPiers = pBridge->GetPierCount();
   bool bCantilever = false;
   bool bContinuousBack, bContinuousAhead;
   pBridge->IsContinuousAtPier(pierIdx, &bContinuousBack, &bContinuousAhead);
   bool bIntegralBack, bIntegralAhead;
   pBridge->IsIntegralAtPier(pierIdx, &bIntegralBack, &bIntegralAhead);

   if (pierIdx == 0)
   {
      Float64 Lc = pBridge->GetCantileverLength(0, 0, pgsTypes::metStart);
      bCantilever = !IsZero(Lc);
   }

   if (pierIdx == pBridge->GetPierCount() - 1)
   {
      Float64 Lc = pBridge->GetCantileverLength((SpanIndexType)(pierIdx - 1), 0, pgsTypes::metEnd);
      bCantilever = !IsZero(Lc);
   }

   // The back side of the pier can be used if it is not the first pier, unless there is a cantilever, then first pier is ok,
   // or if the is continuity at the pier
   *pbUseBack  = ((pierIdx == nPiers - 1 && bCantilever) || bContinuousBack  || bIntegralBack);

   // The ahead side of the pier can be used if it is not the last pier, unless there is a cantilever, then last pier is ok,
   // or if the is continuity at the pier
   *pbUseAhead = ((pierIdx == 0          && bCantilever) || bContinuousAhead || bIntegralAhead);
}

void CDeckRegionGrid::GetData(CCastDeckActivity& activity)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   std::vector<CCastingRegion> vRegions;
   ROWCOL nRows = GetRowCount();
   for ( ROWCOL row = 1; row <= nRows; row++ )
   {
      vRegions.emplace_back(GetCastingRegion(row,pDisplayUnits));
   }

   activity.SetCastingRegions(vRegions);
}

CCastingRegion CDeckRegionGrid::GetCastingRegion(ROWCOL row,IEAFDisplayUnits* pDisplayUnits)
{
   CCastingRegion::RegionType type = GetRegionType(row);
   if (type == CCastingRegion::Span)
   {
      return GetSpanData(row);
   }
   else
   {
      return GetPierData(row,pDisplayUnits);
   }
}

CCastingRegion::RegionType CDeckRegionGrid::GetRegionType(ROWCOL row)
{
   CGXStyle style;
   GetStyleRowCol(row, 0, style);
   const CDeckRegionType& regionType = dynamic_cast<const CDeckRegionType&>(style.GetUserAttribute(0));
   return regionType.m_Type;
}

CCastingRegion CDeckRegionGrid::GetSpanData(ROWCOL row)
{
   CGXStyle style;
   GetStyleRowCol(row, 0, style);
   const CDeckRegionType& regionType = dynamic_cast<const CDeckRegionType&>(style.GetUserAttribute(0));
   SpanIndexType spanIdx = regionType.m_Index;
   IndexType sequenceIdx = _tstoi(GetCellValue(row, colSequence)) - 1; // minus one because we add one to when displaying the value
   return CCastingRegion(spanIdx, sequenceIdx);
}

CCastingRegion CDeckRegionGrid::GetPierData(ROWCOL row, IEAFDisplayUnits* pDisplayUnits)
{
   CGXStyle style;
   GetStyleRowCol(row, 0, style);
   const CDeckRegionType& regionType = dynamic_cast<const CDeckRegionType&>(style.GetUserAttribute(0));
   PierIndexType pierIdx = regionType.m_Index;

   Float64 start = _tstof(GetCellValue(row, colXback));
   if (_tstoi(GetCellValue(row, colXbackUnit)) == 1L)
   {
      start /= -100;
   }
   else
   {
      start = ::ConvertToSysUnits(start, pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   }

   Float64 end = _tstof(GetCellValue(row, colXahead));
   if (_tstoi(GetCellValue(row, colXaheadUnit)) == 1L)
   {
      end /= -100;
   }
   else
   {
      end = ::ConvertToSysUnits(end, pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   }

   IndexType sequenceIdx = _tstoi(GetCellValue(row, colSequence)) - 1; // minus one because we add one to when displaying the value
   return CCastingRegion(pierIdx, start, end, sequenceIdx);
}

void CDeckRegionGrid::SetData(const CCastDeckActivity& activity)
{
   if (activity.GetCastingType() == CCastDeckActivity::Staged)
   {
      GetParam()->EnableUndo(FALSE);
      GetParam()->SetLockReadOnly(FALSE);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
      GET_IFACE2(pBroker, IBridge, pBridge);

      const auto& vRegions = activity.GetCastingRegions();
      for (const auto& region : vRegions)
      {
         SetRegionData(region, pBridge, pDisplayUnits);
      }

      ResizeColWidthsToFit(CGXRange(0, 0, GetRowCount(), GetColCount()));

      GetParam()->SetLockReadOnly(TRUE);
      GetParam()->EnableUndo(TRUE);
   }
}

void CDeckRegionGrid::SetRegionData(const CCastingRegion& region, IBridge* pBridge, IEAFDisplayUnits* pDisplayUnits)
{
   if (region.m_Type == CCastingRegion::Pier)
   {
      SetPierData(region,pBridge,pDisplayUnits);
   }
   else
   {
      SetSpanData(region,pBridge,pDisplayUnits);
   }
}

void CDeckRegionGrid::SetPierData(const CCastingRegion& region, IBridge* pBridge, IEAFDisplayUnits* pDisplayUnits)
{
   BOOL bUseBack, bUseAhead;
   GetPierUsage(region.m_Index, pBridge, &bUseBack, &bUseAhead);
   ATLASSERT(bUseBack == TRUE || bUseAhead == TRUE);
   SetPierData(region.m_Index, bUseBack, region.m_Start, bUseAhead, region.m_End, region.m_SequenceIndex, pDisplayUnits);
}

void CDeckRegionGrid::SetPierData(PierIndexType pierIdx, BOOL bUseBack, Float64 Xback, BOOL bUseAhead, Float64 Xahead, IndexType sequenceIdx,IEAFDisplayUnits* pDisplayUnits)
{
   ROWCOL row = GetPierRow(pierIdx);

   SetStyleRange(CGXRange(row, colSpan), CGXStyle().SetEnabled(FALSE).SetInterior(::GetSysColor(COLOR_BTNFACE)));

   CString strPierLabel;
   strPierLabel.Format(_T("%d"), LABEL_PIER(pierIdx));
   SetStyleRange(CGXRange(row, colPier), CGXStyle()
      .SetValue(strPierLabel)
      .SetHorizontalAlignment(DT_CENTER)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
   );

   CString strChoice;
   strChoice.Format(_T("%s\n%s"), pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str(), _T("%"));

   if (bUseBack)
   {
      LONG choice = 0;
      if (Xback < 0)
      {
         Xback *= -100;
         choice = 1;
      }
      else
      {
         Xback = ::ConvertFromSysUnits(Xback, pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
      }
      SetStyleRange(CGXRange(row, colXback), CGXStyle().SetEnabled(TRUE).SetHorizontalAlignment(DT_RIGHT).SetValue(Xback));
      SetStyleRange(CGXRange(row, colXbackUnit), CGXStyle()
         .SetEnabled(TRUE)
         .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
         .SetChoiceList(strChoice)
         .SetValue(choice)
      );
   }
   else
   {
      SetStyleRange(CGXRange(row, colXback, row, colXbackUnit),CGXStyle()
         .SetEnabled(FALSE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
         .SetValue(_T("-"))
      );
   }

   if (bUseAhead)
   {
      LONG choice = 0;
      if (Xahead < 0)
      {
         Xahead *= -100;
         choice = 1;
      }
      else
      {
         Xahead = ::ConvertFromSysUnits(Xahead, pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
      }
      SetStyleRange(CGXRange(row, colXahead), CGXStyle().SetEnabled(TRUE).SetHorizontalAlignment(DT_RIGHT).SetValue(Xahead));
      SetStyleRange(CGXRange(row, colXaheadUnit), CGXStyle()
         .SetEnabled(TRUE)
         .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
         .SetChoiceList(strChoice)
         .SetValue(choice)
      );
   }
   else
   {
      SetStyleRange(CGXRange(row, colXahead, row, colXaheadUnit), CGXStyle()
         .SetEnabled(FALSE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
         .SetValue(_T("-"))
      );
   }

   if (bUseBack || bUseAhead)
   {
      SetStyleRange(CGXRange(row, colSequence), CGXStyle().SetHorizontalAlignment(DT_CENTER).SetValue((LONG)LABEL_INDEX(sequenceIdx)));
   }
   else
   {
      SetStyleRange(CGXRange(row, colSequence), CGXStyle()
         .SetEnabled(FALSE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
      );
   }
}

void CDeckRegionGrid::SetSpanData(const CCastingRegion& region, IBridge* pBridge,IEAFDisplayUnits* pDisplayUnits)
{
   Float64 L = pBridge->GetSpanLength(region.m_Index);
   SetSpanData(region.m_Index, L, region.m_SequenceIndex, pDisplayUnits);
}

void CDeckRegionGrid::SetSpanData(SpanIndexType spanIdx, Float64 L, IndexType sequenceIdx, IEAFDisplayUnits* pDisplayUnits)
{
   ROWCOL row = GetSpanRow(spanIdx);

   CString strSpanLabel;
   strSpanLabel.Format(_T("%d"), LABEL_SPAN(spanIdx));
   SetStyleRange(CGXRange(row, colSpan), CGXStyle()
      .SetEnabled(FALSE)
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_VCENTER)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetValue(strSpanLabel)
   );

   SetStyleRange(CGXRange(row, colPier), CGXStyle().SetEnabled(FALSE).SetInterior(::GetSysColor(COLOR_BTNFACE)));

   CString strSpanLength;
   strSpanLength.Format(_T("L = %s"), FormatDimension(L, pDisplayUnits->GetSpanLengthUnit(), true));

   SetStyleRange(CGXRange(row, colXback, row, colXaheadUnit), CGXStyle()
      .SetEnabled(FALSE)
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_VCENTER)
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetValue(strSpanLength)
   );

   SetStyleRange(CGXRange(row, colSequence), CGXStyle().SetEnabled(TRUE).SetHorizontalAlignment(DT_CENTER).SetValue((LONG)LABEL_INDEX(sequenceIdx)));
}

ROWCOL CDeckRegionGrid::GetPierRow(PierIndexType pierIdx)
{
   ROWCOL nRows = GetRowCount();
   for (ROWCOL row = 1; row <= nRows; row++)
   {
      CGXStyle style;
      GetStyleRowCol(row, 0, style);
      if (style.GetIncludeUserAttribute(0))
      {
         const CDeckRegionType& regionType = dynamic_cast<const CDeckRegionType&>(style.GetUserAttribute(0));
         if (regionType.m_Type == CCastingRegion::Pier && regionType.m_Index == pierIdx)
         {
            return row;
         }
      }
   }
   ATLASSERT(false); // pier not found
   return -1;
}

ROWCOL CDeckRegionGrid::GetSpanRow(SpanIndexType spanIdx)
{
   ROWCOL nRows = GetRowCount();
   for (ROWCOL row = 1; row <= nRows; row++)
   {
      CGXStyle style;
      GetStyleRowCol(row, 0, style);
      if (style.GetIncludeUserAttribute(0))
      {
         const CDeckRegionType& regionType = dynamic_cast<const CDeckRegionType&>(style.GetUserAttribute(0));
         if (regionType.m_Type == CCastingRegion::Span && regionType.m_Index == spanIdx)
         {
            return row;
         }
      }
   }
   ATLASSERT(false); // pier not found
   return -1;
}

CString CDeckRegionGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
{
    if (IsCurrentCell(nRow, nCol) && IsActiveCurrentCell())
    {
        CString s;
        CGXControl* pControl = GetControl(nRow, nCol);
        pControl->GetValue(s);
        return s;
    }
    else
    {
        return GetValueRowCol(nRow, nCol);
    }
}

BOOL CDeckRegionGrid::OnEndEditing(ROWCOL nRow,ROWCOL nCol)
{
   ResizeColWidthsToFit(CGXRange(0, 0, GetRowCount(), GetColCount()));
   
   CCastDeckDlg* pParent = (CCastDeckDlg*)GetParent();
   pParent->OnDeckRegionsChanged();
   return CGXGridWnd::OnEndEditing(nRow,nCol);
}

BOOL CDeckRegionGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
{
   if (nCol == colSequence)
   {
      CString str = GetCellValue(nRow, nCol);
      int i = _tstoi(str);
      if (str.IsEmpty() || (i == 0) || (i < 0))
      {
         // can't be converted
         SetWarningText(_T("Sequence number must be a positive integer"));
         return FALSE;
      }
   }

   return CGXGridWnd::OnValidateCell(nRow, nCol);
}

void CDeckRegionGrid::Enable(BOOL bEnable)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CGXStyle style;
   CGXRange range;
   if (bEnable)
   {
      ROWCOL nRows = GetRowCount();

      // Column Headings
      style.SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetEnabled(TRUE)
         .SetReadOnly(TRUE);

      range = CGXRange(0, 0, 0, GetColCount());
      SetStyleRange(range, style);

      // Row Headings
      style.SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetEnabled(TRUE)
         .SetReadOnly(TRUE);

      range = CGXRange(0, 0, nRows, 0);
      SetStyleRange(range, style);

      // main field styles
      style.SetEnabled(TRUE)
         .SetReadOnly(FALSE)
         .SetInterior(::GetSysColor(COLOR_WINDOW))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

      for (ROWCOL row = 1; row <= nRows; row++)
      {
         SetStyleRange(CGXRange(row, colSpan, row, colSequence), CGXStyle()
            .SetEnabled(FALSE)
            .SetReadOnly(TRUE)
            .SetInterior(::GetSysColor(COLOR_BTNFACE))
            .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
            );

         CCastingRegion::RegionType type = GetRegionType(row);
         if (type == CCastingRegion::Span)
         {
            SetStyleRange(CGXRange(row, colSequence), style);
         }
         else
         {
            BOOL bUseBack(FALSE), bUseAhead(FALSE);
            CGXStyle theStyle;
            GetStyleRowCol(row, colXbackUnit, theStyle);
            if (theStyle.GetIncludeChoiceList())
            {
               range = CGXRange(row, colXback, row, colXbackUnit);
               SetStyleRange(range, style);
               bUseBack = TRUE;
            }

            GetStyleRowCol(row, colXaheadUnit, theStyle);
            if (theStyle.GetIncludeChoiceList())
            {
               range = CGXRange(row, colXahead, row, colXaheadUnit);
               SetStyleRange(range, style);
               bUseAhead = TRUE;
            }

            if (bUseBack || bUseAhead)
            {
               range = CGXRange(row, colSequence);
               SetStyleRange(range, style);
            }
         }
      }
   }
   else
   {
      style.SetEnabled(FALSE)
         .SetReadOnly(TRUE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_GRAYTEXT));

      range = CGXRange(0, 0, GetRowCount(), GetColCount());
      SetStyleRange(range, style);
   }

   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(FALSE);
}
