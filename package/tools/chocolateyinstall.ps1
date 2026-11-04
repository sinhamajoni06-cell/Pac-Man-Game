$ErrorActionPreference = 'Stop'

# The placeholders are filled in by the GitHub workflow (.github/workflows/package.yml)
$packageArgs = @{
  packageName    = $env:ChocolateyPackageName
  fileType       = 'exe'
  url64bit       = 'https://github.com/__REPO__/releases/download/__TAG__/__FILE__'
  checksum64     = '__CHECKSUM__'
  checksumType64 = 'sha256'
  silentArgs     = '/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP- /ALLUSERS'
  validExitCodes = @(0)
}

Install-ChocolateyPackage @packageArgs
