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

#include "stdafx.h"
#include "TxDOTAgent_i.h"
#include "TxDOTAppPlugin.h"
#include "TxDOTOptionalDesignDocTemplate.h"
#include "TxDOTOptionalDesignDoc.h"
#include "TxDOTOptionalDesignView.h"
#include "TxDOTOptionalDesignChildFrame.h"
#include "resource.h"
#include "TxDOTCommandLineInfo.h"


BEGIN_MESSAGE_MAP(CMyCmdTarget,CCmdTarget)
END_MESSAGE_MAP()


BOOL CTxDOTAppPlugin::Init(CEAFApp* pParent)
{
   ::GXInit();

   // use manage state because we need exe's state below
   AFX_MANAGE_STATE(AfxGetAppModuleState());

   // TRICKY: Must lock temporary ole control maps in app module or the report browser window
   //         will vanish after about 10 seconds. See http://support.microsoft.com/kb/161874
   //         for a sketchy discription
   AfxLockTempMaps();

   return TRUE;
}

void CTxDOTAppPlugin::Terminate()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   // see tricky in Init
   AfxUnlockTempMaps();
}

void CTxDOTAppPlugin::IntegrateWithUI(BOOL bIntegrate)
{
   // no UI integration
}

CEAFDocTemplate* CTxDOTAppPlugin::CreateDocTemplate()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CTxDOTOptionalDesignDocTemplate* pDocTemplate = new CTxDOTOptionalDesignDocTemplate(
		IDR_TXDOTOPTIONALDESIGN,
      this,
		RUNTIME_CLASS(CTxDOTOptionalDesignDoc),
		RUNTIME_CLASS(CTxDOTOptionalDesignChildFrame), // substitute your own child frame if needed
		RUNTIME_CLASS(CTxDOTOptionalDesignView));

   pDocTemplate->SetPlugin(this);

   return pDocTemplate;
}

HMENU CTxDOTAppPlugin::GetSharedMenuHandle()
{
   return NULL;
}

UINT CTxDOTAppPlugin::GetDocumentResourceID()
{
   return IDR_TXDOTOPTIONALDESIGN;
}

CString CTxDOTAppPlugin::GetName()
{
   return CString("TOGA - TxDOT Optional Girder Analysis");
}

CString CTxDOTAppPlugin::GetUsageMessage()
{
   CTxDOTCommandLineInfo txCmdInfo;
   return txCmdInfo.GetUsageMessage();
}

BOOL CTxDOTAppPlugin::ProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo)
{
   // cmdInfo is the command line information from the application. The application
   // doesn't know about this plug-in at the time the command line parameters are parsed
   //
   // Re-parse the parameters with our own command line information object
   CTxDOTCommandLineInfo txCmdInfo;
   EAFGetApp()->ParseCommandLine(txCmdInfo);
   cmdInfo = txCmdInfo;

   // could handle processing here, but allow app class to do it
//   if (txCmdInfo.m_DoTogaTest)
//   {
//      return TRUE; // command line parameters handled
//   }

   BOOL bHandled = FALSE;
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   CEAFDocument* pDoc = pFrame->GetDocument();
   if ( pDoc )
   {
      bHandled = pDoc->ProcessCommandLineOptions(cmdInfo);
   }

   // If we get this far and there is one parameter and it isn't a file name and it isn't handled -OR-
   // if there is more than one parameter and it isn't handled there is something wrong
   if ( (1 == txCmdInfo.m_Count && txCmdInfo.m_nShellCommand != CCommandLineInfo::FileOpen) || 
        (1 <  txCmdInfo.m_Count && !bHandled ) )
   {
      cmdInfo.m_bError = TRUE;
      bHandled = TRUE;
   }

   return bHandled;
}


//////////////////////////
// IEAFCommandCallback
BOOL CTxDOTAppPlugin::OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo)
{
   return m_MyCmdTarget.OnCmdMsg(nID,nCode,pExtra,pHandlerInfo);
}

BOOL CTxDOTAppPlugin::GetStatusBarMessageString(UINT nID, CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // load appropriate string
	if ( rMessage.LoadString(nID) )
	{
		// first newline terminates actual string
      rMessage.Replace(_T('\n'),_T('\0'));
	}
	else
	{
		// not found
		TRACE1("Warning: no message line prompt for ID %d.\n", nID);
	}

   return TRUE;
}

BOOL CTxDOTAppPlugin::GetToolTipMessageString(UINT nID, CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString string;
   // load appropriate string
	if ( string.LoadString(nID) )
	{
		// tip is after first newline 
      int pos = string.Find('\n');
      if ( 0 < pos )
         rMessage = string.Mid(pos+1);
	}
	else
	{
		// not found
		TRACE1("Warning: no tool tip for ID %d.\n", nID);
	}

   return TRUE;
}
