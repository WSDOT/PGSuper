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
#include <PgsExt\GirderData.h>
#include <Units\SysUnits.h>
#include <StdIo.h>

#include <Lrfd\StrandPool.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CGirderData
****************************************************************************/


HRESULT CDebondInfo::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   pStrLoad->BeginUnit(_T("DebondInfo"));

   CComVariant var;

   var.vt = VT_I4;
   pStrLoad->get_Property(_T("Strand1"),    &var);
   idxStrand1 = var.iVal;

   var.vt = VT_I4;
   pStrLoad->get_Property(_T("Strand2"),    &var);
   idxStrand2 = var.iVal;

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("Length1"),      &var);
   Length1 = var.dblVal;

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("Length2"),    &var);
   Length2 = var.dblVal;

   pStrLoad->EndUnit();

   return S_OK;
}

HRESULT CDebondInfo::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("DebondInfo"),1.0);
   pStrSave->put_Property(_T("Strand1"),    CComVariant(idxStrand1));
   pStrSave->put_Property(_T("Strand2"),    CComVariant(idxStrand2));
   pStrSave->put_Property(_T("Length1"),    CComVariant(Length1));
   pStrSave->put_Property(_T("Length2"),    CComVariant(Length2));
   pStrSave->EndUnit();

   return S_OK;
}

