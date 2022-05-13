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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\StrandData.h>
#include <Lrfd\StrandPool.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <GenericBridge\Helpers.h>

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

void CDirectStrandFillCollection::RemoveFill(GridIndexType idxGrid) 
{
   std::vector<CDirectStrandFillInfo>::iterator it = m_StrandFill.begin();
   std::vector<CDirectStrandFillInfo>::iterator itend = m_StrandFill.end();
   while(it!=itend)
   {
      if (it->permStrandGridIdx == idxGrid)
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
std::array<std::vector<Float64>, 4> CStrandRow::ms_oldHarpPoints;

CStrandRow::CStrandRow()
{
   m_Z = WBFL::Units::ConvertToSysUnits(2.0,WBFL::Units::Measure::Inch);
   m_Spacing = m_Z;
   m_StrandType = pgsTypes::Straight;
   m_nStrands = 0;

   for ( int i = 0; i < 4; i++ )
   {
      m_Y[i]    = WBFL::Units::ConvertToSysUnits(2.0,WBFL::Units::Measure::Inch);
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
   if ( !IsEqual(m_Z,other.m_Z) )
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

bool CStrandRow::operator<(const CStrandRow& other) const
{
   return (m_StrandType < other.m_StrandType ? true : false);
}

HRESULT CStrandRow::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("StrandRow"),4.0);

   pStrSave->put_Property(_T("InnerSpacing"),CComVariant(m_Z));
   pStrSave->put_Property(_T("Spacing"),CComVariant(m_Spacing));
   
   // added in version 2
   LPCTSTR strStrandType[3]={_T("Straight"),_T("Harped"),_T("Temporary")};
   pStrSave->put_Property(_T("Type"),CComVariant(strStrandType[m_StrandType]));

   pStrSave->put_Property(_T("NStrands"),CComVariant(m_nStrands));

   //pStrSave->put_Property(_T("XStart"),CComVariant(m_X[ZoneBreakType::Start])); // added in version 3 (removed in version 4)
   pStrSave->put_Property(_T("YStart"),CComVariant(m_Y[ZoneBreakType::Start]));
   pStrSave->put_Property(_T("FaceStart"),CComVariant(m_Face[ZoneBreakType::Start] == pgsTypes::TopFace ? _T("Top") : _T("Bottom")));

   // added in version 2
   //pStrSave->put_Property(_T("XLHP"),CComVariant(m_X[ZoneBreakType::LeftBreak])); (removed in version 4)
   pStrSave->put_Property(_T("YLHP"),CComVariant(m_Y[ZoneBreakType::LeftBreak]));
   pStrSave->put_Property(_T("FaceLHP"),CComVariant(m_Face[ZoneBreakType::LeftBreak] == pgsTypes::TopFace ? _T("Top") : _T("Bottom")));

   // added in version 2
   // pStrSave->put_Property(_T("XRHP"),CComVariant(m_X[ZoneBreakType::RightBreak])); (removed in version 4)
   pStrSave->put_Property(_T("YRHP"),CComVariant(m_Y[ZoneBreakType::RightBreak]));
   pStrSave->put_Property(_T("FaceRHP"),CComVariant(m_Face[ZoneBreakType::RightBreak] == pgsTypes::TopFace ? _T("Top") : _T("Bottom")));

   // pStrSave->put_Property(_T("XEnd"),CComVariant(m_X[ZoneBreakType::End])); // added in version 3 (removed in vesrion 4)
   pStrSave->put_Property(_T("YEnd"),CComVariant(m_Y[ZoneBreakType::End]));
   pStrSave->put_Property(_T("FaceEnd"),CComVariant(m_Face[ZoneBreakType::End] == pgsTypes::TopFace ? _T("Top") : _T("Bottom")));

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
      m_Z = var.dblVal;

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

      if ( 2 < version && version < 4 )
      {
         // added in version 3
         // removed in version 4
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("XStart"),&var);
         CStrandRow::ms_oldHarpPoints[ZoneBreakType::Start].push_back(var.dblVal); // retain old value for processing later
      }

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("YStart"),&var);
      m_Y[ZoneBreakType::Start] = var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("FaceStart"),&var);
      m_Face[ZoneBreakType::Start] = (CComBSTR(var.bstrVal) == CComBSTR(_T("Top")) ? pgsTypes::TopFace : pgsTypes::BottomFace);

      // added in version 2
      if (1 < version)
      {
         var.vt = VT_R8;

         if (version < 4)
         {
            // removed in version 4
            pStrLoad->get_Property(_T("XLHP"), &var);
            CStrandRow::ms_oldHarpPoints[ZoneBreakType::LeftBreak].push_back(var.dblVal); // retain old value for processing later
         }

         pStrLoad->get_Property(_T("YLHP"), &var);
         m_Y[ZoneBreakType::LeftBreak] = var.dblVal;

         var.vt = VT_BSTR;
         pStrLoad->get_Property(_T("FaceLHP"), &var);
         m_Face[ZoneBreakType::LeftBreak] = (CComBSTR(var.bstrVal) == CComBSTR(_T("Top")) ? pgsTypes::TopFace : pgsTypes::BottomFace);

         var.vt = VT_R8;

         if (version < 4)
         {
            // removed in version 4
            pStrLoad->get_Property(_T("XRHP"), &var);
            CStrandRow::ms_oldHarpPoints[ZoneBreakType::RightBreak].push_back(var.dblVal); // retain old value for processing later
         }

         pStrLoad->get_Property(_T("YRHP"),&var);
         m_Y[ZoneBreakType::RightBreak] = var.dblVal;

         var.vt = VT_BSTR;
         pStrLoad->get_Property(_T("FaceRHP"),&var);
         m_Face[ZoneBreakType::RightBreak] = (CComBSTR(var.bstrVal) == CComBSTR(_T("Top")) ? pgsTypes::TopFace : pgsTypes::BottomFace);
      }


      if ( 2 < version && version < 4)
      {
         // added in version 3
         // removed in version 4
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("XEnd"),&var);
         CStrandRow::ms_oldHarpPoints[ZoneBreakType::End].push_back(var.dblVal); // retain old value for processing later
      }

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("YEnd"),&var);
      m_Y[ZoneBreakType::End] = var.dblVal;

      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("FaceEnd"),&var);
      m_Face[ZoneBreakType::End] = (CComBSTR(var.bstrVal) == CComBSTR(_T("Top")) ? pgsTypes::TopFace : pgsTypes::BottomFace);

      // removed in version 2
      if ( version < 2 )
      {
         var.vt = VT_BOOL;
         hr = pStrLoad->get_Property(_T("IsTemporaryStrand"),&var);
         m_StrandType = (var.boolVal == VARIANT_TRUE ? pgsTypes::Temporary : pgsTypes::Straight);
      }

      var.vt = VT_BOOL;
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

      // if the debond length is zero, the strand isn't actually debonded... force the debond setting to false
      if (m_bIsDebonded[pgsTypes::metStart] && IsZero(m_DebondLength[pgsTypes::metStart]))
         m_bIsDebonded[pgsTypes::metStart] = false;

      if (m_bIsDebonded[pgsTypes::metEnd] && IsZero(m_DebondLength[pgsTypes::metEnd]))
         m_bIsDebonded[pgsTypes::metEnd] = false;

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
      m_StrandMaterial[i] = lrfdStrandPool::GetInstance()->GetStrand(matPsStrand::Gr1860,matPsStrand::LowRelaxation,matPsStrand::None,matPsStrand::D1524);
   }

   m_HarpPoint[ZoneBreakType::Start]    =  0.0; // 0% length = left end
   m_HarpPoint[ZoneBreakType::LeftBreak]  = -0.4; // 40% length
   m_HarpPoint[ZoneBreakType::RightBreak] = -0.6; // 60% length
   m_HarpPoint[ZoneBreakType::End]      = -1.0; // 100% length = right end

   ResetPrestressData();

   m_NumPermStrandsType = pgsTypes::sdtStraightHarped;
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

   if ( IsDirectStrandModel(m_NumPermStrandsType))
   {
      for (Uint16 i = 0; i < 4; i++)
      {
         if (!IsEqual(m_HarpPoint[i], rOther.m_HarpPoint[i]))
         {
            return false;
         }
      }

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
   else if ( m_NumPermStrandsType == pgsTypes::sdtDirectSelection )
   {
      if ( m_StraightStrandFill != rOther.m_StraightStrandFill )
      {
         return false;
      }

      if ( m_HarpedStrandFill != rOther.m_HarpedStrandFill )
      {
         return false;
      }

      if ( m_TemporaryStrandFill != rOther.m_TemporaryStrandFill )
      {
         return false;
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

   for ( int i = 0; i < 2; i++ )
   {
      if ( m_HpOffsetAtEnd[i] != rOther.m_HpOffsetAtEnd[i] )
      {
         return false;
      }

      if ( m_HpOffsetAtHp[i] != rOther.m_HpOffsetAtHp[i] )
      {
         return false;
      }
   }

   if ( m_TempStrandUsage != rOther.m_TempStrandUsage )
   {
      return false;
   }

   if ( m_AdjustableStrandType != rOther.m_AdjustableStrandType )
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

      if ( version < 16 )
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("HpOffsetAtEnd"), &var );
         m_HpOffsetAtEnd[pgsTypes::metStart] = var.dblVal;
         m_HpOffsetAtEnd[pgsTypes::metEnd]   = var.dblVal;
      }
      else
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("HpOffsetAtStart"), &var );
         m_HpOffsetAtEnd[pgsTypes::metStart] = var.dblVal;
         hr = pStrLoad->get_Property(_T("HpOffsetAtEnd"), &var );
         m_HpOffsetAtEnd[pgsTypes::metEnd]   = var.dblVal;
      }

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

      if ( version < 16 )
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("HpOffsetAtHp"), &var );
         m_HpOffsetAtHp[pgsTypes::metStart] = var.dblVal;
         m_HpOffsetAtHp[pgsTypes::metEnd]   = var.dblVal;
      }
      else
      {
         var.Clear();
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("HpOffsetAtHp1"), &var );
         m_HpOffsetAtHp[pgsTypes::metStart] = var.dblVal;
         hr = pStrLoad->get_Property(_T("HpOffsetAtHp2"), &var );
         m_HpOffsetAtHp[pgsTypes::metEnd]   = var.dblVal;
      }

      if (version<8.0)
      {
         m_NumPermStrandsType = pgsTypes::sdtStraightHarped;
      }
      else
      {
         var.Clear();
         var.vt = VT_I4;
         hr = pStrLoad->get_Property(_T("NumPermStrandsType"), &var );
         m_NumPermStrandsType = (pgsTypes::StrandDefinitionType)var.lVal;
      }

      if (IsDirectStrandModel(m_NumPermStrandsType))
      {
         // added in version 13 for sdtDirectRowInput and version 17 for sdtDirectStrandInput
         hr = pStrLoad->BeginUnit(_T("StrandRows"));
         var.Clear();

         Float64 strand_row_version;
         pStrLoad->get_Version(&strand_row_version);
         if (1 < strand_row_version)
         {
            // added in version 2.0 of StrandRows
            var.vt = VT_R8;
            hr = pStrLoad->BeginUnit(_T("HarpPoints"));
            hr = pStrLoad->get_Property(_T("Start"), &var);
            m_HarpPoint[ZoneBreakType::Start] = var.dblVal;

            hr = pStrLoad->get_Property(_T("LeftHP"), &var);
            m_HarpPoint[ZoneBreakType::LeftBreak] = var.dblVal;

            hr = pStrLoad->get_Property(_T("RightHP"), &var);
            m_HarpPoint[ZoneBreakType::RightBreak] = var.dblVal;

            hr = pStrLoad->get_Property(_T("End"), &var);
            m_HarpPoint[ZoneBreakType::End] = var.dblVal;

            hr = pStrLoad->EndUnit(); // HarpPoints
         }

         var.vt = VT_INDEX;
         hr = pStrLoad->get_Property(_T("Count"),&var);
         IndexType nRows = VARIANT2INDEX(var);
         for ( RowIndexType rowIdx = 0; rowIdx < nRows; rowIdx++ )
         {
            CStrandRow strandRow;
            hr = strandRow.Load(pStrLoad,pProgress);
            m_StrandRows.push_back(strandRow);
         }
         hr = pStrLoad->EndUnit(); // StrandRows
         std::sort(m_StrandRows.begin(),m_StrandRows.end());

         // Before version 4 of CStrandRow data we each individual strand row could
         // define its own harp point locations. This gave us the possibility of many, many
         // harp points. In version 4, we moved a single harp point definition to CStrandData.
         // To deal with older files, we capture the old harp point definition from each strand
         // row in a static data member of CStrandRow. Use the old data, if present, to
         // define the harp point locations
         for (int i = 0; i < 4; i++)
         {
            if (0 < CStrandRow::ms_oldHarpPoints[i].size())
            {
               // for lack of anything better/easier, just use the first value
               m_HarpPoint[i] = CStrandRow::ms_oldHarpPoints[i].front();
            }
         }
      }
      else
      {
         // stopped write this out in version 13 if NumPerStrandType is sdtDirectRowInput
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

            // Was a bug in the design algorithm pre-2.9 that allowed extended strand data to creep in if there were no 
            // strands. Kill this data if it exists
            if( m_Nstrands[pgsTypes::Straight] == 0)
            {
               m_NextendedStrands[pgsTypes::Straight][pgsTypes::metStart].clear();
               m_NextendedStrands[pgsTypes::Straight][pgsTypes::metEnd].clear();
            }
         }

         if (12 <= version && m_NumPermStrandsType == pgsTypes::sdtDirectSelection)
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


      if (14.0 <= version)
      {
         var.Clear();
         var.vt = VT_I4;
         pStrLoad->get_Property(_T("AdjustableStrandType"),&var);
         m_AdjustableStrandType = (pgsTypes::AdjustableStrandType)var.lVal;
      }
      else
      {
         // At this point we can assume that very old versions are harped. Howeve, very recent versions were set
         // in the library, but we can't know that at this time. So set to harped and fix the descrepancy 
         // later when resolving library conflicts
         m_AdjustableStrandType = pgsTypes::asHarped;
      }

      if ( 5.0 <= version && !IsDirectStrandModel(m_NumPermStrandsType))
      {
         // not writing this data if NumPermStrandsType is sdtDirectRowInput... this was added in version 13 of the data block
         // not writing this data if NumPermStrandsType is sdtDirectStrandInput... this was added in version 17 of the data block

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
            if(0 < debond_info.Length[pgsTypes::metStart] || 0 < debond_info.Length[pgsTypes::metEnd])
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
            if (0 < debond_info.Length[pgsTypes::metStart] || 0 < debond_info.Length[pgsTypes::metEnd])
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
            if (0 < debond_info.Length[pgsTypes::metStart] || 0 < debond_info.Length[pgsTypes::metEnd])
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
         Int64 key = var.lVal;
         if ( version < 15 )
         {
            key |= matPsStrand::None; // add default encoding for stand coating type... added in version 15
         }
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
         Int64 key = var.lVal;
         if ( version < 15 )
         {
            key |= matPsStrand::None; // add default encoding for stand coating type... added in version 15
         }
         m_StrandMaterial[pgsTypes::Straight] = pPool->GetStrand(key);

         hr = pStrLoad->get_Property(_T("HarpedStrandMaterialKey"),&var);
         key = var.lVal;
         if ( version < 15 )
         {
            key |= matPsStrand::None; // add default encoding for stand coating type... added in version 15
         }
         m_StrandMaterial[pgsTypes::Harped] = pPool->GetStrand(key);

         hr = pStrLoad->get_Property(_T("TemporaryStrandMaterialKey"),&var);
         key = var.lVal;
         if ( version < 15 )
         {
            key |= matPsStrand::None; // add default encoding for stand coating type... added in version 15
         }
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

   if ( IsDirectStrandModel(m_NumPermStrandsType))
   {
      ProcessStrandRowData();
   }

   return hr;
}

