///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

// SegmentSpacingGrid.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SegmentSpacingGrid.h"
#include "GirderSegmentSpacingPage.h"
#include "TemporarySupportDlg.h"

#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\BeamFactory.h>

#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CSegmentSpacingGrid, CS_DBLCLKS, 0, 0, 0);

void DDV_SpacingGrid(CDataExchange* pDX,int nIDC,CSegmentSpacingGrid* pGrid)
{
   if (!pDX->m_bSaveAndValidate )
      return;

   pDX->PrepareCtrl(nIDC);
   if ( !pGrid->ValidateSpacing() )
   {
      pDX->Fail();
   }
}


class CNameGroupData : public CGXAbstractUserAttribute
{
public:
   CNameGroupData(GirderIndexType firstGdrIdx,GirderIndexType lastGdrIdx)
   {
      m_FirstGirderIdx = firstGdrIdx;
      m_LastGirderIdx  = lastGdrIdx;
   }

   virtual CGXAbstractUserAttribute* Clone() const
   {
      return new CNameGroupData(m_FirstGirderIdx,m_LastGirderIdx);
   }

   GirderIndexType m_FirstGirderIdx;
   GirderIndexType m_LastGirderIdx;
};

/////////////////////////////////////////////////////////////////////////////
// CSegmentSpacingGrid

CSegmentSpacingGrid::CSegmentSpacingGrid()
{
//   RegisterClass();
   m_bEnabled = TRUE;
}

CSegmentSpacingGrid::~CSegmentSpacingGrid()
{
}

