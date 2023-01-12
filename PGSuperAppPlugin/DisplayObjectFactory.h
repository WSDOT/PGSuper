///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#if !defined(AFX_DISPLAYOBJECTFACTORY_H__D4D8B50C_0A5F_431C_8D9A_F6DBACB22A89__INCLUDED_)
#define AFX_DISPLAYOBJECTFACTORY_H__D4D8B50C_0A5F_431C_8D9A_F6DBACB22A89__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DisplayObjectFactory.h : header file
//

#include <DManip\DManip.h>

class CPGSDocBase;

/////////////////////////////////////////////////////////////////////////////
// CDisplayObjectFactory command target

class CDisplayObjectFactory : public CCmdTarget
{
public:
	CDisplayObjectFactory(CPGSDocBase* pDoc);

// Attributes
public:

// Operations
public:


// Implementation
protected:
	CDisplayObjectFactory();

	virtual ~CDisplayObjectFactory();

   DECLARE_INTERFACE_MAP()

   // iDisplayObjectFactory Implementation
   BEGIN_INTERFACE_PART(Factory,iDisplayObjectFactory)
      STDMETHOD_(void,Create)(CLIPFORMAT cfFormat,COleDataObject* pDataObject,iDisplayObject** dispObj);
   END_INTERFACE_PART(Factory)

private:
   CComPtr<iDisplayObjectFactory> m_Factory;
   CPGSDocBase* m_pDoc;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DISPLAYOBJECTFACTORY_H__D4D8B50C_0A5F_431C_8D9A_F6DBACB22A89__INCLUDED_)
