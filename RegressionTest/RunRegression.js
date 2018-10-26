///////////////////////////////////////////////////
// 
// RunRegression.js 
// A script for running PGSuper regression tests.
// Run with /? to get help.
//
//
///////////////////////////////////////////////////

// first some defaults
// first some defaults
var PGSuperVersion = "Debug";
var DoSendEmail=false;
var EmailAddress = new String;
var ExecuteCommands=true; // if false, only show commands

var startDate = new Date();
var startTime = startDate.getTime();

// hack for convenience:
// set up drives so pickings' and brice's machines work without command line options
var PGSuperDrive = "C:";
var objNet = new ActiveXObject( "WScript.Network" );
var machine = objNet.ComputerName;
machine = machine.toUpperCase();
if (machine=="RDPTHINKPAD")
   PGSuperDrive = "C:";
else if (machine=="HQA7326026")
   PGSuperDrive = "C:";

var wsShell = new ActiveXObject("WScript.Shell");
var FSO = new ActiveXObject("Scripting.FileSystemObject");

// parse the command line and set options
var st = ParseCommandLine();
if (st!=0)
{
   CleanUpTest();
   WScript.Quit(st);
}

// global vars
var currSpan="All";
var currGirder="EI";

var OldCacheLibraryPathRegistrySetting = new String;
var NewLibraryPathRegistrySetting = new String(PGSuperDrive+"\\Arp\\PGSuper\\RegressionTest\\PGSuperRegTestLibrary.lbr");

var Application = PGSuperDrive+"\\ARP\\PGSuper\\RegFreeCOM\\"+PGSuperVersion+"\\PGSuper.exe ";
var StartFolderSpec = new String( PGSuperDrive+"\\ARP\\PGSuper\\RegressionTest" );
var CurrFolderSpec  = new String( PGSuperDrive+"\\ARP\\PGSuper\\RegressionTest\\Current" );
var DatumFolderSpec = new String( PGSuperDrive+"\\ARP\\PGSuper\\RegressionTest\\Datum" );

var ErrorsExist = "FALSE";

// make sure pgsuper.exe exists
if (!FSO.FileExists(Application))
{
   DisplayMessage("Error - Program File: "+Application+" does not exist - Script Terminated");
   CleanUpTest();
   WScript.Quit(1);
}

if (!FSO.FileExists(NewLibraryPathRegistrySetting))
{
   DisplayMessage("Error - Library File: "+NewLibraryPathRegistrySetting+" does not exist - Script Terminated");
   CleanUpTest();
   WScript.Quit(1);
}

// First clean up results from any old runs and set up environment
var CurrentFolder = StartFolderSpec;
if (ExecuteCommands)
{
    InitTest(CurrentFolder);
}

var CurrCommand="TestR";
// DisplayMessage("Before RunTest, Command is " + CurrCommand);
RunTest(CurrentFolder,CurrCommand);

if(ExecuteCommands)
{
    CheckResults(CurrentFolder);
    CleanUpTest();
}

var st = 0;
if (!ErrorsExist)
{
   DisplayMessage("Test Successful - no errors");
   st = 1;
}

var endDate = new Date();
var endTime = endDate.getTime();
var elapsed = (endTime-startTime)/60000.0;
DisplayMessage("Elapsed Time was: "+elapsed+" Minutes");
  
      
WScript.Quit(st);

