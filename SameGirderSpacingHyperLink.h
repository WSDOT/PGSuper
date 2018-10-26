///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#ifndef INCLUDED_SAMEGIRDERSPACINGHYPERLINE_H_
#define INCLUDED_SAMEGIRDERSPACINGHYPERLINE_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SameGirderSpacingHyperLink.h : header file
//

#include <MfcTools\HyperLink.h>

#define CHANGE_SAMEGIRDERSPACING _T("ChangeSameGirderSpacing")
static const UINT MsgChangeSameGirderSpacing = ::RegisterWindowMessage(CHANGE_SAMEGIRDERSPACING);


/////////////////////////////////////////////////////////////////////////////
// CSameGirderSpacingHyperLink 

class CSameGirderSpacingHyperLink : public CHyperLink
{
public:
   CSameGirderSpacingHyperLink() { m_wParam = 0; m_lParam = 0; }
   WPARAM m_wParam;
   LPARAM m_lParam;
protected:
	virtual void FollowLink();
};

#endif // INCLUDED_SAMEGIRDERSPACINGHYPERLINE_H_
