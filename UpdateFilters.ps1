param(
    [string[]]$ProjectPath,
    [switch]$All
)

$ErrorActionPreference = "Stop"

$msbuildNamespace = "http://schemas.microsoft.com/developer/msbuild/2003"
$itemTags = @(
    "ClCompile",
    "ClInclude",
    "FxCompile",
    "None",
    "ResourceCompile",
    "Image",
    "Text",
    "Natvis"
)

function Get-DefaultProjects {
    $projects = @(
        "application\KentoCompo.vcxproj",
        "engine\KentoCompoEngine.vcxproj"
    )

    return $projects | Where-Object { Test-Path -LiteralPath $_ }
}

function Get-DeterministicGuid {
    param([Parameter(Mandatory = $true)][string]$Text)

    $md5 = [System.Security.Cryptography.MD5]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
        $hash = $md5.ComputeHash($bytes)
        return [guid]::new($hash).ToString("B").ToUpperInvariant()
    }
    finally {
        $md5.Dispose()
    }
}

function Convert-ToFilterName {
    param([Parameter(Mandatory = $true)][string]$Include)

    $normalized = $Include -replace "/", "\"

    while ($normalized.StartsWith("..\")) {
        $normalized = $normalized.Substring(3)
    }

    $directory = Split-Path -Path $normalized -Parent
    if ([string]::IsNullOrWhiteSpace($directory)) {
        return $null
    }

    return $directory
}

function Get-ProjectItems {
    param([Parameter(Mandatory = $true)][xml]$ProjectXml)

    $items = New-Object System.Collections.Generic.List[object]

    foreach ($tag in $itemTags) {
        $nodes = @($ProjectXml.SelectNodes("//*[local-name()='$tag']"))
        foreach ($node in $nodes) {
            $include = $node.GetAttribute("Include")
            if ([string]::IsNullOrWhiteSpace($include)) {
                continue
            }

            $items.Add([pscustomobject]@{
                Tag = $tag
                Include = ($include -replace "/", "\")
                Filter = Convert-ToFilterName $include
            })
        }
    }

    return $items | Sort-Object Tag, Include
}

function Get-FilterAncestors {
    param([Parameter(Mandatory = $true)][string]$Filter)

    $parts = $Filter -split "\\"
    $result = New-Object System.Collections.Generic.List[string]

    for ($i = 0; $i -lt $parts.Count; $i++) {
        $result.Add(($parts[0..$i] -join "\"))
    }

    return $result
}

function New-MsbuildElement {
    param(
        [Parameter(Mandatory = $true)][System.Xml.XmlDocument]$Document,
        [Parameter(Mandatory = $true)][string]$Name
    )

    return $Document.CreateElement($Name, $msbuildNamespace)
}

function Write-FiltersFile {
    param(
        [Parameter(Mandatory = $true)][string]$VcxprojPath
    )

    $resolvedProject = Resolve-Path -LiteralPath $VcxprojPath
    $projectFile = $resolvedProject.Path
    $filtersFile = "$projectFile.filters"

    [xml]$projectXml = Get-Content -LiteralPath $projectFile
    $projectItems = @(Get-ProjectItems $projectXml)

    $filterSet = New-Object "System.Collections.Generic.HashSet[string]"
    foreach ($item in $projectItems) {
        if ([string]::IsNullOrWhiteSpace($item.Filter)) {
            continue
        }

        foreach ($ancestor in Get-FilterAncestors $item.Filter) {
            [void]$filterSet.Add($ancestor)
        }
    }

    $filtersXml = New-Object System.Xml.XmlDocument
    $declaration = $filtersXml.CreateXmlDeclaration("1.0", "utf-8", $null)
    [void]$filtersXml.AppendChild($declaration)

    $root = New-MsbuildElement $filtersXml "Project"
    $root.SetAttribute("ToolsVersion", "4.0")
    [void]$filtersXml.AppendChild($root)

    $filterGroup = New-MsbuildElement $filtersXml "ItemGroup"
    foreach ($filter in ($filterSet | Sort-Object)) {
        $filterNode = New-MsbuildElement $filtersXml "Filter"
        $filterNode.SetAttribute("Include", $filter)

        $identifierNode = New-MsbuildElement $filtersXml "UniqueIdentifier"
        $identifierNode.InnerText = Get-DeterministicGuid $filter

        [void]$filterNode.AppendChild($identifierNode)
        [void]$filterGroup.AppendChild($filterNode)
    }
    [void]$root.AppendChild($filterGroup)

    foreach ($tag in $itemTags) {
        $itemsForTag = @($projectItems | Where-Object { $_.Tag -eq $tag })
        if ($itemsForTag.Count -eq 0) {
            continue
        }

        $itemGroup = New-MsbuildElement $filtersXml "ItemGroup"

        foreach ($item in $itemsForTag) {
            $itemNode = New-MsbuildElement $filtersXml $item.Tag
            $itemNode.SetAttribute("Include", $item.Include)

            if (-not [string]::IsNullOrWhiteSpace($item.Filter)) {
                $filterNode = New-MsbuildElement $filtersXml "Filter"
                $filterNode.InnerText = $item.Filter
                [void]$itemNode.AppendChild($filterNode)
            }

            [void]$itemGroup.AppendChild($itemNode)
        }

        [void]$root.AppendChild($itemGroup)
    }

    $filtersXml.Save($filtersFile)
    Write-Host "Generated: $filtersFile"
}

if (-not $ProjectPath -or $ProjectPath.Count -eq 0) {
    $ProjectPath = Get-DefaultProjects
}

if ($All) {
    $ProjectPath = Get-ChildItem -Path . -Recurse -Filter "*.vcxproj" |
        Where-Object { $_.FullName -notmatch "\\externals\\" } |
        ForEach-Object { $_.FullName }
}

foreach ($project in $ProjectPath) {
    if (-not (Test-Path -LiteralPath $project)) {
        throw "Project file not found: $project"
    }

    Write-FiltersFile $project
}
