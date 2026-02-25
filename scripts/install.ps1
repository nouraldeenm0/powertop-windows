#Requires -Version 5.1
<#
.SYNOPSIS
    PowerTOP Windows Installer
.DESCRIPTION
    Checks for all required and optional dependencies, prompts the user before
    installing anything, builds PowerTOP from source, and optionally installs
    the binary to a directory of your choice.
.NOTES
    Must be run as Administrator.
    Supported compilers: Visual Studio Build Tools (MSVC) or MinGW-w64.
    Optional: PDCurses for the interactive ncurses UI.
#>

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

function Write-Banner {
    Write-Host ""
    Write-Host "=============================================" -ForegroundColor Cyan
    Write-Host "   PowerTOP Windows Installer" -ForegroundColor Cyan
    Write-Host "=============================================" -ForegroundColor Cyan
    Write-Host ""
}

function Write-Step {
    param([string]$Message)
    Write-Host "[*] $Message" -ForegroundColor Yellow
}

function Write-OK {
    param([string]$Message)
    Write-Host "[OK] $Message" -ForegroundColor Green
}

function Write-Missing {
    param([string]$Message)
    Write-Host "[MISSING] $Message" -ForegroundColor Red
}

function Write-Info {
    param([string]$Message)
    Write-Host "    $Message" -ForegroundColor Gray
}

function Confirm-Action {
    param([string]$Prompt)
    $answer = Read-Host "$Prompt [Y/N]"
    return ($answer -match '^[Yy]')
}

function Test-CommandExists {
    param([string]$Command)
    return ($null -ne (Get-Command $Command -ErrorAction SilentlyContinue))
}

function Get-WingetPackage {
    param([string]$Id, [string]$Name)
    Write-Step "Installing $Name via winget..."
    winget install --id $Id --silent --accept-source-agreements --accept-package-agreements
    if ($LASTEXITCODE -ne 0) {
        throw "winget failed to install $Name (exit code $LASTEXITCODE)."
    }
    # Refresh PATH for this session
    $env:PATH = [System.Environment]::GetEnvironmentVariable('PATH', 'Machine') + ';' +
                [System.Environment]::GetEnvironmentVariable('PATH', 'User')
}

function Get-ChocoPackage {
    param([string]$Id, [string]$Name)
    Write-Step "Installing $Name via Chocolatey..."
    choco install $Id -y
    if ($LASTEXITCODE -ne 0) {
        throw "Chocolatey failed to install $Name (exit code $LASTEXITCODE)."
    }
    $env:PATH = [System.Environment]::GetEnvironmentVariable('PATH', 'Machine') + ';' +
                [System.Environment]::GetEnvironmentVariable('PATH', 'User')
}

function Install-Chocolatey {
    Write-Step "Installing Chocolatey package manager..."
    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
    $env:PATH = [System.Environment]::GetEnvironmentVariable('PATH', 'Machine') + ';' +
                [System.Environment]::GetEnvironmentVariable('PATH', 'User')
}

# ---------------------------------------------------------------------------
# Admin check
# ---------------------------------------------------------------------------

function Assert-Administrator {
    $currentUser = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal   = New-Object Security.Principal.WindowsPrincipal($currentUser)
    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        Write-Host ""
        Write-Host "ERROR: This script must be run as Administrator." -ForegroundColor Red
        Write-Host "       Right-click PowerShell and choose 'Run as Administrator', then try again." -ForegroundColor Red
        Write-Host ""
        exit 1
    }
}

# ---------------------------------------------------------------------------
# Dependency detection
# ---------------------------------------------------------------------------

function Get-CMakeVersion {
    if (-not (Test-CommandExists 'cmake')) { return $null }
    try {
        $raw = (cmake --version 2>&1 | Select-Object -First 1) -as [string]
        if ($raw -match '(\d+)\.(\d+)\.(\d+)') {
            return [Version]"$($Matches[1]).$($Matches[2]).$($Matches[3])"
        }
    } catch {}
    return $null
}

function Get-CompilerInfo {
    # Check for cl.exe (MSVC)
    if (Test-CommandExists 'cl') {
        return @{ Name = 'MSVC (cl.exe)'; Type = 'msvc' }
    }
    # Check for g++ (MinGW-w64)
    if (Test-CommandExists 'g++') {
        $ver = (g++ --version 2>&1 | Select-Object -First 1) -as [string]
        return @{ Name = "MinGW-w64 g++ ($ver)"; Type = 'mingw' }
    }
    # Check if Visual Studio Build Tools are installed but not in PATH
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $vsPath = & $vsWhere -latest -property installationPath 2>$null
        if ($vsPath) {
            return @{ Name = "Visual Studio (found at $vsPath, not in PATH)"; Type = 'vs_found' }
        }
    }
    return $null
}

