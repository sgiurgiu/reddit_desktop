param(
    [string]
    $libMpvDir = "E:\projects\mpv",
    [string]
    $libMpvIncludeDir = "E:\projects\mpv\include",    
    [string]
    $youtubeDlExe = "E:\projects\mpv\youtube-dl.exe"
)


# Invokes a Cmd.exe shell script and updates the environment.
function Invoke-CmdScript {
  param(
    [String] $scriptName
  )
  $cmdLine = """$scriptName"" $args & set"
  & $Env:SystemRoot\system32\cmd.exe /c $cmdLine |
  select-string '^([^=]*)=(.*)$' | foreach-object {
    $varName = $_.Matches[0].Groups[1].Value
    $varValue = $_.Matches[0].Groups[2].Value
    set-item Env:$varName $varValue
  }
}

Invoke-CmdScript "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

$buildDir = ".\build-release"
$packagesDir = ".\packages"

Remove-Item -LiteralPath $buildDir -Force -Recurse -ErrorAction Ignore
Remove-Item -LiteralPath $packagesDir -Force -Recurse -ErrorAction Ignore
New-Item -ItemType Directory -Force -Path $buildDir
New-Item -ItemType Directory -Force -Path $packagesDir
set-item Env:DISTRIBUTION "windows"
set-item Env:libMpvDir "$libMpvDir"
set-item Env:libMpvIncludeDir "$libMpvIncludeDir"
set-item Env:youtubeDlExe "$youtubeDlExe"


echo "Using lib mpv directory: $libMpvDir"
echo "Using lib mpv include directory: $libMpvIncludeDir"
echo "Using youtubeDlExe directory: $youtubeDlExe"

try {
  cmake --workflow --preset=release-windows

  Copy-Item "$buildDir\*.msi" -Destination $packagesDir
  exit 0
}catch {
  Write-Host "Error building: $($_.Exception.Message)"
  exit 1
}



