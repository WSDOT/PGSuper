///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"

#include "LoadsViewControllerImp.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CLoadsViewController::CLoadsViewController()
{
   m_pFrame = nullptr;
   m_pView = nullptr;
}

CLoadsViewController::~CLoadsViewController()
{
}

void CLoadsViewController::Init(CEditLoadsChildFrame* pFrame, IEAFViewController* pStandardController)
{
   m_pFrame = pFrame;
   CView* pView = m_pFrame->GetActiveView();
   ATLASSERT(pView->IsKindOf(RUNTIME_CLASS(CEditLoadsView)));
   m_pView = (CEditLoadsView*)pView;
   m_pStdController = pStandardController;
}

//////////////////////////////////////////
// IEAFViewController
bool CLoadsViewController::IsOpen() const
{
   return m_pStdController->IsOpen();
}

void CLoadsViewController::Close()
{
   m_pStdController->Close();
}

void CLoadsViewController::Minimize()
{
   m_pStdController->Minimize();
}

void CLoadsViewController::Maximize()
{
   m_pStdController->Maximize();
}

void CLoadsViewController::Restore()
{
   m_pStdController->Restore();
}

//////////////////////////////////////////
// ILoadsViewController
void CLoadsViewController::SortBy(Field field,Direction direction)
{
   if (IsOpen())
   {
      m_pView->SortBy((CEditLoadsView::Field)field, (CEditLoadsView::Direction)direction);
   }
}

IndexType CLoadsViewController::GetLoadCount() const
{
   if (IsOpen())
   {
      return m_pView->GetLoadCount();
   }
   else
   {
      return 0;
   }
}

std::_tstring CLoadsViewController::GetFieldValue(IndexType idx, Field field) const
{
   if (IsOpen())
   {
      return m_pView->GetFieldValue(idx, (CEditLoadsView::Field)field);
   }
   else
   {
      return _T("");
   }
}

void CLoadsViewController::DeleteLoad(IndexType idx)
{
   if (IsOpen())
   {
      m_pView->DeleteLoad(idx);
   }
}
