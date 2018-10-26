///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include <pgsExt\PrestressData.h>
#include <StrData.cpp>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CDebondInfo
****************************************************************************/
HRESULT CDebondInfo::Load(IStructuredLoad* pStrLoad)
{
   pStrLoad->BeginUnit(_T("DebondInfo"));

   double version;
   pStrLoad->get_Version(&version);

   CComVariant var;

   if (1.0 < version)
   {
      needsConversion = false;

      var.vt = VT_I4;
      pStrLoad->get_Property(_T("Strand"),    &var);
      strandTypeGridIdx = var.iVal;
   }
   else
   {
      // Old version - we will need to convert to grid index in CProjectAgentImp::ConvertLegacyDebondData
      needsConversion = true;

      var.vt = VT_I4;
      pStrLoad->get_Property(_T("Strand1"),    &var); // save only index 1 - old version was indexed to total filled strands 
      strandTypeGridIdx = var.iVal;

      var.vt = VT_I4;
      pStrLoad->get_Property(_T("Strand2"),    &var);
   }

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("Length1"),      &var);
   Length1 = var.dblVal;

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("Length2"),    &var);
   Length2 = var.dblVal;

   pStrLoad->EndUnit();

   return S_OK;
}

HRESULT CDebondInfo::Save(IStructuredSave* pStrSave)
{
   pStrSave->BeginUnit(_T("DebondInfo"),2.0);
   pStrSave->put_Property(_T("Strand"),     CComVariant(strandTypeGridIdx));
   pStrSave->put_Property(_T("Length1"),    CComVariant(Length1));
   pStrSave->put_Property(_T("Length2"),    CComVariant(Length2));
   pStrSave->EndUnit();

   return S_OK;
}

bool CDebondInfo::operator==(const CDebondInfo& rOther) const
{
   if ( strandTypeGridIdx != rOther.strandTypeGridIdx )
      return false;

   if ( Length1 != rOther.Length1 )
      return false;

   if ( Length2 != rOther.Length2 )
      return false;

   return true;
}

bool CDebondInfo::operator!=(const CDebondInfo& rOther) const
{
   return !CDebondInfo::operator==(rOther);
}

/****************************************************************************
CLASS
   CDirectStrandFillInfo
****************************************************************************/
HRESULT CDirectStrandFillInfo::Load(IStructuredLoad* pStrLoad)
{
   pStrLoad->BeginUnit(_T("DirectStrandFillInfo"));

   CComVariant var;

   var.vt = VT_I4;
   pStrLoad->get_Property(_T("idxStrandGrid"),    &var);
   permStrandGridIdx = var.iVal;

   var.vt = VT_I4;
   pStrLoad->get_Property(_T("numFilled"),    &var);
   numFilled = var.iVal;

   pStrLoad->EndUnit();

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
      return false;

   if ( numFilled != rOther.numFilled )
      return false;

   return true;
}

bool CDirectStrandFillInfo::operator!=(const CDirectStrandFillInfo& rOther) const
{
   return !CDirectStrandFillInfo::operator==(rOther);
}