// --------------------- functions ------------------
function RunTest (currFolder, currCommand) 
{
   // find all .pgs files and run pgsuper on them
   var Folder = FSO.GetFolder(currFolder);
   var ff = new Enumerator(Folder.SubFolders);
   for (; !ff.atEnd(); ff.moveNext())
   {
      var folderName = new String(ff.item());

      // Don't walk into forbidden 
      if (IsForbiddenFolder(folderName))
      {
         continue;
      }

      var subFolder = FSO.GetFolder(ff.item());

      // Need to parse current command here if in folder name
      var newCommand = ParseCommandFromFolderName(currCommand, subFolder.Name);

      var fc = new Enumerator(subFolder.Files);
      for (; !fc.atEnd(); fc.moveNext())
      {
        s = new String(fc.item());

        idx = s.indexOf(".pgs");
        if (-1 != idx)
        {
           // Get span and girder from file name
           var newSG = new SpanGirder();
           newSG = ParseGirderFromFileName(s, currSpan, currGirder);

           var outFile= new String;
           if (newCommand!="TestR")
           {
              outFile = s.substring(0,idx) + "@" + newCommand + "_" + newSG.m_Span + "_" + newSG.m_Girder + ".Test";
              cmd = Application + " /" + newCommand + " " + s + " " + outFile + " " + newSG.m_Span + " " + newSG.m_Girder;
           }
           else
           {
              // Strip output file name when 1250 is used (1250's generate their own file names)
              outFile = "";
              cmd = Application + " /" + newCommand + " " + s; 
           }

           if(ExecuteCommands)
           {
               DisplayMessage("Running: "+ cmd);
               DisplayMessage("");
               st = wsShell.Run(cmd,1,"TRUE");
           }
           else
           {
               DisplayMessage(cmd);
           }
        }
     }

     RunTest(subFolder,newCommand);
   }
}

function InitTest (currFolder) 
{
   // clean up temporary files 
   CleanFolder(currFolder);
   
   // get current path for master library and replace it with local value
   OldCacheLibraryPathRegistrySetting = wsShell.RegRead("HKEY_CURRENT_USER\\Software\\Washington State Department of Transportation\\PGSuper\\Options\\MasterLibraryCache");
   wsShell.RegWrite("HKEY_CURRENT_USER\\Software\\Washington State Department of Transportation\\PGSuper\\Options\\MasterLibraryCache",
                    NewLibraryPathRegistrySetting);
}

function CleanFolder(currFolder)
{
   var Folder = FSO.GetFolder(currFolder);
   var ff = new Enumerator(Folder.SubFolders);
   for (; !ff.atEnd(); ff.moveNext())
   {
      var folderName = new String(ff.item());

      // Don't walk into forbidden folders
      if (IsForbiddenFolder(folderName))
      {
         continue;
      }

      var subFolder = FSO.GetFolder(ff.item());

      var fc = new Enumerator(subFolder.Files);
      for (; !fc.atEnd(); fc.moveNext())
      {
        s = new String(fc.item());
        s = s.toLowerCase();
        if ( -1!=s.indexOf(".err") || -1!=s.indexOf(".test") || -1!=s.indexOf(".dbr") || -1!=s.indexOf(".dbp") || -1!=s.indexOf(".log") )
        {
           FSO.DeleteFile(s);
        }

      }

      // recurse to subfolders
      CleanFolder(subFolder);
   }
}

function CleanUpTest() 
{
   var wsHShell
   wsShell = new ActiveXObject("WScript.Shell");
   
   // restore registry to where it was before we started
   wsShell.RegWrite("HKEY_CURRENT_USER\\Software\\Washington State Department of Transportation\\PGSuper\\Options\\MasterLibraryCache",
                OldCacheLibraryPathRegistrySetting);
}

function CheckResults(currFolder) 
{
   // first check to see if any .err files exist.
   // clean up empty error files 

   var Folder = FSO.GetFolder(currFolder);
   var ff = new Enumerator(Folder.SubFolders);
   for (; !ff.atEnd(); ff.moveNext())
   {
      var folderName = new String(ff.item());

      // Don't walk into forbidden folders
      if (IsForbiddenFolder(folderName))
      {
         continue;
      }

      var subFolder = FSO.GetFolder(ff.item());

      var fc = new Enumerator(subFolder.Files);
      for (; !fc.atEnd(); fc.moveNext())
      {
        s = new String(fc.item());
        s = s.toLowerCase();
        if ( -1!=s.indexOf(".err") )
        {
           fil = fc.item();
           if (fil.size == 0)
           {
               FSO.DeleteFile(fil);
           }
           else
           {
               ErrorsExist="TRUE";
               DisplayMessage("Test Failed - .err files exist in: "+subFolder);
               continue;
           }
        }

      }

      CheckResults(subFolder) 
   }
  
}