function Test-PDCurses {
    # Look for pdcurses.lib / libpdcurses.a in common locations
    $searchPaths = @(
        "$env:ProgramFiles\PDCurses",
        "$env:ProgramFiles(x86)\PDCurses",
        "C:\PDCurses",
        "C:\tools\pdcurses"
    )
    foreach ($p in $searchPaths) {
        if (Test-Path "$p\pdcurses.lib" -ErrorAction SilentlyContinue) { return $true }
        if (Test-Path "$p\libpdcurses.a" -ErrorAction SilentlyContinue) { return $true }
    }
    # Also try pkg-config if available
    if (Test-CommandExists 'pkg-config') {
        $result = pkg-config --exists pdcurses 2>&1
        if ($LASTEXITCODE -eq 0) { return $true }
    }
    return $false
}

# ---------------------------------------------------------------------------
# Install helpers per dependency
# ---------------------------------------------------------------------------

function Install-CMake {
    if (Test-CommandExists 'winget') {
        Get-WingetPackage -Id 'Kitware.CMake' -Name 'CMake'
    } elseif (Test-CommandExists 'choco') {
        Get-ChocoPackage -Id 'cmake' -Name 'CMake'
    } else {
        Write-Host "    Neither winget nor Chocolatey is available." -ForegroundColor Red
        Write-Host "    Please install CMake manually from: https://cmake.org/download/" -ForegroundColor Yellow
        throw "Cannot install CMake automatically."
    }
}

function Install-VSBuildTools {
    if (Test-CommandExists 'winget') {
        Write-Info "Installing Visual Studio Build Tools 2022 (C++ workload)..."
        winget install --id Microsoft.VisualStudio.2022.BuildTools --silent `
              --accept-source-agreements --accept-package-agreements `
              --override "--quiet --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
        if ($LASTEXITCODE -ne 0) {
            throw "winget failed to install Visual Studio Build Tools."
        }
        $env:PATH = [System.Environment]::GetEnvironmentVariable('PATH', 'Machine') + ';' +
                    [System.Environment]::GetEnvironmentVariable('PATH', 'User')
    } elseif (Test-CommandExists 'choco') {
        Get-ChocoPackage -Id 'visualstudio2022buildtools' -Name 'Visual Studio Build Tools 2022'
        Get-ChocoPackage -Id 'visualstudio2022-workload-vctools' -Name 'VC++ workload'
    } else {
        Write-Host "    Please install Visual Studio Build Tools manually:" -ForegroundColor Yellow
        Write-Host "    https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022" -ForegroundColor Yellow
        throw "Cannot install a C++ compiler automatically."
    }
}

function Install-MinGW {
    if (Test-CommandExists 'winget') {
        Get-WingetPackage -Id 'msys2.msys2' -Name 'MSYS2 (provides MinGW-w64)'
        Write-Host ""
        Write-Host "    MSYS2 has been installed. To finish setting up MinGW-w64:" -ForegroundColor Yellow
        Write-Host "    1. Open 'MSYS2 UCRT64' from the Start menu." -ForegroundColor Yellow
        Write-Host "    2. Run: pacman -S mingw-w64-ucrt-x86_64-gcc cmake" -ForegroundColor Yellow
        Write-Host "    3. Add C:\msys64\ucrt64\bin to your PATH." -ForegroundColor Yellow
        Write-Host "    4. Re-run this installer script." -ForegroundColor Yellow
        Write-Host ""
        throw "Please complete the MSYS2/MinGW-w64 setup (see instructions above) and re-run this script."
    } elseif (Test-CommandExists 'choco') {
        Get-ChocoPackage -Id 'mingw' -Name 'MinGW-w64'
    } else {
        Write-Host "    Please install MinGW-w64 manually from: https://www.mingw-w64.org/" -ForegroundColor Yellow
        throw "Cannot install a C++ compiler automatically."
    }
}

function Install-Git {
    if (Test-CommandExists 'winget') {
        Get-WingetPackage -Id 'Git.Git' -Name 'Git'
    } elseif (Test-CommandExists 'choco') {
        Get-ChocoPackage -Id 'git' -Name 'Git'
    } else {
        Write-Host "    Please install Git manually from: https://git-scm.com/download/win" -ForegroundColor Yellow
        throw "Cannot install Git automatically."
    }
}

