///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#ifndef INCLUDED_SAMEGIRDERTYPEHYPERLINE_H_
#define INCLUDED_SAMEGIRDERTYPEHYPERLINE_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SameGirderTypeHyperLink.h : header file
//

#include <MfcTools\HyperLink.h>

#define CHANGE_SAMEGIRDERTYPE _T("ChangeSameGirderType")
static const UINT MsgChangeSameGirderType = ::RegisterWindowMessage(CHANGE_SAMEGIRDERTYPE);


/////////////////////////////////////////////////////////////////////////////
// CSameGirderTypeHyperLink 

class CSameGirderTypeHyperLink : public CHyperLink
{
public:
   CSameGirderTypeHyperLink() { m_wParam = 0; m_lParam = 0; }
   WPARAM m_wParam;
   LPARAM m_lParam;
protected:
	virtual void FollowLink();
};

#endif // INCLUDED_SAMEGIRDERTYPEHYPERLINE_H_
