///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#include <IFace\Bridge.h>
#include <IFace\TestFileExport.h>

// Utility class for structuring debond data
#include <PgsExt\DebondUtil.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class CTxDataExporter;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   TxDOTCadWriter

   Utility class for writing TxDOT CAD file

DESCRIPTION
   Utility class for writing TxDOT CAD file

LOG
   rdp : 04.09.2009 : Created file
*****************************************************************************/

class TxDOTCadWriter
{
public:
   TxDOTCadWriter():
      m_RowNum(0), m_NonStandardCnt(0)
   {;}

   // Determine what type of strand layout we have
   enum txcwStrandLayout { tslStraight, tslHarped, tslDebondedIBeam, tslMixed };

   // Non-standard string can either be put into a separate table (excel) or integrated into the main table (txt)
   enum txcwNsTableLayout { ttlnTwoTables, ttlnSingleTable };

   txcwStrandLayout GetStrandLayoutType(IBroker* pBroker, const std::vector<CGirderKey>& girderKeys);

   // Main External function to write the file
   int WriteCADDataToFile(CTxDataExporter& rDataExporter, IBroker* pBroker, const CGirderKey& girderKey, txcwStrandLayout strandLayout, txcwNsTableLayout tableLayout);

private:
   Uint32 m_RowNum;
   Uint32 m_NonStandardCnt;
};


