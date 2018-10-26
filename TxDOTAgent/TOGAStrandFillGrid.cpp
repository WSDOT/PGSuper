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

// DebondGrid.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "TOGAStrandFillGrid.h"
#include "TOGAGirderSelectStrandsDlg.h"
#include "PGSuperUnits.h"

#include <system\tokenizer.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define TYPE_COL         1
#define SELECT_CHECK_COL 2
#define DEBOND_CHECK_COL 3
#define DEBOND_VAL_COL   4


// local struct to store data in a grid cell

class CUserData : public CGXAbstractUserAttribute
{
public:
   pgsTypes::StrandType strandType;
   StrandIndexType      strandTypeGridIdx; // index in library collection for the strnad type defined by strandType (harped, straight, or temporary library grid index)
   bool                 isDebondable;
   StrandIndexType      oneOrTwo; // strands available for filling

   CUserData(pgsTypes::StrandType type, StrandIndexType index, bool debondable, StrandIndexType oneTwo):
      strandType(type), strandTypeGridIdx(index), isDebondable(debondable), oneOrTwo(oneTwo)
   {;}

   virtual CGXAbstractUserAttribute* Clone() const
   {
      return new CUserData(strandType,strandTypeGridIdx,isDebondable,oneOrTwo);
   }
};

/////////////////