BEGIN_MESSAGE_MAP(CSegmentSpacingGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CGirderNameGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_COMMAND(IDC_EXPAND, OnExpand)
	ON_COMMAND(IDC_JOIN, OnJoin)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CSegmentSpacingGrid::SetSkewAngle(Float64 skewAngle)
{
   m_SkewAngle = skewAngle;
   UpdateGrid();
}

bool CSegmentSpacingGrid::InputSpacing() const
{
   ATLASSERT(m_MinGirderSpacing.size() == m_MaxGirderSpacing.size());
   std::vector<Float64>::const_iterator minIter, maxIter;
   for ( minIter  = m_MinGirderSpacing.begin(), maxIter  = m_MaxGirderSpacing.begin();
         minIter != m_MinGirderSpacing.end(),   maxIter != m_MaxGirderSpacing.end();
         minIter++, maxIter++ )
   {
      Float64 v = (*maxIter) - (*minIter);
      if ( !IsZero(v) )
         return true;
   }

   return false;
}

/////////////////////////////////////////////////////////////////////////////
// CSegmentSpacingGrid message handlers
void CSegmentSpacingGrid::CustomInit()
{
	Initialize( );

   GetParam()->EnableUndo(FALSE);

		// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELTABLE & ~GX_SELROW));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);
   GetParam()->EnableMoveCols(FALSE);

   UpdateGrid();

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();

   EnableGridToolTips();
   SetStyleRange(CGXRange(0,0,GetRowCount(),GetColCount()),
      CGXStyle()
      .SetWrapText(TRUE)
      .SetAutoSize(TRUE)
      .SetUserAttribute(GX_IDS_UA_TOOLTIPTEXT,CString("To regroup girder spacing, select column headings, right click over the grid, and select Expand or Join from the menu"))
      );
	
   GetParam()->EnableUndo(TRUE);

	SetFocus();
}

void CSegmentSpacingGrid::InitializeGridData(CGirderSpacing2* pSpacing)
{
   m_pSpacing = pSpacing;
   UpdateGrid();
}

void CSegmentSpacingGrid::UpdateGrid()
{
   if ( GetSafeHwnd() == nullptr )
      return; // grid isn't ready for filling

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   pgsTypes::SupportedBeamSpacing spacingType = m_pSpacing->GetBridgeDescription()->GetGirderSpacingType();
   const unitmgtLengthData& spacingUnit = IsGirderSpacing(spacingType) // if
                                        ? pDisplayUnits->GetXSectionDimUnit()     // then
                                        : pDisplayUnits->GetComponentDimUnit();   // else

   GroupIndexType nSpacingGroups = m_pSpacing->GetSpacingGroupCount();
   GirderIndexType nGirders      = m_pSpacing->GetSpacingCount() + 1;

   // Compute the girder spacing correction for skew angle
   Float64 skewCorrection;
   if ( m_pSpacing->GetMeasurementType() == pgsTypes::NormalToItem )
   {
      skewCorrection = 1;
   }
   else
   {
      skewCorrection = fabs(1/cos(m_SkewAngle));
   }

   const ROWCOL num_rows = 2;
   const ROWCOL num_cols = (ROWCOL)nSpacingGroups;

   SetRowCount(num_rows);
	SetColCount(num_cols);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   const CBridgeDescription2* pBridgeDesc = m_pSpacing->GetBridgeDescription();
   pgsTypes::SupportedDeckType deckType  = pBridgeDesc->GetDeckDescription()->GetDeckType();

   m_MinGirderSpacing.clear();
   m_MaxGirderSpacing.clear();
   for ( GroupIndexType spacingGrpIdx = 0; spacingGrpIdx < nSpacingGroups; spacingGrpIdx++ )
   {
      Float64 spacing;
      GirderIndexType firstGdrIdx, lastGdrIdx;
      m_pSpacing->GetSpacingGroup(spacingGrpIdx,&firstGdrIdx,&lastGdrIdx,&spacing);

      // if this is the last spacing group
      // then the last girder index needs to be forced to nGirders-1
      if ( spacingGrpIdx == nSpacingGroups-1 )
      {
         ATLASSERT(lastGdrIdx == nGirders-1); // why is the last girder index not correct?
         lastGdrIdx = nGirders-1;
      }

      CString strHeading;
      if ( firstGdrIdx == lastGdrIdx )
      {
         strHeading.Format(_T("%s (%s)"),LABEL_GIRDER(firstGdrIdx),spacingUnit.UnitOfMeasure.UnitTag().c_str());
      }
      else
      {
         strHeading.Format(_T("%s-%s (%s)"),LABEL_GIRDER(firstGdrIdx),LABEL_GIRDER(lastGdrIdx),spacingUnit.UnitOfMeasure.UnitTag().c_str());
      }

      CNameGroupData userData(firstGdrIdx,lastGdrIdx);

      SetStyleRange(CGXRange(0,ROWCOL(spacingGrpIdx+1)), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetEnabled(FALSE)
         .SetValue(strHeading)
         .SetUserAttribute(0,userData)
         );

      const CGirderGroupData* pGroup = m_pSpacing->GetGirderGroup();
      GroupIndexType grpIdx = pGroup->GetIndex();

      // get valid girder spacing for this group
      Float64 minGirderSpacing = -MAX_GIRDER_SPACING;
      Float64 maxGirderSpacing =  MAX_GIRDER_SPACING;
      for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
      {
         const GirderLibraryEntry* pGdrEntry = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirder(gdrIdx)->GetGirderLibraryEntry();
         const IBeamFactory::Dimensions& dimensions = pGdrEntry->GetDimensions();
         CComPtr<IBeamFactory> factory;
         pGdrEntry->GetBeamFactory(&factory);

         // save spacing range to local class data
         Float64 minGS, maxGS;
         factory->GetAllowableSpacingRange(dimensions,deckType,spacingType,&minGS, &maxGS);
         minGS *= skewCorrection;
         maxGS *= skewCorrection;

         if ( IsGirderSpacing(spacingType) )
         {
            // girder spacing
            minGirderSpacing = Max(minGirderSpacing,minGS);
            maxGirderSpacing = Min(maxGirderSpacing,maxGS);
         }
         else
         {
            // joint spacing
            minGirderSpacing = 0;
            maxGirderSpacing = Min(maxGirderSpacing-minGirderSpacing,maxGS-minGS);
         }
      }

      m_MinGirderSpacing.push_back(minGirderSpacing);
      m_MaxGirderSpacing.push_back(maxGirderSpacing);

      ATLASSERT( minGirderSpacing <= maxGirderSpacing );

      spacing = ForceIntoRange(minGirderSpacing,spacing,maxGirderSpacing);
      m_pSpacing->SetGirderSpacing(spacingGrpIdx,spacing);

      CString strSpacing;
      strSpacing.Format(_T("%s"),FormatDimension(spacing,spacingUnit,false));
      SetStyleRange(CGXRange(1,ROWCOL(spacingGrpIdx+1)), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         .SetEnabled(TRUE)
         .SetReadOnly(FALSE)
         .SetInterior(::GetSysColor(COLOR_WINDOW))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetValue( strSpacing )
         );

      if (maxGirderSpacing < MAX_GIRDER_SPACING)
      {
         if ( IsGirderSpacing(spacingType) )
         {
            // girder spacing
            strSpacing.Format(_T("%s - %s"), 
               FormatDimension(minGirderSpacing,spacingUnit,false),
               FormatDimension(maxGirderSpacing,spacingUnit,false));
         }
         else
         {
            // joint spacing
            strSpacing.Format(_T("%s - %s"), 
               FormatDimension(0.0,spacingUnit,false),
               FormatDimension(maxGirderSpacing-minGirderSpacing,spacingUnit,false));
         }
      }
      else
      {
         strSpacing.Format(_T("%s or more"), 
            FormatDimension(minGirderSpacing,spacingUnit,false));
      }

      SetStyleRange(CGXRange(2,ROWCOL(spacingGrpIdx+1)), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         .SetEnabled(FALSE)
         .SetReadOnly(TRUE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetValue( strSpacing )
         );
   }

   SetStyleRange(CGXRange(0,0), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetEnabled(FALSE)
      .SetValue(_T(""))
      );

   SetStyleRange(CGXRange(1,0), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetEnabled(FALSE)
      .SetValue(_T("Spacing"))
      );

   SetStyleRange(CGXRange(2,0), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetEnabled(FALSE)
      .SetValue(_T("Allowable"))
      );

   // make it so that text fits correctly in header row
   ResizeColWidthsToFit(CGXRange(0,0,num_rows,num_cols),FALSE);

   // this will get the styles of all the cells to be correct if the size of the grid changed
   Enable(m_bEnabled);

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

BOOL CSegmentSpacingGrid::OnRButtonHitRowCol(ROWCOL nHitRow,ROWCOL nHitCol,ROWCOL nDragRow,ROWCOL nDragCol,CPoint point,UINT nFlags,WORD nHitState)
{
   if ( nHitState & GX_HITSTART )
      return TRUE;

   if ( nHitCol != 0 )
   {
      CRowColArray selCols;
      ROWCOL nSelected = GetSelectedCols(selCols);

      if ( 0 < nSelected )
      {
         CMenu menu;
         VERIFY( menu.LoadMenu(IDR_GIRDER_GRID_CONTEXT) );
         ClientToScreen(&point);
         menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);

         return TRUE;
      }
   }

   return FALSE;
}

void CSegmentSpacingGrid::OnExpand()
{
   CRowColArray selCols;
   ROWCOL nSelCols = GetSelectedCols(selCols);

   if ( nSelCols == 0 )
      return;

   if ( nSelCols == GetColCount() )
   {
      m_pSpacing->ExpandAll();
   }
   else
   {
      for ( int i = nSelCols-1; 0 <= i; i-- )
      {
         ROWCOL col = selCols[i];
         GroupIndexType grpIdx = GroupIndexType(col-1);

         m_pSpacing->Expand(grpIdx);
      }
   }

   UpdateGrid();
}

void CSegmentSpacingGrid::OnJoin()
{
   CRowColArray selLeftCols,selRightCols;
   GetSelectedCols(selLeftCols,selRightCols,FALSE,FALSE);
   ROWCOL nSelRanges = (ROWCOL)selLeftCols.GetSize();
   ASSERT( 0 < nSelRanges ); // must be more that one selected column to join

   for (ROWCOL i = 0; i < nSelRanges; i++ )
   {
      ROWCOL firstCol = selLeftCols[i];
      ROWCOL lastCol  = selRightCols[i];

      CGXStyle firstColStyle, lastColStyle;
      GetStyleRowCol(0,firstCol,firstColStyle);
      GetStyleRowCol(0,lastCol, lastColStyle);

      const CNameGroupData& firstColUserData = dynamic_cast<const CNameGroupData&>(firstColStyle.GetUserAttribute(0));
      const CNameGroupData& lastColUserData  = dynamic_cast<const CNameGroupData&>(lastColStyle.GetUserAttribute(0));

      GirderIndexType firstGdrIdx = firstColUserData.m_FirstGirderIdx;
      GirderIndexType lastGdrIdx  = lastColUserData.m_LastGirderIdx;

      m_pSpacing->Join(firstGdrIdx,lastGdrIdx,firstGdrIdx);
   }

   UpdateGrid();
}

void CSegmentSpacingGrid::Enable(BOOL bEnable)
{
   m_bEnabled = bEnable;

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CGXStyle style;
   CGXRange range;
   if ( bEnable )
   {
      // Girder/Joint Spacing

      style.SetEnabled(TRUE)
           .SetReadOnly(FALSE)
           .SetInterior(::GetSysColor(COLOR_WINDOW))
           .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

      if ( 0 < GetColCount() )
      {
         for ( ROWCOL col = 1; col <= GetColCount(); col++ )
         {
            // all of the spacing values
            range = CGXRange(1,col);
            if ( IsEqual(m_MinGirderSpacing[col-1],m_MaxGirderSpacing[col-1]) )
            {
               // disable if there isn't a "range" of input
               style.SetEnabled(FALSE)
                    .SetReadOnly(TRUE)
                    .SetInterior(::GetSysColor(COLOR_BTNFACE))
                    .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
            }

            SetStyleRange(range,style);
         }
      }

      // Column Headings
      style.SetInterior(::GetSysColor(COLOR_BTNFACE))
           .SetEnabled(FALSE)
           .SetReadOnly(TRUE);

      // across the top
      range = CGXRange(0,0,0,GetColCount());
      SetStyleRange(range,style);

      // the "Ahead/Back" box
      range = CGXRange(1,0);
      SetStyleRange(range,style);

      // the "Allowable" box + the actual allowable spacing cells
      range = CGXRange(2,0,2,GetColCount());
      SetStyleRange(range,style);
   }
   else
   {
      style.SetEnabled(FALSE)
           .SetReadOnly(TRUE)
           .SetInterior(::GetSysColor(COLOR_BTNFACE))
           .SetTextColor(::GetSysColor(COLOR_GRAYTEXT));

      range = CGXRange(0,0,GetRowCount(),GetColCount());
      SetStyleRange(range,style);
   }


   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(FALSE);
}

BOOL CSegmentSpacingGrid::ValidateSpacing()
{
   ROWCOL nCols = GetColCount();
   for ( ROWCOL col = 1; col <= nCols; col++ )
   {
      if ( !OnValidateCell(1,col) )
      {
         DisplayWarningText();
         return FALSE;
      }
   }

   return TRUE;
}

BOOL CSegmentSpacingGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
{
   CGXControl* pControl = GetControl(nRow,nCol);
   CWnd* pWnd = (CWnd*)pControl;

   CString strText;
   if ( pControl->IsInit() )
   {
      pControl->GetCurrentText(strText);
   }
   else
   {
      strText = GetValueRowCol(nRow,nCol);
   }

   pgsTypes::SupportedBeamSpacing spacingType = m_pSpacing->GetBridgeDescription()->GetGirderSpacingType();

   Float64 spacing = _tstof(strText); // returns zero if error
   if ( spacing <= 0 )
   {
      if ( IsGirderSpacing(spacingType) )
      {
  	      SetWarningText (_T("Invalid girder spacing value"));
         return FALSE;
      }
      else
      {
         // zero is a valid joint spacing so let it go... if text string is not a number, it will be treated as zero for joints
         if ( !IsZero(spacing) )
         {
  	         SetWarningText (_T("Invalid joint spacing value"));
            return FALSE;
         }
      }
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   if ( IsGirderSpacing(spacingType) )
   {
      spacing = ::ConvertToSysUnits(spacing,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
      Float64 minGirderSpacing = m_MinGirderSpacing[nCol-1];
      Float64 maxGirderSpacing = m_MaxGirderSpacing[nCol-1];
      if ( IsLT(spacing,minGirderSpacing) || IsLT(maxGirderSpacing,spacing) )
      {
         SetWarningText(_T("Girder spacing is out of range"));
         return FALSE;
      }
   }
   else
   {
      spacing = ::ConvertToSysUnits(spacing,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
      Float64 minGirderSpacing = m_MinGirderSpacing[nCol-1];
      Float64 maxGirderSpacing = m_MaxGirderSpacing[nCol-1];
      if ( spacing < 0 || IsGT(maxGirderSpacing-minGirderSpacing,spacing) )
      {
         SetWarningText(_T("Joint spacing is out of range"));
         return FALSE;
      }
   }

	return CGXGridWnd::OnValidateCell(nRow, nCol);
}

BOOL CSegmentSpacingGrid::OnEndEditing(ROWCOL nRow,ROWCOL nCol)
{
   pgsTypes::SupportedBeamSpacing spacingType = m_pSpacing->GetBridgeDescription()->GetGirderSpacingType();

   if ( nRow == 1 && 1 <= nCol )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CString strValue;
      GetCurrentCellControl()->GetCurrentText(strValue);

      Float64 spacing = _tstof(strValue);

      if ( IsGirderSpacing(spacingType) )
      {
         // girder spacing
         spacing = ::ConvertToSysUnits(spacing,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
      }
      else
      {
         // joint spacing
         spacing = ::ConvertToSysUnits(spacing,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
      }

      m_pSpacing->SetGirderSpacing(GroupIndexType(nCol-1),spacing);
      UpdateGrid();
   }

   return CGXGridWnd::OnEndEditing(nRow,nCol);
}
