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

#pragma once

#include <PgsExt\PgsExtExp.h>
#include "pgsExt\BaseCommandLineInfo.h"

/*****************************************************************************
CLASS 
   CPGSuperCommandLineInfo

   Custom command line parser for PGSuper


DESCRIPTION
   Custorm command line parser for special pgsuper command line options

LOG
   rdp : 09.24.1999 : Created file
*****************************************************************************/
class  CPGSuperCommandLineInfo : public CPGSBaseCommandLineInfo
{
public:
   CPGSuperCommandLineInfo();

private:
   virtual LPCTSTR GetAppName() const override;
};

class  CPGSuperProjectImporterCommandLineInfo : public CPGSProjectImporterBaseCommandLineInfo
{
public:
   CPGSuperProjectImporterCommandLineInfo();

private:
   virtual LPCTSTR GetAppName() const override;
};