GRID_IMPLEMENT_REGISTER(CTOGAStrandFillGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CTOGAStrandFillGrid

CTOGAStrandFillGrid::CTOGAStrandFillGrid():
m_pParent(NULL)
{
//   RegisterClass();
}

CTOGAStrandFillGrid::~CTOGAStrandFillGrid()
{
}

BEGIN_MESSAGE_MAP(CTOGAStrandFillGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CTOGAStrandFillGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTOGAStrandFillGrid message handlers

int CTOGAStrandFillGrid::GetColWidth(ROWCOL nCol)
{
   if ( !IsColHidden(nCol) && 
      (nCol == SELECT_CHECK_COL || nCol == DEBOND_CHECK_COL) )
   {
      return 15;
   }

   return CGXGridWnd::GetColWidth(nCol);
}

void CTOGAStrandFillGrid::InsertRow()
{

	ROWCOL nRow = GetRowCount()+1;

	InsertRows(nRow, 1);
   SetRowStyle(nRow);
}

void CTOGAStrandFillGrid::CustomInit(CTOGAGirderSelectStrandsDlg* pParent, const GirderLibraryEntry* pGdrEntry)
{
   // We get our data from dad
   m_pParent = pParent;
   m_pGdrEntry = pGdrEntry;

   // Initialize the grid. For CWnd based grids this call is essential. 
	Initialize( );

   // we want to merge cells
   SetMergeCellsMode(gxnMergeDelayEval);
   SetFrozenCols(2,2); // column 2 is frozen and is a row header column (keeps Select column from scrolling)

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   // Make string for max debond length
   m_strMaxDebondLength = FormatDimension(m_pParent->m_MaxDebondLength, pDisplayUnits->GetXSectionDimUnit(), false);

	GetParam( )->EnableUndo(FALSE);

   const int num_rows = 0;
   const int num_cols = DEBOND_VAL_COL;

	SetRowCount(num_rows);
	SetColCount(num_cols);

   // Turn off selection
	GetParam()->EnableSelection((WORD) (GX_SELNONE));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   // set text along top row
	SetStyleRange(CGXRange(0,0), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
			.SetValue(_T("Strand\nPosition"))
		);

	SetStyleRange(CGXRange(0,TYPE_COL), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
			.SetValue(_T("Strand\nPosition"))
		);


   CGXFont font;
   font.SetOrientation(900); // vertical text 

	SetStyleRange(CGXRange(0,SELECT_CHECK_COL), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetFont(font)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Select"))
		);

	SetStyleRange(CGXRange(0,DEBOND_CHECK_COL), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetFont(font)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Debond"))
		);

   if (m_pParent->m_CanDebondStrands)
   {
      // Name of this header will be set in SymmetricDebond()
	   SetStyleRange(CGXRange(0,DEBOND_VAL_COL), CGXStyle()
            .SetWrapText(TRUE)
			   .SetEnabled(FALSE)          // disables usage as current cell
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
		   );

         CString cv = CString(_T("Debond\nLength\n(")) + 
              CString(pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str()) + 
              CString(_T(")\nBoth Ends"));

         SetStyleRange(CGXRange(0,DEBOND_VAL_COL), CGXStyle().SetValue(cv));
   }

   HideCols(DEBOND_CHECK_COL,DEBOND_VAL_COL,(m_pParent->m_CanDebondStrands ? FALSE : TRUE));

   // make text fit correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();
	SetFocus();

	GetParam( )->EnableUndo(TRUE);

   FillGrid();
}

void CTOGAStrandFillGrid::SetRowStyle(ROWCOL nRow)
{
   SetStyleRange(CGXRange(nRow,0), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         );

   SetStyleRange(CGXRange(nRow,TYPE_COL), CGXStyle()
			//.SetControl(GX_IDS_CTRL_HEADER)
   //      .SetInterior(GXSYSCOLOR( COLOR_BTNFACE ))
         .SetReadOnly(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         );

   SetStyleRange(CGXRange(nRow,SELECT_CHECK_COL), CGXStyle()
			.SetControl(GX_IDS_CTRL_CHECKBOX3D)  //
			.SetValue(_T("0"))
         .SetHorizontalAlignment(DT_CENTER)
         );

   SetStyleRange(CGXRange(nRow,DEBOND_CHECK_COL), CGXStyle()
			.SetControl(GX_IDS_CTRL_CHECKBOX3D)  //
			.SetValue(_T("0"))
         .SetHorizontalAlignment(DT_CENTER)
         );

   SetStyleRange(CGXRange(nRow,DEBOND_VAL_COL), CGXStyle()
		   .SetUserAttribute(GX_IDS_UA_VALID_MIN, _T("0.0e01"))
		   .SetUserAttribute(GX_IDS_UA_VALID_MAX, m_strMaxDebondLength)
		   .SetUserAttribute(GX_IDS_UA_VALID_MSG, _T("Debond length must be zero or greater and value cannot exceed half-girder length"))
         .SetHorizontalAlignment(DT_RIGHT)
         .SetReadOnly(TRUE)
         .SetEnabled(FALSE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
	   );
}

void CTOGAStrandFillGrid::FillGrid()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   StrandIndexType nPerm = m_pGdrEntry->GetPermanentStrandGridSize();
   StrandIndexType nTemp = m_pGdrEntry->GetNumTemporaryStrandCoordinates();

   // Strands are ordered from bottom up. First fill empty grid, then modify values
   ROWCOL nItems = ROWCOL(nPerm+nTemp);
   ROWCOL row = 1;
   while(row <= nItems)
   {
      InsertRow();
      row++;
   }

   // Fill bottom up starting with permanent strands
   StrandIndexType currPositionNo = 0;
   row = (ROWCOL)nItems; // fill bottom up
   StrandIndexType gridIdx = 0;
   while (gridIdx < nPerm)
   {
      GirderLibraryEntry::psStrandType strandType;
      StrandIndexType localIdx; // library grid index for the straight or harped strand grid
      m_pGdrEntry->GetGridPositionFromPermStrandGrid(gridIdx, &strandType, &localIdx);

      StrandIndexType oneOrTwo;
      bool canDebond(false);
      if (strandType==GirderLibraryEntry::stStraight)
      {
         Float64 xs, ys, xe, ye;
         m_pGdrEntry->GetStraightStrandCoordinates(localIdx, &xs, &ys, &xe, &ye, &canDebond);
         oneOrTwo = (xs==0.0 && xe==0.0) ? 1 : 2;
      }
/*
      else if (strandType==GirderLibraryEntry::stHarped)
      {
         Float64 xs, ys, xh, yh, xe, ye;
         m_pGdrEntry->GetHarpedStrandCoordinates(localIdx, &xs, &ys, &xh, &yh, &xe, &ye);
         oneOrTwo = (xs==0.0 && xh==0.0 && xe==0.0) ? 1 : 2;
      }
*/
      else
      {
         ATLASSERT(false);
      }

      CString strStrand;
      if (oneOrTwo==1)
         strStrand.Format(_T("%d"),currPositionNo+1);
      else if (oneOrTwo==2)
         strStrand.Format(_T("%d-%d"),currPositionNo+1,currPositionNo+2);
      else
         ATLASSERT(false); // new fill type?

      SetStyleRange(CGXRange(row,0), CGXStyle().SetValue(strStrand).SetHorizontalAlignment(DT_CENTER));


      CString strType;
      if (strandType==GirderLibraryEntry::stStraight)
      {
         if(canDebond)
            strType = _T("S-DB");
         else
            strType = _T("S");
      }
      else if (strandType==GirderLibraryEntry::stAdjustable)
      {
         ATLASSERT(false); // should never happen in toga

         if(m_pGdrEntry->GetAdjustableStrandType() == pgsTypes::asStraight)
         {
            strType = _T("S-W");
         }
         else
         {
            strType = _T("H");
         }
      }


      SetStyleRange(CGXRange(row,TYPE_COL), CGXStyle().SetValue(strType));

      bool isFilled = IsPermStrandFilled(strandType, localIdx);

      // store strand type and index for later use
      CUserData userData(strandType==GirderLibraryEntry::stStraight ? pgsTypes::Straight : pgsTypes::Harped, 
         localIdx, canDebond, oneOrTwo); // the grid will delete this

      SetStyleRange(CGXRange(row,SELECT_CHECK_COL), CGXStyle()
         .SetValue(isFilled ? _T("1") : _T("0"))
         .SetUserAttribute(0,userData)
         );

      if ( !isFilled )
      {
         // strand not selected... disable the row
         SetStyleRange(CGXRange(row,DEBOND_CHECK_COL,row,DEBOND_VAL_COL), CGXStyle()
			      .SetValue(_T(""))
               .SetReadOnly(TRUE)
               .SetEnabled(FALSE)
               .SetInterior(::GetSysColor(COLOR_BTNFACE))
               .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
               );

         if (!canDebond)
         {
            // strand can't be debonded (it is not debondable)
            // use a disabled static control instead of the check box for debonding
            SetStyleRange(CGXRange(row,DEBOND_CHECK_COL), CGXStyle()
			         .SetControl(GX_IDS_CTRL_STATIC)  //
			         .SetValue(_T(""))
                  .SetReadOnly(TRUE)
                  .SetEnabled(FALSE)
                  .SetInterior(::GetSysColor(COLOR_BTNFACE))
                  .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
                  );
         }
      }
      else if (canDebond)
      {
         // strand can be debonded
         Float64 leftDebond, rightDebond;
         bool bIsDebonded = GetDebondInfo(localIdx, &leftDebond, &rightDebond);
         ATLASSERT(leftDebond==rightDebond);

         SetStyleRange(CGXRange(row,DEBOND_CHECK_COL), CGXStyle()
			      .SetControl(GX_IDS_CTRL_CHECKBOX3D)  //
               .SetValue(bIsDebonded ? _T("1") : _T("0"))
               .SetReadOnly(FALSE)
               .SetEnabled(TRUE)
               );

         if ( bIsDebonded )
         {
            // strand is debonded
            SetStyleRange(CGXRange(row,DEBOND_VAL_COL), CGXStyle()
               .SetValue(FormatDimension(leftDebond, pDisplayUnits->GetXSectionDimUnit(), false))
               .SetReadOnly(FALSE)
               .SetEnabled(TRUE)
               .SetInterior(::GetSysColor(COLOR_WINDOW))
               .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
            );
         }
         else
         {
            // strand is not debonded so blank out the debond lenght data
            SetStyleRange(CGXRange(row,DEBOND_VAL_COL),CGXStyle()
               .SetValue(_T(""))
               );
         }
      }
      else
      {
         // strand can't be debonded (it is not debondable)
         // use a disabled static control instead of the check box for debonding
         SetStyleRange(CGXRange(row,DEBOND_CHECK_COL), CGXStyle()
			      .SetControl(GX_IDS_CTRL_STATIC)  //
			      .SetValue(_T(""))
               .SetReadOnly(TRUE)
               .SetEnabled(FALSE)
               .SetInterior(::GetSysColor(COLOR_BTNFACE))
               .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
               );
      }

      gridIdx++;

      currPositionNo += oneOrTwo;
      row--;
   }

   // Next fill temporary strands
   currPositionNo=0;
   gridIdx = 0;
   while (gridIdx < nTemp)
   {
      ATLASSERT(false); // should not happen in toga

      Float64 xs, ys, xe, ye;
      m_pGdrEntry->GetTemporaryStrandCoordinates(gridIdx, &xs, &ys, &xe, &ye);
      StrandIndexType oneOrTwo = (xs==0.0 && xe==0.0) ? 1 : 2;

      CString strStrand;
      if (oneOrTwo==1)
         strStrand.Format(_T("%d"),currPositionNo+1);
      else if (oneOrTwo==2)
         strStrand.Format(_T("%d-%d"),currPositionNo+1,currPositionNo+2);
      else
         ATLASSERT(false); // new fill type?

      SetStyleRange(CGXRange(row,0), CGXStyle().SetValue(strStrand));
      SetStyleRange(CGXRange(row,TYPE_COL), CGXStyle().SetValue(_T("T")));

      bool isFilled = false; // = IsTempStrandFilled(gridIdx);

      // store strand type and index for later use
      CUserData userData(pgsTypes::Temporary, gridIdx, false, oneOrTwo); // the grid will delete this

      SetStyleRange(CGXRange(row,SELECT_CHECK_COL), CGXStyle()
         .SetValue(isFilled ? _T("1") : _T("0"))
         .SetUserAttribute(0,userData)
         );


      // temporary strands cannot be debonded or extended...
      // put a static control in the grid for each of these columns
      SetStyleRange(CGXRange(row,DEBOND_CHECK_COL), CGXStyle()
			   .SetControl(GX_IDS_CTRL_STATIC)  //
			   .SetValue(_T(""))
            .SetReadOnly(TRUE)
            .SetEnabled(FALSE)
            .SetInterior(::GetSysColor(COLOR_BTNFACE))
            .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
            );

      currPositionNo += oneOrTwo;
      row--;

      gridIdx++;
   }

   ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));

   ScrollCellInView(nItems>0?(ROWCOL)nItems-1:0, GetLeftCol());

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

bool CTOGAStrandFillGrid::UpdateData(bool doCheckData)
{
   // Strand fill information is kept up to date in OnClickedButtonRowCol
   // However, debonding and extended strand information must be taken care of here
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   // clear out old debond data
   m_pParent->m_StraightDebond.clear();

   // walk the entire grid and find debonded strands
   ROWCOL nRows = this->GetRowCount();
   for (ROWCOL nRow=1; nRow<=nRows; nRow++)
   {
      CGXStyle style;
      GetStyleRowCol(nRow, SELECT_CHECK_COL, style);
      if ( style.GetValue() == _T("0") )
         continue; // strand are not selected... continue to the next row

      const CUserData& userData = dynamic_cast<const CUserData&>(style.GetUserAttribute(0));

      CGXStyle debondCheckColStyle;
      GetStyleRowCol(nRow, DEBOND_CHECK_COL, debondCheckColStyle);
      if ( debondCheckColStyle.GetValue() == _T("1") )
      {
         // Strand is debonded
         Float64 leftDebond(0.0), rightDebond(0.0);

         CGXStyle style;
         GetStyleRowCol(nRow, DEBOND_VAL_COL, style);
         CString strval = style.GetValue();
         if( !strval.IsEmpty())
         {
            bool st = sysTokenizer::ParseDouble(strval, &leftDebond);
            if(!st && doCheckData)
            {
               AfxMessageBox( _T("Debond length is not a number - must be a postive number"), MB_ICONEXCLAMATION);
               this->SetCurrentCell(nRow,DEBOND_VAL_COL,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
               return false;
            }

            leftDebond  = ::ConvertToSysUnits(leftDebond,  pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
         }

         rightDebond = leftDebond;

         if (doCheckData)
         {
            if (leftDebond<=0.0 || rightDebond<=0.0)
            {
               AfxMessageBox( _T("Debond lengths must be greater than zero."), MB_ICONEXCLAMATION);
               return false;
            }
            else if (leftDebond>m_pParent->m_MaxDebondLength || rightDebond>m_pParent->m_MaxDebondLength)
            {
               AfxMessageBox( _T("Debond lengths must less than half of girder length."), MB_ICONEXCLAMATION);
               return false;
            }
         }


         if (userData.strandTypeGridIdx != INVALID_INDEX)
         {
            CDebondData dbinfo;
            dbinfo.strandTypeGridIdx = userData.strandTypeGridIdx;
            dbinfo.Length[pgsTypes::metStart] = leftDebond;
            dbinfo.Length[pgsTypes::metEnd]   = rightDebond;

            m_pParent->m_StraightDebond.push_back(dbinfo);
         }
      }
   }

   return true;
}

void CTOGAStrandFillGrid::OnClickedButtonRowCol(ROWCOL nRow, ROWCOL nCol)
{
   if ( nRow == 0 )
      return;

   if(nCol == SELECT_CHECK_COL)
   {
      // Click on the "Select" check box

      CGXStyle style;
      GetStyleRowCol(nRow, nCol, style);
      const CUserData& userData = dynamic_cast<const CUserData&>(style.GetUserAttribute(0));

      CString strval = style.GetValue();
      bool bIsChecked = (strval == _T("1") ? true : false);
      GetParam()->SetLockReadOnly(FALSE);

      if(bIsChecked)
      {
         // just selected the strand
         AddStrandFill(&userData);

         // If the strand is debondable, enable the debond check box
         if ( userData.isDebondable )
         {
            SetStyleRange(CGXRange(nRow,DEBOND_CHECK_COL),CGXStyle()
               .SetValue(_T("0")) 
               .SetReadOnly(FALSE)
               .SetEnabled(TRUE)
               .SetInterior(::GetSysColor(COLOR_WINDOW))
               .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
               );
         }
      }
      else
      {
         // just unchecked the box
         RemoveStrandFill(&userData);

         // Clear and disable the rest of the row
         SetStyleRange(CGXRange(nRow,DEBOND_CHECK_COL,nRow,DEBOND_VAL_COL), CGXStyle()
			      .SetValue(_T(""))
               .SetReadOnly(TRUE)
               .SetEnabled(FALSE)
               .SetInterior(::GetSysColor(COLOR_BTNFACE))
               .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
               );
      }

      GetParam()->SetLockReadOnly(TRUE);

      UpdateParent();
   }
   else if ( nCol == DEBOND_CHECK_COL )
   {
      GetParam()->SetLockReadOnly(FALSE);

      CGXStyle style;
      GetStyleRowCol(nRow, nCol, style);
      CString strval = style.GetValue();
      bool bIsChecked = (strval == _T("1") ? true : false);
      if ( bIsChecked )
      {
         // Debond box was just checked (debonding enabled)
         SetStyleRange(CGXRange(nRow,DEBOND_VAL_COL), CGXStyle()
			      .SetValue(_T(""))
               .SetReadOnly(FALSE)
               .SetEnabled(TRUE)
               .SetInterior(::GetSysColor(COLOR_WINDOW))
               .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
               );
      }
      else
      {
         // Debond box was just unchecked (debonding removed)... clear the debond information in this row
         SetStyleRange(CGXRange(nRow,DEBOND_VAL_COL), CGXStyle()
			      .SetValue(_T(""))
               .SetReadOnly(TRUE)
               .SetEnabled(FALSE)
               .SetInterior(::GetSysColor(COLOR_BTNFACE))
               .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
               );
      }

      GetParam()->SetLockReadOnly(TRUE);

      UpdateParent();
   }
   else
   {
      ATLASSERT(false);
   }
}

void CTOGAStrandFillGrid::UpdateParent()
{
   CTOGAGirderSelectStrandsDlg* pdlg = (CTOGAGirderSelectStrandsDlg*)GetParent();
   pdlg->OnNumStrandsChanged();
}

bool CTOGAStrandFillGrid::IsPermStrandFilled(GirderLibraryEntry::psStrandType strandType, StrandIndexType idxStrandGrid)
{
   if (strandType==GirderLibraryEntry::stStraight)
   {
      return m_pParent->m_DirectFilledStraightStrands.IsStrandFilled(idxStrandGrid);
   }
   else
   {
      ATLASSERT(false);
      return false;
   }
}

void CTOGAStrandFillGrid::RemoveStrandFill(const CUserData* pUserData)
{
   if (pUserData->strandType == pgsTypes::Straight)
   {
      m_pParent->m_DirectFilledStraightStrands.RemoveFill(pUserData->strandTypeGridIdx);
   }
   else
      ATLASSERT(false);
}

void CTOGAStrandFillGrid::AddStrandFill(const CUserData* pUserData)
{
   CDirectStrandFillInfo fillinf(pUserData->strandTypeGridIdx, pUserData->oneOrTwo);

   if (pUserData->strandType == pgsTypes::Straight)
   {
      m_pParent->m_DirectFilledStraightStrands.AddFill(fillinf);
   }
   else
      ATLASSERT(false);
}

bool CTOGAStrandFillGrid::GetDebondInfo(StrandIndexType straightStrandGridIdx, Float64* pleftDebond, Float64* prightDebond)
{
   std::vector<CDebondData>::iterator it    = m_pParent->m_StraightDebond.begin();
   std::vector<CDebondData>::iterator itend = m_pParent->m_StraightDebond.end();
   while(it!=itend)
   {
      if (it->strandTypeGridIdx == straightStrandGridIdx)
      {
         // found our strand
         *pleftDebond  = it->Length[pgsTypes::metStart];
         *prightDebond = it->Length[pgsTypes::metEnd] != -1.0 ? it->Length[pgsTypes::metEnd] : it->Length[pgsTypes::metStart];
         return true; // strand is debonded
      }

      it++;
   }

   *pleftDebond  = 0.0;
   *prightDebond = 0.0;
   return false;
}

void CTOGAStrandFillGrid::ToggleFill(ROWCOL rowNo)
{
   ROWCOL nRows = this->GetRowCount();
   if(rowNo <= nRows)
   {
      GetParam()->EnableUndo(FALSE);
      GetParam()->SetLockReadOnly(FALSE);

     CString strval = GetValueRowCol(rowNo, SELECT_CHECK_COL);
     bool val = strval!=_T("1");

     CGXControl* pControl = GetControl(rowNo, SELECT_CHECK_COL);
     CGXCheckBox* pCheck = dynamic_cast<CGXCheckBox*>(pControl);
     if (pCheck != NULL) // only set check boxes
     {
        SetValueRange(CGXRange(rowNo, SELECT_CHECK_COL), val ? _T("1") : _T("0"));
     }

     OnClickedButtonRowCol(rowNo, SELECT_CHECK_COL);

      GetParam()->SetLockReadOnly(TRUE);
	   GetParam()->EnableUndo(TRUE);
   }
   else
      ATLASSERT(false);

   ScrollCellInView(rowNo, SELECT_CHECK_COL);
}
