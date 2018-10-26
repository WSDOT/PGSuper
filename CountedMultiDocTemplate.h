///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#if !defined CountedMultiDocTemplate_H_
#define CountedMultiDocTemplate_H_

#include "stdafx.h"

class CCountedMultiDocTemplate : public CMultiDocTemplate
{
public:
   CCountedMultiDocTemplate(UINT nIDResource,
                            CRuntimeClass* pDocClass,
                            CRuntimeClass* pFrameClass,
                            CRuntimeClass* pViewClass,
                            int maxViewCount = -1);
   virtual ~CCountedMultiDocTemplate();

   int GetMaxViewCount() const;

	virtual void InitialUpdateFrame(CFrameWnd* pFrame, CDocument* pDoc,BOOL bMakeVisible = TRUE);

   DECLARE_DYNAMIC(CCountedMultiDocTemplate)

private:
   int m_MaxViewCount; // maximum number of views that can be displayed at
                       // one time. -1 = unlimited
};

inline int CCountedMultiDocTemplate::GetMaxViewCount() const
{
   return m_MaxViewCount;
}

class CPGSuperCountedMultiDocTemplate : public CCountedMultiDocTemplate
{
public:
   CPGSuperCountedMultiDocTemplate(UINT nIDResource,
                            CRuntimeClass* pDocClass,
                            CRuntimeClass* pFrameClass,
                            CRuntimeClass* pViewClass,
                            int maxViewCount = -1);

	virtual void InitialUpdateFrame(CFrameWnd* pFrame, CDocument* pDoc,BOOL bMakeVisible = TRUE);
};

#endif // CountedMultiDocTemplate_H_