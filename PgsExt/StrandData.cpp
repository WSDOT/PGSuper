///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <PgsExt\StrandData.h>
#include <Lrfd\StrandPool.h>
#include <PsgLib\GirderLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CDirectStrandFillInfo
****************************************************************************/
HRESULT CDirectStrandFillInfo::Load(IStructuredLoad* pStrLoad)
{
   CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit(_T("DirectStrandFillInfo"));

      CComVariant var;

      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("idxStrandGrid"),    &var);
      permStrandGridIdx = VARIANT2INDEX(var);

      hr = pStrLoad->get_Property(_T("numFilled"),    &var);
      numFilled = VARIANT2INDEX(var);

      hr = pStrLoad->EndUnit();
   }
   catch(...)
   {
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   return S_OK;
}

HRESULT CDirectStrandFillInfo::Save(IStructuredSave* pStrSave) const
{
   pStrSave->BeginUnit(_T("DirectStrandFillInfo"),1.0);
   pStrSave->put_Property(_T("idxStrandGrid"), CComVariant(permStrandGridIdx));
   pStrSave->put_Property(_T("numFilled"),     CComVariant(numFilled));
   pStrSave->EndUnit();

   return S_OK;
}

bool CDirectStrandFillInfo::operator==(const CDirectStrandFillInfo& rOther) const
{
   if ( permStrandGridIdx != rOther.permStrandGridIdx )
   {
      return false;
   }

   if ( numFilled != rOther.numFilled )
   {
      return false;
   }

   return true;
}

bool CDirectStrandFillInfo::operator!=(const CDirectStrandFillInfo& rOther) const
{
   return !CDirectStrandFillInfo::operator==(rOther);
}

StrandIndexType CDirectStrandFillCollection::GetFilledStrandCount() const
{
   StrandIndexType cnt(0);

   const_iterator it    = begin();
   const_iterator itend = end();
   while(it!=itend)
   {
      cnt += it->numFilled;
      it++;
   }

   return cnt;
}

bool CDirectStrandFillCollection::IsStrandFilled(GridIndexType indexGrid) const
{
   return 0 < GetFillCountAtIndex(indexGrid) ? true : false;
}

StrandIndexType CDirectStrandFillCollection::GetFillCountAtIndex(GridIndexType indexGrid) const
{
   const_iterator it    = begin();
   const_iterator itend = end();
   while(it!=itend)
   {
      if (it->permStrandGridIdx == indexGrid)
      {
         return it->numFilled;
      }

      it++;
   }

   return 0;
}

void CDirectStrandFillCollection::RemoveFill(StrandIndexType index) 
{
   std::vector<CDirectStrandFillInfo>::iterator it = m_StrandFill.begin();
   std::vector<CDirectStrandFillInfo>::iterator itend = m_StrandFill.end();
   while(it!=itend)
   {
      if (it->permStrandGridIdx == index)
      {
         m_StrandFill.erase(it);
         return;
      }
      it++;
   }

   ATLASSERT(false); // not found?
}

void CDirectStrandFillCollection::AddFill(const CDirectStrandFillInfo& fillInf)
{
   ATLASSERT(fillInf.numFilled>0); // compressed container should not contain unfilled strands

   StrandIndexType localIdx = fillInf.permStrandGridIdx;

   // Fill in sorted order
   CDirectStrandFillCollection::const_iterator it = m_StrandFill.begin();
   CDirectStrandFillCollection::const_iterator itend = m_StrandFill.end();
   while(it!=itend)
   {
      ATLASSERT(it->permStrandGridIdx!=localIdx); // should never have duplicates
      if (it->permStrandGridIdx > localIdx)
      {
         m_StrandFill.insert(it, fillInf);
         return;
      }
      it++;
   }

   m_StrandFill.push_back(fillInf);
}

const CDirectStrandFillInfo& CDirectStrandFillCollection::GetFill(CollectionIndexType fillNo) const
{
   ATLASSERT(fillNo < m_StrandFill.size());
   return m_StrandFill[fillNo];
}

/////////////////////////////////////////////////////////////////////////////////////
CStrandRow::CStrandRow()
{
   m_InnerSpacing = ::ConvertToSysUnits(2.0,unitMeasure::Inch);
   m_Spacing = m_InnerSpacing;
   m_StrandType = pgsTypes::Straight;
   m_nStrands = 0;

   Float64 x[] = {0,-0.4,-0.6,-1.0};
   for ( int i = 0; i < 4; i++ )
   {
      m_X[i]    = x[i];
      m_Y[i]    = ::ConvertToSysUnits(2.0,unitMeasure::Inch);
      m_Face[i] = pgsTypes::BottomFace;
   }

   m_bIsExtendedStrand[pgsTypes::metStart] = false;
   m_bIsExtendedStrand[pgsTypes::metEnd] = false;

   m_bIsDebonded[pgsTypes::metStart] = false;
   m_bIsDebonded[pgsTypes::metEnd] = false;
   m_DebondLength[pgsTypes::metStart] = 0;
   m_DebondLength[pgsTypes::metEnd] = 0;
}

bool CStrandRow::operator==(const CStrandRow& other) const
{
   if ( !IsEqual(m_InnerSpacing,other.m_InnerSpacing) )
   {
      return false;
   }

   if ( !IsEqual(m_Spacing,other.m_Spacing) )
   {
      return false;
   }

   if ( m_StrandType != other.m_StrandType )
   {
      return false;
   }

   if ( m_nStrands != other.m_nStrands )
   {
      return false;
   }

   for ( int i = 0; i < 4; i++ )
   {
      if ( !IsEqual(m_X[i],other.m_X[i]) )
      {
         return false;
      }

      if ( !IsEqual(m_Y[i],other.m_Y[i]) )
      {
         return false;
      }

      if ( m_Face[i] != other.m_Face[i] )
      {
         return false;
      }
   }

   if ( m_StrandType != pgsTypes::Temporary )
   {
      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType end = (pgsTypes::MemberEndType)i;

         if ( m_bIsExtendedStrand[end] != other.m_bIsExtendedStrand[end] )
         {
            return false;
         }

         if ( m_bIsDebonded[end] )
         {
            if ( m_bIsDebonded[end] != other.m_bIsDebonded[end] )
            {
               return false;
            }

            if ( !IsEqual(m_DebondLength[end],other.m_DebondLength[end]) )
            {
               return false;
            }
         }
      }
   }

   return true;
}

bool CStrandRow::operator!=(const CStrandRow& other) const
{
   return !operator==(other);
}

HRESULT CStrandRow::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("StrandRow"),2.0);

   pStrSave->put_Property(_T("InnerSpacing"),CComVariant(m_InnerSpacing));
   pStrSave->put_Property(_T("Spacing"),CComVariant(m_Spacing));
   
   // added in version 2
   LPCTSTR strStrandType[3]={_T("Straight"),_T("Harped"),_T("Temporary")};
   pStrSave->put_Property(_T("Type"),CComVariant(strStrandType[m_StrandType]));

   pStrSave->put_Property(_T("NStrands"),CComVariant(m_nStrands));

   pStrSave->put_Property(_T("YStart"),CComVariant(m_Y[LOCATION_START]));
   pStrSave->put_Property(_T("FaceStart"),CComVariant(m_Face[LOCATION_START] == pgsTypes::TopFace ? _T("Top") : _T("Bottom")));

   // added in version 2
   pStrSave->put_Property(_T("XLHP"),CComVariant(m_X[LOCATION_LEFT_HP]));
   pStrSave->put_Property(_T("YLHP"),CComVariant(m_Y[LOCATION_LEFT_HP]));
   pStrSave->put_Property(_T("FaceLHP"),CComVariant(m_Face[LOCATION_LEFT_HP] == pgsTypes::TopFace ? _T("Top") : _T("Bottom")));

   // added in version 2
   pStrSave->put_Property(_T("XRHP"),CComVariant(m_X[LOCATION_RIGHT_HP]));
   pStrSave->put_Property(_T("YRHP"),CComVariant(m_Y[LOCATION_RIGHT_HP]));
   pStrSave->put_Property(_T("FaceRHP"),CComVariant(m_Face[LOCATION_RIGHT_HP] == pgsTypes::TopFace ? _T("Top") : _T("Bottom")));

   pStrSave->put_Property(_T("YEnd"),CComVariant(m_Y[LOCATION_END]));
   pStrSave->put_Property(_T("FaceEnd"),CComVariant(m_Face[LOCATION_END] == pgsTypes::TopFace ? _T("Top") : _T("Bottom")));

   //pStrSave->put_Property(_T("IsTemporaryStrand"),CComVariant(m_bIsTemporaryStrand)); // removed in version 2

   pStrSave->put_Property(_T("IsExtendedStrandStart"),CComVariant(m_bIsExtendedStrand[pgsTypes::metStart]));
   pStrSave->put_Property(_T("IsExtendedStrandEnd"),CComVariant(m_bIsExtendedStrand[pgsTypes::metEnd]));
   pStrSave->put_Property(_T("IsDebondedStart"),CComVariant(m_bIsDebonded[pgsTypes::metStart]));
   pStrSave->put_Property(_T("IsDebondedEnd"),CComVariant(m_bIsDebonded[pgsTypes::metEnd]));
   pStrSave->put_Property(_T("DebondLengthStart"),CComVariant(m_DebondLength[pgsTypes::metStart]));
   pStrSave->put_Property(_T("DebondLengthEnd"),CComVariant(m_DebondLength[pgsTypes::metEnd]));

   pStrSave->EndUnit();

   return S_OK;
}

