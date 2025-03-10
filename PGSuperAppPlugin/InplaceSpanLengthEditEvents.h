///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#if !defined INCLUDED_INPLACESPANLENGTHEDITEVENTS_H_
#define INCLUDED_INPLACESPANLENGTHEDITEVENTS_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "InplaceEditDisplayObjectEvents.h"

class CInplaceSpanLengthEditEvents : public CInplaceEditDisplayObjectEvents
{
public:
   CInplaceSpanLengthEditEvents(IBroker* pBroker,SpanIndexType spanIdx);

protected:
   virtual void Handle_OnChanged(iDisplayObject* pDO);

private:
   SpanIndexType m_SpanIdx;
};


#endif //INCLUDED_INPLACESPANLENGTHEDITEVENTS_H_