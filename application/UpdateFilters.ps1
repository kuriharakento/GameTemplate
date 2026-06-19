# UpdateFilters.ps1
# Automatically synchronize vcxproj and vcxproj.filters based on physical directory structure

# $ErrorActionPreference = "Continue"

$PSScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Definition
if (-not $PSScriptRoot) {
    $PSScriptRoot = (Get-Item .).FullName
}

Write-Host "Project Root: $PSScriptRoot" -ForegroundColor Cyan

$VcxprojPath = Join-Path $PSScriptRoot "KentoCompo.vcxproj"
$FiltersPath = Join-Path $PSScriptRoot "KentoCompo.vcxproj.filters"

# Target directories to scan
$TargetDirs = @("engine", "application")

# Mapping of file extensions to MSBuild build tags
$ExtMap = @{
    ".cpp" = "ClCompile"
    ".h"   = "ClInclude"
    ".hpp" = "ClInclude"
    ".hlsl" = "FxCompile"
    ".hlsli" = "None"
}

Write-Host "Scanning physical files..." -ForegroundColor Cyan

# Scan physical files in target directories
$PhysicalFiles = @{}
foreach ($dir in $TargetDirs) {
    $dirPath = Join-Path $PSScriptRoot $dir
    if (Test-Path $dirPath) {
        $files = Get-ChildItem -Path $dirPath -Recurse -File
        foreach ($file in $files) {
            $ext = $file.Extension.ToLower()
            if ($ExtMap.ContainsKey($ext)) {
                # Generate relative path
                $relPath = $file.FullName.Replace($PSScriptRoot + "\", "")
                $relPath = $relPath -replace '/', '\'
                $PhysicalFiles[$relPath] = $ExtMap[$ext]
            }
        }
    }
}
Write-Host "Found $($PhysicalFiles.Count) source/header files physically." -ForegroundColor Green

if ($PhysicalFiles.Count -eq 0) {
    Write-Error "No physical files found! Check directories."
    exit
}

# --- 1. Update vcxproj ---
if (Test-Path $VcxprojPath) {
    Write-Host "Updating KentoCompo.vcxproj..." -ForegroundColor Cyan
    $xml = New-Object System.Xml.XmlDocument
    $xml.Load($VcxprojPath)

    # Collect existing registered file info (Include -> Tag, Metadata)
    $ExistingFiles = @{}
    $ItemGroups = $xml.SelectNodes("//*[local-name()='ItemGroup']")
    foreach ($group in $ItemGroups) {
        foreach ($tag in @("ClCompile", "ClInclude", "FxCompile", "None")) {
            $nodes = $group.SelectNodes("*[local-name()='$tag']")
            foreach ($node in $nodes) {
                $inc = $node.GetAttribute("Include")
                if ($inc) {
                    # Store child nodes (like ExcludedFromBuild metadata)
                    $meta = @{}
                    foreach ($child in $node.ChildNodes) {
                        $meta[$child.Name] = $child.InnerText
                    }
                    $ExistingFiles[$inc] = @{ Tag = $tag; Meta = $meta }
                }
            }
        }
    }

    # Merging logic
    $MergedFiles = @{}
    
    # Keep existing registrations that are outside target directories (e.g. externals\, main.cpp)
    foreach ($inc in $ExistingFiles.Keys) {
        $isInTarget = $false
        foreach ($target in $TargetDirs) {
            if ($inc.StartsWith("$target\")) {
                $isInTarget = $true
                break
            }
        }
        if (-not $isInTarget) {
            $MergedFiles[$inc] = $ExistingFiles[$inc]
        }
    }

    # Add newly scanned physical files
    foreach ($inc in $PhysicalFiles.Keys) {
        $meta = @{}
        if ($ExistingFiles.ContainsKey($inc)) {
            $meta = $ExistingFiles[$inc].Meta
        }
        $MergedFiles[$inc] = @{ Tag = $PhysicalFiles[$inc]; Meta = $meta }
    }

    # Remove existing ItemGroups containing files
    $groupsToRemove = @()
    foreach ($group in $ItemGroups) {
        $hasFile = $false
        foreach ($tag in @("ClCompile", "ClInclude", "FxCompile", "None")) {
            if ($group.SelectNodes("*[local-name()='$tag']").Count -gt 0) {
                $hasFile = $true
                break
            }
        }
        if ($hasFile) {
            $groupsToRemove += $group
        }
    }
    foreach ($group in $groupsToRemove) {
        if ($group.ParentNode) {
            $group.ParentNode.RemoveChild($group) | Out-Null
        }
    }

    # Recreate clean ItemGroups
    $SortedKeys = $MergedFiles.Keys | Sort-Object
    $CompileGroup = $xml.CreateElement("ItemGroup", "http://schemas.microsoft.com/developer/msbuild/2003")
    $HeaderGroup = $xml.CreateElement("ItemGroup", "http://schemas.microsoft.com/developer/msbuild/2003")
    $ShaderGroup = $xml.CreateElement("ItemGroup", "http://schemas.microsoft.com/developer/msbuild/2003")
    $OtherGroup = $xml.CreateElement("ItemGroup", "http://schemas.microsoft.com/developer/msbuild/2003")

    foreach ($key in $SortedKeys) {
        $tag = $MergedFiles[$key].Tag
        $meta = $MergedFiles[$key].Meta

        $elem = $xml.CreateElement($tag, "http://schemas.microsoft.com/developer/msbuild/2003")
        $elem.SetAttribute("Include", $key)

        if ($meta -ne $null) {
            foreach ($metaKey in $meta.Keys) {
                $metaChild = $xml.CreateElement($metaKey, "http://schemas.microsoft.com/developer/msbuild/2003")
                $metaChild.InnerText = $meta[$metaKey]
                $elem.AppendChild($metaChild) | Out-Null
            }
        }

        switch ($tag) {
            "ClCompile" { $CompileGroup.AppendChild($elem) | Out-Null }
            "ClInclude" { $HeaderGroup.AppendChild($elem) | Out-Null }
            "FxCompile" { $ShaderGroup.AppendChild($elem) | Out-Null }
            default { $OtherGroup.AppendChild($elem) | Out-Null }
        }
    }

    # Insert before the last Import targets element
    $lastImport = $xml.SelectSingleNode("//*[local-name()='Import'][contains(@Project, 'targets')]")
    if ($lastImport -and $lastImport.ParentNode) {
        $lastImport.ParentNode.InsertBefore($CompileGroup, $lastImport) | Out-Null
        $lastImport.ParentNode.InsertBefore($HeaderGroup, $lastImport) | Out-Null
        $lastImport.ParentNode.InsertBefore($ShaderGroup, $lastImport) | Out-Null
        $lastImport.ParentNode.InsertBefore($OtherGroup, $lastImport) | Out-Null
    } else {
        $xml.DocumentElement.AppendChild($CompileGroup) | Out-Null
        $xml.DocumentElement.AppendChild($HeaderGroup) | Out-Null
        $xml.DocumentElement.AppendChild($ShaderGroup) | Out-Null
        $xml.DocumentElement.AppendChild($OtherGroup) | Out-Null
    }

    # Save vcxproj
    $xml.Save($VcxprojPath)
    Write-Host "KentoCompo.vcxproj updated successfully!" -ForegroundColor Green
} else {
    Write-Error "Error: KentoCompo.vcxproj not found at $VcxprojPath"
}

# --- 2. Update vcxproj.filters ---
if (Test-Path $FiltersPath) {
    Write-Host "Updating KentoCompo.vcxproj.filters..." -ForegroundColor Cyan
    $xml = New-Object System.Xml.XmlDocument
    $xml.Load($FiltersPath)

    # Collect existing UUID map
    $UuidMap = @{}
    $ExistingFilters = $xml.SelectNodes("//*[local-name()='Filter']")
    foreach ($f in $ExistingFilters) {
        $inc = $f.GetAttribute("Include")
        $ui = $f.SelectSingleNode("*[local-name()='UniqueIdentifier']")
        if ($inc -and $ui -and $ui.InnerText) {
            $UuidMap[$inc] = $ui.InnerText
        }
    }

    # Remove existing filter elements for reconstruction
    $ItemGroups = $xml.SelectNodes("//*[local-name()='ItemGroup']")
    $groupsToRemove = @()
    foreach ($group in $ItemGroups) {
        $groupsToRemove += $group
    }
    foreach ($group in $groupsToRemove) {
        if ($group.ParentNode) {
            $group.ParentNode.RemoveChild($group) | Out-Null
        }
    }

    # 2.1 Recreate filter paths ItemGroup
    $FilterPaths = @()
    foreach ($key in $MergedFiles.Keys) {
        $dirname = Split-Path $key
        if ($dirname) {
            $parts = $dirname.Split('\')
            for ($i = 1; $i -le $parts.Length; $i++) {
                $fp = [string]::Join('\', $parts[0..($i-1)])
                if ($FilterPaths -notcontains $fp) {
                    $FilterPaths += $fp
                }
            }
        }
    }
    $FilterPaths = $FilterPaths | Sort-Object

    $FilterGroup = $xml.CreateElement("ItemGroup", "http://schemas.microsoft.com/developer/msbuild/2003")
    foreach ($fp in $FilterPaths) {
        $fNode = $xml.CreateElement("Filter", "http://schemas.microsoft.com/developer/msbuild/2003")
        $fNode.SetAttribute("Include", $fp)
        
        $uiNode = $xml.CreateElement("UniqueIdentifier", "http://schemas.microsoft.com/developer/msbuild/2003")
        if ($UuidMap.ContainsKey($fp)) {
            $uiNode.InnerText = $UuidMap[$fp]
        } else {
            # Generate new UUID
            $uiNode.InnerText = [guid]::NewGuid().ToString("B").ToUpper()
        }
        $fNode.AppendChild($uiNode) | Out-Null
        $FilterGroup.AppendChild($fNode) | Out-Null
    }
    $xml.DocumentElement.AppendChild($FilterGroup) | Out-Null

    # 2.2 Recreate file item filters
    $CompileGroup = $xml.CreateElement("ItemGroup", "http://schemas.microsoft.com/developer/msbuild/2003")
    $HeaderGroup = $xml.CreateElement("ItemGroup", "http://schemas.microsoft.com/developer/msbuild/2003")
    $ShaderGroup = $xml.CreateElement("ItemGroup", "http://schemas.microsoft.com/developer/msbuild/2003")
    $OtherGroup = $xml.CreateElement("ItemGroup", "http://schemas.microsoft.com/developer/msbuild/2003")

    foreach ($key in $SortedKeys) {
        $tag = $MergedFiles[$key].Tag
        $dirname = Split-Path $key

        $elem = $xml.CreateElement($tag, "http://schemas.microsoft.com/developer/msbuild/2003")
        $elem.SetAttribute("Include", $key)

        if ($dirname) {
            $fChild = $xml.CreateElement("Filter", "http://schemas.microsoft.com/developer/msbuild/2003")
            $fChild.InnerText = $dirname
            $elem.AppendChild($fChild) | Out-Null
        }

        switch ($tag) {
            "ClCompile" { $CompileGroup.AppendChild($elem) | Out-Null }
            "ClInclude" { $HeaderGroup.AppendChild($elem) | Out-Null }
            "FxCompile" { $ShaderGroup.AppendChild($elem) | Out-Null }
            default { $OtherGroup.AppendChild($elem) | Out-Null }
        }
    }

    $xml.DocumentElement.AppendChild($CompileGroup) | Out-Null
    $xml.DocumentElement.AppendChild($HeaderGroup) | Out-Null
    $xml.DocumentElement.AppendChild($ShaderGroup) | Out-Null
    $xml.DocumentElement.AppendChild($OtherGroup) | Out-Null

    # Save filters file
    $xml.Save($FiltersPath)
    Write-Host "KentoCompo.vcxproj.filters updated successfully!" -ForegroundColor Green
} else {
    Write-Error "Error: KentoCompo.vcxproj.filters not found at $FiltersPath"
}

Write-Host "Visual Studio projects synchronized successfully!" -ForegroundColor Green
