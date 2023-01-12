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
// Utility constants, stuff for TxDOT CAD export
#define CAD_DELIM	_T(" ")
#define CAD_SPACE	_T(" ")

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   TestFileWriter

   Utility class for writing .test files

DESCRIPTION
   Utility class for writing .test files

LOG
   rdp : 04.09.2009 : Created file in txdotagent
   rdp : 06.19.2019 : moved to testagent
*****************************************************************************/
static const int BF_SIZ=1024; // buffer size

// Main External functions that write the file
int Test_WriteCADDataToFile (FILE *fp, IBroker* pBroker, const CGirderKey& girderKey, bool designSucceeded);
int Test_WriteDistributionFactorsToFile (FILE *fp, IBroker* pBroker, const CGirderKey& girderKey);


// Local utility class that does the real writing
class CadWriterWorkerBee
{
public:
   enum Justification {jLeft, jCenter, jRight};

   CadWriterWorkerBee(bool doWriteTitles); 

   void WriteFloat64(Float64 val, LPCTSTR title, Int16 colWidth, Int16 nChars, LPCTSTR format);
   void WriteInt16(Int16 val, LPCTSTR title, Int16 colWidth, Int16 nchars, LPCTSTR format);
   void WriteInt32(Int32 val, LPCTSTR title, Int16 colWidth, Int16 nchars, LPCTSTR format);
   void WriteString(LPCTSTR val, LPCTSTR title, Int16 colWidth, Int16 nchars, LPCTSTR format);
   void WriteStringEx(LPCTSTR val, LPCTSTR title, Int16 lftPad, Int16 nchars, Int16 rhtPad, LPCTSTR format);
   void WriteBlankSpaces(Int16 ns);
   void WriteBlankSpacesNoTitle(Int16 ns);
   void WriteToFile(FILE* fp);

private:
   bool m_DoWriteTitles;

   // The text to be written
   TCHAR m_DataLine[BF_SIZ];
   TCHAR m_TitleLine[BF_SIZ];
   TCHAR m_DashLine[BF_SIZ];

   // Cursors for location of last write
   LPTSTR m_DataLineCursor;
   LPTSTR m_TitleLineCursor;
   LPTSTR m_DashLineCursor;

   CadWriterWorkerBee(); // no default const
   void WriteTitle(LPCTSTR title, Int16 colWidth);

   // remaining buffer sizes for *printf_s type functions
   size_t DataBufferRemaining() const
   {
      return sizeof(*m_DataLineCursor)*(m_DataLine+BF_SIZ-m_DataLineCursor)/sizeof(TCHAR);
   }
};

// Workhorse for writing debond information

class TestFileWriter : public TxDOTDebondTool
{
public:

   TestFileWriter(const CSegmentKey& segmentKey, Float64 girderLength, bool isUBeam, IStrandGeometry* pStrandGeometry):
   TxDOTDebondTool(segmentKey, girderLength, pStrandGeometry), 
   m_isUBeam(isUBeam)
   {;}

   void WriteInitialData(CadWriterWorkerBee& workerBee);
   void WriteFinalData(FILE *fp, bool isExtended, bool isIBeam, Int16 extraSpacesForSlabOffset);

   StrandIndexType GetTotalNumDebonds() const
   {
      return this->m_NumDebonded;
   }

private:
   void WriteRowData(CadWriterWorkerBee& workerBee, const RowData& row,Float64 Hg) const;
   bool m_isUBeam;
};

