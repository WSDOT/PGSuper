///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include <PgsExt\CastDeckActivity.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCastDeckActivity::CCastDeckActivity()
{
   m_CuringDuration = 14.0; // days (WSDOT Std. Specs 6-02.3(11)B2
   m_Age = 14.0; // days
   m_bEnabled = false;
}

CCastDeckActivity::CCastDeckActivity(const CCastDeckActivity& rOther)
{
   MakeCopy(rOther);
}

CCastDeckActivity::~CCastDeckActivity()
{
}

CCastDeckActivity& CCastDeckActivity::operator= (const CCastDeckActivity& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CCastDeckActivity::operator==(const CCastDeckActivity& rOther) const
{
   if ( m_bEnabled != rOther.m_bEnabled )
   {
      return false;
   }

   if ( !IsEqual(m_Age,rOther.m_Age) )
   {
      return false;
   }

   if ( !IsEqual(m_CuringDuration,rOther.m_CuringDuration) )
   {
      return false;
   }

   return true;
}

bool CCastDeckActivity::operator!=(const CCastDeckActivity& rOther) const
{
   return !operator==(rOther);
}

void CCastDeckActivity::Enable(bool bEnable)
{
   m_bEnabled = bEnable;
}

bool CCastDeckActivity::IsEnabled() const
{
   return m_bEnabled;
}

void CCastDeckActivity::SetConcreteAgeAtContinuity(Float64 age)
{
   m_Age = age;
}

Float64 CCastDeckActivity::GetConcreteAgeAtContinuity() const
{
   return m_Age;
}

void CCastDeckActivity::SetCuringDuration(Float64 duration)
{
   m_CuringDuration = duration;
}

Float64 CCastDeckActivity::GetCuringDuration() const
{
   return m_CuringDuration;
}

HRESULT CCastDeckActivity::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit(_T("CastDeck"));

      Float64 version;
      pStrLoad->get_Version(&version);

      CComVariant var;
      var.vt = VT_BOOL;
      hr = pStrLoad->get_Property(_T("Enabled"),&var);
      m_bEnabled = (var.boolVal == VARIANT_TRUE ? true : false);

      if ( m_bEnabled )
      {
         var.vt = VT_R8;
         hr = pStrLoad->get_Property(_T("AgeAtContinuity"),&var);
         m_Age = var.dblVal;

         if ( 1 < version )
         {
            // added in version 2
            hr = pStrLoad->get_Property(_T("CuringDuration"),&var);
            m_CuringDuration = var.dblVal;
         }
         else
         {
            m_CuringDuration = m_Age;
         }
      }

      pStrLoad->EndUnit();
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   };

   return S_OK;
}

HRESULT CCastDeckActivity::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("CastDeck"),2.0);
   pStrSave->put_Property(_T("Enabled"),CComVariant(m_bEnabled));
   if ( m_bEnabled )
   {
      pStrSave->put_Property(_T("AgeAtContinuity"),CComVariant(m_Age));
      pStrSave->put_Property(_T("CuringDuration"),CComVariant(m_CuringDuration)); // added in version 2
   }
   pStrSave->EndUnit();

   return S_OK;
}

void CCastDeckActivity::MakeCopy(const CCastDeckActivity& rOther)
{
   m_bEnabled = rOther.m_bEnabled;
   m_Age      = rOther.m_Age;
   m_CuringDuration = rOther.m_CuringDuration;
}

void CCastDeckActivity::MakeAssignment(const CCastDeckActivity& rOther)
{
   MakeCopy(rOther);
}