HRESULT CStrandRow::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   CHRException hr;
   try
   {
      CComVariant var;
      hr = pStrLoad->BeginUnit(_T("StrandRow"));

      Float64 version;
      pStrLoad->get_Version(&version);

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("InnerSpacing"),&var);
      m_InnerSpacing = var.dblVal;

      hr = pStrLoad->get_Property(_T("Spacing"),&var);
      m_Spacing = var.dblVal;

      // added in version 2
      if ( 1.0 < version )
      {
         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("Type"),&var);
         if ( CComBSTR(var.bstrVal) == CComBSTR("Straight") )
         {
            m_StrandType = pgsTypes::Straight;
         }
         else if ( CComBSTR(var.bstrVal) == CComBSTR("Harped") )
         {
            m_StrandType = pgsTypes::Harped;
         }
         else
         {
            m_StrandType = pgsTypes::Temporary;
         }
      }

      var.vt = VT_INDEX;
      hr = pStrLoad->get_Property(_T("NStrands"),&var);
      m_nStrands = VARIANT2INDEX(var);

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("YStart"),&var);
      m_Y[LOCATION_START] = var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("FaceStart"),&var);
      m_Face[LOCATION_START] = (CComBSTR(var.bstrVal) == CComBSTR(_T("Top")) ? pgsTypes::TopFace : pgsTypes::BottomFace);

      // added in version 2
      if ( 1 < version )
      {
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("XLHP"),&var);
         m_X[LOCATION_LEFT_HP] = var.dblVal;

         pStrLoad->get_Property(_T("YLHP"),&var);
         m_Y[LOCATION_LEFT_HP] = var.dblVal;

         var.vt = VT_BSTR;
         pStrLoad->get_Property(_T("FaceLHP"),&var);
         m_Face[LOCATION_LEFT_HP] = (CComBSTR(var.bstrVal) == CComBSTR(_T("Top")) ? pgsTypes::TopFace : pgsTypes::BottomFace);

         var.vt = VT_R8;
         pStrLoad->get_Property(_T("XRHP"),&var);
         m_X[LOCATION_RIGHT_HP] = var.dblVal;

         pStrLoad->get_Property(_T("YRHP"),&var);
         m_Y[LOCATION_RIGHT_HP] = var.dblVal;

         var.vt = VT_BSTR;
         pStrLoad->get_Property(_T("FaceRHP"),&var);
         m_Face[LOCATION_RIGHT_HP] = (CComBSTR(var.bstrVal) == CComBSTR(_T("Top")) ? pgsTypes::TopFace : pgsTypes::BottomFace);
      }

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("YEnd"),&var);
      m_Y[LOCATION_END] = var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("FaceEnd"),&var);
      m_Face[LOCATION_END] = (CComBSTR(var.bstrVal) == CComBSTR(_T("Top")) ? pgsTypes::TopFace : pgsTypes::BottomFace);

      // removed in version 2
      if ( version < 2 )
      {
         var.vt = VT_BOOL;
         hr = pStrLoad->get_Property(_T("IsTemporaryStrand"),&var);
         m_StrandType = (var.boolVal == VARIANT_TRUE ? pgsTypes::Temporary : pgsTypes::Straight);
      }

      hr = pStrLoad->get_Property(_T("IsExtendedStrandStart"),&var);
      m_bIsExtendedStrand[pgsTypes::metStart] = (var.boolVal == VARIANT_TRUE ? true : false);

      hr = pStrLoad->get_Property(_T("IsExtendedStrandEnd"),&var);
      m_bIsExtendedStrand[pgsTypes::metEnd] = (var.boolVal == VARIANT_TRUE ? true : false);

      hr = pStrLoad->get_Property(_T("IsDebondedStart"),&var);
      m_bIsDebonded[pgsTypes::metStart] = (var.boolVal == VARIANT_TRUE ? true : false);

      hr = pStrLoad->get_Property(_T("IsDebondedEnd"),&var);
      m_bIsDebonded[pgsTypes::metEnd] = (var.boolVal == VARIANT_TRUE ? true : false);

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("DebondLengthStart"),&var);
      m_DebondLength[pgsTypes::metStart] = var.dblVal;

      hr = pStrLoad->get_Property(_T("DebondLengthEnd"),&var);
      m_DebondLength[pgsTypes::metEnd] = var.dblVal;

      pStrLoad->EndUnit();
   }
   catch(...)
   {
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////
CStrandData::CStrandData()
{
   for ( int i = 0; i < 3; i++ )
   {
      m_StrandMaterial[i] = lrfdStrandPool::GetInstance()->GetStrand(matPsStrand::Gr1860,matPsStrand::LowRelaxation,matPsStrand::D1524);
   }

   ResetPrestressData();

   m_NumPermStrandsType = CStrandData::npsStraightHarped;
   m_bConvertExtendedStrands = false;
}  

CStrandData::CStrandData(const CStrandData& rOther)
{
   MakeCopy(rOther);
}

CStrandData::~CStrandData()
{
}

//======================== OPERATORS  =======================================
CStrandData& CStrandData::operator= (const CStrandData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CStrandData::operator==(const CStrandData& rOther) const
{
   for ( Uint16 i = 0; i < 3; i++ )
   {
      if ( m_StrandMaterial[i] != rOther.m_StrandMaterial[i] )
      {
         return false;
      }
   }

   if (m_NumPermStrandsType != rOther.m_NumPermStrandsType)
   {
      return false;
   }

   if ( m_NumPermStrandsType == npsUser )
   {
      if ( m_StrandRows.size() != rOther.m_StrandRows.size() )
      {
         return false;
      }

      CStrandRowCollection::const_iterator iter(m_StrandRows.begin());
      CStrandRowCollection::const_iterator iterEnd(m_StrandRows.end());
      CStrandRowCollection::const_iterator otherIter(rOther.m_StrandRows.begin());
      for ( ; iter != iterEnd; iter++, otherIter++ )
      {
         const CStrandRow& strandRow(*iter);
         const CStrandRow& otherStrandRow(*otherIter);
         if ( strandRow != otherStrandRow )
         {
            return false;
         }
      }
   }

   for ( Uint16 i = 0; i < 4; i++ )
   {
      if ( m_bPjackCalculated[i] != rOther.m_bPjackCalculated[i] )
      {
         return false;
      }

      if ( m_Nstrands[i] != rOther.m_Nstrands[i] )
      {
         return false;
      }

      if ( !IsEqual(m_Pjack[i],rOther.m_Pjack[i]) )
      {
         return false;
      }

      if (i<3)
      {
         if ( m_Debond[i] != rOther.m_Debond[i] )
         {
            return false;
         }
      }
   }

   if ( m_HsoHpMeasurement != rOther.m_HsoHpMeasurement )
   {
      return false;
   }

   if ( m_HsoEndMeasurement != rOther.m_HsoEndMeasurement )
   {
      return false;
   }

   if ( m_HpOffsetAtEnd != rOther.m_HpOffsetAtEnd )
   {
      return false;
   }

   if ( m_HpOffsetAtHp != rOther.m_HpOffsetAtHp )
   {
      return false;
   }

   if ( m_TempStrandUsage != rOther.m_TempStrandUsage )
   {
      return false;
   }

   if ( m_bSymmetricDebond != rOther.m_bSymmetricDebond )
   {
      return false;
   }

   for ( int i = 0; i < 3; i++ )
   {
      if ( m_NextendedStrands[i][pgsTypes::metStart] != rOther.m_NextendedStrands[i][pgsTypes::metStart] )
      {
         return false;
      }

      if ( m_NextendedStrands[i][pgsTypes::metEnd] != rOther.m_NextendedStrands[i][pgsTypes::metEnd] )
      {
         return false;
      }
   }

   return true;
}

bool CStrandData::operator!=(const CStrandData& rOther) const
{
   return !operator==(rOther);
}

HRESULT CStrandData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress,Float64* pVersion)
{
   USES_CONVERSION;

   CHRException hr;

   CComVariant var;

   try
   {
      hr = pStrLoad->BeginUnit(_T("PrestressData"));  // named this for historical reasons
      Float64 version;
      hr = pStrLoad->get_Version(&version);
      *pVersion = version;

      if (version<6.0)
      {
         m_HsoEndMeasurement = hsoLEGACY;
      }
      else
      {
         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("HsoEndMeasurement"), &var );
         m_HsoEndMeasurement = (HarpedStrandOffsetType)(var.lVal);
      }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("HpOffsetAtEnd"), &var );
      m_HpOffsetAtEnd = var.dblVal;

      if (version<6.0)
      {
         m_HsoHpMeasurement = hsoLEGACY;
      }
      else
      {
         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("HsoHpMeasurement"), &var );
         m_HsoHpMeasurement = (HarpedStrandOffsetType)(var.lVal);
      }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("HpOffsetAtHp"), &var );
      m_HpOffsetAtHp = var.dblVal;

      if (version<8.0)
      {
         m_NumPermStrandsType = npsStraightHarped;
      }
      else
      {
         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("NumPermStrandsType"), &var );
         m_NumPermStrandsType = (PermanentStrandType)var.lVal;
      }

      if ( m_NumPermStrandsType == npsUser )
      {
         // added in version 13
         hr = pStrLoad->BeginUnit(_T("StrandRows"));
         var.Clear();
         var.vt = VT_INDEX;
         hr = pStrLoad->get_Property(_T("Count"),&var);
         IndexType nRows = VARIANT2INDEX(var);
         for ( RowIndexType rowIdx = 0; rowIdx < nRows; rowIdx++ )
         {
            CStrandRow strandRow;
            hr = strandRow.Load(pStrLoad,pProgress);
            m_StrandRows.push_back(strandRow);
         }
         hr = pStrLoad->EndUnit();
      }
      else
      {
         // stopped write this out in version 13 if NumPerStrandType is npsUser
         var.Clear();
         var.vt = VT_INDEX;
         hr = pStrLoad->get_Property(_T("NumHarpedStrands"), &var );
         m_Nstrands[pgsTypes::Harped] = VARIANT2INDEX(var);

         var.Clear();
         var.vt = VT_INDEX;
         hr = pStrLoad->get_Property(_T("NumStraightStrands"), &var );
         m_Nstrands[pgsTypes::Straight] = VARIANT2INDEX(var);

         var.Clear();
         var.vt = VT_INDEX;
         hr = pStrLoad->get_Property(_T("NumTempStrands"), &var );
         m_Nstrands[pgsTypes::Temporary] = VARIANT2INDEX(var);

         if (version<8.0)
         {
            m_Nstrands[pgsTypes::Permanent] = 0;
         }
         else
         {
            var.Clear();
            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("NumPermanentStrands"), &var );
            m_Nstrands[pgsTypes::Permanent] = VARIANT2INDEX(var);
         }
         
         if ( 12 <= version )
         {
            // added in version 12
            hr = pStrLoad->BeginUnit(_T("ExtendedStrands"));

            Float64 extendedStrandVersion;
            hr = pStrLoad->get_Version(&extendedStrandVersion);
            if ( extendedStrandVersion < 2.0 )
            {
               m_bConvertExtendedStrands = true;
            }

            hr = pStrLoad->BeginUnit(_T("Start"));

            hr = pStrLoad->BeginUnit(_T("Straight"));
            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("Count"),&var);
            StrandIndexType nStrands = VARIANT2INDEX(var);
            for ( StrandIndexType idx = 0; idx < nStrands; idx++ )
            {
               hr = pStrLoad->get_Property(_T("Strand"),&var);
               StrandIndexType strandIdx = VARIANT2INDEX(var);
               m_NextendedStrands[pgsTypes::Straight][pgsTypes::metStart].push_back(strandIdx);
            }
            hr = pStrLoad->EndUnit(); // Straight

            hr = pStrLoad->BeginUnit(_T("Harped"));
            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("Count"),&var);
            nStrands = VARIANT2INDEX(var);
            for ( StrandIndexType idx = 0; idx < nStrands; idx++ )
            {
               hr = pStrLoad->get_Property(_T("Strand"),&var);
               StrandIndexType strandIdx = VARIANT2INDEX(var);
               m_NextendedStrands[pgsTypes::Harped][pgsTypes::metStart].push_back(strandIdx);
            }
            hr = pStrLoad->EndUnit(); // Harped

            hr = pStrLoad->BeginUnit(_T("Temporary"));
            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("Count"),&var);
            nStrands = VARIANT2INDEX(var);
            for ( StrandIndexType idx = 0; idx < nStrands; idx++ )
            {
               hr = pStrLoad->get_Property(_T("Strand"),&var);
               StrandIndexType strandIdx = VARIANT2INDEX(var);
               m_NextendedStrands[pgsTypes::Temporary][pgsTypes::metStart].push_back(strandIdx);
            }
            hr = pStrLoad->EndUnit(); // Temporary
            hr = pStrLoad->EndUnit(); // Start

            hr = pStrLoad->BeginUnit(_T("End"));
            hr = pStrLoad->BeginUnit(_T("Straight"));
            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("Count"),&var);
            nStrands = VARIANT2INDEX(var);
            for ( StrandIndexType idx = 0; idx < nStrands; idx++ )
            {
               hr = pStrLoad->get_Property(_T("Strand"),&var);
               StrandIndexType strandIdx = VARIANT2INDEX(var);
               m_NextendedStrands[pgsTypes::Straight][pgsTypes::metEnd].push_back(strandIdx);
            }
            hr = pStrLoad->EndUnit(); // Straight

            hr = pStrLoad->BeginUnit(_T("Harped"));
            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("Count"),&var);
            nStrands = VARIANT2INDEX(var);
            for ( StrandIndexType idx = 0; idx < nStrands; idx++ )
            {
               hr = pStrLoad->get_Property(_T("Strand"),&var);
               StrandIndexType strandIdx = VARIANT2INDEX(var);
               m_NextendedStrands[pgsTypes::Harped][pgsTypes::metEnd].push_back(strandIdx);
            }
            hr = pStrLoad->EndUnit(); // Harped

            hr = pStrLoad->BeginUnit(_T("Temporary"));
            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("Count"),&var);
            nStrands = VARIANT2INDEX(var);
            for ( StrandIndexType idx = 0; idx < nStrands; idx++ )
            {
               hr = pStrLoad->get_Property(_T("Strand"),&var);
               StrandIndexType strandIdx = VARIANT2INDEX(var);
               m_NextendedStrands[pgsTypes::Temporary][pgsTypes::metEnd].push_back(strandIdx);
            }
            hr = pStrLoad->EndUnit(); // Temporary
            hr = pStrLoad->EndUnit(); // End

            hr = pStrLoad->EndUnit(); // ExtendedStrands
         }

         if (12 <= version && m_NumPermStrandsType == CStrandData::npsDirectSelection)
         {
            hr = hr = pStrLoad->BeginUnit(_T("DirectSelectStrandFill"));
            if (FAILED(hr))
            {
               ATLASSERT(false);
               return hr;
            }

            hr = pStrLoad->BeginUnit(_T("StraightStrands"));
            var.Clear();
            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("NumStraightFill"), &var );
            StrandIndexType nums = VARIANT2INDEX(var);
            for (StrandIndexType is=0; is<nums; is++)
            {
               CDirectStrandFillInfo fi;
               hr = fi.Load(pStrLoad);
               m_StraightStrandFill.AddFill(fi);
            }
            hr = pStrLoad->EndUnit();

            hr = pStrLoad->BeginUnit(_T("HarpedStrands"));
            var.Clear();
            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("NumHarpedFill"), &var );
            nums = VARIANT2INDEX(var);
            for (StrandIndexType is=0; is<nums; is++)
            {
               CDirectStrandFillInfo fi;
               hr = fi.Load(pStrLoad);
               m_HarpedStrandFill.AddFill(fi);
            }
            hr = pStrLoad->EndUnit();

            hr = pStrLoad->BeginUnit(_T("TemporaryStrands"));
            var.Clear();
            var.vt = VT_INDEX;
            hr = pStrLoad->get_Property(_T("NumTemporaryFill"), &var );
            nums = VARIANT2INDEX(var);
            for (StrandIndexType is=0; is<nums; is++)
            {
               CDirectStrandFillInfo fi;
               hr = fi.Load(pStrLoad);
               m_TemporaryStrandFill.AddFill(fi);
            }
            hr = pStrLoad->EndUnit();

            hr = pStrLoad->EndUnit(); // end DirectSelectStrandFill
         }
      }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("PjHarped"), &var );
      m_Pjack[pgsTypes::Harped] = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("PjStraight"), &var );
      m_Pjack[pgsTypes::Straight] = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("PjTemp"), &var );
      m_Pjack[pgsTypes::Temporary] = var.dblVal;

      if (version<8.0)
      {
         m_Pjack[pgsTypes::Permanent] = 0.0;
      }
      else
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("PjPermanent"), &var );
         m_Pjack[pgsTypes::Permanent] = var.dblVal;
      }

      var.Clear();
      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("CalcPjHarped"), &var );
      m_bPjackCalculated[pgsTypes::Harped] = (var.lVal!=0);

      var.Clear();
      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("CalcPjStraight"), &var );
      m_bPjackCalculated[pgsTypes::Straight] = (var.lVal!=0);

      var.Clear();
      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("CalcPjTemp"), &var );
      m_bPjackCalculated[pgsTypes::Temporary] = (var.lVal!=0);

      if (version<8.0)
      {
         m_bPjackCalculated[pgsTypes::Permanent] = false;
      }
      else
      {
         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("CalcPjPermanent"), &var );
         m_bPjackCalculated[pgsTypes::Permanent] = (var.lVal!=0);
      }

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("LastUserPjHarped"), &var );
      m_LastUserPjack[pgsTypes::Harped] = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("LastUserPjStraight"), &var );
      m_LastUserPjack[pgsTypes::Straight] = var.dblVal;

      var.Clear();
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("LastUserPjTemp"), &var );
      m_LastUserPjack[pgsTypes::Temporary] = var.dblVal;

      if (version<8.0)
      {
         m_LastUserPjack[pgsTypes::Permanent] = 0.0;
      }
      else
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("LastUserPjPermanent"), &var );
         m_LastUserPjack[pgsTypes::Permanent] = var.dblVal;
      }

      if ( version < 3.1 )
      {
         m_TempStrandUsage = pgsTypes::ttsPretensioned;
      }
      else
      {
         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("TempStrandUsage"),&var);
         m_TempStrandUsage = (pgsTypes::TTSUsage)var.lVal;
      }

      // in an earlier version of the constructor for this class,
      // TempStrandUsage was not initialized properly. This caused the variable to
      // be unset and bogus values to be stored... if the value of TempStrandUsage is
      // bogus, set it to a reasonable value
      if ( m_TempStrandUsage != pgsTypes::ttsPretensioned && 
           m_TempStrandUsage != pgsTypes::ttsPTBeforeShipping && 
           m_TempStrandUsage != pgsTypes::ttsPTBeforeLifting && 
           m_TempStrandUsage != pgsTypes::ttsPTAfterLifting )
      {
         m_TempStrandUsage = pgsTypes::ttsPretensioned;
      }


      if ( 5.0 <= version && m_NumPermStrandsType != npsUser )
      {
         // not writing this data if NumPermStrandsType is npsUser... this was added in version 13 of the data block

         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("SymmetricDebond"),&var);

         m_bSymmetricDebond = (var.lVal != 0);

         m_Debond[pgsTypes::Straight].clear();
         hr = pStrLoad->BeginUnit(_T("StraightStrandDebonding"));
         long nDebondInfo;
         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("DebondInfoCount"),&var);
         nDebondInfo = var.lVal;

         int i = 0;
         for ( i = 0; i < nDebondInfo; i++ )
         {
            CDebondData debond_info;
            hr = debond_info.Load(pStrLoad,pProgress);
            m_Debond[pgsTypes::Straight].push_back(debond_info);
         }
         hr = pStrLoad->EndUnit();


         m_Debond[pgsTypes::Harped].clear();
         hr = pStrLoad->BeginUnit(_T("HarpedStrandDebonding"));
         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("DebondInfoCount"),&var);
         nDebondInfo = var.lVal;

         for ( i = 0; i < nDebondInfo; i++ )
         {
            CDebondData debond_info;
            hr = debond_info.Load(pStrLoad,pProgress);
            m_Debond[pgsTypes::Harped].push_back(debond_info);
         }
         hr = pStrLoad->EndUnit();


         m_Debond[pgsTypes::Temporary].clear();
         hr = pStrLoad->BeginUnit(_T("TemporaryStrandDebonding"));
         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("DebondInfoCount"),&var);
         nDebondInfo = var.lVal;

         for ( i = 0; i < nDebondInfo; i++ )
         {
            CDebondData debond_info;
            hr = debond_info.Load(pStrLoad,pProgress);
            m_Debond[pgsTypes::Temporary].push_back(debond_info);
         }
         hr = pStrLoad->EndUnit();
      }


      if ( version < 9 )
      {
         m_StrandMaterial[pgsTypes::Straight]  = 0; // not used in pre-version 9 of this data block
         m_StrandMaterial[pgsTypes::Harped]    = 0; // not used in pre-version 9 of this data block
         m_StrandMaterial[pgsTypes::Temporary] = 0; // not used in pre-version 9 of this data block
         // the Project Agent will set this value later
      }
      else if ( version < 11 )
      {
         lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("StrandMaterialKey"),&var);
         Int32 key = var.lVal;
         m_StrandMaterial[pgsTypes::Straight] = pPool->GetStrand(key);
         ATLASSERT(m_StrandMaterial[pgsTypes::Straight] != 0);
         m_StrandMaterial[pgsTypes::Harped]    = m_StrandMaterial[pgsTypes::Straight];
         m_StrandMaterial[pgsTypes::Temporary] = m_StrandMaterial[pgsTypes::Straight];
      }
      else
      {
         lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("StraightStrandMaterialKey"),&var);
         Int32 key = var.lVal;
         m_StrandMaterial[pgsTypes::Straight] = pPool->GetStrand(key);

         hr = pStrLoad->get_Property(_T("HarpedStrandMaterialKey"),&var);
         key = var.lVal;
         m_StrandMaterial[pgsTypes::Harped] = pPool->GetStrand(key);

         hr = pStrLoad->get_Property(_T("TemporaryStrandMaterialKey"),&var);
         key = var.lVal;
         m_StrandMaterial[pgsTypes::Temporary] = pPool->GetStrand(key);
      }

      // before version 10, there was other data in this unit.
      // the parent object will load it and end the unit,
      // otherwise, end it here
      if ( 9 < version )
      {
         hr = pStrLoad->EndUnit(); // end PrestressData
      }
   }
   catch(...)
   {
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   if ( m_NumPermStrandsType == npsUser )
   {
      ProcessStrandRowData();
   }

   return hr;
}