function ParseCommandFromFolderName(currCommand, folderName)
{
  var idx = folderName.indexOf("-")
  if ( -1 != idx)
  {
     // We have a -, so parse command to end of name
     var s = folderName.substr(idx+1); 
     s = s.toUpperCase();

     var s3 = s.substr(0,3); // take first three characters to check for Tx commands
     var s4 = s.substr(0,4); // take first four characters to check for Tx commands
     var cmd;

     if (s4=="TXAT")
     {
        cmd = "TxAT";
     }
     else if (s4=="TXDT")
     {
        cmd = "TxDT";
     }
     else if (s4=="TXDX")
     {
        cmd = "TxDx";
     }
     else if (s=="TESTDF")
     {
        cmd = "TestDF";
     }
     else if (s3=="125")
     {
        cmd = "TestR"; // only full regression for now
     }
     else
     {
         DisplayMessage("Error - Invalid Folder Name, could not parse command: "+s);
         CleanUpTest();
         WScript.Quit(1);
     }

     return cmd;
     
  }
  else
  {
     // No new command, use old one
     return currCommand;
  }
}

// Use bizarre javascript syntax to create a spanGirder combination so our function below has something to return
function SpanGirder()
{
   this.m_Span = "Error";
   this.m_Girder = "Error";
}


function ParseGirderFromFileName(fileName, currSpan, currGirder)
{
  // Our defaults if nothing is parsed
  var newSpanGirder = new SpanGirder();
  newSpanGirder.m_Span = currSpan;
  newSpanGirder.m_Girder = currGirder;

  // First strip path from fileName
  var slashIdx = fileName.lastIndexOf("\\");
  if ( -1 != slashIdx)
  {
     fileName = fileName.substring(slashIdx, fileName.length);
  }

  var firstDashIdx = fileName.indexOf("-")
  if ( -1 != firstDashIdx)
  {
     var secondDashIdx = fileName.indexOf("-",firstDashIdx+1);
     var firstDotIdx = fileName.indexOf(".");
     
     // span is first
     var spanName = new String;
     if (-1 != secondDashIdx)
     {
         spanName = fileName.substring(firstDashIdx+1,secondDashIdx);
     }
     else
     {
         spanName = fileName.substring(firstDashIdx+1,firstDotIdx);
     }

     spanName = spanName.toUpperCase();
     if (spanName=="ALL")
     {
        newSpanGirder.m_Span = "ALL";
     }
     else
     {
         // span must be a number
         var spanNum = new Number(spanName);
         if (!isNaN(spanNum))
         {
            newSpanGirder.m_Span = spanNum;
         }
         else
         {
            DisplayMessage("Error - Could not parse span number from File Name: "+fileName);
            CleanUpTest();
            WScript.Quit(1);
         }
     }

     // Go after girder name next
     if (-1 != secondDashIdx)
     {
         var girderName = new String;
         girderName = fileName.substring(secondDashIdx+1,firstDotIdx);
         girderName = girderName.toUpperCase();

         if (girderName=="ALL")
         {
            newSpanGirder.m_Girder = "ALL"
         }
         else if (girderName=="EI")
         {
            newSpanGirder.m_Girder = "EI";
         }
         else
         {
            // valid girder names at this time are A-Z
            if (girderName.length==1 && girderName>="A" && girderName<="Z")
            {
               newSpanGirder.m_Girder = girderName;
            }
            else
            {
               DisplayMessage("Error - Could not parse girder number from File Name: "+fileName);
               CleanUpTest();
               WScript.Quit(1);
            }
         }
     }
  }

  return newSpanGirder;
}