HRESULT CStrandData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("PrestressData"),17.0);

   pStrSave->put_Property(_T("HsoEndMeasurement"), CComVariant(m_HsoEndMeasurement));
   pStrSave->put_Property(_T("HpOffsetAtStart"), CComVariant(m_HpOffsetAtEnd[pgsTypes::metStart])); // added in version 16
   pStrSave->put_Property(_T("HpOffsetAtEnd"), CComVariant(m_HpOffsetAtEnd[pgsTypes::metEnd]));
   pStrSave->put_Property(_T("HsoHpMeasurement"), CComVariant(m_HsoHpMeasurement));
   pStrSave->put_Property(_T("HpOffsetAtHp1"), CComVariant(m_HpOffsetAtHp[pgsTypes::metStart])); // changed in version 16
   pStrSave->put_Property(_T("HpOffsetAtHp2"), CComVariant(m_HpOffsetAtHp[pgsTypes::metEnd])); // added in version 16

   pStrSave->put_Property(_T("NumPermStrandsType"), CComVariant(m_NumPermStrandsType));

   if ( IsDirectStrandModel(m_NumPermStrandsType))
   {
      // added in version 13 for sdtDirectRowInput and version 17 for sdtDirectStrandInput
      pStrSave->BeginUnit(_T("StrandRows"),2.0);
      
      pStrSave->BeginUnit(_T("HarpPoints"), 1.0); // added in version 2 of StrandRows
      pStrSave->put_Property(_T("Start"), CComVariant(m_HarpPoint[ZoneBreakType::Start]));
      pStrSave->put_Property(_T("LeftHP"), CComVariant(m_HarpPoint[ZoneBreakType::LeftBreak]));
      pStrSave->put_Property(_T("RightHP"), CComVariant(m_HarpPoint[ZoneBreakType::RightBreak]));
      pStrSave->put_Property(_T("End"), CComVariant(m_HarpPoint[ZoneBreakType::End]));
      pStrSave->EndUnit(); // HarpPoints

      pStrSave->put_Property(_T("Count"),CComVariant(m_StrandRows.size()));
      CStrandRowCollection::iterator iter(m_StrandRows.begin());
      CStrandRowCollection::iterator iterEnd(m_StrandRows.end());
      for ( ; iter != iterEnd; iter++ )
      {
         CStrandRow& strandRow(*iter);
         strandRow.Save(pStrSave,pProgress);
      }
      pStrSave->EndUnit(); // StrandRows
   }
   else
   {
      // stopped writing this out in version 13 if NumPerStrandType is sdtDirectRowInput
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

      if (m_NumPermStrandsType == pgsTypes::sdtDirectSelection)
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
   pStrSave->put_Property(_T("AdjustableStrandType"),CComVariant(m_AdjustableStrandType)); // added version 14.

   if ( IsGridBasedStrandModel(m_NumPermStrandsType))
   {
      // not writing this data if NumPermStrandsType is sdtDirectRowInput... this was added in version 13 of the data block
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
   // version 15... strand pool key began including a value for strand coating type
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   Int64 key = pPool->GetStrandKey(m_StrandMaterial[pgsTypes::Straight]);
   pStrSave->put_Property(_T("StraightStrandMaterialKey"),CComVariant(key));
   
   key = pPool->GetStrandKey(m_StrandMaterial[pgsTypes::Harped]);
   pStrSave->put_Property(_T("HarpedStrandMaterialKey"),CComVariant(key));
   
   key = pPool->GetStrandKey(m_StrandMaterial[pgsTypes::Temporary]);
   pStrSave->put_Property(_T("TemporaryStrandMaterialKey"),CComVariant(key));

   pStrSave->EndUnit(); // PrestressData

   return hr;
}

void CStrandData::ClearExtendedStrands()
{
   for (int i = 0; i < 3; i++)
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      for (int j = 0; j < 2; j++)
      {
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)j;
         m_NextendedStrands[strandType][endType].clear();
      }
   }
}