HRESULT CStrandData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("PrestressData"),13.0);

   pStrSave->put_Property(_T("HsoEndMeasurement"), CComVariant(m_HsoEndMeasurement));
   pStrSave->put_Property(_T("HpOffsetAtEnd"), CComVariant(m_HpOffsetAtEnd));
   pStrSave->put_Property(_T("HsoHpMeasurement"), CComVariant(m_HsoHpMeasurement));
   pStrSave->put_Property(_T("HpOffsetAtHp"), CComVariant(m_HpOffsetAtHp));

   pStrSave->put_Property(_T("NumPermStrandsType"), CComVariant(m_NumPermStrandsType));

   if ( m_NumPermStrandsType == npsUser )
   {
      // added in version 13
      pStrSave->BeginUnit(_T("StrandRows"),1.0);
      pStrSave->put_Property(_T("Count"),CComVariant(m_StrandRows.size()));
      CStrandRowCollection::iterator iter(m_StrandRows.begin());
      CStrandRowCollection::iterator iterEnd(m_StrandRows.end());
      for ( ; iter != iterEnd; iter++ )
      {
         CStrandRow& strandRow(*iter);
         strandRow.Save(pStrSave,pProgress);
      }
      pStrSave->EndUnit();
   }
   else
   {
      // stopped write this out in version 13 if NumPerStrandType is npsUser
      pStrSave->put_Property(_T("NumHarpedStrands"), CComVariant(m_Nstrands[pgsTypes::Harped]));
      pStrSave->put_Property(_T("NumStraightStrands"), CComVariant(m_Nstrands[pgsTypes::Straight]));
      pStrSave->put_Property(_T("NumTempStrands"), CComVariant(m_Nstrands[pgsTypes::Temporary]));
      pStrSave->put_Property(_T("NumPermanentStrands"), CComVariant(m_Nstrands[pgsTypes::Permanent]));


      // added in version 12
      pStrSave->BeginUnit(_T("ExtendedStrands"),2.0); // storing grid index in version 2 (version 1 was strand index)
      pStrSave->BeginUnit(_T("Start"),1.0);
      pStrSave->BeginUnit(_T("Straight"),1.0);
      pStrSave->put_Property(_T("Count"),CComVariant(m_NextendedStrands[pgsTypes::Straight][pgsTypes::metStart].size()));
      std::vector<GridIndexType>::iterator iter(m_NextendedStrands[pgsTypes::Straight][pgsTypes::metStart].begin());
      std::vector<GridIndexType>::iterator end(m_NextendedStrands[pgsTypes::Straight][pgsTypes::metStart].end());
      for ( ; iter != end; iter++ )
      {
         pStrSave->put_Property(_T("Strand"),CComVariant(*iter));
      }
      pStrSave->EndUnit(); // Straight

      pStrSave->BeginUnit(_T("Harped"),1.0);
      pStrSave->put_Property(_T("Count"),CComVariant(m_NextendedStrands[pgsTypes::Harped][pgsTypes::metStart].size()));
      iter = m_NextendedStrands[pgsTypes::Harped][pgsTypes::metStart].begin();
      end  = m_NextendedStrands[pgsTypes::Harped][pgsTypes::metStart].end();
      for ( ; iter != end; iter++ )
      {
         pStrSave->put_Property(_T("Strand"),CComVariant(*iter));
      }
      pStrSave->EndUnit(); // Harped

      pStrSave->BeginUnit(_T("Temporary"),1.0);
      pStrSave->put_Property(_T("Count"),CComVariant(m_NextendedStrands[pgsTypes::Temporary][pgsTypes::metStart].size()));
      iter = m_NextendedStrands[pgsTypes::Temporary][pgsTypes::metStart].begin();
      end  = m_NextendedStrands[pgsTypes::Temporary][pgsTypes::metStart].end();
      for ( ; iter != end; iter++ )
      {
         pStrSave->put_Property(_T("Strand"),CComVariant(*iter));
      }
      pStrSave->EndUnit(); // Temporary
       pStrSave->EndUnit(); // Start

      pStrSave->BeginUnit(_T("End"),1.0);
      pStrSave->BeginUnit(_T("Straight"),1.0);
      pStrSave->put_Property(_T("Count"),CComVariant(m_NextendedStrands[pgsTypes::Straight][pgsTypes::metEnd].size()));
      iter = m_NextendedStrands[pgsTypes::Straight][pgsTypes::metEnd].begin();
      end  = m_NextendedStrands[pgsTypes::Straight][pgsTypes::metEnd].end();
      for ( ; iter != end; iter++ )
      {
         pStrSave->put_Property(_T("Strand"),CComVariant(*iter));
      }
      pStrSave->EndUnit(); // Straight

      pStrSave->BeginUnit(_T("Harped"),1.0);
      pStrSave->put_Property(_T("Count"),CComVariant(m_NextendedStrands[pgsTypes::Harped][pgsTypes::metEnd].size()));
      iter = m_NextendedStrands[pgsTypes::Harped][pgsTypes::metEnd].begin();
      end  = m_NextendedStrands[pgsTypes::Harped][pgsTypes::metEnd].end();
      for ( ; iter != end; iter++ )
      {
         pStrSave->put_Property(_T("Strand"),CComVariant(*iter));
      }
      pStrSave->EndUnit(); // Harped

      pStrSave->BeginUnit(_T("Temporary"),1.0);
      pStrSave->put_Property(_T("Count"),CComVariant(m_NextendedStrands[pgsTypes::Temporary][pgsTypes::metEnd].size()));
      iter = m_NextendedStrands[pgsTypes::Temporary][pgsTypes::metEnd].begin();
      end  = m_NextendedStrands[pgsTypes::Temporary][pgsTypes::metEnd].end();
      for ( ; iter != end; iter++ )
      {
         pStrSave->put_Property(_T("Strand"),CComVariant(*iter));
      }
      pStrSave->EndUnit(); // Temporary
      pStrSave->EndUnit(); // End
      pStrSave->EndUnit(); // ExtendedStrands

      if (m_NumPermStrandsType == CStrandData::npsDirectSelection)
      {
         // Added this in version 12
         pStrSave->BeginUnit(_T("DirectSelectStrandFill"), 1.0);

         pStrSave->BeginUnit(_T("StraightStrands"), 1.0);
         pStrSave->put_Property(_T("NumStraightFill"), CComVariant( StrandIndexType(m_StraightStrandFill.size())));
         for (CDirectStrandFillCollection::const_iterator its=m_StraightStrandFill.begin(); its!=m_StraightStrandFill.end(); its++)
         {
            its->Save(pStrSave);
         }
         pStrSave->EndUnit();

         pStrSave->BeginUnit(_T("HarpedStrands"), 1.0);
         pStrSave->put_Property(_T("NumHarpedFill"), CComVariant( StrandIndexType(m_HarpedStrandFill.size())));
         for (CDirectStrandFillCollection::const_iterator its=m_HarpedStrandFill.begin(); its!=m_HarpedStrandFill.end(); its++)
         {
            its->Save(pStrSave);
         }
         pStrSave->EndUnit();

         pStrSave->BeginUnit(_T("TemporaryStrands"), 1.0);
         pStrSave->put_Property(_T("NumTemporaryFill"), CComVariant( StrandIndexType(m_TemporaryStrandFill.size())));
         for (CDirectStrandFillCollection::const_iterator its=m_TemporaryStrandFill.begin(); its!=m_TemporaryStrandFill.end(); its++)
         {
            its->Save(pStrSave);
         }
         pStrSave->EndUnit();

         pStrSave->EndUnit(); // DirectSelectStrandFill
      }
   }

   pStrSave->put_Property(_T("PjHarped"), CComVariant(m_Pjack[pgsTypes::Harped]));
   pStrSave->put_Property(_T("PjStraight"), CComVariant(m_Pjack[pgsTypes::Straight]));
   pStrSave->put_Property(_T("PjTemp"), CComVariant(m_Pjack[pgsTypes::Temporary]));
   pStrSave->put_Property(_T("PjPermanent"), CComVariant(m_Pjack[pgsTypes::Permanent]));
   pStrSave->put_Property(_T("CalcPjHarped"), CComVariant(m_bPjackCalculated[pgsTypes::Harped]));
   pStrSave->put_Property(_T("CalcPjStraight"), CComVariant(m_bPjackCalculated[pgsTypes::Straight]));
   pStrSave->put_Property(_T("CalcPjTemp"), CComVariant(m_bPjackCalculated[pgsTypes::Temporary]));
   pStrSave->put_Property(_T("CalcPjPermanent"), CComVariant(m_bPjackCalculated[pgsTypes::Permanent]));
   pStrSave->put_Property(_T("LastUserPjHarped"), CComVariant(m_LastUserPjack[pgsTypes::Harped]));
   pStrSave->put_Property(_T("LastUserPjStraight"), CComVariant(m_LastUserPjack[pgsTypes::Straight]));
   pStrSave->put_Property(_T("LastUserPjTemp"), CComVariant(m_LastUserPjack[pgsTypes::Temporary]));
   pStrSave->put_Property(_T("LastUserPjPermanent"), CComVariant(m_LastUserPjack[pgsTypes::Permanent]));
   pStrSave->put_Property(_T("TempStrandUsage"),CComVariant(m_TempStrandUsage));

   if ( m_NumPermStrandsType != npsUser )
   {
      // not writing this data if NumPermStrandsType is npsUser... this was added in version 13 of the data block
      pStrSave->put_Property(_T("SymmetricDebond"),CComVariant(m_bSymmetricDebond));

      pStrSave->BeginUnit(_T("StraightStrandDebonding"),1.0);
      StrandIndexType nDebondInfo = m_Debond[pgsTypes::Straight].size();
      pStrSave->put_Property(_T("DebondInfoCount"),CComVariant(nDebondInfo));
      std::vector<CDebondData>::iterator debond_iter;
      for ( debond_iter = m_Debond[pgsTypes::Straight].begin(); debond_iter != m_Debond[pgsTypes::Straight].end(); debond_iter++ )
      {
         CDebondData& debond_info = *debond_iter;
         debond_info.Save(pStrSave,pProgress);
      }
      pStrSave->EndUnit(); // StraightStrandDebonding


      pStrSave->BeginUnit(_T("HarpedStrandDebonding"),1.0);
      nDebondInfo = m_Debond[pgsTypes::Harped].size();
      pStrSave->put_Property(_T("DebondInfoCount"),CComVariant(nDebondInfo));
      for ( debond_iter = m_Debond[pgsTypes::Harped].begin(); debond_iter != m_Debond[pgsTypes::Harped].end(); debond_iter++ )
      {
         CDebondData& debond_info = *debond_iter;
         debond_info.Save(pStrSave,pProgress);
      }
      pStrSave->EndUnit(); // HarpedStrandDebonding


      pStrSave->BeginUnit(_T("TemporaryStrandDebonding"),1.0);
      nDebondInfo = m_Debond[pgsTypes::Temporary].size();
      pStrSave->put_Property(_T("DebondInfoCount"),CComVariant(nDebondInfo));
      for ( debond_iter = m_Debond[pgsTypes::Temporary].begin(); debond_iter != m_Debond[pgsTypes::Temporary].end(); debond_iter++ )
      {
         CDebondData& debond_info = *debond_iter;
         debond_info.Save(pStrSave,pProgress);
      }
      pStrSave->EndUnit(); // TemporaryStrandDebonding
   }


   ///////////////// Added with data block version 11
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   Int32 key = pPool->GetStrandKey(m_StrandMaterial[pgsTypes::Straight]);
   pStrSave->put_Property(_T("StraightStrandMaterialKey"),CComVariant(key));
   
   key = pPool->GetStrandKey(m_StrandMaterial[pgsTypes::Harped]);
   pStrSave->put_Property(_T("HarpedStrandMaterialKey"),CComVariant(key));
   
   key = pPool->GetStrandKey(m_StrandMaterial[pgsTypes::Temporary]);
   pStrSave->put_Property(_T("TemporaryStrandMaterialKey"),CComVariant(key));

   pStrSave->EndUnit(); // PrestressData

   return hr;
}