/****************************************************************************
CLASS
   CPrestressData
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CPrestressData::CPrestressData()
{
   ResetPrestressData();

   NumPermStrandsType = NPS_STRAIGHT_HARPED;

   bConvertExtendedStrands = false;
}

CPrestressData::CPrestressData(const CPrestressData& rOther)
{
   MakeCopy(rOther);
}

CPrestressData::~CPrestressData()
{
}

//======================== OPERATORS  =======================================
CPrestressData& CPrestressData::operator= (const CPrestressData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CPrestressData::operator == (const CPrestressData& rOther) const
{
   if (NumPermStrandsType != rOther.NumPermStrandsType)
      return false;

   if (NumPermStrandsType != NPS_DIRECT_SELECTION)
   {
      // Filled in order
      if ( Nstrands[0] != rOther.Nstrands[0] )
         return false;
      if ( Nstrands[1] != rOther.Nstrands[1] )
         return false;
      if ( Nstrands[2] != rOther.Nstrands[2] )
         return false;
      if ( Nstrands[3] != rOther.Nstrands[3] )
         return false;
   }
   else
   {
      // Filled by direct selection
      if (m_StraightStrandFill  != rOther.m_StraightStrandFill)
         return false;
      if (m_HarpedStrandFill    != rOther.m_HarpedStrandFill)
         return false;
      if (m_TemporaryStrandFill != rOther.m_TemporaryStrandFill)
         return false;
   }

   for ( Uint16 i = 0; i < 4; i++ )
   {
      if ( bPjackCalculated[i] != rOther.bPjackCalculated[i] )
         return false;

      if ( !IsEqual(Pjack[i],rOther.Pjack[i]) )
         return false;

      if (i<3)
      {
         if ( Debond[i] != rOther.Debond[i] )
            return false;

         if ( NextendedStrands[i][pgsTypes::metStart] == rOther.NextendedStrands[i][pgsTypes::metStart] )
            return false;

         if ( NextendedStrands[i][pgsTypes::metEnd] == rOther.NextendedStrands[i][pgsTypes::metEnd] )
            return false;
      }
   }

   if ( HsoHpMeasurement != rOther.HsoHpMeasurement )
      return false;

   if ( HsoEndMeasurement != rOther.HsoEndMeasurement )
      return false;

   if ( HpOffsetAtEnd != rOther.HpOffsetAtEnd )
      return false;

   if ( HpOffsetAtHp != rOther.HpOffsetAtHp )
      return false;

   if ( TempStrandUsage != rOther.TempStrandUsage )
      return false;

   if ( bSymmetricDebond != rOther.bSymmetricDebond )
      return false;

   return true;
}

bool CPrestressData::operator != (const CPrestressData& rOther) const
{
   return !operator==( rOther );
}

//======================== OPERATIONS =======================================
///////////////////////////////////////////////////////

int CPrestressData::GetNumPermStrandsType() const
{
   return NumPermStrandsType;
}

void CPrestressData::SetTotalPermanentNstrands(StrandIndexType nPermanent,StrandIndexType nStraight, StrandIndexType nHarped)
{
   ATLASSERT(nPermanent==nStraight+nHarped);

   if (NumPermStrandsType != NPS_TOTAL_NUMBER)
   {
      // Fill type change - clear and direct fill data
      ClearDirectFillData();

      NumPermStrandsType = NPS_TOTAL_NUMBER;
   }

   Nstrands[pgsTypes::Permanent] = nPermanent;
   Nstrands[pgsTypes::Straight]  = nStraight;
   Nstrands[pgsTypes::Harped]    = nHarped;
}

void CPrestressData::SetHarpedStraightNstrands(StrandIndexType nStraight, StrandIndexType nHarped)
{
   if (NumPermStrandsType != NPS_STRAIGHT_HARPED)
   {
      // Fill type change - clear and direct fill data
      ClearDirectFillData();

      NumPermStrandsType = NPS_STRAIGHT_HARPED;
   }

   Nstrands[pgsTypes::Permanent] = nStraight+nHarped;
   Nstrands[pgsTypes::Straight]  = nStraight;
   Nstrands[pgsTypes::Harped]    = nHarped;
}

void CPrestressData::SetTemporaryNstrands(StrandIndexType nStrands)
{
   ATLASSERT(NumPermStrandsType != NPS_DIRECT_SELECTION); // if changing fill mode - don't change temp strands first

   Nstrands[pgsTypes::Temporary] = nStrands;
}

void CPrestressData::SetDirectStrandFillStraight(const DirectStrandFillCollection& rCollection)
{
   if(NumPermStrandsType != NPS_DIRECT_SELECTION)
   {
      NumPermStrandsType = NPS_DIRECT_SELECTION;
   }

   // make sure our collection is clean
   StrandIndexType ns = ProcessDirectFillData(rCollection, m_StraightStrandFill);

   // Update number of strands data
   Nstrands[pgsTypes::Straight]  = ns;
   Nstrands[pgsTypes::Permanent] = ns + Nstrands[pgsTypes::Harped];
}

const DirectStrandFillCollection* CPrestressData::GetDirectStrandFillStraight() const
{
   if(NumPermStrandsType == NPS_DIRECT_SELECTION)
   {
      return &m_StraightStrandFill;
   }
   else
   {
      ATLASSERT(0);
      return NULL;
   }
}

void CPrestressData::SetDirectStrandFillHarped(const DirectStrandFillCollection& rCollection)
{
   if(NumPermStrandsType != NPS_DIRECT_SELECTION)
   {
      NumPermStrandsType = NPS_DIRECT_SELECTION;
   }

   // make sure our collection is clean
   StrandIndexType ns = ProcessDirectFillData(rCollection, m_HarpedStrandFill);

   // Update number of strands data
   Nstrands[pgsTypes::Harped]  = ns;
   Nstrands[pgsTypes::Permanent] = ns + Nstrands[pgsTypes::Straight];
}

const DirectStrandFillCollection* CPrestressData::GetDirectStrandFillHarped() const
{
   if(NumPermStrandsType == NPS_DIRECT_SELECTION)
   {
      return &m_HarpedStrandFill;
   }
   else
   {
      ATLASSERT(0);
      return NULL;
   }
}

void CPrestressData::SetDirectStrandFillTemporary(const DirectStrandFillCollection& rCollection)
{
   if(NumPermStrandsType != NPS_DIRECT_SELECTION)
   {
      NumPermStrandsType = NPS_DIRECT_SELECTION;
   }

   // make sure our collection is clean
   StrandIndexType ns = ProcessDirectFillData(rCollection, m_TemporaryStrandFill);

   // Update number of strands data
   Nstrands[pgsTypes::Temporary]  = ns;
}

const DirectStrandFillCollection* CPrestressData::GetDirectStrandFillTemporary() const
{
   if(NumPermStrandsType == NPS_DIRECT_SELECTION)
   {
      return &m_TemporaryStrandFill;
   }
   else
   {
      ATLASSERT(0);
      return NULL;
   }
}

StrandIndexType CPrestressData::ProcessDirectFillData(const DirectStrandFillCollection& rInCollection, DirectStrandFillCollection& rLocalCollection)
{
   // Clear out old data and make room for new. Dont put any invalid data into collection
   rLocalCollection.clear();
   rLocalCollection.reserve(rInCollection.size());

   StrandIndexType ns(0);
   DirectStrandFillCollection::const_iterator it(rInCollection.begin());
   DirectStrandFillCollection::const_iterator end(rInCollection.end());
   for ( ; it != end; it++ )
   {
      ATLASSERT(it->permStrandGridIdx!=INVALID_INDEX);

      StrandIndexType n = it->numFilled;
      if (n==1 || n==2)
      {
         rLocalCollection.AddFill(*it);

         ns += n;
      }
      else
      {
         ATLASSERT(0); // is there a new valid fill value?
      }
   }

   return ns;
}

StrandIndexType CPrestressData::GetNstrands(pgsTypes::StrandType type) const
{
   ATLASSERT(NumPermStrandsType==NPS_TOTAL_NUMBER || NumPermStrandsType==NPS_STRAIGHT_HARPED  || NumPermStrandsType==NPS_DIRECT_SELECTION);

   return Nstrands[type];
}

void CPrestressData::AddExtendedStrand(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType,GridIndexType gridIdx)
{
   NextendedStrands[strandType][endType].push_back(gridIdx);
   std::sort(NextendedStrands[strandType][endType].begin(),NextendedStrands[strandType][endType].end());
}

const std::vector<StrandIndexType>& CPrestressData::GetExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType) const
{
   return NextendedStrands[strandType][endType];
}

void CPrestressData::SetExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType,const std::vector<StrandIndexType>& extStrands)
{
   NextendedStrands[strandType][endType] = extStrands;
}

// Resets all the prestressing put to default values
void CPrestressData::ResetPrestressData()
{
   for ( Uint16 i = 0; i < 4; i++ )
   {
      Nstrands[i] = 0;
      Pjack[i] = 0;
      bPjackCalculated[i] = true;
      LastUserPjack[i] = 0;

      if (i<3)
      {
         Debond[i].clear();

         NextendedStrands[i][pgsTypes::metStart].clear();
         NextendedStrands[i][pgsTypes::metEnd].clear();
      }
   }

   ClearDirectFillData();

   TempStrandUsage    = pgsTypes::ttsPretensioned;
   bSymmetricDebond   = true;

   // convert to legacy since this is the only measurement that is sure to fit in the girder
   HsoEndMeasurement  = hsoLEGACY;
   HpOffsetAtEnd      = 0.0;
   HsoHpMeasurement   = hsoLEGACY;
   HpOffsetAtHp       = 0.0;
}

void CPrestressData::ClearDirectFillData()
{
   m_StraightStrandFill.clear();
   m_HarpedStrandFill.clear();
   m_TemporaryStrandFill.clear();
}

HRESULT CPrestressData::Load(IStructuredLoad* pStrLoad)
{
   USES_CONVERSION;

   HRESULT hr = S_OK;

   double version;
   pStrLoad->get_Version(&version);

   CComVariant var;

   if (version<6.0)
   {
      HsoEndMeasurement = hsoLEGACY;
   }
   else
   {
      var.Clear();
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("HsoEndMeasurement"), &var );
      HsoEndMeasurement = (HarpedStrandOffsetType)(var.lVal);
   }

   var.Clear();
   var.vt = VT_R8;
   pStrLoad->get_Property(_T("HpOffsetAtEnd"), &var );
   HpOffsetAtEnd = var.dblVal;

   if (version<6.0)
   {
      HsoHpMeasurement = hsoLEGACY;
   }
   else
   {
      var.Clear();
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("HsoHpMeasurement"), &var );
      HsoHpMeasurement = (HarpedStrandOffsetType)(var.lVal);
   }

   var.Clear();
   var.vt = VT_R8;
   pStrLoad->get_Property(_T("HpOffsetAtHp"), &var );
   HpOffsetAtHp = var.dblVal;

   if (version<8.0)
   {
      NumPermStrandsType = NPS_STRAIGHT_HARPED;
   }
   else
   {
      var.Clear();
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("NumPermStrandsType"), &var );
      NumPermStrandsType = var.lVal;
   }

   var.Clear();
   var.vt = VT_UI2;
   pStrLoad->get_Property(_T("NumHarpedStrands"), &var );
   Nstrands[pgsTypes::Harped] = var.uiVal;

   var.Clear();
   var.vt = VT_UI2;
   pStrLoad->get_Property(_T("NumStraightStrands"), &var );
   Nstrands[pgsTypes::Straight] = var.uiVal;

   var.Clear();
   var.vt = VT_UI2;
   pStrLoad->get_Property(_T("NumTempStrands"), &var );
   Nstrands[pgsTypes::Temporary] = var.uiVal;

   if (version<8.0)
   {
      Nstrands[pgsTypes::Permanent] = 0;
   }
   else
   {
      var.Clear();
      var.vt = VT_UI2;
      pStrLoad->get_Property(_T("NumPermanentStrands"), &var );
      Nstrands[pgsTypes::Permanent] = var.uiVal;
   }

   if ( 12 <= version )
   {
      // added in version 12
      pStrLoad->BeginUnit(_T("ExtendedStrands"));

      Float64 extendedStrandVersion;
      pStrLoad->get_Version(&extendedStrandVersion);
      if ( extendedStrandVersion < 2.0 )
         bConvertExtendedStrands = true;

      pStrLoad->BeginUnit(_T("Start"));

      pStrLoad->BeginUnit(_T("Straight"));
      var.vt = VT_UI8;
      pStrLoad->get_Property(_T("Count"),&var);
      StrandIndexType nStrands = var.ullVal;
      for ( StrandIndexType idx = 0; idx < nStrands; idx++ )
      {
         pStrLoad->get_Property(_T("Strand"),&var);
         StrandIndexType strandIdx = var.ullVal;
         NextendedStrands[pgsTypes::Straight][pgsTypes::metStart].push_back(strandIdx);
      }
      pStrLoad->EndUnit(); // Straight

      pStrLoad->BeginUnit(_T("Harped"));
      var.vt = VT_UI8;
      pStrLoad->get_Property(_T("Count"),&var);
      nStrands = var.ullVal;
      for ( StrandIndexType idx = 0; idx < nStrands; idx++ )
      {
         pStrLoad->get_Property(_T("Strand"),&var);
         StrandIndexType strandIdx = var.ullVal;
         NextendedStrands[pgsTypes::Harped][pgsTypes::metStart].push_back(strandIdx);
      }
      pStrLoad->EndUnit(); // Harped

      pStrLoad->BeginUnit(_T("Temporary"));
      var.vt = VT_UI8;
      pStrLoad->get_Property(_T("Count"),&var);
      nStrands = var.ullVal;
      for ( StrandIndexType idx = 0; idx < nStrands; idx++ )
      {
         pStrLoad->get_Property(_T("Strand"),&var);
         StrandIndexType strandIdx = var.ullVal;
         NextendedStrands[pgsTypes::Temporary][pgsTypes::metStart].push_back(strandIdx);
      }
      pStrLoad->EndUnit(); // Temporary
      pStrLoad->EndUnit(); // Start

      pStrLoad->BeginUnit(_T("End"));
      pStrLoad->BeginUnit(_T("Straight"));
      var.vt = VT_UI8;
      pStrLoad->get_Property(_T("Count"),&var);
      nStrands = var.ullVal;
      for ( StrandIndexType idx = 0; idx < nStrands; idx++ )
      {
         pStrLoad->get_Property(_T("Strand"),&var);
         StrandIndexType strandIdx = var.ullVal;
         NextendedStrands[pgsTypes::Straight][pgsTypes::metEnd].push_back(strandIdx);
      }
      pStrLoad->EndUnit(); // Straight

      pStrLoad->BeginUnit(_T("Harped"));
      var.vt = VT_UI8;
      pStrLoad->get_Property(_T("Count"),&var);
      nStrands = var.ullVal;
      for ( StrandIndexType idx = 0; idx < nStrands; idx++ )
      {
         pStrLoad->get_Property(_T("Strand"),&var);
         StrandIndexType strandIdx = var.ullVal;
         NextendedStrands[pgsTypes::Harped][pgsTypes::metEnd].push_back(strandIdx);
      }
      pStrLoad->EndUnit(); // Harped

      pStrLoad->BeginUnit(_T("Temporary"));
      var.vt = VT_UI8;
      pStrLoad->get_Property(_T("Count"),&var);
      nStrands = var.ullVal;
      for ( StrandIndexType idx = 0; idx < nStrands; idx++ )
      {
         pStrLoad->get_Property(_T("Strand"),&var);
         StrandIndexType strandIdx = var.ullVal;
         NextendedStrands[pgsTypes::Temporary][pgsTypes::metEnd].push_back(strandIdx);
      }
      pStrLoad->EndUnit(); // Temporary
      pStrLoad->EndUnit(); // End

      pStrLoad->EndUnit(); // ExtendedStrands
   }

   if (13 <= version && NumPermStrandsType == NPS_DIRECT_SELECTION)
   {
      hr = pStrLoad->BeginUnit(_T("DirectSelectStrandFill"));
      if (FAILED(hr))
      {
         ATLASSERT(0);
         return hr;
      }

      pStrLoad->BeginUnit(_T("StraightStrands"));
      var.Clear();
      var.vt = VT_UI2;
      pStrLoad->get_Property(_T("NumStraightFill"), &var );
      StrandIndexType nums = var.uiVal;
      for (StrandIndexType is=0; is<nums; is++)
      {
         CDirectStrandFillInfo fi;
         fi.Load(pStrLoad);
         m_StraightStrandFill.AddFill(fi);
      }
      pStrLoad->EndUnit();

      pStrLoad->BeginUnit(_T("HarpedStrands"));
      var.Clear();
      var.vt = VT_UI2;
      pStrLoad->get_Property(_T("NumHarpedFill"), &var );
      nums = var.uiVal;
      for (StrandIndexType is=0; is<nums; is++)
      {
         CDirectStrandFillInfo fi;
         fi.Load(pStrLoad);
         m_HarpedStrandFill.AddFill(fi);
      }
      pStrLoad->EndUnit();

      pStrLoad->BeginUnit(_T("TemporaryStrands"));
      var.Clear();
      var.vt = VT_UI2;
      pStrLoad->get_Property(_T("NumTemporaryFill"), &var );
      nums = var.uiVal;
      for (StrandIndexType is=0; is<nums; is++)
      {
         CDirectStrandFillInfo fi;
         fi.Load(pStrLoad);
         m_TemporaryStrandFill.AddFill(fi);
      }
      pStrLoad->EndUnit();

      pStrLoad->EndUnit(); // end DirectSelectStrandFill
   }

   var.Clear();
   var.vt = VT_R8;
   pStrLoad->get_Property(_T("PjHarped"), &var );
   Pjack[pgsTypes::Harped] = var.dblVal;

   var.Clear();
   var.vt = VT_R8;
   pStrLoad->get_Property(_T("PjStraight"), &var );
   Pjack[pgsTypes::Straight] = var.dblVal;

   var.Clear();
   var.vt = VT_R8;
   pStrLoad->get_Property(_T("PjTemp"), &var );
   Pjack[pgsTypes::Temporary] = var.dblVal;

   if (version<8.0)
   {
      Pjack[pgsTypes::Permanent] = 0.0;
   }
   else
   {
      var.Clear();
      var.vt = VT_R8;
      pStrLoad->get_Property(_T("PjPermanent"), &var );
      Pjack[pgsTypes::Permanent] = var.dblVal;
   }

   var.Clear();
   var.vt = VT_I4;
   pStrLoad->get_Property(_T("CalcPjHarped"), &var );
   bPjackCalculated[pgsTypes::Harped] = (var.lVal!=0);

   var.Clear();
   var.vt = VT_I4;
   pStrLoad->get_Property(_T("CalcPjStraight"), &var );
   bPjackCalculated[pgsTypes::Straight] = (var.lVal!=0);

   var.Clear();
   var.vt = VT_I4;
   pStrLoad->get_Property(_T("CalcPjTemp"), &var );
   bPjackCalculated[pgsTypes::Temporary] = (var.lVal!=0);

   if (version<8.0)
   {
      bPjackCalculated[pgsTypes::Permanent] = false;
   }
   else
   {
      var.Clear();
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("CalcPjPermanent"), &var );
      bPjackCalculated[pgsTypes::Permanent] = (var.lVal!=0);
   }

   var.Clear();
   var.vt = VT_R8;
   pStrLoad->get_Property(_T("LastUserPjHarped"), &var );
   LastUserPjack[pgsTypes::Harped] = var.dblVal;

   var.Clear();
   var.vt = VT_R8;
   pStrLoad->get_Property(_T("LastUserPjStraight"), &var );
   LastUserPjack[pgsTypes::Straight] = var.dblVal;

   var.Clear();
   var.vt = VT_R8;
   pStrLoad->get_Property(_T("LastUserPjTemp"), &var );
   LastUserPjack[pgsTypes::Temporary] = var.dblVal;

   if (version<8.0)
   {
      LastUserPjack[pgsTypes::Permanent] = 0.0;
   }
   else
   {
      var.Clear();
      var.vt = VT_R8;
      pStrLoad->get_Property(_T("LastUserPjPermanent"), &var );
      LastUserPjack[pgsTypes::Permanent] = var.dblVal;
   }

   if ( version < 3.1 )
   {
      TempStrandUsage = pgsTypes::ttsPretensioned;
   }
   else
   {
      var.Clear();
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("TempStrandUsage"),&var);
      TempStrandUsage = (pgsTypes::TTSUsage)var.lVal;
   }

   // in an earlier version of the constructor for this class,
   // TempStrandUsage was not initialized properly. This caused the variable to
   // be unset and bogus values to be stored... if the value of TempStrandUsage is
   // bogus, set it to a reasonable value
   if ( TempStrandUsage != pgsTypes::ttsPretensioned && 
        TempStrandUsage != pgsTypes::ttsPTBeforeShipping && 
        TempStrandUsage != pgsTypes::ttsPTBeforeLifting && 
        TempStrandUsage != pgsTypes::ttsPTAfterLifting )
   {
      TempStrandUsage = pgsTypes::ttsPretensioned;
   }


   if ( 5.0 <= version )
   {
      var.Clear();
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("SymmetricDebond"),&var);

      bSymmetricDebond = (var.lVal != 0);

      Debond[pgsTypes::Straight].clear();
      pStrLoad->BeginUnit(_T("StraightStrandDebonding"));
      long nDebondInfo;
      var.Clear();
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("DebondInfoCount"),&var);
      nDebondInfo = var.lVal;

      int i = 0;
      for ( i = 0; i < nDebondInfo; i++ )
      {
         CDebondInfo debond_info;
         debond_info.Load(pStrLoad);
         Debond[pgsTypes::Straight].push_back(debond_info);
      }
      pStrLoad->EndUnit();


      Debond[pgsTypes::Harped].clear();
      pStrLoad->BeginUnit(_T("HarpedStrandDebonding"));
      var.Clear();
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("DebondInfoCount"),&var);
      nDebondInfo = var.lVal;

      for ( i = 0; i < nDebondInfo; i++ )
      {
         CDebondInfo debond_info;
         debond_info.Load(pStrLoad);
         Debond[pgsTypes::Harped].push_back(debond_info);
      }
      pStrLoad->EndUnit();


      Debond[pgsTypes::Temporary].clear();
      pStrLoad->BeginUnit(_T("TemporaryStrandDebonding"));
      var.Clear();
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("DebondInfoCount"),&var);
      nDebondInfo = var.lVal;

      for ( i = 0; i < nDebondInfo; i++ )
      {
         CDebondInfo debond_info;
         debond_info.Load(pStrLoad);
         Debond[pgsTypes::Temporary].push_back(debond_info);
      }
      pStrLoad->EndUnit();
   }

   return hr;
}

HRESULT CPrestressData::Save(IStructuredSave* pStrSave)
{
   HRESULT hr = S_OK;

   // We use the version number from our parent CGirderData

   pStrSave->put_Property(_T("HsoEndMeasurement"), CComVariant(HsoEndMeasurement));
   pStrSave->put_Property(_T("HpOffsetAtEnd"), CComVariant(HpOffsetAtEnd));
   pStrSave->put_Property(_T("HsoHpMeasurement"), CComVariant(HsoHpMeasurement));
   pStrSave->put_Property(_T("HpOffsetAtHp"), CComVariant(HpOffsetAtHp));

   pStrSave->put_Property(_T("NumPermStrandsType"), CComVariant(NumPermStrandsType));

   pStrSave->put_Property(_T("NumHarpedStrands"), CComVariant(Nstrands[pgsTypes::Harped]));
   pStrSave->put_Property(_T("NumStraightStrands"), CComVariant(Nstrands[pgsTypes::Straight]));
   pStrSave->put_Property(_T("NumTempStrands"), CComVariant(Nstrands[pgsTypes::Temporary]));
   pStrSave->put_Property(_T("NumPermanentStrands"), CComVariant(Nstrands[pgsTypes::Permanent]));

   // added in version 12
   pStrSave->BeginUnit(_T("ExtendedStrands"),2.0); // storing grid index in version 2 (version 1 was strand index)
   pStrSave->BeginUnit(_T("Start"),1.0);
   pStrSave->BeginUnit(_T("Straight"),1.0);
   pStrSave->put_Property(_T("Count"),CComVariant(NextendedStrands[pgsTypes::Straight][pgsTypes::metStart].size()));
   std::vector<GridIndexType>::iterator iter(NextendedStrands[pgsTypes::Straight][pgsTypes::metStart].begin());
   std::vector<GridIndexType>::iterator end(NextendedStrands[pgsTypes::Straight][pgsTypes::metStart].end());
   for ( ; iter != end; iter++ )
   {
      pStrSave->put_Property(_T("Strand"),CComVariant(*iter));
   }
   pStrSave->EndUnit(); // Straight

   pStrSave->BeginUnit(_T("Harped"),1.0);
   pStrSave->put_Property(_T("Count"),CComVariant(NextendedStrands[pgsTypes::Harped][pgsTypes::metStart].size()));
   iter = NextendedStrands[pgsTypes::Harped][pgsTypes::metStart].begin();
   end  = NextendedStrands[pgsTypes::Harped][pgsTypes::metStart].end();
   for ( ; iter != end; iter++ )
   {
      pStrSave->put_Property(_T("Strand"),CComVariant(*iter));
   }
   pStrSave->EndUnit(); // Harped

   pStrSave->BeginUnit(_T("Temporary"),1.0);
   pStrSave->put_Property(_T("Count"),CComVariant(NextendedStrands[pgsTypes::Temporary][pgsTypes::metStart].size()));
   iter = NextendedStrands[pgsTypes::Temporary][pgsTypes::metStart].begin();
   end  = NextendedStrands[pgsTypes::Temporary][pgsTypes::metStart].end();
   for ( ; iter != end; iter++ )
   {
      pStrSave->put_Property(_T("Strand"),CComVariant(*iter));
   }
   pStrSave->EndUnit(); // Temporary
    pStrSave->EndUnit(); // Start

   pStrSave->BeginUnit(_T("End"),1.0);
   pStrSave->BeginUnit(_T("Straight"),1.0);
   pStrSave->put_Property(_T("Count"),CComVariant(NextendedStrands[pgsTypes::Straight][pgsTypes::metEnd].size()));
   iter = NextendedStrands[pgsTypes::Straight][pgsTypes::metEnd].begin();
   end  = NextendedStrands[pgsTypes::Straight][pgsTypes::metEnd].end();
   for ( ; iter != end; iter++ )
   {
      pStrSave->put_Property(_T("Strand"),CComVariant(*iter));
   }
   pStrSave->EndUnit(); // Straight

   pStrSave->BeginUnit(_T("Harped"),1.0);
   pStrSave->put_Property(_T("Count"),CComVariant(NextendedStrands[pgsTypes::Harped][pgsTypes::metEnd].size()));
   iter = NextendedStrands[pgsTypes::Harped][pgsTypes::metEnd].begin();
   end  = NextendedStrands[pgsTypes::Harped][pgsTypes::metEnd].end();
   for ( ; iter != end; iter++ )
   {
      pStrSave->put_Property(_T("Strand"),CComVariant(*iter));
   }
   pStrSave->EndUnit(); // Harped

   pStrSave->BeginUnit(_T("Temporary"),1.0);
   pStrSave->put_Property(_T("Count"),CComVariant(NextendedStrands[pgsTypes::Temporary][pgsTypes::metEnd].size()));
   iter = NextendedStrands[pgsTypes::Temporary][pgsTypes::metEnd].begin();
   end  = NextendedStrands[pgsTypes::Temporary][pgsTypes::metEnd].end();
   for ( ; iter != end; iter++ )
   {
      pStrSave->put_Property(_T("Strand"),CComVariant(*iter));
   }
   pStrSave->EndUnit(); // Temporary
   pStrSave->EndUnit(); // End
   pStrSave->EndUnit(); // ExtendedStrands

   if (NumPermStrandsType==NPS_DIRECT_SELECTION)
   {
      // Added this in version 13
      pStrSave->BeginUnit(_T("DirectSelectStrandFill"), 1.0);

      pStrSave->BeginUnit(_T("StraightStrands"), 1.0);
      pStrSave->put_Property(_T("NumStraightFill"), CComVariant( StrandIndexType(m_StraightStrandFill.size())));
      for (DirectStrandFillCollection::const_iterator its=m_StraightStrandFill.begin(); its!=m_StraightStrandFill.end(); its++)
      {
         its->Save(pStrSave);
      }
      pStrSave->EndUnit();

      pStrSave->BeginUnit(_T("HarpedStrands"), 1.0);
      pStrSave->put_Property(_T("NumHarpedFill"), CComVariant( StrandIndexType(m_HarpedStrandFill.size())));
      for (DirectStrandFillCollection::const_iterator its=m_HarpedStrandFill.begin(); its!=m_HarpedStrandFill.end(); its++)
      {
         its->Save(pStrSave);
      }
      pStrSave->EndUnit();

      pStrSave->BeginUnit(_T("TemporaryStrands"), 1.0);
      pStrSave->put_Property(_T("NumTemporaryFill"), CComVariant( StrandIndexType(m_TemporaryStrandFill.size())));
      for (DirectStrandFillCollection::const_iterator its=m_TemporaryStrandFill.begin(); its!=m_TemporaryStrandFill.end(); its++)
      {
         its->Save(pStrSave);
      }
      pStrSave->EndUnit();

      pStrSave->EndUnit(); // DirectSelectStrandFill
   }

   pStrSave->put_Property(_T("PjHarped"), CComVariant(Pjack[pgsTypes::Harped]));
   pStrSave->put_Property(_T("PjStraight"), CComVariant(Pjack[pgsTypes::Straight]));
   pStrSave->put_Property(_T("PjTemp"), CComVariant(Pjack[pgsTypes::Temporary]));
   pStrSave->put_Property(_T("PjPermanent"), CComVariant(Pjack[pgsTypes::Permanent]));
   pStrSave->put_Property(_T("CalcPjHarped"), CComVariant(bPjackCalculated[pgsTypes::Harped]));
   pStrSave->put_Property(_T("CalcPjStraight"), CComVariant(bPjackCalculated[pgsTypes::Straight]));
   pStrSave->put_Property(_T("CalcPjTemp"), CComVariant(bPjackCalculated[pgsTypes::Temporary]));
   pStrSave->put_Property(_T("CalcPjPermanent"), CComVariant(bPjackCalculated[pgsTypes::Permanent]));
   pStrSave->put_Property(_T("LastUserPjHarped"), CComVariant(LastUserPjack[pgsTypes::Harped]));
   pStrSave->put_Property(_T("LastUserPjStraight"), CComVariant(LastUserPjack[pgsTypes::Straight]));
   pStrSave->put_Property(_T("LastUserPjTemp"), CComVariant(LastUserPjack[pgsTypes::Temporary]));
   pStrSave->put_Property(_T("LastUserPjPermanent"), CComVariant(LastUserPjack[pgsTypes::Permanent]));
   pStrSave->put_Property(_T("TempStrandUsage"),CComVariant(TempStrandUsage));

   pStrSave->put_Property(_T("SymmetricDebond"),CComVariant(bSymmetricDebond));

   pStrSave->BeginUnit(_T("StraightStrandDebonding"),1.0);
   CollectionIndexType nDebondInfo = Debond[pgsTypes::Straight].size();
   pStrSave->put_Property(_T("DebondInfoCount"),CComVariant(nDebondInfo));
   std::vector<CDebondInfo>::iterator debond_iter;
   for ( debond_iter = Debond[pgsTypes::Straight].begin(); debond_iter != Debond[pgsTypes::Straight].end(); debond_iter++ )
   {
      CDebondInfo& debond_info = *debond_iter;
      debond_info.Save(pStrSave);
   }
   pStrSave->EndUnit(); // StraightStrandDebonding


   pStrSave->BeginUnit(_T("HarpedStrandDebonding"),1.0);
   nDebondInfo = Debond[pgsTypes::Harped].size();
   pStrSave->put_Property(_T("DebondInfoCount"),CComVariant(nDebondInfo));
   for ( debond_iter = Debond[pgsTypes::Harped].begin(); debond_iter != Debond[pgsTypes::Harped].end(); debond_iter++ )
   {
      CDebondInfo& debond_info = *debond_iter;
      debond_info.Save(pStrSave);
   }
   pStrSave->EndUnit(); // HarpedStrandDebonding

   pStrSave->BeginUnit(_T("TemporaryStrandDebonding"),1.0);
   nDebondInfo = Debond[pgsTypes::Temporary].size();
   pStrSave->put_Property(_T("DebondInfoCount"),CComVariant(nDebondInfo));
   for ( debond_iter = Debond[pgsTypes::Temporary].begin(); debond_iter != Debond[pgsTypes::Temporary].end(); debond_iter++ )
   {
      CDebondInfo& debond_info = *debond_iter;
      debond_info.Save(pStrSave);
   }
   pStrSave->EndUnit(); // TemporaryStrandDebonding

   return hr;
}

void CPrestressData::ClearExtendedStrands(pgsTypes::StrandType strandType,pgsTypes::MemberEndType endType)
{
   NextendedStrands[strandType][endType].clear();
}

void CPrestressData::ClearDebondData()
{
   for (long i=0; i<3; i++)
   {
      Debond[i].clear();
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CPrestressData::MakeCopy(const CPrestressData& rOther)
{
   HsoEndMeasurement  = rOther.HsoEndMeasurement;
   HsoHpMeasurement   = rOther.HsoHpMeasurement;
   HpOffsetAtEnd      = rOther.HpOffsetAtEnd;
   HpOffsetAtHp       = rOther.HpOffsetAtHp;

   m_StraightStrandFill    = rOther.m_StraightStrandFill;
   m_HarpedStrandFill      = rOther.m_HarpedStrandFill;
   m_TemporaryStrandFill   = rOther.m_TemporaryStrandFill;

   NumPermStrandsType = rOther.NumPermStrandsType;

   for ( Uint16 i = 0; i < 4; i++ )
   {
      Nstrands[i]         = rOther.Nstrands[i];
      Pjack[i]            = rOther.Pjack[i];
      bPjackCalculated[i] = rOther.bPjackCalculated[i];
      LastUserPjack[i]    = rOther.LastUserPjack[i];

      if (i<3)
      {
         Debond[i]           = rOther.Debond[i];
         NextendedStrands[i][pgsTypes::metStart]  = rOther.NextendedStrands[i][pgsTypes::metStart];
         NextendedStrands[i][pgsTypes::metEnd]    = rOther.NextendedStrands[i][pgsTypes::metEnd];
      }
   }

   TempStrandUsage    = rOther.TempStrandUsage;
   bSymmetricDebond   = rOther.bSymmetricDebond;

   bConvertExtendedStrands = rOther.bConvertExtendedStrands;
}

void CPrestressData::MakeAssignment(const CPrestressData& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================

StrandIndexType DirectStrandFillCollection::GetFilledStrandCount() const
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

bool DirectStrandFillCollection::IsStrandFilled(GridIndexType index) const
{
   const_iterator it    = begin();
   const_iterator itend = end();
   while(it!=itend)
   {
      if (it->permStrandGridIdx == index)
      {
         // no need to check fill count
         return true;
      }

      it++;
   }

   return false;
}

void DirectStrandFillCollection::RemoveFill(GridIndexType index) 
{
   std::vector<CDirectStrandFillInfo>::iterator it(m_StrandFill.begin());
   std::vector<CDirectStrandFillInfo>::iterator itend(m_StrandFill.end());
   while(it!=itend)
   {
      if (it->permStrandGridIdx == index)
      {
         m_StrandFill.erase(it);
         return;
      }
      it++;
   }

ATLASSERT(0); // not found?
}

void DirectStrandFillCollection::AddFill(const CDirectStrandFillInfo& fillInf)
{
   ATLASSERT(fillInf.numFilled>0); // compressed container should not contain unfilled strands

   StrandIndexType localIdx = fillInf.permStrandGridIdx;

   // Fill in sorted order
   DirectStrandFillCollection::const_iterator it( m_StrandFill.begin() );
   DirectStrandFillCollection::const_iterator itend( m_StrandFill.end() );
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

const CDirectStrandFillInfo& DirectStrandFillCollection::GetFill(CollectionIndexType fillNo) const
{
   ATLASSERT(fillNo < m_StrandFill.size());
   return m_StrandFill[fillNo];
}