void CStrandData::ClearExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType)
{
   m_NextendedStrands[strandType][endType].clear();
}

void CStrandData::ClearDebondData()
{
   for (int i = 0; i < 3; i++)
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      m_Debond[strandType].clear();
   }
}

void CStrandData::SetTotalPermanentNstrands(StrandIndexType nPermanent,StrandIndexType nStraight, StrandIndexType nHarped)
{
   ATLASSERT(nPermanent==nStraight+nHarped);

   if (m_NumPermStrandsType != pgsTypes::sdtTotal)
   {
      // Fill type change - clear and direct fill data
      ClearDirectFillData();

      m_NumPermStrandsType = pgsTypes::sdtTotal;
   }

   m_Nstrands[pgsTypes::Permanent] = nPermanent;
   m_Nstrands[pgsTypes::Straight]  = nStraight;
   m_Nstrands[pgsTypes::Harped]    = nHarped;
}

void CStrandData::SetHarpedStraightNstrands(StrandIndexType nStraight, StrandIndexType nHarped)
{
   if (m_NumPermStrandsType != pgsTypes::sdtStraightHarped)
   {
      // Fill type change - clear and direct fill data
      ClearDirectFillData();

      m_NumPermStrandsType = pgsTypes::sdtStraightHarped;
   }

   m_Nstrands[pgsTypes::Permanent] = nStraight+nHarped;
   m_Nstrands[pgsTypes::Straight]  = nStraight;
   m_Nstrands[pgsTypes::Harped]    = nHarped;
}