function ParseCommandLine()
{
   var didDrive=0; // drive on command line?
   var didVersion=0; // Release or Debug on command line?
   var objArgs = WScript.Arguments;
   var I;
   for (I=0; I<objArgs.Count(); I++)
   {
      var s = new String(objArgs(I));
        
      if (s.charAt(0)=="/" || s.charAt(0)=="-")
      {
       if (s.charAt(1)=="?")
       {
         DisplayMessage("--- Help for Test.js - Regression Testing Script ---");
         DisplayMessage("Command Line options:");
         DisplayMessage("    /?             - Help (you are here)");
         DisplayMessage("    /D<drive-name> - Drive where PGSuper is installed (e.g., /DC:)");
         DisplayMessage("    /V<version>    - Version of PGSuper to test (either \"Debug\" or \"Release\"");
         DisplayMessage("    /N             - No execute. Display but do not execute pgsuper commands.");
         DisplayMessage("");
         return 1;
       }
       else if (s.charAt(1)=="D" || s.charAt(1)=="d")
       {
          var dr = s.substring(2,s.length);
          dr = dr.toUpperCase();
          if (dr.length==1 && dr.charAt(1)!=":")
          {
             DisplayMessage("Invalid format for Drive Letter - must be in form C:");
             DisplayMessage("String was: \""+dr+"\"");
             return 1;
          }
          PGSuperDrive = dr;
          didDrive=1;          
       }
       else if (s.charAt(1)=="V" || s.charAt(1)=="v")
       {
          var ver = s.substring(2,s.length);
          ver = ver.toUpperCase();
          if (ver=="DEBUG" || ver=="RELEASE")
          {
             PGSuperVersion = ver;
             didVersion=1;
          }
          else
          {
             DisplayMessage("Invalid PGSuper version on command line - must be either Release or Debug");
             DisplayMessage("String was: \""+ver+"\"");
             return 1;
          }
       }
       else if (s.charAt(1)=="S" || s.charAt(1)=="s")
       {
          // set up for sending email
          EmailAddress = s.substring(2,s.length);
          DoSendEmail = true;
       }
       else if (s.charAt(1)=="N" || s.charAt(1)=="n")
       {
          // No execute. Display but do not execute commands.
          ExecuteCommands=false;
       }
       else
       {
         DisplayMessage("Error - Bad Command line parameter"+s);
         return 1;
       }
     }
     else
     {
        DisplayMessage("Error - Bad Command line parameter: \""+s+"\"");
        return 1;
     }
   }
   
   return 0;
}

function SendEmail(status)
{
// latch onto Outlook - error will be catastrophic
var ol = new ActiveXObject("Outlook.Application");

// Return a reference to the MAPI layer
var ns = ol.GetNamespace("MAPI");

// Create a new mail message item
var newMail = ol.CreateItem(0); // olMailItem from typelib - dangerous, but only way we can play in jscript

// Add the subject of the mail message
if (st==0)
   newMail.Subject = "PGSuper Regression Test - Passed";
else
   newMail.Subject = "PGSuper Regression Test - Failed";

// Create some body text
newMail.Body = "See attached diff file for more information";

// Add  recipient and test to make sure that the address is valid using the Resolve method
var rec = newMail.Recipients.Add(EmailAddress);
rec.Type = 1; // olTo
if (! rec.Resolve )
{
    DisplayMessage( "Unable to resolve address.");
}

// Attach diff file to message
var attch = newMail.Attachments.Add(DiffFile, 1); //olByValue 
attch.DisplayName = "Diff File";

//Send the mail message
newMail.Send();
}

function IsForbiddenFolder(folderName)
{
   // Don't walk into cvs or special folders
   folderName = folderName.toUpperCase();
   var matchcvs = folderName.match("CVS");
   var matchdtm = folderName.match("DATUM");
   var match125 = folderName.match("1250_Diff");
   if (null!=matchcvs || null!=matchdtm || null!=match125)
   {
      return true;
   }

   return false;
}

function DisplayMessage(msg)
{
   // could make this modal and add dialog-type messages for interactive mode
   WScript.Echo(msg);
}

