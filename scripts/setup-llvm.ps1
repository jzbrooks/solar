$solar_dir = Get-Location | select -ExpandProperty Path
$llvm_version = "11.0.0"
$llvm_targets = "X86;ARM;AArch64"
$llvm_url = "https://github.com/llvm/llvm-project/archive/llvmorg-$llvm_version.zip"
$llvm_zip = "llvm-project.zip"
$llvm_project = "llvm-project-llvmorg-$llvm_version"

if (-Not (Test-Path $llvm_project -PathType Container)) {
	if (-Not (Test-Path $llvm_zip -PathType leaf)) {
		Invoke-WebRequest -Uri $llvm_url -OutFile $llvm_zip
	}

	Add-Type -AssemblyName System.IO.Compression.FileSystem
	[System.IO.Compression.ZipFile]::ExtractToDirectory($llvm_zip, $solar_dir)
}

Set-Location $llvm_project

if (Test-Path .\build -PathType Container) {
	Remove-Item -Path .\build -Recurse
}

New-Item -Path . -Name "build" -ItemType Directory
Set-Location ".\build"

cmake ..\llvm -G "Visual Studio 16 2019" -Thost=x64 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_TARGETS_TO_BUILD="$llvm_targets" -DLLVM_ENABLE_DUMP=ON -DCMAKE_INSTALL_PREFIX="$solar_dir\$llvm_project\llvm" -DLLVM_USE_CRT_RELEASE=MT -DLLVM_USE_CRT_RELWITHDEBINFO=MT -DLLVM_USE_CRT_DEBUG=MTd
cmake --build . --config RelWithDebInfo --target install

Set-Location $solar_dir