void CStrandData::ClearExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType)
{
   m_NextendedStrands[strandType][endType].clear();
}

void CStrandData::ClearDebondData()
{
   for (long i=0; i<3; i++)
   {
      m_Debond[i].clear();
   }
}

void CStrandData::SetTotalPermanentNstrands(StrandIndexType nPermanent,StrandIndexType nStraight, StrandIndexType nHarped)
{
   ATLASSERT(nPermanent==nStraight+nHarped);

   if (m_NumPermStrandsType != CStrandData::npsTotal)
   {
      // Fill type change - clear and direct fill data
      ClearDirectFillData();

      m_NumPermStrandsType = CStrandData::npsTotal;
   }

   m_Nstrands[pgsTypes::Permanent] = nPermanent;
   m_Nstrands[pgsTypes::Straight]  = nStraight;
   m_Nstrands[pgsTypes::Harped]    = nHarped;
}

void CStrandData::SetHarpedStraightNstrands(StrandIndexType nStraight, StrandIndexType nHarped)
{
   if (m_NumPermStrandsType != CStrandData::npsStraightHarped)
   {
      // Fill type change - clear and direct fill data
      ClearDirectFillData();

      m_NumPermStrandsType = CStrandData::npsStraightHarped;
   }

   m_Nstrands[pgsTypes::Permanent] = nStraight+nHarped;
   m_Nstrands[pgsTypes::Straight]  = nStraight;
   m_Nstrands[pgsTypes::Harped]    = nHarped;
}

