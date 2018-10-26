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

// GirderSpacingGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "GirderSpacingGrid.h"

#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>

#include <PgsExt\BridgeDescription.h>
#include <PgsExt\PierData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CGirderSpacingGrid, CS_DBLCLKS, 0, 0, 0);

void DDV_SpacingGrid(CDataExchange* pDX,int nIDC,CGirderSpacingGrid* pGrid)
{
   if (!pDX->m_bSaveAndValidate )
      return;

   pDX->PrepareCtrl(nIDC);
   if ( !pGrid->ValidateGirderSpacing() )
   {
      pDX->Fail();
   }
}

/////////////////////////////////////////////////////////////////////////////
// CGirderSpacingGrid

CGirderSpacingGrid::CGirderSpacingGrid()
{
//   RegisterClass();
   m_pLinkedGrid = NULL;
   m_bEnabled = TRUE;
   m_bLinked = FALSE;
}

CGirderSpacingGrid::~CGirderSpacingGrid()
{
}

BEGIN_MESSAGE_MAP(CGirderSpacingGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CGirderNameGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_COMMAND(IDC_EXPAND, OnExpand)
	ON_COMMAND(IDC_JOIN, OnJoin)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CGirderSpacingGridData CGirderSpacingGrid::GetGirderSpacingData()
{
   return m_GridData;
}

void CGirderSpacingGrid::SetGirderSpacingData(const CGirderSpacingGridData& gridData)
{
   m_GridData = gridData;
}

void CGirderSpacingGrid::SetMeasurementType(pgsTypes::MeasurementType mt)
{
   m_GridData.m_GirderSpacing.SetMeasurementType(mt);
}

void CGirderSpacingGrid::SetMeasurementLocation(pgsTypes::MeasurementLocation ml)
{
   m_GridData.m_GirderSpacing.SetMeasurementLocation(ml);
}

void CGirderSpacingGrid::SetPierSkewAngle(Float64 skewAngle)
{
   m_PierSkewAngle = skewAngle;
   FillGrid();
}

bool CGirderSpacingGrid::InputSpacing() const
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
// CGirderSpacingGrid message handlers
void CGirderSpacingGrid::Init(pgsTypes::SupportedBeamSpacing girderSpacingType,bool bSharedGirderCount,const CGirderSpacing* pGirderSpacing,const CGirderTypes* pGirderTypes,pgsTypes::PierFaceType pierFace,PierIndexType pierIdx,Float64 skewAngle,bool bAbutment)
{
   m_GirderSpacingType = girderSpacingType;

   m_bSharedGirderCount   = bSharedGirderCount;

   m_PierSkewAngle = skewAngle;

   m_GridData.m_GirderSpacing = *pGirderSpacing;
   m_GridData.m_GirderTypes   = *pGirderTypes;
   m_GridData.m_PierFace      = pierFace;

   m_PierIdx = pierIdx;
   m_bAbutment = bAbutment;
}

void CGirderSpacingGrid::CustomInit(pgsTypes::SupportedBeamSpacing girderSpacingType,bool bSharedGirderCount,const CGirderSpacing* pGirderSpacing,const CGirderTypes* pGirderTypes,pgsTypes::PierFaceType pierFace,PierIndexType pierIdx,Float64 skewAngle,bool bAbutment)
{
   // Initialize the grid. For CWnd based grids this call is
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
   Init(girderSpacingType,bSharedGirderCount,pGirderSpacing,pGirderTypes,pierFace,pierIdx,skewAngle,bAbutment);
   CustomInit();
}

void CGirderSpacingGrid::CustomInit()
{
	Initialize( );

   GetParam()->EnableUndo(FALSE);

		// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELTABLE & ~GX_SELROW));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);
   GetParam()->EnableMoveCols(FALSE);

   FillGrid();

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();

   EnableGridToolTips();
   SetStyleRange(CGXRange(0,0,GetRowCount(),GetColCount()),
      CGXStyle()
      .SetWrapText(TRUE)
      .SetAutoSize(TRUE)
      .SetUserAttribute(GX_IDS_UA_TOOLTIPTEXT,"To regroup girder spacing, select column headings, right click over the grid, and select Expand or Join from the menu")
      );
	
   GetParam()->EnableUndo(TRUE);

	SetFocus();
}

void CGirderSpacingGrid::FillGrid(const CGirderSpacing* pGirderSpacing)
{
   m_GridData.m_GirderSpacing = *pGirderSpacing;

   FillGrid();
}

void CGirderSpacingGrid::FillGrid()
{
   if ( GetSafeHwnd() == NULL )
      return; // grid isn't ready for filling

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   const unitmgtLengthData& spacingUnit = IsGirderSpacing(m_GirderSpacingType) // if
                                        ? pDisplayUnits->GetXSectionDimUnit()     // then
                                        : pDisplayUnits->GetComponentDimUnit();   // else

   GroupIndexType nSpacingGroups = m_GridData.m_GirderSpacing.GetSpacingGroupCount();
   GirderIndexType nGirders      = m_GridData.m_GirderSpacing.GetSpacingCount() + 1;

   // get the pier direction so the allowable girder spacing can be skew corrected
   Float64 skewCorrection;
   if ( m_GridData.m_GirderSpacing.GetMeasurementType() == pgsTypes::NormalToItem )
   {
      skewCorrection = 1;
   }
   else
   {
      skewCorrection = fabs(1/cos(m_PierSkewAngle));
   }


   const int num_rows = 2;
   const int num_cols = (int)nSpacingGroups;

   SetRowCount(num_rows);
	SetColCount(num_cols);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   const CSpanData* pSpan                = m_GridData.m_GirderTypes.GetSpan();
   const CBridgeDescription* pBridgeDesc = pSpan->GetBridgeDescription();
   pgsTypes::SupportedDeckType deckType  = pBridgeDesc->GetDeckDescription()->DeckType;

   m_MinGirderSpacing.clear();
   m_MaxGirderSpacing.clear();
   for ( GroupIndexType grpIdx = 0; grpIdx < nSpacingGroups; grpIdx++ )
   {
      Float64 spacing;
      GirderIndexType firstGdrIdx, lastGdrIdx;
      m_GridData.m_GirderSpacing.GetSpacingGroup(grpIdx,&firstGdrIdx,&lastGdrIdx,&spacing);

      // if this is the last group and the girder spacing is uniform but the girder count is not shared
      // then the last girder index needs to be forced to nGirders-1
      if ( grpIdx == nSpacingGroups-1 && 
          ( IsBridgeSpacing(m_GirderSpacingType) || !m_bSharedGirderCount) )
      {
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

      UserData* pUserData = new UserData(firstGdrIdx,lastGdrIdx); // the grid will delete this

      SetStyleRange(CGXRange(0,ROWCOL(grpIdx+1)), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetEnabled(FALSE)
         .SetValue(strHeading)
         .SetItemDataPtr((void*)pUserData)
         );

      // get valid girder spacing for this group
      Float64 minGirderSpacing = -MAX_GIRDER_SPACING;
      Float64 maxGirderSpacing =  MAX_GIRDER_SPACING;
      for ( GirderIndexType gdrIdx = firstGdrIdx; gdrIdx <= lastGdrIdx; gdrIdx++ )
      {
         const GirderLibraryEntry* pGdrEntry = m_GridData.m_GirderTypes.GetGirderLibraryEntry(gdrIdx);
         const IBeamFactory::Dimensions& dimensions = pGdrEntry->GetDimensions();
         CComPtr<IBeamFactory> factory;
         pGdrEntry->GetBeamFactory(&factory);

         // save spacing range to local class data
         Float64 minGS, maxGS;
         factory->GetAllowableSpacingRange(dimensions,deckType,m_GirderSpacingType,&minGS, &maxGS);
         minGS *= skewCorrection;
         maxGS *= skewCorrection;

         if ( IsGirderSpacing(m_GirderSpacingType) )
         {
            // girder spacing
            minGirderSpacing = _cpp_max(minGirderSpacing,minGS);
            maxGirderSpacing = _cpp_min(maxGirderSpacing,maxGS);
         }
         else
         {
            // joint spacing
            minGirderSpacing = 0;
            maxGirderSpacing = _cpp_min(maxGirderSpacing-minGirderSpacing,maxGS-minGS);
         }
      }

      m_MinGirderSpacing.push_back(minGirderSpacing);
      m_MaxGirderSpacing.push_back(maxGirderSpacing);

      ATLASSERT( minGirderSpacing <= maxGirderSpacing );

      spacing = ForceIntoRange(minGirderSpacing,spacing,maxGirderSpacing);
      m_GridData.m_GirderSpacing.SetGirderSpacing(grpIdx,spacing);


      CString strSpacing;
      strSpacing.Format(_T("%s"),FormatDimension(spacing,spacingUnit,false));
      SetStyleRange(CGXRange(1,ROWCOL(grpIdx+1)), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         .SetEnabled(TRUE)
         .SetReadOnly(FALSE)
         .SetInterior(::GetSysColor(COLOR_WINDOW))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetValue( strSpacing )
         );

      if (maxGirderSpacing < MAX_GIRDER_SPACING)
      {
         if ( IsGirderSpacing(m_GirderSpacingType) )
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

      SetStyleRange(CGXRange(2,ROWCOL(grpIdx+1)), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         .SetEnabled(FALSE)
         .SetReadOnly(TRUE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetValue( strSpacing )
         );
   }

   CString strPier;
   strPier.Format(_T("%s %d"), m_bAbutment ? _T("Abut") : _T("Pier"), m_PierIdx+1);

   SetStyleRange(CGXRange(0,0), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetEnabled(FALSE)
      .SetValue(strPier)
      );

   SetStyleRange(CGXRange(1,0), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetEnabled(FALSE)
      .SetValue(m_GridData.m_PierFace == pgsTypes::Ahead ? _T("Ahead") : _T("Back"))
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

   if ( m_pLinkedGrid )
   {
      CGirderSpacingGridData spacingData = GetGirderSpacingData();
      spacingData.m_PierFace = (spacingData.m_PierFace == pgsTypes::Ahead ? pgsTypes::Back : pgsTypes::Ahead);
      m_pLinkedGrid->SetGirderSpacingData(spacingData);
      m_pLinkedGrid->FillGrid();
   }
}

void CGirderSpacingGrid::SetGirderSpacingType(pgsTypes::SupportedBeamSpacing girderSpacingType)
{
   if ( !m_bLinked )
   {
      m_GirderSpacingType = girderSpacingType;

      // if we are moving to general spacing, then the girder spacing isn't associated with a span 
      // (make the span null so the spacing object can't go to the parent bridge and get spacing)
      if ( IsSpanSpacing(girderSpacingType) )
         m_GridData.m_GirderSpacing.SetSpan(NULL);
   }

   FillGrid();
}

void CGirderSpacingGrid::SharedGirderCount(bool bShare)
{
   if ( !m_bLinked )
   {
      m_bSharedGirderCount = bShare;
      
      if ( !m_bSharedGirderCount )
         m_GridData.m_GirderSpacing.SetSpan(NULL);
   }

   FillGrid();
}

BOOL CGirderSpacingGrid::OnRButtonHitRowCol(ROWCOL nHitRow,ROWCOL nHitCol,ROWCOL nDragRow,ROWCOL nDragCol,CPoint point,UINT nFlags,WORD nHitState)
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

void CGirderSpacingGrid::OnExpand()
{
   CRowColArray selCols;
   ROWCOL nSelCols = GetSelectedCols(selCols);

   if ( nSelCols == 0 )
      return;

   if ( nSelCols == GetColCount() )
   {
      m_GridData.m_GirderSpacing.ExpandAll();
   }
   else
   {
      for ( int i = nSelCols-1; 0 <= i; i-- )
      {
         ROWCOL col = selCols[i];
         GroupIndexType grpIdx = GroupIndexType(col-1);

         m_GridData.m_GirderSpacing.Expand(grpIdx);
      }
   }

   FillGrid();
}

void CGirderSpacingGrid::OnJoin()
{
   CRowColArray selLeftCols,selRightCols;
   GetSelectedCols(selLeftCols,selRightCols,FALSE,FALSE);
   ROWCOL nSelRanges = (ROWCOL)selLeftCols.GetSize();
   if ( nSelRanges == 0 )
      return;

   for (ROWCOL i = 0; i < nSelRanges; i++ )
   {
      ROWCOL firstCol = selLeftCols[i];
      ROWCOL lastCol  = selRightCols[i];

      CGXStyle firstColStyle, lastColStyle;
      GetStyleRowCol(0,firstCol,firstColStyle);
      GetStyleRowCol(0,lastCol, lastColStyle);

      UserData* pFirstColUserData = (UserData*)firstColStyle.GetItemDataPtr();
      UserData* pLastColUserData =  (UserData*)lastColStyle.GetItemDataPtr();

      GirderIndexType firstGdrIdx = pFirstColUserData->first;
      GirderIndexType lastGdrIdx  = pLastColUserData->second;

      m_GridData.m_GirderSpacing.Join(firstGdrIdx,lastGdrIdx,firstGdrIdx);
   }

   FillGrid();
}

void CGirderSpacingGrid::AddGirders(GirderIndexType nGirders)
{
   if ( !m_bLinked )
   {
      m_GridData.m_GirderSpacing.AddGirders(nGirders);
      m_GridData.m_GirderSpacing.SetSpan(NULL);
      
      m_GridData.m_GirderTypes.AddGirders(nGirders);
   }

   FillGrid();
}

void CGirderSpacingGrid::RemoveGirders(GirderIndexType nGirders)
{
   if ( !m_bLinked )
   {
      m_GridData.m_GirderSpacing.RemoveGirders(nGirders);
      m_GridData.m_GirderSpacing.SetSpan(NULL);

      m_GridData.m_GirderTypes.RemoveGirders(nGirders);
   }

   FillGrid();
}

void CGirderSpacingGrid::SetGirderCount(GirderIndexType nGirders)
{
   if ( !m_bLinked )
   {
      m_GridData.m_GirderSpacing.SetGirderCount(nGirders);
      m_GridData.m_GirderSpacing.SetSpan(NULL);

      m_GridData.m_GirderTypes.SetGirderCount(nGirders);
   }

   FillGrid();
}

void CGirderSpacingGrid::Enable(BOOL bEnable)
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

void CGirderSpacingGrid::SetLinkedGrid(CGirderSpacingGrid* pLinkedGrid)
{
   // if we are currently linked to a grid... unlink
   if ( m_pLinkedGrid )
   {
      m_pLinkedGrid->IsLinked(false);
      m_pLinkedGrid = NULL;
   }

   m_pLinkedGrid = pLinkedGrid;

   if ( m_pLinkedGrid )
   {
      // we are now linked....
      CGirderSpacingGridData spacingData = GetGirderSpacingData();
      spacingData.m_PierFace = (spacingData.m_PierFace == pgsTypes::Ahead ? pgsTypes::Back : pgsTypes::Ahead);
      m_pLinkedGrid->SetGirderSpacingData(spacingData);
      m_pLinkedGrid->FillGrid();

      m_pLinkedGrid->IsLinked(true);
   }
}

bool CGirderSpacingGrid::IsLinked(bool bLinked)
{
   bool b = m_bLinked;
   m_bLinked = bLinked;
   return b;
}

BOOL CGirderSpacingGrid::ValidateGirderSpacing()
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

BOOL CGirderSpacingGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
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


   Float64 spacing = _tstof(strText); // returns zero if error
   if ( spacing <= 0 )
   {
      if ( IsGirderSpacing(m_GirderSpacingType) )
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
   if ( IsGirderSpacing(m_GirderSpacingType) )
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

BOOL CGirderSpacingGrid::OnEndEditing(ROWCOL nRow,ROWCOL nCol)
{
   if ( nRow == 1 && 1 <= nCol )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CString strValue;
      GetCurrentCellControl()->GetCurrentText(strValue);

      Float64 spacing = _tstof(strValue);

      if ( IsGirderSpacing(m_GirderSpacingType) )
      {
         // girder spacing
         spacing = ::ConvertToSysUnits(spacing,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
      }
      else
      {
         // joint spacing
         spacing = ::ConvertToSysUnits(spacing,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
      }

      m_GridData.m_GirderSpacing.SetGirderSpacing(nCol-1,spacing);
      FillGrid();
   }

   return CGXGridWnd::OnEndEditing(nRow,nCol);
}