void CStrandData::SetTemporaryNstrands(StrandIndexType nStrands)
{
   ATLASSERT(m_NumPermStrandsType != pgsTypes::sdtDirectSelection); // if changing fill mode - don't change temp strands first

   m_Nstrands[pgsTypes::Temporary] = nStrands;
}

void CStrandData::SetDirectStrandFillStraight(const CDirectStrandFillCollection& rCollection)
{
   if(m_NumPermStrandsType != pgsTypes::sdtDirectSelection)
   {
      m_NumPermStrandsType = pgsTypes::sdtDirectSelection;
   }

   // make sure our collection is clean
   StrandIndexType ns = ProcessDirectFillData(rCollection, m_StraightStrandFill);

   // Update number of strands data
   m_Nstrands[pgsTypes::Straight]  = ns;
   m_Nstrands[pgsTypes::Permanent] = ns + m_Nstrands[pgsTypes::Harped];
}

const CDirectStrandFillCollection* CStrandData::GetDirectStrandFillStraight() const
{
   if(m_NumPermStrandsType == pgsTypes::sdtDirectSelection)
   {
      return &m_StraightStrandFill;
   }
   else
   {
      ATLASSERT(false);
      return nullptr;
   }
}

void CStrandData::SetDirectStrandFillHarped(const CDirectStrandFillCollection& rCollection)
{
   if(m_NumPermStrandsType != pgsTypes::sdtDirectSelection)
   {
      m_NumPermStrandsType = pgsTypes::sdtDirectSelection;
   }

   // make sure our collection is clean
   StrandIndexType ns = ProcessDirectFillData(rCollection, m_HarpedStrandFill);

   // Update number of strands data
   m_Nstrands[pgsTypes::Harped]  = ns;
   m_Nstrands[pgsTypes::Permanent] = ns + m_Nstrands[pgsTypes::Straight];
}

