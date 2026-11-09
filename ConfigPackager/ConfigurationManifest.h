///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

// Reading and writing ConfigurationPackages.json, the file a configuration server publishes
// to list its configurations, plus the MD5 helpers used to tell whether a configuration changed.
//
// This file is shared by PgsExt (the programs) and ConfigPackager (the publishing tool).
// It uses only the standard library, Boost.PropertyTree and Win32, so it builds in both.
//
// Example:
// {
//   "schemaVersion": 1,
//   "applications": {
//     "PGSuper": {
//       "configurations": [
//         {
//           "name": "WSDOT",
//           "webLink": "https://wsdot.wa.gov/eesc/bridge/software/configurations/LibraryInfo.html",
//           "packages": [
//             { "appVersion": "9.0.0", "url": "PGSuper/9.0.0/WSDOT.zip", "md5": "0123456789abcdef0123456789abcdef" }
//           ]
//         }
//       ]
//     }
//   }
// }

#include <string>
#include <vector>

namespace ConfigManifest
{
   // Name of the file a configuration server publishes
   inline const wchar_t* GetManifestFileName() { return L"ConfigurationPackages.json"; }

   // Name of the legacy file, e.g. PGSuperPackages.ini
   std::wstring GetLegacyIniFileName(const std::wstring& appName);

   // Highest manifest format this code understands
   const int CurrentSchemaVersion = 1;

   struct Package
   {
      std::wstring AppVersion; // lowest program version this package is for (x.y.z)
      std::wstring Url;        // relative to the manifest, or a full https:// address or file path
      std::wstring Md5;        // optional MD5 checksum of the package (32 hex digits, lower case)
   };

   struct Configuration
   {
      std::wstring Name;
      std::wstring WebLink;
      std::vector<Package> Packages;
   };

   struct Application
   {
      std::wstring Name; // PGSuper, PGSplice, TOGA
      std::vector<Configuration> Configurations;

      Configuration* FindConfiguration(const std::wstring& name);
      const Configuration* FindConfiguration(const std::wstring& name) const;
   };

   struct Manifest
   {
      int SchemaVersion = CurrentSchemaVersion;
      std::vector<Application> Applications;

      // Application names are compared without regard to case
      Application* FindApplication(const std::wstring& name);
      const Application* FindApplication(const std::wstring& name) const;
      Application& GetOrAddApplication(const std::wstring& name);
   };

   // Reads a legacy <App>Packages.ini file (full path) into the same data model. Sections in the original
   // format (separate library file and template folder, only readable from FTP) are skipped.
   Application ReadLegacyIni(const std::wstring& fullPath, const std::wstring& appName);

   // Parses manifest text (UTF-8, optional byte order mark). Returns false and a message if the text isn't a valid manifest.
   bool ParseManifest(const std::string& utf8Text, Manifest& manifest, std::wstring& error);

   // Reads and parses a manifest file
   bool ReadManifest(const std::wstring& fileName, Manifest& manifest, std::wstring& error);

   // Writes a manifest file (UTF-8, no byte order mark)
   std::string FormatManifest(const Manifest& manifest);
   bool WriteManifest(const std::wstring& fileName, const Manifest& manifest, std::wstring& error);

   // Version strings have the form x.y.z. A fourth (build) part is accepted and ignored.
   bool ParseVersion(const std::wstring& version, unsigned int parts[3]);

   // Returns <0, 0, >0. Both versions must be valid.
   int CompareVersions(const std::wstring& v1, const std::wstring& v2);

   // Returns the package with the highest AppVersion that is not newer than programVersion, or nullptr if there is none
   const Package* FindCompatiblePackage(const Configuration& configuration, const std::wstring& programVersion);

   // True if url is a full address (has a scheme such as https://) or an absolute file path
   bool IsAbsoluteLocation(const std::wstring& url);

   // Case-insensitive helpers
   bool StartsWithNoCase(const std::wstring& str, const std::wstring& prefix);
   bool EndsWithNoCase(const std::wstring& str, const std::wstring& suffix);
   bool EqualsNoCase(const std::wstring& a, const std::wstring& b);

   // UTF-8 / UTF-16 conversion
   std::wstring FromUtf8(const std::string& str);
   std::string ToUtf8(const std::wstring& str);

   // MD5 of a file as 32 lower-case hex digits. Returns an empty string if the file can't be read.
   std::wstring ComputeFileMd5(const std::wstring& fileName);

   // True if str is 32 hex digits
   bool IsMd5String(const std::wstring& str);

   // Reads an md5deep style hash list: one "hash  path" line per file. Hashes are returned in lower case.
   // A single-line .md5 file next to a package is the same format.
   bool ReadMd5List(const std::wstring& fileName, std::vector<std::wstring>& hashes);

   // Writes a hash list for a file, or for every file in a folder and its subfolders
   bool WriteMd5List(const std::wstring& fileOrFolder, const std::wstring& listFileName, std::wstring& error);

   // True if the MD5 of the file, or of every file in the folder and its subfolders, is in the hash list
   bool IsInMd5List(const std::wstring& fileOrFolder, const std::vector<std::wstring>& hashes);
};