void CStrandData::SetTemporaryNstrands(StrandIndexType nStrands)
{
   ATLASSERT(m_NumPermStrandsType != CStrandData::npsDirectSelection); // if changing fill mode - don't change temp strands first

   m_Nstrands[pgsTypes::Temporary] = nStrands;
}

void CStrandData::SetDirectStrandFillStraight(const CDirectStrandFillCollection& rCollection)
{
   if(m_NumPermStrandsType != CStrandData::npsDirectSelection)
   {
      m_NumPermStrandsType = CStrandData::npsDirectSelection;
   }

   // make sure our collection is clean
   StrandIndexType ns = ProcessDirectFillData(rCollection, m_StraightStrandFill);

   // Update number of strands data
   m_Nstrands[pgsTypes::Straight]  = ns;
   m_Nstrands[pgsTypes::Permanent] = ns + m_Nstrands[pgsTypes::Harped];
}

const CDirectStrandFillCollection* CStrandData::GetDirectStrandFillStraight() const
{
   if(m_NumPermStrandsType == CStrandData::npsDirectSelection)
   {
      return &m_StraightStrandFill;
   }
   else
   {
      ATLASSERT(false);
      return NULL;
   }
}

void CStrandData::SetDirectStrandFillHarped(const CDirectStrandFillCollection& rCollection)
{
   if(m_NumPermStrandsType != CStrandData::npsDirectSelection)
   {
      m_NumPermStrandsType = CStrandData::npsDirectSelection;
   }

   // make sure our collection is clean
   StrandIndexType ns = ProcessDirectFillData(rCollection, m_HarpedStrandFill);

   // Update number of strands data
   m_Nstrands[pgsTypes::Harped]  = ns;
   m_Nstrands[pgsTypes::Permanent] = ns + m_Nstrands[pgsTypes::Straight];
}

