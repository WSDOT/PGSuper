///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2008  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#include "StdAfx.h"
#include "ChangeUnits.h"
#include "PGSuper.h"
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnChangeUnits::txnChangeUnits(CPGSuperDoc* pDoc,pgsTypes::UnitMode oldUnits,pgsTypes::UnitMode newUnits)
{
   m_pPGSuperDoc = pDoc;
   m_UnitMode[0] = oldUnits;
   m_UnitMode[1] = newUnits;
}

std::string txnChangeUnits::Name() const
{
   return "Change Units";
}

txnTransaction* txnChangeUnits::CreateClone() const
{
   return new txnChangeUnits(m_pPGSuperDoc,m_UnitMode[0],m_UnitMode[1]);
}

bool txnChangeUnits::IsUndoable()
{
   return true;
}

bool txnChangeUnits::IsRepeatable()
{
   return false;
}

bool txnChangeUnits::Execute()
{
   return DoExecute(1);
}

void txnChangeUnits::Undo()
{
   DoExecute(0);
}

bool txnChangeUnits::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   AfxGetBroker(&pBroker);

   GET_IFACE2(pBroker,IProjectSettings,pProjSettings);
   GET_IFACE2(pBroker,IEvents,pEvents);
   pEvents->HoldEvents();

   Int32 old_units = pProjSettings->GetUnitsMode();
   if ( old_units != m_UnitMode[i] )
   {
      CComPtr<IDocUnitSystem> pDocUnitSystem;
      m_pPGSuperDoc->GetDocUnitSystem(&pDocUnitSystem);

      pDocUnitSystem->put_UnitMode(UnitModeType(m_UnitMode[i]));
      pProjSettings->SetUnitsMode( m_UnitMode[i] );
   }

   pEvents->FirePendingEvents();

   return true;
}