# ---------------------------------------------------------------------------
# Main installation logic
# ---------------------------------------------------------------------------

function Main {
    Write-Banner

    # --- Administrator check -----------------------------------------------
    Assert-Administrator

    Write-Host "This script will:" -ForegroundColor White
    Write-Host "  1. Check for required dependencies (CMake, C++ compiler, Git)" -ForegroundColor White
    Write-Host "  2. Check for optional dependencies (PDCurses)" -ForegroundColor White
    Write-Host "  3. Ask your permission before installing anything" -ForegroundColor White
    Write-Host "  4. Build PowerTOP from source" -ForegroundColor White
    Write-Host "  5. Optionally install the built binary" -ForegroundColor White
    Write-Host ""

    if (-not (Confirm-Action "Do you want to continue?")) {
        Write-Host "Aborted." -ForegroundColor Yellow
        exit 0
    }
    Write-Host ""

    # -----------------------------------------------------------------------
    # Step 1: Check dependencies
    # -----------------------------------------------------------------------
    Write-Host "----------------------------------------------" -ForegroundColor DarkGray
    Write-Host " Checking dependencies..." -ForegroundColor White
    Write-Host "----------------------------------------------" -ForegroundColor DarkGray

    $missingRequired = [System.Collections.Generic.List[string]]::new()

    # --- CMake ---
    $cmakeVersion = Get-CMakeVersion
    $minCMake     = [Version]"3.14.0"
    if ($null -eq $cmakeVersion) {
        Write-Missing "CMake — not found"
        $missingRequired.Add('cmake')
    } elseif ($cmakeVersion -lt $minCMake) {
        Write-Missing "CMake $cmakeVersion — too old (need >= 3.14)"
        $missingRequired.Add('cmake')
    } else {
        Write-OK "CMake $cmakeVersion"
    }

    # --- C++ compiler ---
    $compilerInfo = Get-CompilerInfo
    if ($null -eq $compilerInfo) {
        Write-Missing "C++ compiler — not found (need MSVC or MinGW-w64 g++)"
        $missingRequired.Add('compiler')
    } else {
        Write-OK "C++ compiler: $($compilerInfo.Name)"
    }

    # --- Git ---
    if (-not (Test-CommandExists 'git')) {
        Write-Missing "Git — not found"
        $missingRequired.Add('git')
    } else {
        $gitVer = (git --version 2>&1) -as [string]
        Write-OK "Git ($gitVer)"
    }

    Write-Host ""
    Write-Host "----------------------------------------------" -ForegroundColor DarkGray
    Write-Host " Checking optional dependencies..." -ForegroundColor White
    Write-Host "----------------------------------------------" -ForegroundColor DarkGray

    $hasPDCurses = Test-PDCurses
    if ($hasPDCurses) {
        Write-OK "PDCurses — found (interactive UI will be available)"
    } else {
        Write-Host "[OPT] PDCurses — not found (PowerTOP will run in report-only mode)" -ForegroundColor DarkYellow
        Write-Info "PDCurses provides the interactive ncurses UI."
        Write-Info "Get it from: https://pdcurses.org/"
    }

    Write-Host ""

    # -----------------------------------------------------------------------
    # Step 2: Install missing required dependencies (with prompts)
    # -----------------------------------------------------------------------
    if ($missingRequired.Count -gt 0) {
        Write-Host "----------------------------------------------" -ForegroundColor DarkGray
        Write-Host " Missing required dependencies:" -ForegroundColor Red
        foreach ($dep in $missingRequired) { Write-Host "   - $dep" -ForegroundColor Red }
        Write-Host "----------------------------------------------" -ForegroundColor DarkGray
        Write-Host ""

        if (-not (Confirm-Action "Install all missing required dependencies now?")) {
            Write-Host ""
            Write-Host "Cannot build PowerTOP without these dependencies." -ForegroundColor Red
            Write-Host "Please install them manually and re-run this script." -ForegroundColor Yellow
            exit 1
        }
        Write-Host ""

        # Ensure we have at least one package manager
        if (-not (Test-CommandExists 'winget') -and -not (Test-CommandExists 'choco')) {
            Write-Step "No package manager (winget/choco) found."
            if (Confirm-Action "Install Chocolatey package manager?") {
                Install-Chocolatey
                Write-OK "Chocolatey installed."
            } else {
                Write-Host "Cannot install dependencies without a package manager." -ForegroundColor Red
                Write-Host "Install winget (https://aka.ms/getwinget) or Chocolatey (https://chocolatey.org) and re-run." -ForegroundColor Yellow
                exit 1
            }
        }

        foreach ($dep in $missingRequired) {
            switch ($dep) {
                'cmake'    { Install-CMake }
                'compiler' {
                    Write-Host ""
                    Write-Host "Choose a C++ compiler to install:" -ForegroundColor White
                    Write-Host "  [1] Visual Studio Build Tools 2022 (MSVC) — recommended" -ForegroundColor White
                    Write-Host "  [2] MinGW-w64 via MSYS2" -ForegroundColor White
                    Write-Host "  [3] Skip (I will install it manually)" -ForegroundColor White
                    $choice = Read-Host "Enter 1, 2, or 3"
                    switch ($choice) {
                        '1' { Install-VSBuildTools }
                        '2' { Install-MinGW }
                        '3' {
                            Write-Host "Skipped. Install a C++ compiler and re-run this script." -ForegroundColor Yellow
                            exit 1
                        }
                        default {
                            Write-Host "Invalid choice. Aborting." -ForegroundColor Red
                            exit 1
                        }
                    }
                }
                'git'      { Install-Git }
            }
            Write-OK "$dep installed."
        }

        Write-Host ""
        Write-Host "All required dependencies are installed." -ForegroundColor Green
        Write-Host ""
    } else {
        Write-Host "All required dependencies are present." -ForegroundColor Green
        Write-Host ""
    }

    # -----------------------------------------------------------------------
    # Step 3: Locate source tree
    # -----------------------------------------------------------------------
    Write-Host "----------------------------------------------" -ForegroundColor DarkGray
    Write-Host " Locating PowerTOP source..." -ForegroundColor White
    Write-Host "----------------------------------------------" -ForegroundColor DarkGray

    # If this script is inside the repo (scripts/), go up one level.
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
    $repoRoot  = $null

    if (Test-Path (Join-Path $scriptDir '..\CMakeLists.txt')) {
        $repoRoot = (Resolve-Path (Join-Path $scriptDir '..')).Path
        Write-OK "Source found at: $repoRoot"
    } elseif (Test-Path (Join-Path $PSScriptRoot 'CMakeLists.txt')) {
        $repoRoot = $PSScriptRoot
        Write-OK "Source found at: $repoRoot"
    } else {
        Write-Info "Source not found relative to this script."
        $defaultCloneDir = Join-Path $env:USERPROFILE 'powertop-windows'
        Write-Host "    Default clone directory: $defaultCloneDir" -ForegroundColor Gray
        $cloneDir = Read-Host "    Enter directory to clone into (or press Enter for default)"
        if ([string]::IsNullOrWhiteSpace($cloneDir)) { $cloneDir = $defaultCloneDir }

        if (Test-Path $cloneDir) {
            Write-Info "Directory already exists. Pulling latest changes..."
            Push-Location $cloneDir
            git pull
            Pop-Location
        } else {
            Write-Step "Cloning PowerTOP repository..."
            git clone https://github.com/nouraldeenm0/powertop-windows.git $cloneDir
        }
        $repoRoot = $cloneDir
        Write-OK "Source ready at: $repoRoot"
    }
    Write-Host ""

    # -----------------------------------------------------------------------
    # Step 4: Configure with CMake
    # -----------------------------------------------------------------------
    Write-Host "----------------------------------------------" -ForegroundColor DarkGray
    Write-Host " Configuring build..." -ForegroundColor White
    Write-Host "----------------------------------------------" -ForegroundColor DarkGray

    $buildDir = Join-Path $repoRoot 'build-win'

    # Choose generator based on available compiler
    $compilerInfo = Get-CompilerInfo
    $cmakeGenerator = $null
    $cmakeExtraArgs = @()

    if ($null -ne $compilerInfo -and $compilerInfo.Type -eq 'mingw') {
        $cmakeGenerator = 'MinGW Makefiles'
    } else {
        # Default to VS 2022; fall back gracefully if not present
        $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        if (Test-Path $vsWhere) {
            $vsVer = & $vsWhere -latest -property catalog_productLineVersion 2>$null
            switch ($vsVer) {
                '2022' { $cmakeGenerator = 'Visual Studio 17 2022' }
                '2019' { $cmakeGenerator = 'Visual Studio 16 2019' }
                '2017' { $cmakeGenerator = 'Visual Studio 15 2017' }
                default { $cmakeGenerator = 'Visual Studio 17 2022' }
            }
        } else {
            $cmakeGenerator = 'Visual Studio 17 2022'
        }
    }

    Write-Info "Build directory : $buildDir"
    Write-Info "CMake generator : $cmakeGenerator"

    if (-not (Test-Path $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir | Out-Null
    }

    Push-Location $buildDir
    try {
        $cmakeArgs = @('..', '-G', $cmakeGenerator) + $cmakeExtraArgs
        Write-Step "Running: cmake $($cmakeArgs -join ' ')"
        cmake @cmakeArgs
        if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed." }
        Write-OK "CMake configuration succeeded."
        Write-Host ""

        # -----------------------------------------------------------------------
        # Step 5: Build
        # -----------------------------------------------------------------------
        Write-Host "----------------------------------------------" -ForegroundColor DarkGray
        Write-Host " Building PowerTOP..." -ForegroundColor White
        Write-Host "----------------------------------------------" -ForegroundColor DarkGray

        if ($cmakeGenerator -match 'Visual Studio') {
            cmake --build . --config Release
        } else {
            cmake --build .
        }
        if ($LASTEXITCODE -ne 0) { throw "Build failed." }
        Write-OK "Build succeeded."
    } finally {
        Pop-Location
    }
    Write-Host ""

    # -----------------------------------------------------------------------
    # Step 6: Locate the built binary
    # -----------------------------------------------------------------------
    $binaryName = 'powertop.exe'
    $candidates = @(
        (Join-Path $buildDir "Release\$binaryName"),
        (Join-Path $buildDir $binaryName)
    )
    $builtBinary = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1

    if ($null -eq $builtBinary) {
        Write-Host "WARNING: Could not locate built $binaryName under $buildDir." -ForegroundColor DarkYellow
        Write-Host "         Build may have succeeded but binary path is non-standard." -ForegroundColor DarkYellow
    } else {
        Write-OK "Binary: $builtBinary"
    }
    Write-Host ""

    # -----------------------------------------------------------------------
    # Step 7: Optional install
    # -----------------------------------------------------------------------
    Write-Host "----------------------------------------------" -ForegroundColor DarkGray
    Write-Host " Installation (optional)" -ForegroundColor White
    Write-Host "----------------------------------------------" -ForegroundColor DarkGray

    if ($null -ne $builtBinary -and (Confirm-Action "Install PowerTOP to a directory and add it to your PATH?")) {
        $defaultInstall = Join-Path $env:ProgramFiles 'PowerTOP'
        $installDir = Read-Host "    Installation directory (Enter for default: $defaultInstall)"
        if ([string]::IsNullOrWhiteSpace($installDir)) { $installDir = $defaultInstall }

        if (-not (Test-Path $installDir)) {
            New-Item -ItemType Directory -Path $installDir | Out-Null
        }

        Copy-Item -Path $builtBinary -Destination $installDir -Force
        Write-OK "Copied $binaryName to $installDir"

        # Add to system PATH if not already present
        $currentPath = [System.Environment]::GetEnvironmentVariable('PATH', 'Machine')
        if ($currentPath -notlike "*$installDir*") {
            [System.Environment]::SetEnvironmentVariable(
                'PATH',
                "$currentPath;$installDir",
                'Machine'
            )
            # Update current session
            $env:PATH = "$env:PATH;$installDir"
            Write-OK "Added $installDir to system PATH."
            Write-Info "Open a new terminal for the PATH change to take effect."
        } else {
            Write-Info "$installDir is already in system PATH."
        }
    } else {
        if ($null -ne $builtBinary) {
            Write-Info "Skipping install. You can run PowerTOP directly from:"
            Write-Info "    $builtBinary"
            Write-Info "PowerTOP must be run as Administrator."
        }
    }

    # -----------------------------------------------------------------------
    # Done
    # -----------------------------------------------------------------------
    Write-Host ""
    Write-Host "=============================================" -ForegroundColor Cyan
    Write-Host "   PowerTOP installation complete!" -ForegroundColor Cyan
    Write-Host "=============================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Run 'powertop --help' to see available options." -ForegroundColor White
    Write-Host "Remember: PowerTOP must be run as Administrator." -ForegroundColor Yellow
    Write-Host ""
}

Main
