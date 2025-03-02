#define Version GetEnv('VERSION')
#define ProjectName GetEnv('PROJECT_NAME')
#define ProductName GetEnv('PRODUCT_NAME')
#define Publisher GetEnv('COMPANY_NAME')
#define Year GetDateTimeString("yyyy","","")

; 'Types': What get displayed during the setup
[Types]
Name: "full"; Description: "Standalone application"

; Components are used inside the script and can be composed of a set of 'Types'
[Components]
Name: "standalone"; Description: "Standalone application"; Types: full

[Setup]
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
AppName={#ProductName}
OutputBaseFilename={#ProductName}-{#Version}-Windows
AppCopyright=Copyright (C) {#Year} {#Publisher}
AppPublisher={#Publisher}
AppVersion={#Version}
DefaultDirName="{commonpf64}\{#Publisher}\{#ProductName}"
DisableDirPage=yes
DisableTypesPage=yes

; MAKE SURE YOU READ/MODIFY THE EULA BEFORE USING IT
LicenseFile="resources\EULA"
UninstallFilesDir="{commonappdata}\{#ProductName}\uninstall"

; MSVC adds a .ilk when building the plugin. Let's not include that.
[Files]
Source: "..\Builds\{#ProjectName}_artefacts\Release\{#ProductName}.exe"; DestDir: "{commonpf64}\{#Publisher}\{#ProductName}"; Flags: ignoreversion; Components: standalone
Source: "..\Builds\Release\SDL3.dll"; DestDir: "{commonpf64}\{#Publisher}\{#ProductName}"; Flags: ignoreversion; Components: standalone

[Icons]
Name: "{autoprograms}\{#ProductName}"; Filename: "{commonpf64}\{#Publisher}\{#ProductName}\{#ProductName}.exe"; Components: standalone
Name: "{autoprograms}\Uninstall {#ProductName}"; Filename: "{uninstallexe}"
