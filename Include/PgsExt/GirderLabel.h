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

#pragma once
#include <PgsExt\PgsExtExp.h>

#include <string>

#define LABEL_GIRDER(_g_) pgsGirderLabel::GetGirderLabel(_g_).c_str()
#define LABEL_SPAN(_s_) (Int16)(_s_ + 1)
#define LABEL_PIER(_p_) (Int16)(_p_ + 1)

class PGSEXTCLASS pgsGirderLabel
{
public:
   static std::_tstring GetGirderLabel(GirderIndexType gdrIdx);
   static bool UseAlphaLabel();
   static bool UseAlphaLabel(bool bUseAlpha);

private:
   static bool ms_bUseAlpha;
   pgsGirderLabel(void);
   ~pgsGirderLabel(void);
};
