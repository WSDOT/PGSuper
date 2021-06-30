///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#pragma once

#include <PgsExt\BridgeDescription2.h>

// Local data structures for Bearing Data at pier
typedef struct BearingPierData
{
   typedef enum BPDType {
      bpdAhead, // Ahead bearing line
      bpdCL,    // Centerline bearing line (continuous interior piers only)
      bpdBack   // Back bearing line
   } BPDType;

   BPDType                 m_BPDType;
   std::vector< CBearingData2 > m_BearingsForGirders; // for each girder at bearing line if BearingType is brtPier,
                                                      // if for all girders is in slot[0]

   PierIndexType  m_PierIndex;  // pier and group associated with this bearing line
   GroupIDType    m_pGroupIndex;
} BearingPierData;

typedef std::vector<BearingPierData> BearingPierDataVec;
typedef BearingPierDataVec::iterator BearingPierDataIter;
typedef BearingPierDataVec::const_iterator BearingPierDataConstIter;

class BearingInputData
{
public:
   pgsTypes::BearingType m_BearingType;

   // Different Data for each layout type
   CBearingData2 m_SingleBearing;
   BearingPierDataVec m_Bearings; // for both brtPier and brtGirder. if brtPier, data is in m_BearingsForGirders[0]

   // General
   GirderIndexType m_MaxGirdersPerSpan;

   // Functions to copy data to/from bridge description
   void CopyToBridgeDescription(CBridgeDescription2* pBridgeDescr) const;
   void CopyFromBridgeDescription(const CBridgeDescription2* pBridgeDescr);
};

// Shared class for getting and validating data in a grid
template <class G_TYPE> class BearingGridDataGetter
{
public:
   CBearingData2 GetBrgData(G_TYPE* pGrid, ROWCOL row, const unitmgtLengthData* pCompUnit, CDataExchange* pDX);

// grid columns of interest
   ROWCOL m_BearingShapeCol;
   ROWCOL m_BearingCountCol;
   ROWCOL m_BearingSpacingCol;
   ROWCOL m_BearingLengthCol;
   ROWCOL m_BearingWidthCol;
   ROWCOL m_BearingHeightCol;
   ROWCOL m_BearingRecessHeightCol;
   ROWCOL m_BearingRecessLengthCol;
   ROWCOL m_BearingSolePlateCol;
};

// BearingPierGrid.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CBearingPierGrid window

class CBearingPierGrid : public CGXGridWnd
{
	GRID_DECLARE_REGISTER()
// Construction
public:
	CBearingPierGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBearingPierGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBearingPierGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CBearingPierGrid)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // virtual overrides for grid
   virtual int GetColWidth(ROWCOL nCol);
   virtual void OnModifyCell(ROWCOL nRow,ROWCOL nCol);
   virtual BOOL OnValidateCell(ROWCOL nRow, ROWCOL nCol);

public:
   // custom init for grid
   void CustomInit();

   void FillGrid(const BearingInputData& BearingData);
   void GetData(BearingInputData* pData, CDataExchange* pDX);
   // get a cell value whether is is selected or not
   CString GetCellValue(ROWCOL nRow, ROWCOL nCol);

private:

   // set up styles for interior rows
   void SetRowStyle(ROWCOL nRow);

   const unitmgtLengthData* m_pCompUnit;
   BearingPierData m_BearingPierData;
   BearingGridDataGetter<CBearingPierGrid> m_DGetter;
};