const CDirectStrandFillCollection* CStrandData::GetDirectStrandFillHarped() const
{
   if(m_NumPermStrandsType == pgsTypes::sdtDirectSelection)
   {
      return &m_HarpedStrandFill;
   }
   else
   {
      ATLASSERT(false);
      return nullptr;
   }
}

void CStrandData::SetDirectStrandFillTemporary(const CDirectStrandFillCollection& rCollection)
{
   if(m_NumPermStrandsType != pgsTypes::sdtDirectSelection)
   {
      m_NumPermStrandsType = pgsTypes::sdtDirectSelection;
   }

   // make sure our collection is clean
   StrandIndexType ns = ProcessDirectFillData(rCollection, m_TemporaryStrandFill);

   // Update number of strands data
   m_Nstrands[pgsTypes::Temporary]  = ns;
}

const CDirectStrandFillCollection* CStrandData::GetDirectStrandFillTemporary() const
{
   if(m_NumPermStrandsType == pgsTypes::sdtDirectSelection)
   {
      return &m_TemporaryStrandFill;
   }
   else
   {
      ATLASSERT(false);
      return nullptr;
   }
}

void CStrandData::SetHarpPoints(Float64 X0, Float64 X1, Float64 X2, Float64 X3)
{
   m_HarpPoint[ZoneBreakType::Start]    = X0;
   m_HarpPoint[ZoneBreakType::LeftBreak]  = X1;
   m_HarpPoint[ZoneBreakType::RightBreak] = X2;
   m_HarpPoint[ZoneBreakType::End]      = X3;
}

