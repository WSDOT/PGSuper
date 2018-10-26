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
#include <PgsExt\DebondData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDebondData::CDebondData() :
   strandTypeGridIdx(INVALID_INDEX)
{ 
   Length[pgsTypes::metStart] = 0.0;
   Length[pgsTypes::metEnd] = 0.0;
}

HRESULT CDebondData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   pStrLoad->BeginUnit(_T("DebondInfo"));

   Float64 version;
   pStrLoad->get_Version(&version);

   CComVariant var;

   if (1.0 < version)
   {
      needsConversion = false;

      var.vt = VT_INDEX;
      pStrLoad->get_Property(_T("Strand"),    &var);
      strandTypeGridIdx = VARIANT2INDEX(var);
   }
   else
   {
      // Old version - we will need to convert to grid index in CProjectAgentImp::ConvertLegacyDebondData
      needsConversion = true;

      var.vt = VT_INDEX;
      pStrLoad->get_Property(_T("Strand1"),    &var); // save only index 1 - old version was indexed to total filled strands 
      strandTypeGridIdx = VARIANT2INDEX(var);

      pStrLoad->get_Property(_T("Strand2"),    &var);
   }

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("Length1"),      &var);
   Length[pgsTypes::metStart] = var.dblVal;

   var.vt = VT_R8;
   pStrLoad->get_Property(_T("Length2"),    &var);
   Length[pgsTypes::metEnd] = var.dblVal;

   pStrLoad->EndUnit();

   return S_OK;
}

HRESULT CDebondData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("DebondInfo"),2.0);
   pStrSave->put_Property(_T("Strand"),    CComVariant(strandTypeGridIdx));
   pStrSave->put_Property(_T("Length1"),    CComVariant(Length[pgsTypes::metStart]));
   pStrSave->put_Property(_T("Length2"),    CComVariant(Length[pgsTypes::metEnd]));
   pStrSave->EndUnit();

   return S_OK;
}

bool CDebondData::operator==(const CDebondData& rOther) const
{
   if ( strandTypeGridIdx != rOther.strandTypeGridIdx )
   {
      return false;
   }

   for ( int i = 0; i < 2; i++ )
   {
      if ( Length[i] != rOther.Length[i] )
      {
         return false;
      }
   }

   return true;
}

bool CDebondData::operator!=(const CDebondData& rOther) const
{
   return !CDebondData::operator==(rOther);
}