// Template instantiation
template <class G_TYPE> CBearingData2 BearingGridDataGetter<G_TYPE>::GetBrgData(G_TYPE* pGrid, ROWCOL row, const unitmgtLengthData* pCompUnit, CDataExchange* pDX)
{
   CBearingData2 bd;

   // Shape
   bool isRound = false;
   CString str = pGrid->GetCellValue(row, m_BearingShapeCol);
   if (str == _T("Round"))
   {
      bd.Shape = bsRound;
      isRound = true;
   }
   else
   {
      bd.Shape = bsRectangular;
   }

   // Count
   str = pGrid->GetCellValue(row,m_BearingCountCol);
   unsigned long uval;
   if(sysTokenizer::ParseULong(str, &uval))
   {
      bd.BearingCount = uval;

      if (uval < 1)
      {
         AfxMessageBox( _T("Bearing count out of range"), MB_ICONEXCLAMATION);
         pGrid->SetCurrentCell(row,m_BearingCountCol,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
         pDX->Fail();
      }
   }
   else
   {
      AfxMessageBox( _T("Bearing Count is not a number - must be a positive number"), MB_ICONEXCLAMATION);
      pGrid->SetCurrentCell(row,m_BearingCountCol,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
      pDX->Fail();
   }

   // Spacing
   Float64 val;
   if (bd.BearingCount > 1)
   {
      str = pGrid->GetCellValue(row, m_BearingSpacingCol);
      if (sysTokenizer::ParseDouble(str, &val))
      {
         val = ::ConvertToSysUnits(val, pCompUnit->UnitOfMeasure);

         bd.Spacing = val;

         if (val <= 0.0)
         {
            AfxMessageBox(_T("Bearing spacing must be greater than zero"), MB_ICONEXCLAMATION);
            pGrid->SetCurrentCell(row, m_BearingSpacingCol, GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
            pDX->Fail();
         }
      }
      else
      {
         AfxMessageBox(_T("Bearing Spacing is not a number - must be a positive number"), MB_ICONEXCLAMATION);
         pGrid->SetCurrentCell(row, m_BearingSpacingCol, GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
         pDX->Fail();
      }
   }

   // Length
   str = pGrid->GetCellValue(row, m_BearingLengthCol);
   if (sysTokenizer::ParseDouble(str, &val))
   {
      val = ::ConvertToSysUnits(val, pCompUnit->UnitOfMeasure);

      bd.Length = val;

      if (val < 0.0)
      {
         AfxMessageBox(_T("Bearing length must be zero or greater"), MB_ICONEXCLAMATION);
         pGrid->SetCurrentCell(row, m_BearingLengthCol, GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
         pDX->Fail();
      }
   }
   else
   {
      AfxMessageBox(_T("Bearing Length is not a number - must be a positive number"), MB_ICONEXCLAMATION);
      pGrid->SetCurrentCell(row, m_BearingLengthCol, GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
      pDX->Fail();
   }

   // Width
   if (!isRound)
   {
      str = pGrid->GetCellValue(row, m_BearingWidthCol);
      if (sysTokenizer::ParseDouble(str, &val))
      {
         val = ::ConvertToSysUnits(val, pCompUnit->UnitOfMeasure);

         bd.Width = val;

         if (val < 0.0)
         {
            AfxMessageBox(_T("Bearing Width must be zero or greater"), MB_ICONEXCLAMATION);
            pGrid->SetCurrentCell(row, m_BearingWidthCol, GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
            pDX->Fail();
         }
      }
      else
      {
         AfxMessageBox(_T("Bearing Width is not a number - must be a positive number"), MB_ICONEXCLAMATION);
         pGrid->SetCurrentCell(row, m_BearingWidthCol, GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
         pDX->Fail();
      }
   }

   // Height
   str = pGrid->GetCellValue(row, m_BearingHeightCol);
   val;
   if (sysTokenizer::ParseDouble(str, &val))
   {
      val = ::ConvertToSysUnits(val, pCompUnit->UnitOfMeasure);

      bd.Height = val;

      if (val < 0.0)
      {
         AfxMessageBox(_T("Bearing Height must be zero or greater"), MB_ICONEXCLAMATION);
         pGrid->SetCurrentCell(row, m_BearingHeightCol, GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
         pDX->Fail();
      }
   }
   else
   {
      AfxMessageBox(_T("Bearing Height is not a number - must be a positive number"), MB_ICONEXCLAMATION);
      pGrid->SetCurrentCell(row, m_BearingHeightCol, GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
      pDX->Fail();
   }

   // Recess Height
   str = pGrid->GetCellValue(row, m_BearingRecessHeightCol);
   if (sysTokenizer::ParseDouble(str, &val))
   {
      val = ::ConvertToSysUnits(val, pCompUnit->UnitOfMeasure);

      bd.RecessHeight = val;

      if (val < 0.0)
      {
         AfxMessageBox(_T("Bearing Recess Height must be zero or greater"), MB_ICONEXCLAMATION);
         pGrid->SetCurrentCell(row, m_BearingRecessHeightCol, GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
         pDX->Fail();
      }
   }
   else
   {
      AfxMessageBox(_T("Bearing Recess Hieght is not a number - must be a positive number"), MB_ICONEXCLAMATION);
      pGrid->SetCurrentCell(row, m_BearingRecessHeightCol, GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
      pDX->Fail();
   }

   // Recess Length
   str = pGrid->GetCellValue(row, m_BearingRecessLengthCol);
   if (sysTokenizer::ParseDouble(str, &val))
   {
      val = ::ConvertToSysUnits(val, pCompUnit->UnitOfMeasure);

      bd.RecessLength = val;

      if (val < 0.0)
      {
         AfxMessageBox(_T("Bearing Recess Length must be zero or greater"), MB_ICONEXCLAMATION);
         pGrid->SetCurrentCell(row, m_BearingRecessLengthCol, GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
         pDX->Fail();
      }
   }
   else
   {
      AfxMessageBox(_T("Bearing Recess Hieght is not a number - must be a positive number"), MB_ICONEXCLAMATION);
      pGrid->SetCurrentCell(row, m_BearingRecessLengthCol, GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
      pDX->Fail();
   }


   // SolePlate
   str = pGrid->GetCellValue(row, m_BearingSolePlateCol);
   if (sysTokenizer::ParseDouble(str, &val))
   {
      val = ::ConvertToSysUnits(val, pCompUnit->UnitOfMeasure);

      bd.SolePlateHeight = val;

      if (val < 0.0)
      {
         AfxMessageBox(_T("Bearing Sole Plate must be zero or greater"), MB_ICONEXCLAMATION);
         pGrid->SetCurrentCell(row, m_BearingSolePlateCol, GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
         pDX->Fail();
      }
   }
   else
   {
      AfxMessageBox(_T("Bearing Sole Plate is not a number - must be a positive number"), MB_ICONEXCLAMATION);
      pGrid->SetCurrentCell(row, m_BearingSolePlateCol, GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
      pDX->Fail();
   }

   return bd;
}
