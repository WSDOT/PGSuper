///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\ColumnData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CColumnData
****************************************************************************/

CColumnData::CColumnData(CPierData2* pPier) :
m_pPier(pPier)
{
   m_HeightMeasurementType = chtHeight;
   m_Height = WBFL::Units::ConvertToSysUnits(20.0,WBFL::Units::Measure::Feet);

   m_ShapeType = cstCircle;
   m_D1 = WBFL::Units::ConvertToSysUnits(4.0,WBFL::Units::Measure::Feet);
   m_D2 = m_D1;

   m_TransverseFixity = pgsTypes::ctftTopFixedBottomFixed;
}

CColumnData::CColumnData(const CColumnData& rOther)
{
   MakeCopy(rOther);
}

CColumnData::~CColumnData()
{
}

CColumnData& CColumnData::operator= (const CColumnData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CColumnData::operator==(const CColumnData& rOther) const
{
   if ( m_HeightMeasurementType != rOther.m_HeightMeasurementType )
   {
      return false;
   }

   if ( !IsEqual(m_Height,rOther.m_Height) )
   {
      return false;
   }

   if ( m_ShapeType != rOther.m_ShapeType )
   {
      return false;
   }

   if ( !IsEqual(m_D1,rOther.m_D1) )
   {
      return false;
   }

   if ( m_ShapeType == cstRectangle && !IsEqual(m_D2,rOther.m_D2) )
   {
      return false;
   }

   if ( m_TransverseFixity != rOther.m_TransverseFixity )
   {
      return false;
   }

   return true;
}

bool CColumnData::operator!=(const CColumnData& rOther) const
{
   return !operator==(rOther);
}

HRESULT CColumnData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("Column"),2.0);

   pStrSave->put_Property(_T("Height"),CComVariant(m_Height));
   pStrSave->put_Property(_T("HeightMeasurement"),CComVariant(m_HeightMeasurementType));
   pStrSave->put_Property(_T("Shape"),CComVariant(m_ShapeType));
   pStrSave->put_Property(_T("D1"),CComVariant(m_D1));
   pStrSave->put_Property(_T("D2"),CComVariant(m_D2));
   pStrSave->put_Property(_T("TransverseFixity"),CComVariant(m_TransverseFixity)); // added in version 2

   pStrSave->EndUnit(); // Column

   return hr;
}

HRESULT CColumnData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   CHRException hr;

   try
   {
      CComVariant var;
      hr = pStrLoad->BeginUnit(_T("Column"));

      Float64 version;
      pStrLoad->get_Version(&version);

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Height"),&var);
      m_Height = var.dblVal;

      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("HeightMeasurement"),&var);
      m_HeightMeasurementType = (ColumnHeightMeasurementType)var.lVal;

      hr = pStrLoad->get_Property(_T("Shape"),&var);
      m_ShapeType = (ColumnShapeType)var.lVal;

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("D1"),&var);
      m_D1 = var.dblVal;

      hr = pStrLoad->get_Property(_T("D2"),&var);
      m_D2 = var.dblVal;

      if ( 1 < version )
      {
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("TransverseFixity"),&var);
         m_TransverseFixity = (pgsTypes::ColumnTransverseFixityType)var.lVal;
      }

      hr = pStrLoad->EndUnit(); // Column
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   PGS_ASSERT_VALID;

   return S_OK;
}

void CColumnData::SetPier(CPierData2* pPier)
{
   m_pPier = pPier;
}

const CPierData2* CColumnData::GetPier() const
{
   return m_pPier;
}

CPierData2* CColumnData::GetPier()
{
   return m_pPier;
}

void CColumnData::MakeCopy(const CColumnData& rOther)
{
   m_pPier = rOther.m_pPier;
   m_HeightMeasurementType = rOther.m_HeightMeasurementType;
   m_Height = rOther.m_Height;

   m_ShapeType = rOther.m_ShapeType;
   m_D1 = rOther.m_D1;
   m_D2 = rOther.m_D2;

   m_TransverseFixity = rOther.m_TransverseFixity;

   PGS_ASSERT_VALID;
}

void CColumnData::MakeAssignment(const CColumnData& rOther)
{
   MakeCopy( rOther );
}

void CColumnData::SetColumnHeightMeasurementType(ColumnHeightMeasurementType heightMeasure)
{
   m_HeightMeasurementType = heightMeasure;
}

CColumnData::ColumnHeightMeasurementType CColumnData::GetColumnHeightMeasurementType() const
{
   return m_HeightMeasurementType;
}

void CColumnData::SetColumnHeight(Float64 h,ColumnHeightMeasurementType heightMeasure)
{
   m_Height = h;
   m_HeightMeasurementType = heightMeasure;
}

void CColumnData::SetColumnHeight(Float64 h)
{
   m_Height = h;
}

Float64 CColumnData::GetColumnHeight() const
{
   return m_Height;
}

void CColumnData::SetColumnShape(CColumnData::ColumnShapeType shapeType)
{
   m_ShapeType = shapeType;
}

CColumnData::ColumnShapeType CColumnData::GetColumnShape() const
{
   return m_ShapeType;
}

void CColumnData::SetColumnDimensions(Float64 D1,Float64 D2)
{
   m_D1 = D1;
   m_D2 = D2;
}

void CColumnData::GetColumnDimensions(Float64* pD1,Float64* pD2) const
{
   *pD1 = m_D1;
   *pD2 = m_D2;
}

void CColumnData::SetTransverseFixity(pgsTypes::ColumnTransverseFixityType columnFixity)
{
   m_TransverseFixity = columnFixity;
}

pgsTypes::ColumnTransverseFixityType CColumnData::GetTransverseFixity() const
{
   return m_TransverseFixity;
}

#if defined _DEBUG
void CColumnData::AssertValid() const
{
}
#endif