bool CDebondInfo::operator==(const CDebondInfo& rOther) const
{
   if ( idxStrand1 != rOther.idxStrand1 )
      return false;

   if ( idxStrand2 != rOther.idxStrand2 )
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



////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CGirderData::CGirderData()
{
   ResetPrestressData();

   NumPermStrandsType = NPS_STRAIGHT_HARPED;

   Condition = pgsTypes::cfGood;
   ConditionFactor = 1.0;
}  

CGirderData::CGirderData(const CGirderData& rOther)
{
   MakeCopy(rOther);
}

CGirderData::~CGirderData()
{
}

///////////////////////////////////////////////////////
// Resets all the prestressing put to default values
void CGirderData::ResetPrestressData()
{

   for ( Uint16 i = 0; i < 4; i++ )
   {
      Nstrands[i] = 0;
      Pjack[i] = 0;
      bPjackCalculated[i] = true;
      LastUserPjack[i] = 0;

      if (i<3)
         Debond[i].clear();
   }

   TempStrandUsage    = pgsTypes::ttsPretensioned;
   bSymmetricDebond   = true;

   // convert to legacy since this is the only measurement that is sure to fit in the girder
   HsoEndMeasurement  = hsoLEGACY;
   HpOffsetAtEnd      = 0.0;
   HsoHpMeasurement   = hsoLEGACY;
   HpOffsetAtHp       = 0.0;
}

//======================== OPERATORS  =======================================
CGirderData& CGirderData::operator= (const CGirderData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CGirderData::operator==(const CGirderData& rOther) const
{
   if (NumPermStrandsType != rOther.NumPermStrandsType)
      return false;

   for ( Uint16 i = 0; i < 4; i++ )
   {
      if ( bPjackCalculated[i] != rOther.bPjackCalculated[i] )
         return false;

      if ( Nstrands[i] != rOther.Nstrands[i] )
         return false;

      if ( !IsEqual(Pjack[i],rOther.Pjack[i]) )
         return false;

      if (i<3)
      {
         if ( Debond[i] != rOther.Debond[i] )
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

   if ( Material != rOther.Material )
      return false;

   if ( bSymmetricDebond != rOther.bSymmetricDebond )
      return false;

   if ( ShearData != rOther.ShearData )
      return false;

   if ( LongitudinalRebarData != rOther.LongitudinalRebarData )
      return false;

   if ( HandlingData != rOther.HandlingData )
      return false;

   if ( Condition != rOther.Condition )
      return false;

   if ( !IsEqual(ConditionFactor,rOther.ConditionFactor) )
      return false;

   return true;
}

int CGirderData::GetChangeType(const CGirderData& rOther) const
{
   int ct = ctNone;

   if (NumPermStrandsType != rOther.NumPermStrandsType)
         ct |= ctPrestress;

   for ( int i = 0; i < 3; i++ )
   {
      if ( Material.pStrandMaterial[i] != rOther.Material.pStrandMaterial[i] )
      {
            ct |= ctStrand;
            break;
      }
   }

   // first check if prestressing changed
   Uint16 i = 0;
   for ( i = 0; i < 4; i++ )
   {
      if ( Nstrands[i] != rOther.Nstrands[i] ||
           Pjack[i] != rOther.Pjack[i] )
      {
         ct |= ctPrestress;
         break;
      }
   }

   for ( i = 0; i < 3; i++ )
   {
      if ( Debond[i] != rOther.Debond[i] )
      {
         ct |= ctPrestress;
         break;
      }

      for ( int j = 0; j < 2; j++ )
      {
         if ( NextendedStrands[i][j] != rOther.NextendedStrands[i][j] )
         {
            ct |= ctPrestress;
            break;
         }
      }
   }

   if ( bSymmetricDebond != rOther.bSymmetricDebond )
   {
      ct |= ctPrestress;
   }

   if ( !IsEqual(HpOffsetAtEnd,rOther.HpOffsetAtEnd) )
   {
      ct |= ctPrestress;
   }
   else if ( !IsEqual(HpOffsetAtHp,rOther.HpOffsetAtHp) )
   {
      ct |= ctPrestress;
   }
   else if ( HsoEndMeasurement != rOther.HsoEndMeasurement )
   {
      ct |= ctPrestress;
   }
   else if ( HsoHpMeasurement != rOther.HsoHpMeasurement )
   {
      ct |= ctPrestress;
   }
   else if ( TempStrandUsage != rOther.TempStrandUsage )
   {
      ct |= ctPrestress;
   }

   if ( Material.Type != rOther.Material.Type ) 
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.Fci,rOther.Material.Fci) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.Fc,rOther.Material.Fc) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual( Material.WeightDensity, rOther.Material.WeightDensity ) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.StrengthDensity,rOther.Material.StrengthDensity) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.MaxAggregateSize,rOther.Material.MaxAggregateSize) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.EcK1,rOther.Material.EcK1) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.EcK2,rOther.Material.EcK2) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.CreepK1,rOther.Material.CreepK1) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.CreepK2,rOther.Material.CreepK2) )
   {
      ct |= ctConcrete;
   }
    else if ( !IsEqual(Material.ShrinkageK1,rOther.Material.ShrinkageK1) )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.ShrinkageK2,rOther.Material.ShrinkageK2) )
   {
      ct |= ctConcrete;
   }
   else if ( Material.bUserEci != rOther.Material.bUserEci )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.Eci,rOther.Material.Eci) )
   {
      ct |= ctConcrete;
   }
   else if ( Material.bUserEc != rOther.Material.bUserEc )
   {
      ct |= ctConcrete;
   }
   else if ( !IsEqual(Material.Ec,rOther.Material.Ec) )
   {
      ct |= ctConcrete;
   }

   if ( !IsEqual(HandlingData.LeftLiftPoint,  rOther.HandlingData.LeftLiftPoint) ||
        !IsEqual(HandlingData.RightLiftPoint, rOther.HandlingData.RightLiftPoint) )
   {
      ct |= ctLifting;
   }


   if ( !IsEqual(HandlingData.LeadingSupportPoint,  rOther.HandlingData.LeadingSupportPoint) ||
        !IsEqual(HandlingData.TrailingSupportPoint, rOther.HandlingData.TrailingSupportPoint) )
   {
      ct |= ctShipping;
   }

   if ( Condition != rOther.Condition || !IsEqual(ConditionFactor,rOther.ConditionFactor) )
   {
      ct |= ctCondition;
   }

   if ( ShearData != rOther.ShearData )
   {
      ct |= ctStirrups;
   }

   if ( LongitudinalRebarData != rOther.LongitudinalRebarData )
   {
      ct |= ctLongitRebar;
   }

   return ct;
}



bool CGirderData::operator!=(const CGirderData& rOther) const
{
   return !operator==(rOther);
}