void CStrandData::GetHarpPoints(Float64* pX0, Float64* pX1, Float64* pX2, Float64* pX3) const
{
   *pX0 = m_HarpPoint[ZoneBreakType::Start];
   *pX1 = m_HarpPoint[ZoneBreakType::LeftBreak];
   *pX2 = m_HarpPoint[ZoneBreakType::RightBreak];
   *pX3 = m_HarpPoint[ZoneBreakType::End];
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

const CStrandRow& CStrandData::GetStrandRow(RowIndexType rowIdx) const
{
   return m_StrandRows[rowIdx];
}

bool CStrandData::GetStrandRow(pgsTypes::StrandType strandType, StrandIndexType strandIdx,const CStrandRow** ppStrandRow) const
{
   IndexType typeCount = 0;
   for (const auto& strandRow : m_StrandRows)
   {
      if (strandRow.m_StrandType == strandType)
      {
         if (typeCount <= strandIdx && strandIdx < (typeCount + strandRow.m_nStrands))
         {
            *ppStrandRow = &strandRow;
            return true;
         }
         typeCount += strandRow.m_nStrands;
      }
   }

   ATLASSERT(false); // should never get here or rowIdx is invalid for the strand type given
   *ppStrandRow = nullptr;
   return false;
}

void CStrandData::SetStrandCount(pgsTypes::StrandType strandType,StrandIndexType nStrands)
{
   ATLASSERT(!IsDirectStrandModel(m_NumPermStrandsType));
   m_Nstrands[strandType] = nStrands;
}

StrandIndexType CStrandData::GetStrandCount(pgsTypes::StrandType strandType) const
{
   return m_Nstrands[strandType];
}

pgsTypes::AdjustableStrandType CStrandData::GetAdjustableStrandType() const
{
   return m_AdjustableStrandType;
}

void CStrandData::SetAdjustableStrandType(pgsTypes::AdjustableStrandType type)
{
   if (type != pgsTypes::asStraightOrHarped)
   {
      m_AdjustableStrandType = type;
   }
   else
   {
      ATLASSERT(false); // bridge project data should never be this value
      m_AdjustableStrandType = pgsTypes::asHarped;
   }
}

void CStrandData::AddExtendedStrand(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType,GridIndexType gridIdx)
{
   m_NextendedStrands[strandType][endType].push_back(gridIdx);
   std::sort(m_NextendedStrands[strandType][endType].begin(),m_NextendedStrands[strandType][endType].end());
}

const std::vector<GridIndexType>& CStrandData::GetExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType) const
{
   return m_NextendedStrands[strandType][endType];
}

std::vector<GridIndexType>& CStrandData::GetExtendedStrands(pgsTypes::StrandType strandType, pgsTypes::MemberEndType endType)
{
   return m_NextendedStrands[strandType][endType];
}

void CStrandData::SetExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType,const std::vector<StrandIndexType>& extStrands)
{
   m_NextendedStrands[strandType][endType] = extStrands;
}

bool CStrandData::IsExtendedStrand(pgsTypes::StrandType strandType,GridIndexType gridIdx,pgsTypes::MemberEndType endType) const
{
   std::vector<GridIndexType>::const_iterator found = std::find(m_NextendedStrands[strandType][endType].begin(),m_NextendedStrands[strandType][endType].end(),gridIdx);
   return found != m_NextendedStrands[strandType][endType].end();
}

StrandIndexType CStrandData::GetExtendedStrandCount(pgsTypes::StrandType strandType, pgsTypes::MemberEndType endType) const
{
   if (strandType == pgsTypes::Permanent)
   {
      ATLASSERT(false); // should never be called
      return 0;
   }

   StrandIndexType nExtendedStrands = 0;
   if (m_NumPermStrandsType == pgsTypes::sdtDirectRowInput)
   {
      CStrandRowCollection::const_iterator rowIter(m_StrandRows.begin());
      CStrandRowCollection::const_iterator rowIterEnd(m_StrandRows.end());
      for (; rowIter != rowIterEnd; rowIter++)
      {
         const CStrandRow& row = *rowIter;
         if (row.m_StrandType == strandType && row.m_bIsExtendedStrand[endType])
         {
            nExtendedStrands += row.m_nStrands;
         }
      }
   }
   else
   {
      nExtendedStrands = m_NextendedStrands[strandType][endType].size();
   }

   return nExtendedStrands;
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
   m_HsoHpMeasurement   = hsoLEGACY;
   for ( int i = 0; i < 2; i++ )
   {
      m_HpOffsetAtEnd[i] = 0.0;
      m_HpOffsetAtHp[i]  = 0.0;
   }

   m_AdjustableStrandType = pgsTypes::asHarped;
}

void CStrandData::ClearDirectFillData()
{
   m_StraightStrandFill.clear();
   m_HarpedStrandFill.clear();
   m_TemporaryStrandFill.clear();
}