const CDirectStrandFillCollection* CStrandData::GetDirectStrandFillHarped() const
{
   if(m_NumPermStrandsType == CStrandData::npsDirectSelection)
   {
      return &m_HarpedStrandFill;
   }
   else
   {
      ATLASSERT(false);
      return NULL;
   }
}

void CStrandData::SetDirectStrandFillTemporary(const CDirectStrandFillCollection& rCollection)
{
   if(m_NumPermStrandsType != CStrandData::npsDirectSelection)
   {
      m_NumPermStrandsType = CStrandData::npsDirectSelection;
   }

   // make sure our collection is clean
   StrandIndexType ns = ProcessDirectFillData(rCollection, m_TemporaryStrandFill);

   // Update number of strands data
   m_Nstrands[pgsTypes::Temporary]  = ns;
}

const CDirectStrandFillCollection* CStrandData::GetDirectStrandFillTemporary() const
{
   if(m_NumPermStrandsType == CStrandData::npsDirectSelection)
   {
      return &m_TemporaryStrandFill;
   }
   else
   {
      ATLASSERT(false);
      return NULL;
   }
}

void CStrandData::AddStrandRow(const CStrandRow& strandRow)
{
   m_StrandRows.push_back(strandRow);
   ProcessStrandRowData();
}

void CStrandData::AddStrandRows(const CStrandRowCollection& strandRows)
{
   m_StrandRows.insert(m_StrandRows.end(),strandRows.begin(),strandRows.end());
   ProcessStrandRowData();
}