//======================== OPERATIONS =======================================
HRESULT CGirderData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress, 
                          Float64 fc,Float64 weightDensity,Float64 strengthDensity,Float64 maxAggSize)
{
   USES_CONVERSION;

   HRESULT hr = S_OK;

   double parentVersion;
   pStrLoad->get_ParentVersion(&parentVersion);

   CComBSTR bstrParentUnit;
   pStrLoad->get_ParentUnit(&bstrParentUnit);

   pStrLoad->BeginUnit(_T("PrestressData"));  // named this for historical reasons
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
         debond_info.Load(pStrLoad,pProgress);
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
         debond_info.Load(pStrLoad,pProgress);
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
         debond_info.Load(pStrLoad,pProgress);
         Debond[pgsTypes::Temporary].push_back(debond_info);
      }
      pStrLoad->EndUnit();
   }

   if ( version < 9 )
   {
      Material.pStrandMaterial[pgsTypes::Straight]  = 0; // not used in pre-version 9 of this data block
      Material.pStrandMaterial[pgsTypes::Harped]    = 0; // not used in pre-version 9 of this data block
      Material.pStrandMaterial[pgsTypes::Temporary] = 0; // not used in pre-version 9 of this data block
      // the Project Agent will set this value later
   }
   else if ( version < 11 )
   {
      lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

      var.Clear();
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("StrandMaterialKey"),&var);
      Int32 key = var.lVal;
      Material.pStrandMaterial[pgsTypes::Straight] = pPool->GetStrand(key);
      ATLASSERT(Material.pStrandMaterial[pgsTypes::Straight] != 0);
      Material.pStrandMaterial[pgsTypes::Harped] = Material.pStrandMaterial[pgsTypes::Straight];
      Material.pStrandMaterial[pgsTypes::Temporary] = Material.pStrandMaterial[pgsTypes::Straight];
   }
   else
   {
      lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();

      var.Clear();
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("StraightStrandMaterialKey"),&var);
      Int32 key = var.lVal;
      Material.pStrandMaterial[pgsTypes::Straight] = pPool->GetStrand(key);

      pStrLoad->get_Property(_T("HarpedStrandMaterialKey"),&var);
      key = var.lVal;
      Material.pStrandMaterial[pgsTypes::Harped] = pPool->GetStrand(key);

      pStrLoad->get_Property(_T("TemporaryStrandMaterialKey"),&var);
      key = var.lVal;
      Material.pStrandMaterial[pgsTypes::Temporary] = pPool->GetStrand(key);
   }

   if ( (bstrParentUnit == CComBSTR("Prestressing") && parentVersion < 4) ||
        (bstrParentUnit == CComBSTR("GirderTypes")  && parentVersion < 2) ||
        (bstrParentUnit == CComBSTR("Girders")      && parentVersion < 3) 
        )
   {
      var.Clear();
      var.vt = VT_R8;
      pStrLoad->get_Property(_T("Fci"), &var );
      Material.Fci = var.dblVal;

      if (3.0 <= version)
      {
         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("Fc"), &var );
         Material.Fc = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("WeightDensity"), &var );
         Material.WeightDensity = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("StrengthDensity"), &var );
         Material.StrengthDensity = var.dblVal;

         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("MaxAggregateSize"), &var );
         Material.MaxAggregateSize = var.dblVal;

      }
      else
      {
         Material.Fc               = fc;
         Material.WeightDensity    = weightDensity;
         Material.StrengthDensity  = strengthDensity;
         Material.MaxAggregateSize = maxAggSize;
      }

      if ( 4 <= version )
      {
         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("K1"), &var );
         Material.EcK1 = var.dblVal;
      }

      if ( 7 <= version )
      {
         var.Clear();
         var.vt = VT_BOOL;
         pStrLoad->get_Property(_T("UserEci"), &var );
         Material.bUserEci = (var.boolVal == VARIANT_TRUE ? true : false);

         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("Eci"), &var);
         Material.Eci = var.dblVal;

         var.Clear();
         var.vt = VT_BOOL;
         pStrLoad->get_Property(_T("UserEc"), &var );
         Material.bUserEc = (var.boolVal == VARIANT_TRUE ? true : false);

         var.Clear();
         var.vt = VT_R8;
         pStrLoad->get_Property(_T("Ec"), &var);
         Material.Ec = var.dblVal;
      }
   }

   pStrLoad->EndUnit(); // end PrestressData

   pStrLoad->get_Version(&version);

   if ( bstrParentUnit == CComBSTR("Girders") && 2.0 < version )
   {
      Material.Load(pStrLoad,pProgress);
   }

   if ( (bstrParentUnit == CComBSTR("Girders")     && 1.0 < version) ||
        (bstrParentUnit == CComBSTR("GirderTypes") && 1.0 < version)
      ) // if parent version greater than 1, then load shear and long rebar data
   {
      ShearData.Load(pStrLoad,pProgress);
      LongitudinalRebarData.Load(pStrLoad,pProgress);
      HandlingData.Load(pStrLoad,pProgress);
   }

   if ( SUCCEEDED(pStrLoad->BeginUnit(_T("Condition"))) )
   {
      var.vt = VT_I4;
      pStrLoad->get_Property(_T("ConditionFactorType"),&var);
      Condition = (pgsTypes::ConditionFactorType)(var.lVal);

      var.vt = VT_R8;
      pStrLoad->get_Property(_T("ConditionFactor"),&var);
      ConditionFactor = var.dblVal;
   
      pStrLoad->EndUnit();
   }

   return hr;
}

