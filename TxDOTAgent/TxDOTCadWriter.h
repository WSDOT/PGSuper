///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
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

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#include <IFace\Bridge.h>
#include <IFace\TxDOTCadExport.h>

// Utility class for structuring debond data
#include <PgsExt\DebondUtil.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
// Utility constants, stuff for TxDOT CAD export
#define CAD_DELIM	" "
#define CAD_SPACE	" "

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   TxDOTCadWriter

   Utility class for writing TxDOT CAD file

DESCRIPTION
   Utility class for writing TxDOT CAD file

COPYRIGHT
   Copyright © 1997-2009
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 04.09.2009 : Created file
*****************************************************************************/
static const int BF_SIZ=256; // buffer size

// Main External functions that write the file
int TxDOT_WriteCADDataToFile (FILE *fp, IBroker* pBroker, SpanIndexType span, GirderIndexType gdr, TxDOTCadExportFormatType format, bool designSucceeded);
int TxDOT_WriteDistributionFactorsToFile (FILE *fp, IBroker* pBroker, SpanIndexType span, GirderIndexType gdr);


// Local utility class that does the real writing
class CadWriterWorkerBee
{
public:
   CadWriterWorkerBee(bool doWriteTitles); 

   void WriteFloat64(Float64 val, const char* title, Int16 nchars, const char* format, bool doDelim);
   void WriteInt16(Int16 val, const char* title, Int16 nchars, const char* format, bool doDelim);
   void WriteString(const char* val, const char* title, Int16 nchars, const char* format, bool doDelim);
   void WriteBlankSpaces(Int16 ns);
   void WriteToFile(FILE* fp);

private:
   bool m_DoWriteTitles;

   // The text to be written
   char m_DataLine[BF_SIZ];
   char m_TitleLine[BF_SIZ];
   char m_DashLine[BF_SIZ];

   // Cursors for location of last write
   char* m_DataLineCursor;
   char* m_TitleLineCursor;
   char* m_DashLineCursor;

   CadWriterWorkerBee(); // no default const
   void WriteTitle(const char* title, Int16 nchars, bool doDelim);

   // remaining buffer sizes for *printf_s type functions
   size_t DataBufferRemaining() const
   {
      return sizeof(*m_DataLineCursor)*(m_DataLine+BF_SIZ-m_DataLineCursor);
   }
};

// Workhorse for writing debond information

class TxDOTCadWriter : public TxDOTDebondTool
{
public:
   TxDOTCadWriter(SpanIndexType span, GirderIndexType gdr, Float64 girderLength, IStrandGeometry* pStrandGeometry):
   TxDOTDebondTool(span, gdr, girderLength, pStrandGeometry)
   {;}

   void WriteInitialData(CadWriterWorkerBee& workerBee);
   void WriteFinalData(FILE *fp, bool isExtended);
private:
   void WriteRowData(CadWriterWorkerBee& workerBee, const RowData& row, Int16 strandsInRow) const;
};

