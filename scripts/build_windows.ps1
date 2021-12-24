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

$buildDir = ".\build"
$packagesDir = ".\packages"

Remove-Item -LiteralPath $buildDir -Force -Recurse -ErrorAction Ignore
Remove-Item -LiteralPath $packagesDir -Force -Recurse -ErrorAction Ignore
New-Item -ItemType Directory -Force -Path $buildDir
New-Item -ItemType Directory -Force -Path $packagesDir



echo "Using lib mpv directory: $libMpvDir"
echo "Using lib mpv include directory: $libMpvIncludeDir"
echo "Using youtubeDlExe directory: $youtubeDlExe"

    

cmake -B $buildDir -S . -G Ninja -DCMAKE_TOOLCHAIN_FILE=E:/projects/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DENABLE_TESTS=False -DCMAKE_BUILD_TYPE=Release -DCPACK_GENERATOR=WIX -DLIBMPV_DIR="$libMpvDir" -DLIBMPV_INCLUDE="$libMpvIncludeDir" -DYOUTUBE_DL="$youtubeDlExe"

cmake --build $buildDir -- -j4 package

Copy-Item "$buildDir\*.msi" -Destination $packagesDir