HRESULT CGirderData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("PrestressData"),12.0);

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
   pStrSave->BeginUnit(_T("ExtendedStrands"),1.0);
   pStrSave->BeginUnit(_T("Start"),1.0);
   pStrSave->BeginUnit(_T("Straight"),1.0);
   pStrSave->put_Property(_T("Count"),CComVariant(NextendedStrands[pgsTypes::Straight][pgsTypes::metStart].size()));
   std::vector<StrandIndexType>::iterator iter(NextendedStrands[pgsTypes::Straight][pgsTypes::metStart].begin());
   std::vector<StrandIndexType>::iterator end(NextendedStrands[pgsTypes::Straight][pgsTypes::metStart].end());
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
      debond_info.Save(pStrSave,pProgress);
   }
   pStrSave->EndUnit(); // StraightStrandDebonding


   pStrSave->BeginUnit(_T("HarpedStrandDebonding"),1.0);
   nDebondInfo = Debond[pgsTypes::Harped].size();
   pStrSave->put_Property(_T("DebondInfoCount"),CComVariant(nDebondInfo));
   for ( debond_iter = Debond[pgsTypes::Harped].begin(); debond_iter != Debond[pgsTypes::Harped].end(); debond_iter++ )
   {
      CDebondInfo& debond_info = *debond_iter;
      debond_info.Save(pStrSave,pProgress);
   }
   pStrSave->EndUnit(); // HarpedStrandDebonding


   pStrSave->BeginUnit(_T("TemporaryStrandDebonding"),1.0);
   nDebondInfo = Debond[pgsTypes::Temporary].size();
   pStrSave->put_Property(_T("DebondInfoCount"),CComVariant(nDebondInfo));
   for ( debond_iter = Debond[pgsTypes::Temporary].begin(); debond_iter != Debond[pgsTypes::Temporary].end(); debond_iter++ )
   {
      CDebondInfo& debond_info = *debond_iter;
      debond_info.Save(pStrSave,pProgress);
   }
   pStrSave->EndUnit(); // TemporaryStrandDebonding

   ///////////////// Added with data block version 11
   lrfdStrandPool* pPool = lrfdStrandPool::GetInstance();
   Int32 key = pPool->GetStrandKey(Material.pStrandMaterial[pgsTypes::Straight]);
   pStrSave->put_Property(_T("StraightStrandMaterialKey"),CComVariant(key));
   
   key = pPool->GetStrandKey(Material.pStrandMaterial[pgsTypes::Harped]);
   pStrSave->put_Property(_T("HarpedStrandMaterialKey"),CComVariant(key));
   
   key = pPool->GetStrandKey(Material.pStrandMaterial[pgsTypes::Temporary]);
   pStrSave->put_Property(_T("TemporaryStrandMaterialKey"),CComVariant(key));

   // moved out of this data block in version 10 for this data block and version 3 of parent
   //pStrSave->put_Property(_T("Fci"), CComVariant(Material.Fci ));
   //pStrSave->put_Property(_T("Fc"),               CComVariant(Material.Fc));
   //pStrSave->put_Property(_T("WeightDensity"),    CComVariant(Material.WeightDensity));
   //pStrSave->put_Property(_T("StrengthDensity"),  CComVariant(Material.StrengthDensity));
   //pStrSave->put_Property(_T("MaxAggregateSize"), CComVariant(Material.MaxAggregateSize));
   //pStrSave->put_Property(_T("K1"),               CComVariant(Material.K1));
   //pStrSave->put_Property(_T("UserEci"),          CComVariant(Material.bUserEci));
   //pStrSave->put_Property(_T("Eci"),              CComVariant(Material.Eci));
   //pStrSave->put_Property(_T("UserEc"),           CComVariant(Material.bUserEc));
   //pStrSave->put_Property(_T("Ec"),               CComVariant(Material.Ec));

   pStrSave->EndUnit(); // PrestressData

   // Moved here with version 3 of parent data block
   Material.Save(pStrSave,pProgress);

   ShearData.Save(pStrSave,pProgress);
   LongitudinalRebarData.Save(pStrSave,pProgress);
   HandlingData.Save(pStrSave,pProgress);

   pStrSave->BeginUnit(_T("Condition"),1.0);
   pStrSave->put_Property(_T("ConditionFactorType"),CComVariant(Condition));
   pStrSave->put_Property(_T("ConditionFactor"),CComVariant(ConditionFactor));
   pStrSave->EndUnit();

   return hr;
}

