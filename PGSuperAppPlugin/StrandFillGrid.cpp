///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include "PGSuperDoc.h"
#include "resource.h"
#include "StrandFillGrid.h"
#include "GirderSelectStrandsPage.h"
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
#define FIRST_DEBOND_COL 4
#define LAST_DEBOND_COL  5
#define FIRST_EXTEND_COL 6
#define LAST_EXTEND_COL  7


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

GRID_IMPLEMENT_REGISTER(CStrandFillGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CStrandFillGrid

CStrandFillGrid::CStrandFillGrid():
m_pParent(nullptr)
{
//   RegisterClass();
}

CStrandFillGrid::~CStrandFillGrid()
{
}

BEGIN_MESSAGE_MAP(CStrandFillGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CStrandFillGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CStrandFillGrid message handlers

int CStrandFillGrid::GetColWidth(ROWCOL nCol)
{
   if ( !IsColHidden(nCol) && 
      (nCol == SELECT_CHECK_COL || nCol == DEBOND_CHECK_COL || (FIRST_EXTEND_COL <= nCol && nCol <= LAST_EXTEND_COL)) )
   {
      return 15;
   }

   return CGXGridWnd::GetColWidth(nCol);
}

void CStrandFillGrid::InsertRow()
{

	ROWCOL nRow = GetRowCount()+1;

	InsertRows(nRow, 1);
   SetRowStyle(nRow);
}

void CStrandFillGrid::CustomInit(CGirderSelectStrandsPage* pParent, const GirderLibraryEntry* pGdrEntry)
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
   const int num_cols = 7;

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

   if (m_pParent->m_bCanDebondStrands)
   {
      // Name of this header will be set in SymmetricDebond()
	   SetStyleRange(CGXRange(0,FIRST_DEBOND_COL), CGXStyle()
            .SetWrapText(TRUE)
			   .SetEnabled(FALSE)          // disables usage as current cell
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
		   );

         CString cv = CString(_T("Debond\nLength\n(")) + 
              CString(pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str()) + 
              CString(_T(")\nRight"));

	      SetStyleRange(CGXRange(0,LAST_DEBOND_COL), CGXStyle()
               .SetWrapText(TRUE)
			      .SetEnabled(FALSE)          // disables usage as current cell
               .SetHorizontalAlignment(DT_CENTER)
               .SetVerticalAlignment(DT_VCENTER)
  		         .SetValue(cv)
		      );
   }

	SetStyleRange(CGXRange(0,FIRST_EXTEND_COL), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetFont(font)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetWrapText(TRUE)
			.SetValue(_T("Extend Left"))
		);

	SetStyleRange(CGXRange(0,LAST_EXTEND_COL), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetFont(font)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetWrapText(TRUE)
			.SetValue(_T("Extend Right"))
		);

   SymmetricDebond(m_pParent->m_bSymmetricDebond);

   HideCols(DEBOND_CHECK_COL,LAST_DEBOND_COL,(m_pParent->m_bCanDebondStrands ? FALSE : TRUE));
   HideCols(FIRST_EXTEND_COL,LAST_EXTEND_COL,(m_pParent->m_bCanExtendStrands ? FALSE : TRUE));

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

void CStrandFillGrid::SetRowStyle(ROWCOL nRow)
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

   SetStyleRange(CGXRange(nRow,FIRST_DEBOND_COL,nRow,LAST_DEBOND_COL), CGXStyle()
		   .SetUserAttribute(GX_IDS_UA_VALID_MIN, _T("0.0e01"))
		   .SetUserAttribute(GX_IDS_UA_VALID_MAX, m_strMaxDebondLength)
		   .SetUserAttribute(GX_IDS_UA_VALID_MSG, _T("Debond length must be zero or greater and value cannot exceed half-girder length"))
         .SetHorizontalAlignment(DT_RIGHT)
         .SetReadOnly(TRUE)
         .SetEnabled(FALSE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
	   );

   SetStyleRange(CGXRange(nRow,FIRST_EXTEND_COL), CGXStyle()
			.SetControl(GX_IDS_CTRL_CHECKBOX3D)  //
			.SetValue(_T("0"))
         .SetHorizontalAlignment(DT_CENTER)
         );

   SetStyleRange(CGXRange(nRow,LAST_EXTEND_COL), CGXStyle()
			.SetControl(GX_IDS_CTRL_CHECKBOX3D)  //
			.SetValue(_T("0"))
         .SetHorizontalAlignment(DT_CENTER)
         );
}

void CStrandFillGrid::FillGrid()
{
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

      // only straight strands can be extended
      bool canExtend = strandType==GirderLibraryEntry::stStraight;

      StrandIndexType oneOrTwo;
      bool canDebond(false);
      if (strandType==GirderLibraryEntry::stStraight)
      {
         Float64 xs, ys, xe, ye;
         m_pGdrEntry->GetStraightStrandCoordinates(localIdx, &xs, &ys, &xe, &ye, &canDebond);
         oneOrTwo = (xs==0.0 && xe==0.0) ? 1 : 2;
      }
      else if (strandType==GirderLibraryEntry::stAdjustable)
      {
         Float64 xs, ys, xh, yh, xe, ye;
         m_pGdrEntry->GetHarpedStrandCoordinates(localIdx, &xs, &ys, &xh, &yh, &xe, &ye);
         oneOrTwo = (xs==0.0 && xh==0.0 && xe==0.0) ? 1 : 2;
      }
      else
      {
         ATLASSERT(false);
      }

      CString strStrand;
      if (oneOrTwo==1)
      {
         strStrand.Format(_T("%d"),currPositionNo+1);
      }
      else if (oneOrTwo==2)
      {
         strStrand.Format(_T("%d-%d"),currPositionNo+1,currPositionNo+2);
      }
      else
      {
         ATLASSERT(false); // new fill type?
      }

      SetStyleRange(CGXRange(row,0), CGXStyle().SetValue(strStrand).SetHorizontalAlignment(DT_CENTER));


      CString strType;
      if (strandType==GirderLibraryEntry::stStraight)
      {
         if(canDebond)
         {
            strType = _T("S-DB");
         }
         else
         {
            strType = _T("S");
         }
      }
      else if (strandType == GirderLibraryEntry::stAdjustable)
      {
         if(m_pParent->m_AdjustableStrandType == pgsTypes::asStraight)
         {
            strType = _T("A-S");
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

      bool bIsExtendedLeft  = IsStrandExtended(userData.strandTypeGridIdx,pgsTypes::metStart);
      bool bIsExtendedRight = IsStrandExtended(userData.strandTypeGridIdx,pgsTypes::metEnd);

      if ( !isFilled )
      {
         // strand not selected... disable the row
         SetStyleRange(CGXRange(row,DEBOND_CHECK_COL,row,LAST_EXTEND_COL), CGXStyle()
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

         if (!canExtend)
         {
            SetStyleRange(CGXRange(row,FIRST_EXTEND_COL), CGXStyle()
			         .SetControl(GX_IDS_CTRL_STATIC)  //
			         .SetValue(_T(""))
                  .SetReadOnly(TRUE)
                  .SetEnabled(FALSE)
                  .SetInterior(::GetSysColor(COLOR_BTNFACE))
                  .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
                  );

            SetStyleRange(CGXRange(row,LAST_EXTEND_COL), CGXStyle()
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

         SetStyleRange(CGXRange(row,DEBOND_CHECK_COL), CGXStyle()
			      .SetControl(GX_IDS_CTRL_CHECKBOX3D)  //
               .SetValue(bIsDebonded ? _T("1") : _T("0"))
               .SetReadOnly(FALSE)
               .SetEnabled(TRUE)
               );

         if ( bIsDebonded )
         {
            CComPtr<IBroker> pBroker;
            EAFGetBroker(&pBroker);
            GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

            // strand is debonded
            SetStyleRange(CGXRange(row,FIRST_DEBOND_COL), CGXStyle()
               .SetValue(FormatDimension(leftDebond, pDisplayUnits->GetXSectionDimUnit(), false))
               .SetReadOnly(FALSE)
               .SetEnabled(TRUE)
               .SetInterior(::GetSysColor(COLOR_WINDOW))
               .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
            );

            // fill both cells if we need later
            if (m_pParent->m_bSymmetricDebond)
            {
               rightDebond = leftDebond;
            }

            SetStyleRange(CGXRange(row,LAST_DEBOND_COL), CGXStyle()
               .SetValue(FormatDimension(rightDebond, pDisplayUnits->GetXSectionDimUnit(), false))
               .SetReadOnly(FALSE)
               .SetEnabled(TRUE)
               .SetInterior(::GetSysColor(COLOR_WINDOW))
               .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
            );

            // strands cannot be extended if we are debonding... disable the extended strand check boxes
            SetStyleRange(CGXRange(row,FIRST_EXTEND_COL,row,LAST_EXTEND_COL),CGXStyle()
               .SetValue(_T("0"))
               .SetEnabled(FALSE)
               .SetReadOnly(TRUE)
               );
         }
         else
         {
            // strand is not debonded so blank out the debond lenght data
            SetStyleRange(CGXRange(row,FIRST_DEBOND_COL,row,LAST_DEBOND_COL),CGXStyle()
               .SetValue(_T(""))
               );

            // strand can be extended because it is not debonded
            if ( bIsExtendedLeft || bIsExtendedRight )
            {
               // Can't debond if the strand is extended... disable the check box
               SetStyleRange(CGXRange(row,DEBOND_CHECK_COL), CGXStyle()
			            .SetControl(GX_IDS_CTRL_CHECKBOX3D)  //
                     .SetValue(_T("0"))
                     .SetReadOnly(TRUE)
                     .SetEnabled(FALSE)
                     );
            }

            SetStyleRange(CGXRange(row,FIRST_EXTEND_COL),CGXStyle()
               .SetValue(bIsExtendedLeft ? _T("1") : _T("0"))
               .SetEnabled(TRUE)
               .SetReadOnly(FALSE)
               );

            SetStyleRange(CGXRange(row,LAST_EXTEND_COL),CGXStyle()
               .SetValue(bIsExtendedRight ? _T("1") : _T("0"))
               .SetEnabled(TRUE)
               .SetReadOnly(FALSE)
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

         if (canExtend)
         {
         // Set the extended strand data
         SetStyleRange(CGXRange(row,FIRST_EXTEND_COL),CGXStyle()
            .SetValue(bIsExtendedLeft ? _T("1") : _T("0"))
            .SetEnabled(TRUE)
            .SetReadOnly(FALSE)
            );

         SetStyleRange(CGXRange(row,LAST_EXTEND_COL),CGXStyle()
            .SetValue(bIsExtendedRight ? _T("1") : _T("0"))
            .SetEnabled(TRUE)
            .SetReadOnly(FALSE)
            );
         }
         else
         {
            // no controls needed if we can't extend
            SetStyleRange(CGXRange(row,FIRST_EXTEND_COL), CGXStyle()
			         .SetControl(GX_IDS_CTRL_STATIC)  //
			         .SetValue(_T(""))
                  .SetReadOnly(TRUE)
                  .SetEnabled(FALSE)
                  .SetInterior(::GetSysColor(COLOR_BTNFACE))
                  .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
                  );

            SetStyleRange(CGXRange(row,LAST_EXTEND_COL), CGXStyle()
			         .SetControl(GX_IDS_CTRL_STATIC)  //
			         .SetValue(_T(""))
                  .SetReadOnly(TRUE)
                  .SetEnabled(FALSE)
                  .SetInterior(::GetSysColor(COLOR_BTNFACE))
                  .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
                  );
         }
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
      Float64 xs, ys, xe, ye;
      m_pGdrEntry->GetTemporaryStrandCoordinates(gridIdx, &xs, &ys, &xe, &ye);
      StrandIndexType oneOrTwo = (xs==0.0 && xe==0.0) ? 1 : 2;

      CString strStrand;
      if (oneOrTwo==1)
      {
         strStrand.Format(_T("%d"),currPositionNo+1);
      }
      else if (oneOrTwo==2)
      {
         strStrand.Format(_T("%d-%d"),currPositionNo+1,currPositionNo+2);
      }
      else
      {
         ATLASSERT(false); // new fill type?
      }

      SetStyleRange(CGXRange(row,0), CGXStyle().SetValue(strStrand));
      SetStyleRange(CGXRange(row,TYPE_COL), CGXStyle().SetValue(_T("T")));

      bool isFilled = IsTempStrandFilled(gridIdx);

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

      SetStyleRange(CGXRange(row,FIRST_EXTEND_COL), CGXStyle()
			   .SetControl(GX_IDS_CTRL_STATIC)  //
			   .SetValue(_T(""))
            .SetReadOnly(TRUE)
            .SetEnabled(FALSE)
            .SetInterior(::GetSysColor(COLOR_BTNFACE))
            .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
            );

      SetStyleRange(CGXRange(row,LAST_EXTEND_COL), CGXStyle()
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

bool CStrandFillGrid::UpdateData(bool doCheckData)
{
   // Strand fill information is kept up to date in OnClickedButtonRowCol
   // However, debonding and extended strand information must be taken care of here
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2_NOCHECK(pBroker,IEAFDisplayUnits,pDisplayUnits); // may or may not be used, depends on input parameters

   // clear out old debond data
   m_pParent->m_StraightDebond.clear();

   // clear out old extended strand information
   m_pParent->m_ExtendedStrands[pgsTypes::metStart].clear();
   m_pParent->m_ExtendedStrands[pgsTypes::metEnd].clear();

   // walk the entire grid and find debonded and extended strands
   ROWCOL nRows = this->GetRowCount();
   for (ROWCOL nRow=1; nRow<=nRows; nRow++)
   {
      CGXStyle select_check_style;
      GetStyleRowCol(nRow, SELECT_CHECK_COL, select_check_style);
      if ( select_check_style.GetValue() == _T("0") )
      {
         continue; // strand are not selected... continue to the next row
      }

      const CUserData& userData = dynamic_cast<const CUserData&>(select_check_style.GetUserAttribute(0));

      CGXStyle style;
      GetStyleRowCol(nRow, DEBOND_CHECK_COL, style);
      if ( style.GetValue() == _T("1") )
      {
         // Strand is debonded
         Float64 leftDebond(0.0), rightDebond(0.0);

         CGXStyle style;
         GetStyleRowCol(nRow, FIRST_DEBOND_COL, style);
         CString strval = style.GetValue();
         if( !strval.IsEmpty())
         {
            bool st = sysTokenizer::ParseDouble(strval, &leftDebond);
            if(!st && doCheckData)
            {
               AfxMessageBox( _T("Debond length is not a number - value must be zero or greater"), MB_ICONEXCLAMATION);
               this->SetCurrentCell(nRow,FIRST_DEBOND_COL,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
               return false;
            }

            leftDebond  = ::ConvertToSysUnits(leftDebond,  pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
         }

         if (m_pParent->m_bSymmetricDebond)
         {
            if (leftDebond <= 0.0 && doCheckData)
            {
               AfxMessageBox( _T("Debond length must be greater than zero"), MB_ICONEXCLAMATION);
               this->SetCurrentCell(nRow,FIRST_DEBOND_COL,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
               return false;
            }

            rightDebond = leftDebond;
         }
         else
         {
            GetStyleRowCol(nRow, LAST_DEBOND_COL, style);
            CString strval = style.GetValue();
            if( !strval.IsEmpty())
            {
               bool st = sysTokenizer::ParseDouble(strval, &rightDebond);
               if(!st && doCheckData)
               {
                  AfxMessageBox( _T("Debond length is not a number - must be zero or greater"), MB_ICONEXCLAMATION);
                  this->SetCurrentCell(nRow,LAST_DEBOND_COL,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
                  return false;
               }

               rightDebond = ::ConvertToSysUnits(rightDebond, pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);

               if ((leftDebond < 0.0 || rightDebond < 0.0) && doCheckData)
               {
                  AfxMessageBox( _T("Debond length must be zero or greater"), MB_ICONEXCLAMATION);
                  this->SetCurrentCell(nRow,FIRST_DEBOND_COL,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
                  return false;
               }

            }
         }

         if (doCheckData)
         {
            if (leftDebond <= 0.0 && rightDebond <= 0.0)
            {
               AfxMessageBox( _T("Debond length must be greater than zero."), MB_ICONEXCLAMATION);
               return false;
            }
            else if (m_pParent->m_MaxDebondLength < leftDebond || 
                     m_pParent->m_MaxDebondLength < rightDebond)
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
      else
      {
         CGXStyle extendStyle;
         GetStyleRowCol(nRow, FIRST_EXTEND_COL, extendStyle);
         CString strCheck = extendStyle.GetValue();
         if ( strCheck == _T("1") ) 
         {
            ATLASSERT(userData.strandTypeGridIdx != INVALID_INDEX);
            ATLASSERT(userData.strandType == pgsTypes::Straight);
            m_pParent->m_ExtendedStrands[pgsTypes::metStart].push_back(userData.strandTypeGridIdx);
         }

         GetStyleRowCol(nRow, LAST_EXTEND_COL, extendStyle);
         strCheck = extendStyle.GetValue();
         if ( strCheck == _T("1") )
         {
            ATLASSERT(userData.strandTypeGridIdx != INVALID_INDEX);
            ATLASSERT(userData.strandType == pgsTypes::Straight);
            m_pParent->m_ExtendedStrands[pgsTypes::metEnd].push_back(userData.strandTypeGridIdx);
         }
      }
   }

   return true;
}

void CStrandFillGrid::OnClickedButtonRowCol(ROWCOL nRow, ROWCOL nCol)
{
   if ( nRow == 0 )
   {
      return;
   }

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

         // If strands are extendable, enable the check boxes
         if ( userData.strandType==pgsTypes::Straight && m_pParent->m_bCanExtendStrands )
         {
            SetStyleRange(CGXRange(nRow,FIRST_EXTEND_COL,nRow,LAST_EXTEND_COL),CGXStyle()
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
         SetStyleRange(CGXRange(nRow,DEBOND_CHECK_COL,nRow,LAST_EXTEND_COL), CGXStyle()
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
         SetStyleRange(CGXRange(nRow,FIRST_DEBOND_COL,nRow,LAST_DEBOND_COL), CGXStyle()
			      .SetValue(_T(""))
               .SetReadOnly(FALSE)
               .SetEnabled(TRUE)
               .SetInterior(::GetSysColor(COLOR_WINDOW))
               .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
               );

         // Disable extended strand check boxes
         SetStyleRange(CGXRange(nRow,FIRST_EXTEND_COL,nRow,LAST_EXTEND_COL), CGXStyle()
		         .SetValue(_T(""))
               .SetReadOnly(TRUE)
               .SetEnabled(FALSE)
               .SetInterior(::GetSysColor(COLOR_BTNFACE))
               .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
               );
      }
      else
      {
         // Debond box was just unchecked (debonding removed)... clear the debond information in this row
         SetStyleRange(CGXRange(nRow,FIRST_DEBOND_COL,nRow,LAST_DEBOND_COL), CGXStyle()
			      .SetValue(_T(""))
               .SetReadOnly(TRUE)
               .SetEnabled(FALSE)
               .SetInterior(::GetSysColor(COLOR_BTNFACE))
               .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
               );

         if ( m_pParent->m_bCanExtendStrands )
         {
            // Strands can be extended so the check boxes have to be enabled
            SetStyleRange(CGXRange(nRow,FIRST_EXTEND_COL,nRow,LAST_EXTEND_COL), CGXStyle()
			         .SetValue(_T("0"))
                  .SetReadOnly(FALSE)
                  .SetEnabled(TRUE)
                  .SetInterior(::GetSysColor(COLOR_WINDOW))
                  .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
                  );
         }
      }

      GetParam()->SetLockReadOnly(TRUE);

      UpdateParent();
   }
   else if ( FIRST_EXTEND_COL <= nCol && nCol <= LAST_EXTEND_COL )
   {
      GetParam()->SetLockReadOnly(FALSE);

      CGXStyle style;
      GetStyleRowCol(nRow, FIRST_EXTEND_COL, style);
      bool bExtendLeft = style.GetValue() == _T("1") ? true : false;

      GetStyleRowCol(nRow, LAST_EXTEND_COL, style);
      bool bExtendRight = style.GetValue() == _T("1") ? true : false;

      GetStyleRowCol(nRow, SELECT_CHECK_COL, style);
      ATLASSERT( style.GetValue() != _T("0") );

      const CUserData& userData = dynamic_cast<const CUserData&>(style.GetUserAttribute(0));

      if ( userData.isDebondable ) // uncheck debond if debondable 
      {
         SetStyleRange(CGXRange(nRow,DEBOND_CHECK_COL), CGXStyle()
               .SetControl(GX_IDS_CTRL_CHECKBOX3D)  //
               .SetValue(_T("0"))
               .SetReadOnly(bExtendLeft || bExtendRight ? TRUE  : FALSE)
               .SetEnabled(bExtendLeft  || bExtendRight ? FALSE : TRUE)
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


void CStrandFillGrid::UpdateParent()
{
   m_pParent->OnNumStrandsChanged();
}

void CStrandFillGrid::SymmetricDebond(BOOL bSymmetricDebond)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   m_pParent->m_bSymmetricDebond = bSymmetricDebond;

   CString strColHeading = CString(_T("Debond\nLength\n(")) + 
                           CString(pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());

   if ( bSymmetricDebond )
   {
      VERIFY(HideCols(LAST_DEBOND_COL,LAST_DEBOND_COL));
      strColHeading += _T(")\nBoth");
   }
   else
   {
      VERIFY(HideCols(LAST_DEBOND_COL,LAST_DEBOND_COL,FALSE));
      strColHeading += _T(")\nLeft");
   }

   SetStyleRange(CGXRange(0,FIRST_DEBOND_COL), CGXStyle().SetValue(strColHeading));

   // make text fit correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,0,GetColCount()));
}

bool CStrandFillGrid::IsPermStrandFilled(GirderLibraryEntry::psStrandType strandType, StrandIndexType idxStrandGrid)
{
   if (strandType==GirderLibraryEntry::stStraight)
   {
      return m_pParent->m_pStrands->GetDirectStrandFillStraight()->IsStrandFilled(idxStrandGrid);
   }
   if (strandType==GirderLibraryEntry::stAdjustable)
   {
      return m_pParent->m_pStrands->GetDirectStrandFillHarped()->IsStrandFilled(idxStrandGrid);
   }
   else
   {
      ATLASSERT(false);
      return false;
   }
}

bool CStrandFillGrid::IsTempStrandFilled(StrandIndexType idxStrandGrid)
{
   return m_pParent->m_pStrands->GetDirectStrandFillTemporary()->IsStrandFilled(idxStrandGrid);
}

void CStrandFillGrid::RemoveStrandFill(const CUserData* pUserData)
{
   if (pUserData->strandType == pgsTypes::Straight)
   {
      CDirectStrandFillCollection collection = *m_pParent->m_pStrands->GetDirectStrandFillStraight();
      collection.RemoveFill(pUserData->strandTypeGridIdx);
      m_pParent->m_pStrands->SetDirectStrandFillStraight(collection);
   }
   else if (pUserData->strandType == pgsTypes::Harped)
   {
      CDirectStrandFillCollection collection = *m_pParent->m_pStrands->GetDirectStrandFillHarped();
      collection.RemoveFill(pUserData->strandTypeGridIdx);
      m_pParent->m_pStrands->SetDirectStrandFillHarped(collection);
   }
   else if (pUserData->strandType == pgsTypes::Temporary)
   {
      CDirectStrandFillCollection collection = *m_pParent->m_pStrands->GetDirectStrandFillTemporary();
      collection.RemoveFill(pUserData->strandTypeGridIdx);
      m_pParent->m_pStrands->SetDirectStrandFillTemporary(collection);
   }
   else
   {
      ATLASSERT(false);
   }
}

void CStrandFillGrid::AddStrandFill(const CUserData* pUserData)
{
   CDirectStrandFillInfo fillinf(pUserData->strandTypeGridIdx, pUserData->oneOrTwo);

   if (pUserData->strandType == pgsTypes::Straight)
   {
      CDirectStrandFillCollection collection = *m_pParent->m_pStrands->GetDirectStrandFillStraight();
      collection.AddFill(fillinf);
      m_pParent->m_pStrands->SetDirectStrandFillStraight(collection);
   }
   else if (pUserData->strandType == pgsTypes::Harped)
   {
      CDirectStrandFillCollection collection = *m_pParent->m_pStrands->GetDirectStrandFillHarped();
      collection.AddFill(fillinf);
      m_pParent->m_pStrands->SetDirectStrandFillHarped(collection);
   }
   else if (pUserData->strandType == pgsTypes::Temporary)
   {
      CDirectStrandFillCollection collection = *m_pParent->m_pStrands->GetDirectStrandFillTemporary();
      collection.AddFill(fillinf);
      m_pParent->m_pStrands->SetDirectStrandFillTemporary(collection);
   }
   else
      ATLASSERT(false);
}

bool CStrandFillGrid::IsStrandExtended(GridIndexType gridIdx,pgsTypes::MemberEndType endType)
{
   std::vector<GridIndexType>::iterator iter(m_pParent->m_ExtendedStrands[endType].begin());
   std::vector<GridIndexType>::iterator end(m_pParent->m_ExtendedStrands[endType].end());
   while ( iter != end )
   {
      GridIndexType idx = *iter++;
      if ( idx == gridIdx )
      {
         return true;
      }
   }

   return false;
}

bool CStrandFillGrid::GetDebondInfo(StrandIndexType straightStrandGridIdx, Float64* pleftDebond, Float64* prightDebond)
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

void CStrandFillGrid::ToggleFill(ROWCOL rowNo)
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
     if (pCheck != nullptr) // only set check boxes
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