void CStrandData::SetStrandRows(const CStrandRowCollection& strandRows)
{
   m_StrandRows = strandRows;
   ProcessStrandRowData();
}

const CStrandRowCollection& CStrandData::GetStrandRows() const
{
   return m_StrandRows;
}

void CStrandData::SetStrandCount(pgsTypes::StrandType strandType,StrandIndexType nStrands)
{
   ATLASSERT( m_NumPermStrandsType != npsUser);
   m_Nstrands[strandType] = nStrands;
}

StrandIndexType CStrandData::GetStrandCount(pgsTypes::StrandType strandType) const
{
   return m_Nstrands[strandType];
}

void CStrandData::AddExtendedStrand(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType,GridIndexType gridIdx)
{
   m_NextendedStrands[strandType][endType].push_back(gridIdx);
   std::sort(m_NextendedStrands[strandType][endType].begin(),m_NextendedStrands[strandType][endType].end());
}

const std::vector<StrandIndexType>& CStrandData::GetExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType) const
{
   return m_NextendedStrands[strandType][endType];
}

void CStrandData::SetExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType,const std::vector<StrandIndexType>& extStrands)
{
   m_NextendedStrands[strandType][endType] = extStrands;
}

// Resets all the prestressing put to default values
void CStrandData::ResetPrestressData()
{
   for ( Uint16 i = 0; i < 4; i++ )
   {
      m_Nstrands[i] = 0;
      m_Pjack[i] = 0;
      m_bPjackCalculated[i] = true;
      m_LastUserPjack[i] = 0;

      if (i<3)
      {
         m_Debond[i].clear();

         m_NextendedStrands[i][pgsTypes::metStart].clear();
         m_NextendedStrands[i][pgsTypes::metEnd].clear();
      }
   }

   ClearDirectFillData();

   m_TempStrandUsage    = pgsTypes::ttsPretensioned;
   m_bSymmetricDebond   = true;

   // convert to legacy since this is the only measurement that is sure to fit in the girder
   m_HsoEndMeasurement  = hsoLEGACY;
   m_HpOffsetAtEnd      = 0.0;
   m_HsoHpMeasurement   = hsoLEGACY;
   m_HpOffsetAtHp       = 0.0;
}

void CStrandData::ClearDirectFillData()
{
   m_StraightStrandFill.clear();
   m_HarpedStrandFill.clear();
   m_TemporaryStrandFill.clear();
}

StrandIndexType CStrandData::GetDebondCount(pgsTypes::StrandType strandType,const GirderLibraryEntry* pGirderLibEntry) const
{
   if (strandType==pgsTypes::Permanent)
   {
      ATLASSERT(false); // should never be called
      return 0;
   }

   StrandIndexType nDebondedStrands = 0;
   if ( m_NumPermStrandsType == npsUser )
   {
      CStrandRowCollection::const_iterator rowIter(m_StrandRows.begin());
      CStrandRowCollection::const_iterator rowIterEnd(m_StrandRows.end());
      for ( ; rowIter != rowIterEnd; rowIter++ )
      {
         const CStrandRow& row = *rowIter;
         if ( row.m_bIsDebonded )
         {
            nDebondedStrands += row.m_nStrands;
         }
      }
   }
   else
   {
      ATLASSERT( m_NumPermStrandsType == npsTotal || m_NumPermStrandsType == npsStraightHarped || m_NumPermStrandsType == npsDirectSelection );

      std::vector<CDebondData>::const_iterator iter(m_Debond[strandType].begin());
      std::vector<CDebondData>::const_iterator end(m_Debond[strandType].end());
      for ( ; iter != end; iter++ )
      {
         const CDebondData& debond_info = *iter;
         if (debond_info.strandTypeGridIdx != INVALID_INDEX )
         {
            if ( strandType == pgsTypes::Straight )
            {
               Float64 Xstart, Ystart, Xend, Yend;
               bool bCanDebond;
               pGirderLibEntry->GetStraightStrandCoordinates(debond_info.strandTypeGridIdx,&Xstart,&Ystart,&Xend,&Yend,&bCanDebond);
               ATLASSERT(bCanDebond == true);
               if ( IsZero(Xstart) )
               {
                  nDebondedStrands += 2;
               }
               else
               {
                  nDebondedStrands++;
               }
            }
            else
            {
               ATLASSERT(false); // only straight strands are supported
            }
         }
      }
   }

   return nDebondedStrands;
}

const std::vector<CDebondData>& CStrandData::GetDebonding(pgsTypes::StrandType strandType) const
{
   ATLASSERT(strandType != pgsTypes::Permanent);
   return m_Debond[strandType];
}

void CStrandData::SetDebonding(pgsTypes::StrandType strandType,const std::vector<CDebondData>& vDebond)
{
   ATLASSERT(strandType != pgsTypes::Permanent);
   m_Debond[strandType] = vDebond;
}

std::vector<CDebondData>& CStrandData::GetDebonding(pgsTypes::StrandType strandType)
{
   ATLASSERT(strandType != pgsTypes::Permanent);
   return m_Debond[strandType];
}

bool CStrandData::IsSymmetricDebond() const
{
   return m_bSymmetricDebond;
}

void CStrandData::IsSymmetricDebond(bool bIsSymmetric)
{
   m_bSymmetricDebond = bIsSymmetric;
}

void CStrandData::SetStrandMaterial(pgsTypes::StrandType strandType,const matPsStrand* pStrandMaterial)
{
   ATLASSERT(strandType != pgsTypes::Permanent);
   m_StrandMaterial[strandType] = pStrandMaterial;
}

const matPsStrand* CStrandData::GetStrandMaterial(pgsTypes::StrandType strandType) const
{
   ATLASSERT(strandType != pgsTypes::Permanent);
   return m_StrandMaterial[strandType];
}

void CStrandData::IsPjackCalculated(pgsTypes::StrandType strandType,bool bIsCalculated)
{
   m_bPjackCalculated[strandType] = bIsCalculated;
}

bool CStrandData::IsPjackCalculated(pgsTypes::StrandType strandType) const
{
   return m_bPjackCalculated[strandType];
}

void CStrandData::SetPjack(pgsTypes::StrandType strandType,Float64 Pjack)
{
   m_Pjack[strandType] = Pjack;
}

Float64 CStrandData::GetPjack(pgsTypes::StrandType strandType) const
{
   return m_Pjack[strandType];
}

void CStrandData::SetLastUserPjack(pgsTypes::StrandType strandType,Float64 Pjack)
{
   m_LastUserPjack[strandType] = Pjack;
}

