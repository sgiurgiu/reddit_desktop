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

Invoke-CmdScript "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

$buildDir = ".\build"
$packagesDir = ".\packages"

Remove-Item -LiteralPath $buildDir -Force -Recurse
Remove-Item -LiteralPath $packagesDir -Force -Recurse
New-Item -ItemType Directory -Force -Path $buildDir
New-Item -ItemType Directory -Force -Path $packagesDir

cmake -B $buildDir -S . -G Ninja -DCMAKE_TOOLCHAIN_FILE=E:/projects/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DENABLE_TESTS=False -DCMAKE_BUILD_TYPE=Release -DCPACK_GENERATOR=WIX -DLIBMPV_DIR=E:\projects\mpv -DLIBMPV_INCLUDE=E:\projects\mpv\include -DYOUTUBE_DL=E:\projects\mpv\youtube-dl.exe 

cmake --build $buildDir -- package

Copy-Item "$($buildDir\*.msi)" -Destination $packagesDir