void CGirderData::ClearDebondData()
{
   for (long i=0; i<3; i++)
   {
      Debond[i].clear();
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

long CGirderData::GetDebondCount(pgsTypes::StrandType strandType) const
{
   if (strandType==pgsTypes::Permanent)
   {
      ATLASSERT(0); // should never be called
      return 0;
   }
   else
   {
      long count = 0;
      std::vector<CDebondInfo>::const_iterator iter;
      for ( iter = Debond[strandType].begin(); iter != Debond[strandType].end(); iter++ )
      {
         const CDebondInfo& debond_info = *iter;
         if (debond_info.idxStrand1 >= 0 )
            count++;

         if (debond_info.idxStrand2 >= 0 )
            count++;
      }

      return count;
   }
}

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CGirderData::MakeCopy(const CGirderData& rOther)
{
   CopyPrestressingFrom(rOther);
   CopyMaterialFrom(rOther);
   CopyShearDataFrom(rOther);
   CopyLongitudinalRebarFrom(rOther);
   CopyHandlingDataFrom(rOther);

   Condition = rOther.Condition;
   ConditionFactor = rOther.ConditionFactor;
}

void CGirderData::CopyPrestressingFrom(const CGirderData& rOther)
{
   HsoEndMeasurement  = rOther.HsoEndMeasurement;
   HsoHpMeasurement   = rOther.HsoHpMeasurement;
   HpOffsetAtEnd      = rOther.HpOffsetAtEnd;
   HpOffsetAtHp       = rOther.HpOffsetAtHp;

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

         for ( Uint16 j = 0; j < 2; j++ )
         {
            NextendedStrands[i][j] = rOther.NextendedStrands[i][j];
         }
      }
   }

   TempStrandUsage    = rOther.TempStrandUsage;
   bSymmetricDebond   = rOther.bSymmetricDebond;

   ShearData             = rOther.ShearData;
   LongitudinalRebarData = rOther.LongitudinalRebarData;
   HandlingData          = rOther.HandlingData;

   Material.pStrandMaterial[pgsTypes::Straight]  = rOther.Material.pStrandMaterial[pgsTypes::Straight];
   Material.pStrandMaterial[pgsTypes::Harped]    = rOther.Material.pStrandMaterial[pgsTypes::Harped];
   Material.pStrandMaterial[pgsTypes::Temporary] = rOther.Material.pStrandMaterial[pgsTypes::Temporary];
}

void CGirderData::CopyShearDataFrom(const CGirderData& rOther)
{
   ShearData = rOther.ShearData;
}

void CGirderData::CopyLongitudinalRebarFrom(const CGirderData& rOther)
{
   LongitudinalRebarData = rOther.LongitudinalRebarData;
}

void CGirderData::CopyHandlingDataFrom(const CGirderData& rOther)
{
   HandlingData = rOther.HandlingData;
}

void CGirderData::CopyMaterialFrom(const CGirderData& rOther)
{
   Material.Type              = rOther.Material.Type;
   Material.Fci               = rOther.Material.Fci;
   Material.Fc                = rOther.Material.Fc;
   Material.WeightDensity     = rOther.Material.WeightDensity;
   Material.StrengthDensity   = rOther.Material.StrengthDensity;
   Material.MaxAggregateSize  = rOther.Material.MaxAggregateSize;
   Material.EcK1              = rOther.Material.EcK1;
   Material.EcK2              = rOther.Material.EcK2;
   Material.CreepK1           = rOther.Material.CreepK1;
   Material.CreepK2           = rOther.Material.CreepK2;
   Material.ShrinkageK1       = rOther.Material.ShrinkageK1;
   Material.ShrinkageK2       = rOther.Material.ShrinkageK2;
   Material.bUserEci          = rOther.Material.bUserEci;
   Material.Eci               = rOther.Material.Eci;
   Material.bUserEc           = rOther.Material.bUserEc;
   Material.Ec                = rOther.Material.Ec;
   Material.bHasFct           = rOther.Material.bHasFct;
   Material.Fct               = rOther.Material.Fct;
}


void CGirderData::MakeAssignment(const CGirderData& rOther)
{
   MakeCopy( rOther );
}
