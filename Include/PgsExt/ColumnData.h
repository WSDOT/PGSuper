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
#pragma once

#include <PgsExt\PgsExtExp.h>

class CPierData2;

class PGSEXTCLASS CColumnData
{
public:
   enum ColumnHeightMeasurementType
   {
      chtHeight,
      chtBottomElevation
   };

   enum ColumnShapeType
   {
      cstCircle,
      cstRectangle
   };

   CColumnData(CPierData2* pPier = nullptr);
   CColumnData(const CColumnData& rOther); 
   virtual ~CColumnData();

   CColumnData& operator = (const CColumnData& rOther);

   bool operator==(const CColumnData& rOther) const;
   bool operator!=(const CColumnData& rOther) const;

   void SetPier(CPierData2* pPier);
   const CPierData2* GetPier() const;
   CPierData2* GetPier();

   void SetColumnHeightMeasurementType(ColumnHeightMeasurementType heightMeasure);
   ColumnHeightMeasurementType GetColumnHeightMeasurementType() const;
   void SetColumnHeight(Float64 h,ColumnHeightMeasurementType heightMeasure);
   void SetColumnHeight(Float64 h);
   Float64 GetColumnHeight() const;

   void SetColumnShape(ColumnShapeType shapeType);
   ColumnShapeType GetColumnShape() const;

   // If the column shape is circular, D1 is the diameter of the column and D2 is ignored
   // If rectangular, D1 is the width of the column in the plane of the pier and D2 is
   // its depth perpendicular to the plane of the pier.
   void SetColumnDimensions(Float64 D1,Float64 D2);
   void GetColumnDimensions(Float64* pD1,Float64* pD2) const;

   // Set/Get the transverse fixity at the base of the column
   // This is used for transverse pier analysis only
   void SetTransverseFixity(pgsTypes::ColumnTransverseFixityType columnFixity);
   pgsTypes::ColumnTransverseFixityType GetTransverseFixity() const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

#if defined _DEBUG
   void AssertValid() const;
#endif

protected:
   //------------------------------------------------------------------------
   void MakeCopy(const CColumnData& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CColumnData& rOther);

private:
   CPierData2* m_pPier;
   ColumnHeightMeasurementType m_HeightMeasurementType;
   Float64 m_Height;
   ColumnShapeType m_ShapeType;
   Float64 m_D1, m_D2;

   pgsTypes::ColumnTransverseFixityType m_TransverseFixity; // fixity used for pier analysis
};
