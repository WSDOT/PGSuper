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

// TxDOTAgentImp.cpp : Implementation of CTxDOTAgentImp

#include "stdafx.h"
#include "TxDOTAgent_i.h"
#include "TxDOTCadExporter.h"
#include "TxDOTCadWriter.h"
#include "ExportCadData.h" 

#include <EAF\EAFAutoProgress.h>
#include <IFace\Selection.h>
#include <IFace\TxDOTCadExport.h>
#include <MfcTools\XUnwind.h>

bool DoesFileExist(const CString& filename)
{
   if (filename.IsEmpty())
      return false;
   else
   {
      CFileFind finder;
      BOOL is_file;
      is_file = finder.FindFile(filename);
      return (is_file!=0);
   }
}


// CTxDOTCadExporter

/////////////////////////////////////////////////////////////////////////////
// IPGSuperExporter
STDMETHODIMP CTxDOTCadExporter::Init(UINT nCmdID)
{
   // we don't need to save our command ID
   return S_OK;
}

STDMETHODIMP CTxDOTCadExporter::GetName(BSTR*  bstrText)
{
   *bstrText = CComBSTR("TxDOT CAD Data Exporter");
   return S_OK;
}

STDMETHODIMP CTxDOTCadExporter::GetMenuText(BSTR*  bstrText)
{
   *bstrText = CComBSTR("TxDOT &CAD Data...");
   return S_OK;
}

STDMETHODIMP CTxDOTCadExporter::GetBitmapHandle(HBITMAP* phBmp)
{
   *phBmp = NULL;
   return S_OK;
}

STDMETHODIMP CTxDOTCadExporter::GetCommandHintText(BSTR*  bstrText)
{
   *bstrText = CComBSTR("Export TxDOT CAD Data\nExport TxDOT CAD Data");
   return S_OK;
}

STDMETHODIMP CTxDOTCadExporter::Export(IBroker* pBroker)
{
	//Create CAD data text file using name specified by user.			   
	//																   
	//jam.15aug2005  -  Created.								    	   
	//																   
	//User is first presented with a modal dialog for entry of span	   
	//number and girder number. If a valid span/girder id pair is        
	//entered, the the user is prompted for a filename.  When a valid    
	//filename is entered, data is gathered from various agents of the   
	//the application.  Once the data has been successfully gathered,    
	//a new file is created, and the data is written to the file		   
	//according to a strict predefined format.             		

	CString default_name = "CADexport.txt",initial_filespec, initial_dir;
	INT_PTR		stf = IDOK;
	TCHAR	strFilter[] = {_T("CAD Export Files (*.txt)|*.txt||")};

   GET_IFACE2(pBroker,ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSpanIdx();
   spanIdx = spanIdx == ALL_SPANS ? 0 : spanIdx; // default to 0
   GirderIndexType gdrIdx = pSelection->GetGirderIdx();
   gdrIdx = gdrIdx == ALL_GIRDERS ? 0 : gdrIdx;

   SpanGirderHashType hash = HashSpanGirder(spanIdx,gdrIdx);
   std::vector<SpanGirderHashType> gdrlist;
   gdrlist.push_back(hash);

	/* Create ExportCADData dialog box object */
   BOOL bIsExtended = FALSE;
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
	   exportCADData  caddlg (pBroker, NULL);
      caddlg.m_SelGdrs = gdrlist;
      caddlg.m_IsExtended = bIsExtended;

	   /* Open the ExportCADData dialog box */
	   stf = caddlg.DoModal();
	   if (stf == IDOK)
	   {
		   /* Get user's span & beam id values */
		   gdrlist = caddlg.m_SelGdrs;
         bIsExtended = caddlg.m_IsExtended;
	   }
	   else
	   {
		   /* Just do nothing if CANCEL */
	       return S_OK;
	   }
   }

	/* Create SAVEAS file dialog box object */
	CFileDialog  fildlg(FALSE,_T("txt"),default_name,OFN_HIDEREADONLY, strFilter);

	/* Open the SAVEAS dialog box */
	stf = fildlg.DoModal();
	if (stf == IDOK)
	{
		/* Get full pathname of selected file */
		CString file_path = fildlg.GetPathName();

		/* See if the file currently exists */
		if (DoesFileExist(file_path))
		{
			/* See if the user wants to overwrite file */
			CString msg(_T(" The file: "));
			msg += file_path + _T(" exists. Overwrite it?");
			int stm = AfxMessageBox(msg,MB_YESNOCANCEL|MB_ICONQUESTION);
			if (stm != IDYES) 
            return S_OK;
		}

      bool did_throw=false;

      // Create progress window in own scope
      try
      {
	      /* Create progress bar (before needing one) to remain alive during this task */
	      /* (otherwise, progress bars will be repeatedly created & destroyed on the fly) */
         GET_IFACE2(pBroker,IProgress,pProgress);

         bool multi = gdrlist.size()>1;
         DWORD mask = multi ? PW_ALL : PW_ALL|PW_NOGAUGE; // Progress window has a cancel button,
         CEAFAutoProgress ap(pProgress,0,mask); 

         if (multi)
            pProgress->Init(0,(short)gdrlist.size(),1);  // and for multi-girders, a gauge.

		   /* Open/create the specified text file */
      	FILE	*fp = NULL;
         if (_tfopen_s(&fp,LPCTSTR(file_path), _T("w+")) != 0 || fp == NULL)
         {
			   AfxMessageBox (_T("Warning: File Cannot be Created."));
			   return S_OK;
		   }

         // dialog can only ask for normal or extended format
         TxDOTCadExportFormatType format = bIsExtended ? tcxTest : tcxNormal;

	      /* Write CAD data to text file */
         for (std::vector<SpanGirderHashType>::iterator it = gdrlist.begin(); it!= gdrlist.end(); it++)
         {
            SpanIndexType span;
            GirderIndexType gdr;
            UnhashSpanGirder(*it, &span, &gdr);

	         if (CAD_SUCCESS != TxDOT_WriteCADDataToFile(fp, pBroker, span, gdr, format,true) )
            {
		         AfxMessageBox (_T("Warning: An error occured while writing to File"));
		         return S_OK;
            }

            pProgress->Increment();
         }

		   /* Close the open text file */
		   fclose (fp);

      } // autoprogress scope
      catch(...)
      {
         // must catch so progress window goes out of scope and gets destroyed
         // must rethrow to get the exception into MFC
         throw; 
      }

		/* Notify completion */
	   CString msg(_T("File: "));
	   msg += file_path + _T(" creation complete.");
      AfxMessageBox(msg,MB_ICONINFORMATION|MB_OK);

	}
   return S_OK;
}