StrandIndexType CStrandData::GetDebondCount(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType,const GirderLibraryEntry* pGirderLibEntry) const
{
   if (strandType == pgsTypes::Permanent)
   {
      ATLASSERT(false); // should never be called
      return 0;
   }

   StrandIndexType nDebondedStrands = 0;
   if ( m_NumPermStrandsType == pgsTypes::sdtDirectRowInput)
   {
      CStrandRowCollection::const_iterator rowIter(m_StrandRows.begin());
      CStrandRowCollection::const_iterator rowIterEnd(m_StrandRows.end());
      for ( ; rowIter != rowIterEnd; rowIter++ )
      {
         const CStrandRow& row = *rowIter;
         if ( row.m_StrandType == strandType && row.m_bIsDebonded[endType] )
         {
            nDebondedStrands += row.m_nStrands;
         }
      }
   }
   else if (m_NumPermStrandsType == pgsTypes::sdtDirectStrandInput)
   {
      CStrandRowCollection::const_iterator rowIter(m_StrandRows.begin());
      CStrandRowCollection::const_iterator rowIterEnd(m_StrandRows.end());
      for (; rowIter != rowIterEnd; rowIter++)
      {
         const CStrandRow& row = *rowIter;
         if (row.m_StrandType == strandType && row.m_bIsDebonded[endType])
         {
            nDebondedStrands++;
         }
      }
   }
   else
   {
      ATLASSERT( m_NumPermStrandsType == pgsTypes::sdtTotal || m_NumPermStrandsType == pgsTypes::sdtStraightHarped || m_NumPermStrandsType == pgsTypes::sdtDirectSelection );

      for ( const auto& debond_info : m_Debond[strandType])
      {
         if (debond_info.strandTypeGridIdx != INVALID_INDEX )
         {
            if ( strandType == pgsTypes::Straight )
            {
               Float64 Xstart, Ystart, Xend, Yend;
               bool bCanDebond;
               pGirderLibEntry->GetStraightStrandCoordinates(debond_info.strandTypeGridIdx,&Xstart,&Ystart,&Xend,&Yend,&bCanDebond);
               ATLASSERT(bCanDebond == true);
               if ( !IsZero(Xstart) )
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

bool CStrandData::IsDebonded(pgsTypes::StrandType strandType,GridIndexType gridIdx,pgsTypes::MemberEndType endType,Float64* pLdebond) const
{
   *pLdebond = 0;
   auto found = std::find_if(m_Debond[strandType].begin(),m_Debond[strandType].end(),FindDebondByGridIndex(gridIdx));
   if ( found == m_Debond[strandType].end() )
   {
      return false;
   }

   const auto& debondData = *found;
   *pLdebond = debondData.Length[endType];
   return true;
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

void CStrandData::SetHarpStrandOffsetAtEnd(pgsTypes::MemberEndType endType,Float64 offset)
{
   m_HpOffsetAtEnd[endType] = offset;
}

Float64 CStrandData::GetHarpStrandOffsetAtEnd(pgsTypes::MemberEndType endType) const
{
   return m_HpOffsetAtEnd[endType];
}

void CStrandData::SetHarpStrandOffsetMeasurementAtHarpPoint(HarpedStrandOffsetType offsetType)
{
   m_HsoHpMeasurement = offsetType;
}

HarpedStrandOffsetType CStrandData::GetHarpStrandOffsetMeasurementAtHarpPoint() const
{
   return m_HsoHpMeasurement;
}

void CStrandData::SetHarpStrandOffsetAtHarpPoint(pgsTypes::MemberEndType endType,Float64 offset)
{
   m_HpOffsetAtHp[endType] = offset;
}

Float64 CStrandData::GetHarpStrandOffsetAtHarpPoint(pgsTypes::MemberEndType endType) const
{
   return m_HpOffsetAtHp[endType];
}

void CStrandData::SetTemporaryStrandUsage(pgsTypes::TTSUsage ttsUsage)
{
   m_TempStrandUsage = ttsUsage;
}

pgsTypes::TTSUsage CStrandData::GetTemporaryStrandUsage() const
{
   return m_TempStrandUsage;
}

void CStrandData::SetStrandDefinitionType(pgsTypes::StrandDefinitionType permStrandsType)
{
   m_NumPermStrandsType = permStrandsType;
}

pgsTypes::StrandDefinitionType CStrandData::GetStrandDefinitionType() const
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

   for ( int i = 0; i < 2; i++ )
   {
      m_HpOffsetAtEnd[i]      = rOther.m_HpOffsetAtEnd[i];
      m_HpOffsetAtHp[i]       = rOther.m_HpOffsetAtHp[i];
   }

   m_NumPermStrandsType = rOther.m_NumPermStrandsType;

   m_StraightStrandFill    = rOther.m_StraightStrandFill;
   m_HarpedStrandFill      = rOther.m_HarpedStrandFill;
   m_TemporaryStrandFill   = rOther.m_TemporaryStrandFill;

   for ( Uint16 i = 0; i < 4; i++ )
   {
      m_HarpPoint[i] = rOther.m_HarpPoint[i];

      m_Nstrands[i]         = rOther.m_Nstrands[i];
      m_Pjack[i]            = rOther.m_Pjack[i];
      m_bPjackCalculated[i] = rOther.m_bPjackCalculated[i];
      m_LastUserPjack[i]    = rOther.m_LastUserPjack[i];

      if (i < 3)
      {
         m_Debond[i]           = rOther.m_Debond[i];
         m_NextendedStrands[i][pgsTypes::metStart]  = rOther.m_NextendedStrands[i][pgsTypes::metStart];
         m_NextendedStrands[i][pgsTypes::metEnd]    = rOther.m_NextendedStrands[i][pgsTypes::metEnd];
      }
   }

   m_TempStrandUsage    = rOther.m_TempStrandUsage;
   m_bSymmetricDebond   = rOther.m_bSymmetricDebond;

   m_AdjustableStrandType = rOther.m_AdjustableStrandType;

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
   for(const auto& fillInfo : rInCollection)
   {
      ATLASSERT(fillInfo.permStrandGridIdx != INVALID_INDEX);

      StrandIndexType n = fillInfo.numFilled;
      if (n==1 || n==2)
      {
         rLocalCollection.AddFill(fillInfo);

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
   std::array<StrandIndexType,3> nStrands = {0,0,0};

   GridIndexType rowStartGridIdx = 0; // strand strand grid index for the first grid point in the current strand row

   // reset the vectors
   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      m_Debond[strandType].clear();
      m_NextendedStrands[strandType][pgsTypes::metStart].clear();
      m_NextendedStrands[strandType][pgsTypes::metEnd].clear();
   }

   std::sort(m_StrandRows.begin(),m_StrandRows.end()); // must be in sorted order so we work with each strand type one at a time

   pgsTypes::StrandType lastStrandType = pgsTypes::Permanent; // not valid for this case so it is a good one to start with
   for ( auto& strandRow : m_StrandRows)
   {
      if (lastStrandType != strandRow.m_StrandType)
      {
         // strand type changed so we are on to a new grid... reset rowStartGridIdx
         rowStartGridIdx = 0;
         lastStrandType = strandRow.m_StrandType;
      }

      GridIndexType nRowGridPoints = strandRow.m_nStrands / 2; // # grid points in this row

      if (m_NumPermStrandsType == pgsTypes::sdtDirectStrandInput)
      {
         nRowGridPoints = 1;
      }

      for (GridIndexType rowGridIdx = 0; rowGridIdx < nRowGridPoints; rowGridIdx++)
      {
         GridIndexType gridIdx = rowStartGridIdx + rowGridIdx; // global grid point index

         // extended strands
         for (int i = 0; i < 2; i++)
         {
            pgsTypes::MemberEndType end = (pgsTypes::MemberEndType)(i);
            if (strandRow.m_bIsExtendedStrand[end])
            {
               ATLASSERT(strandRow.m_StrandType != pgsTypes::Temporary); // extended strands can't be temporary
               ATLASSERT(!strandRow.m_bIsDebonded[end]); // extended strands can't be debonded

               m_NextendedStrands[strandRow.m_StrandType][end].push_back(gridIdx);
            } // end if strand extension
         }

         // debonded strands
         if ( (strandRow.m_bIsDebonded[pgsTypes::metStart] && !IsZero(strandRow.m_DebondLength[pgsTypes::metStart])) || 
              (strandRow.m_bIsDebonded[pgsTypes::metEnd]   && !IsZero(strandRow.m_DebondLength[pgsTypes::metEnd])) )
         {
            CDebondData debondData;
            debondData.needsConversion = false;
            debondData.strandTypeGridIdx = gridIdx;

            for (int i = 0; i < 2; i++)
            {
               pgsTypes::MemberEndType end = (pgsTypes::MemberEndType)(i);
               if (strandRow.m_bIsDebonded[end])
               {
                  ATLASSERT(!strandRow.m_bIsExtendedStrand[end]); // debonded strands can't be extended
                  debondData.Length[end] = strandRow.m_DebondLength[end];
               }
               else
               {
                  debondData.Length[end] = 0;
               }
            }

            m_Debond[strandRow.m_StrandType].push_back(debondData);
         } // end if debond
      } // next grid point

      rowStartGridIdx += nRowGridPoints;
      nStrands[strandRow.m_StrandType] += strandRow.m_nStrands;
   } // next strand row

   m_Nstrands[pgsTypes::Straight]  = nStrands[pgsTypes::Straight];
   m_Nstrands[pgsTypes::Harped]    = nStrands[pgsTypes::Harped];
   m_Nstrands[pgsTypes::Temporary] = nStrands[pgsTypes::Temporary];
   m_Nstrands[pgsTypes::Permanent] = nStrands[pgsTypes::Straight] + nStrands[pgsTypes::Harped];
}

#if defined _DEBUG
void CStrandData::AssertValid()
{
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

   // permanent strands must be the same grade and type
   // which implies strands all have same properties (such as ultimate and yield strands and modulus of elasticity)
   // this assumption of them being all the same is inherent throught the software
   ATLASSERT(pPool->CompareStrands(m_StrandMaterial[pgsTypes::Straight], m_StrandMaterial[pgsTypes::Harped]));
   //ATLASSERT(pPool->CompareStrands(m_StrandMaterial[pgsTypes::Straight], m_StrandMaterial[pgsTypes::Temporary]));
   //ATLASSERT(pPool->CompareStrands(m_StrandMaterial[pgsTypes::Harped], m_StrandMaterial[pgsTypes::Temporary]));
}
#endif