Float64 CStrandData::GetLastUserPjack(pgsTypes::StrandType strandType) const
{
   return m_LastUserPjack[strandType];
}

void CStrandData::SetHarpStrandOffsetMeasurementAtEnd(HarpedStrandOffsetType offsetType)
{
   m_HsoEndMeasurement = offsetType;
}

HarpedStrandOffsetType CStrandData::GetHarpStrandOffsetMeasurementAtEnd() const
{
   return m_HsoEndMeasurement;
}

void CStrandData::SetHarpStrandOffsetAtEnd(Float64 offset)
{
   m_HpOffsetAtEnd = offset;
}

Float64 CStrandData::GetHarpStrandOffsetAtEnd() const
{
   return m_HpOffsetAtEnd;
}

void CStrandData::SetHarpStrandOffsetMeasurementAtHarpPoint(HarpedStrandOffsetType offsetType)
{
   m_HsoHpMeasurement = offsetType;
}

HarpedStrandOffsetType CStrandData::GetHarpStrandOffsetMeasurementAtHarpPoint() const
{
   return m_HsoHpMeasurement;
}

void CStrandData::SetHarpStrandOffsetAtHarpPoint(Float64 offset)
{
   m_HpOffsetAtHp = offset;
}

Float64 CStrandData::GetHarpStrandOffsetAtHarpPoint() const
{
   return m_HpOffsetAtHp;
}

void CStrandData::SetTemporaryStrandUsage(pgsTypes::TTSUsage ttsUsage)
{
   m_TempStrandUsage = ttsUsage;
}

pgsTypes::TTSUsage CStrandData::GetTemporaryStrandUsage() const
{
   return m_TempStrandUsage;
}

void CStrandData::SetStrandDefinitionType(CStrandData::PermanentStrandType permStrandsType)
{
   m_NumPermStrandsType = permStrandsType;
}

CStrandData::PermanentStrandType CStrandData::GetStrandDefinitionType() const
{
   return m_NumPermStrandsType;
}

////////////////////////// PROTECTED  ///////////////////////////////////////

void CStrandData::MakeCopy(const CStrandData& rOther)
{
   m_StrandMaterial[pgsTypes::Straight]  = rOther.m_StrandMaterial[pgsTypes::Straight];
   m_StrandMaterial[pgsTypes::Harped]    = rOther.m_StrandMaterial[pgsTypes::Harped];
   m_StrandMaterial[pgsTypes::Temporary] = rOther.m_StrandMaterial[pgsTypes::Temporary];

   m_HsoEndMeasurement  = rOther.m_HsoEndMeasurement;
   m_HsoHpMeasurement   = rOther.m_HsoHpMeasurement;
   m_HpOffsetAtEnd      = rOther.m_HpOffsetAtEnd;
   m_HpOffsetAtHp       = rOther.m_HpOffsetAtHp;

   m_NumPermStrandsType = rOther.m_NumPermStrandsType;

   m_StraightStrandFill    = rOther.m_StraightStrandFill;
   m_HarpedStrandFill      = rOther.m_HarpedStrandFill;
   m_TemporaryStrandFill   = rOther.m_TemporaryStrandFill;

   for ( Uint16 i = 0; i < 4; i++ )
   {
      m_Nstrands[i]         = rOther.m_Nstrands[i];
      m_Pjack[i]            = rOther.m_Pjack[i];
      m_bPjackCalculated[i] = rOther.m_bPjackCalculated[i];
      m_LastUserPjack[i]    = rOther.m_LastUserPjack[i];

      if (i<3)
      {
         m_Debond[i]           = rOther.m_Debond[i];
         m_NextendedStrands[i][pgsTypes::metStart]  = rOther.m_NextendedStrands[i][pgsTypes::metStart];
         m_NextendedStrands[i][pgsTypes::metEnd]    = rOther.m_NextendedStrands[i][pgsTypes::metEnd];
      }
   }

   m_TempStrandUsage    = rOther.m_TempStrandUsage;
   m_bSymmetricDebond   = rOther.m_bSymmetricDebond;

   m_bConvertExtendedStrands = rOther.m_bConvertExtendedStrands;

   m_StrandRows = rOther.m_StrandRows;
}

void CStrandData::MakeAssignment(const CStrandData& rOther)
{
   MakeCopy( rOther );
}

StrandIndexType CStrandData::ProcessDirectFillData(const CDirectStrandFillCollection& rInCollection, CDirectStrandFillCollection& rLocalCollection)
{
   // Clear out old data and make room for new. Dont put any invalid data into collection
   rLocalCollection.clear();
   rLocalCollection.reserve(rInCollection.size());

   StrandIndexType ns(0);
   CDirectStrandFillCollection::const_iterator iter(rInCollection.begin());
   CDirectStrandFillCollection::const_iterator iterEnd(rInCollection.end());
   for ( ; iter != iterEnd; iter++ )
   {
      ATLASSERT(iter->permStrandGridIdx!=INVALID_INDEX);

      StrandIndexType n = iter->numFilled;
      if (n==1 || n==2)
      {
         rLocalCollection.AddFill(*iter);

         ns += n;
      }
      else
      {
         ATLASSERT(false); // is there a new valid fill value?
      }
   }

   return ns;
}

void CStrandData::ProcessStrandRowData()
{
   StrandIndexType nStrands[3] = {0,0,0};

   GridIndexType rowStartGridIdx = 0; // strand strand grid index for the first grid point in the current strand row

   CStrandRowCollection::iterator iter(m_StrandRows.begin());
   CStrandRowCollection::iterator iterEnd(m_StrandRows.end());
   for ( ; iter != iterEnd; iter++ )
   {
      CStrandRow& strandRow(*iter);

      GridIndexType nRowGridPoints = strandRow.m_nStrands/2; // # grid points in this row

      for ( GridIndexType rowGridIdx = 0; rowGridIdx < nRowGridPoints; rowGridIdx++ )
      {
         GridIndexType gridIdx = rowStartGridIdx + rowGridIdx; // global grid point index

         // extended strands
         for ( int i = 0; i < 2; i++ )
         {
            pgsTypes::MemberEndType end = (pgsTypes::MemberEndType)(i);
            if ( strandRow.m_bIsExtendedStrand[end] )
            {
               ATLASSERT(strandRow.m_StrandType != pgsTypes::Temporary); // extended strands can't be temporary
               ATLASSERT(!strandRow.m_bIsDebonded[end]); // extended strands can't be debonded

               m_NextendedStrands[pgsTypes::Straight][end].push_back(gridIdx);
            } // end if strand extension
         }

         // debonded strands
         if ( strandRow.m_bIsDebonded[pgsTypes::metStart] || strandRow.m_bIsDebonded[pgsTypes::metEnd] )
         {
            ATLASSERT(strandRow.m_StrandType != pgsTypes::Temporary); // temporary strands are not debonded

            CDebondData debondData;
            debondData.needsConversion = false;
            debondData.strandTypeGridIdx = gridIdx;

            for ( int i = 0; i < 2; i++ )
            {
               pgsTypes::MemberEndType end = (pgsTypes::MemberEndType)(i);
               if ( strandRow.m_bIsDebonded[end] )
               {
                  ATLASSERT(!strandRow.m_bIsExtendedStrand[end] ); // debonded strands can't be extended
                  debondData.Length[end] = strandRow.m_DebondLength[end];
               }
               else
               {
                  debondData.Length[end] = 0;
               }
            }

            m_Debond[pgsTypes::Straight].push_back(debondData);
         } // end if debond
      } // next grid point


      rowStartGridIdx += nRowGridPoints;
      nStrands[strandRow.m_StrandType] += strandRow.m_nStrands;

   } // next strand row

   m_Nstrands[pgsTypes::Straight]  = nStrands[pgsTypes::Straight];
   m_Nstrands[pgsTypes::Harped]    = nStrands[pgsTypes::Harped];
   m_Nstrands[pgsTypes::Temporary] = nStrands[pgsTypes::Temporary];
   m_Nstrands[pgsTypes::Permanent] = 0;

   m_bSymmetricDebond = false;
}

