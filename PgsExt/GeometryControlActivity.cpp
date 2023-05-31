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
#include <PgsExt\GeometryControlActivity.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CGeometryControlActivity::CGeometryControlActivity()
{
   m_ActivityType = pgsTypes::gcaDisabled;
}

CGeometryControlActivity::CGeometryControlActivity(const CGeometryControlActivity& rOther)
{
   MakeCopy(rOther);
}

CGeometryControlActivity::~CGeometryControlActivity()
{
}

CGeometryControlActivity& CGeometryControlActivity::operator= (const CGeometryControlActivity& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CGeometryControlActivity::operator==(const CGeometryControlActivity& rOther) const
{
   if (m_ActivityType != rOther.m_ActivityType)
   {
      return false;
   }

   return true;
}

bool CGeometryControlActivity::operator!=(const CGeometryControlActivity& rOther) const
{
   return !operator==(rOther);
}

bool CGeometryControlActivity::IsEnabled() const
{
   return m_ActivityType != pgsTypes::gcaDisabled;
}

void CGeometryControlActivity::Clear()
{
   m_ActivityType = pgsTypes::gcaDisabled;
}

void CGeometryControlActivity::SetGeometryControlEventType(pgsTypes::GeometryControlActivityType type)
{
   m_ActivityType = type;
}

pgsTypes::GeometryControlActivityType CGeometryControlActivity::GetGeometryControlEventType() const
{
   return m_ActivityType;
}

bool CGeometryControlActivity::IsGeometryControlEvent() const
{
   return m_ActivityType == pgsTypes::gcaGeometryControlEvent;
}

bool CGeometryControlActivity::IsSpecCheck() const
{
   // control event always gets check
   return m_ActivityType == pgsTypes::gcaGeometryControlEvent || m_ActivityType == pgsTypes::gcaSpecCheckEvent;
}

bool CGeometryControlActivity::IsReport() const
{
   // control event and spec check are always reported
   return IsEnabled();
}

HRESULT CGeometryControlActivity::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit(_T("GeometryControlActivity"));

      Float64 version;
      pStrLoad->get_Version(&version);

      CComVariant var;
      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("ActivityType"),&var);
      m_ActivityType = (pgsTypes::GeometryControlActivityType)var.lVal;

      pStrLoad->EndUnit();
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   };

   return S_OK;
}

HRESULT CGeometryControlActivity::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   pStrSave->BeginUnit(_T("GeometryControlActivity"),1.0);
   pStrSave->put_Property(_T("ActivityType"),CComVariant(m_ActivityType));
   pStrSave->EndUnit();

   return S_OK;
}

void CGeometryControlActivity::MakeCopy(const CGeometryControlActivity& rOther)
{
   m_ActivityType = rOther.m_ActivityType;
}

void CGeometryControlActivity::MakeAssignment(const CGeometryControlActivity& rOther)
{
   MakeCopy(rOther);
}

void CGeometryControlActivity::Update()
{
   // Nothing to do here now
}