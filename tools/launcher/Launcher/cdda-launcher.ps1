#requires -Version 5.1
<#
    CDDA Fork Launcher
    ------------------
    Downloads builds of Cataclysm: Dark Days Ahead and installs them into YOUR
    folder (the folder above this Launcher folder). Two build sources:
      - Xbro10 tester builds: the fork's "testing-build-*" GitHub releases
        (CDDA experimental plus Backrooms Expanded and Survivor Minds). Default.
      - Official CDDA experimental: CleverRaven/Cataclysm-DDA releases.
    It also checks the fork's testing branch for a newer copy of this launcher.

    The source lives in the fork at tools/launcher/ (branch dev, shipped on testing).

        <root>\Game\            the installed game (replaced on every update)
        <root>\Game_previous\   the build before that (for rolling back)
        <root>\UserData\        saves, config, templates, user mods  (never touched by updates)
        <root>\Downloads\       cached build zips
        <root>\Backups\         save backups

    The game is started with --userdir pointing at UserData, so updates can
    never overwrite saves, settings or mods.

    Start it with "CDDA Fork Launcher.exe" in the root folder (or the older "Start CDDA Launcher.bat").
#>
param(
    [switch]$SelfTest,          # run non-GUI checks and print results
    [string]$Screenshot = '',   # open the window, wait for the build list, save a PNG, close
    [string]$Page = '',         # open on this page: game, mods, saves, settings (used with -Screenshot)
    [string]$Theme = '',        # use this theme for this run only: black, blue, light
    [string]$RootOverride = '', # use another root folder (for testing)
    [switch]$InstallNewest,     # command line: install the newest build from the chosen source, no window
    [string]$AddMod = ''        # command line: install a mod from a GitHub link, no window
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
Add-Type -AssemblyName System.Windows.Forms, System.Drawing, System.IO.Compression, System.IO.Compression.FileSystem, Microsoft.VisualBasic
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

# ---------------------------------------------------------------- paths
$script:LauncherDir = $PSScriptRoot
$script:Root = if ($RootOverride) { $RootOverride } else { Split-Path $PSScriptRoot -Parent }
$script:GameDir     = Join-Path $Root 'Game'
$script:PrevDir     = Join-Path $Root 'Game_previous'
$script:StagingDir  = Join-Path $Root '_staging'
$script:UserDir     = Join-Path $Root 'UserData'
$script:UserModsDir = Join-Path $UserDir 'mods'
$script:DownloadDir = Join-Path $Root 'Downloads'
$script:BackupDir   = Join-Path $Root 'Backups'
$script:SettingsFile = if ($RootOverride) { Join-Path $Root 'settings.json' } else { Join-Path $LauncherDir 'settings.json' }
$script:LauncherVersion = '2026-11-15.3'
# Where a newer launcher is published (the testing branch is what testers get).
$script:LauncherUpdateUrl = 'https://raw.githubusercontent.com/Xbro10/Cataclysm-DDA/testing/tools/launcher/Launcher/cdda-launcher.ps1'
$script:LauncherExeName = 'CDDA Fork Launcher.exe'
$script:LauncherHome = Split-Path $PSScriptRoot -Parent
$script:LauncherAssets = @('Launcher/launcher.ico', 'Launcher/launcher-icon.png', $LauncherExeName)

# Colour themes. Accent is the PLAY button and the selected page; Gold matches the icon.
$script:Themes = [ordered]@{
    black = @{
        Label = 'Black'; Dark = $true
        Back = '#121212'; Side = '#0A0A0A'; Surface = '#1B1B1B'; Raised = '#262626'; Border = '#333333'
        Text = '#E8E8E8'; Muted = '#9A9A9A'; Accent = '#CFC46F'; AccentText = '#25231A'; Hover = '#2E2E2E'; Warn = '#E0A040'
    }
    blue = @{
        Label = 'Dark blue'; Dark = $true
        Back = '#0E1726'; Side = '#09111D'; Surface = '#132036'; Raised = '#1A2B47'; Border = '#26395A'
        Text = '#DCE6F2'; Muted = '#8FA3BE'; Accent = '#4A90E2'; AccentText = '#FFFFFF'; Hover = '#20345A'; Warn = '#F0B050'
    }
    light = @{
        Label = 'Light'; Dark = $false
        Back = '#F3F3F3'; Side = '#E3E3E3'; Surface = '#FFFFFF'; Raised = '#E9E9E9'; Border = '#C8C8C8'
        Text = '#1E1E1E'; Muted = '#666666'; Accent = '#2E7D32'; AccentText = '#FFFFFF'; Hover = '#D6D6D6'; Warn = '#B85C00'
    }
}
$script:Sources = [ordered]@{
    fork = @{
        Label = 'Xbro10 tester builds'; Repo = 'Xbro10/Cataclysm-DDA'; TagLike = 'testing-build-*'
        GfxPattern = '^CDDA-Xbro10-testing-.*\.zip$'; SndPattern = ''
        Title = 'Cataclysm: DDA  -  Xbro10 tester builds'
    }
    official = @{
        Label = 'Official CDDA experimental'; Repo = 'CleverRaven/Cataclysm-DDA'; TagLike = 'cdda-experimental-*'
        GfxPattern = '^cdda-windows-with-graphics-x64-.*\.zip$'; SndPattern = '^cdda-windows-with-graphics-and-sounds-x64-.*\.zip$'
        Title = 'Cataclysm: Dark Days Ahead  -  experimental builds'
    }
}
$script:ExeName = 'cataclysm-tiles.exe'
$script:ProcName = 'cataclysm-tiles'
$script:UA = 'cdda-fork-launcher'

foreach ($d in @($UserDir, $UserModsDir, $DownloadDir)) {
    if (-not (Test-Path -LiteralPath $d)) { New-Item -ItemType Directory -Force -Path $d | Out-Null }
}

# ---------------------------------------------------------------- settings
function Get-DefaultModSource {
    # Only developers have a fork checkout; testers get the mods inside the tester build.
    $candidates = @('D:\dev\Cataclysm-DDA\data\mods', (Join-Path $Root 'Cataclysm-DDA-master\data\mods'))
    foreach ($p in $candidates) { if (Test-Path -LiteralPath $p) { return $p } }
    return ''
}

function Read-Settings {
    $s = @{
        Source = 'fork'; Variant = 'graphics'; ModSource = (Get-DefaultModSource); CloseOnPlay = $false; ModOrigins = @{}
        Theme = 'black'; CheckBuildsOnStart = $true; CheckLauncherOnStart = $true
    }
    if (Test-Path -LiteralPath $SettingsFile) {
        try {
            $j = Get-Content -LiteralPath $SettingsFile -Raw | ConvertFrom-Json
            if ($j.Source -and $script:Sources.Contains([string]$j.Source)) { $s.Source = [string]$j.Source }
            if ($j.Theme -and $script:Themes.Contains([string]$j.Theme)) { $s.Theme = [string]$j.Theme }
            if ($null -ne $j.CheckBuildsOnStart) { $s.CheckBuildsOnStart = [bool]$j.CheckBuildsOnStart }
            if ($null -ne $j.CheckLauncherOnStart) { $s.CheckLauncherOnStart = [bool]$j.CheckLauncherOnStart }
            if ($j.Variant)   { $s.Variant = [string]$j.Variant }
            if ($j.ModSource) { $s.ModSource = [string]$j.ModSource }
            if ($null -ne $j.CloseOnPlay) { $s.CloseOnPlay = [bool]$j.CloseOnPlay }
            if ($j.ModOrigins) {
                foreach ($prop in $j.ModOrigins.PSObject.Properties) { $s.ModOrigins[$prop.Name] = [string]$prop.Value }
            }
        } catch { }
    }
    return $s
}

function Save-Settings {
    try { $script:Settings | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $SettingsFile -Encoding UTF8 } catch { }
}

$script:Settings = Read-Settings

# ---------------------------------------------------------------- small helpers
function Get-ShortTag([string]$tag) { return ($tag -replace '^cdda-experimental-', '' -replace '^testing-build-', 'tester ') }
function Get-Source { return $script:Sources[$script:Settings.Source] }
function Get-FetchParams {
    $src = Get-Source
    return @{ Repo = $src.Repo; TagLike = $src.TagLike; GfxPattern = $src.GfxPattern; SndPattern = $src.SndPattern }
}
# Which source an installed tag came from ('' if neither, e.g. a hand-copied build).
function Get-TagSource([string]$tag) {
    foreach ($k in $script:Sources.Keys) { if ($tag -like $script:Sources[$k].TagLike) { return $k } }
    return ''
}

# A downloaded launcher must parse as PowerShell before it replaces this one.
function Test-LauncherScript([string]$text) {
    $tokens = $null; $errors = $null
    [void][System.Management.Automation.Language.Parser]::ParseInput($text, [ref]$tokens, [ref]$errors)
    return (@($errors).Count -eq 0 -and $text -match 'CDDA Fork Launcher')
}

# Inside a git checkout (tools\launcher\Launcher) the file is the source copy, so never self-update it.
function Test-InRepo { return (Test-Path -LiteralPath (Join-Path $LauncherDir '..\..\..\.git')) }

function Install-LauncherUpdate([string]$text) {
    $self = Join-Path $LauncherDir 'cdda-launcher.ps1'
    Copy-Item -LiteralPath $self -Destination "$self.bak" -Force
    # UTF-8 with a BOM: Windows PowerShell 5.1 reads BOM-less files as ANSI.
    [System.IO.File]::WriteAllText($self, $text.TrimStart([char]0xFEFF), (New-Object System.Text.UTF8Encoding($true)))
}

# Files that travel with the script (the icon and the .exe). Self-update fetches them too, so they reach everyone.
# Names are relative to the launcher's home folder (the one holding the .exe and the Launcher folder).
function Get-AssetPath([string]$name) { return (Join-Path $LauncherHome ($name -replace '/', '\')) }
function Save-LauncherAssets($assets) {
    foreach ($a in @($assets)) {
        if ($a -and $a.Bytes -and $a.Bytes.Length -gt 0) {
            try { [System.IO.File]::WriteAllBytes((Get-AssetPath $a.Name), $a.Bytes) } catch { }   # a locked file just waits for next time
        }
    }
}
function Get-MissingAssets { return @($LauncherAssets | Where-Object { -not (Test-Path -LiteralPath (Get-AssetPath $_)) }) }

function Format-Size($bytes) {
    if (-not $bytes) { return '-' }
    return ('{0:N0} MB' -f ($bytes / 1MB))
}

function Get-InstalledInfo([string]$dir) {
    $f = Join-Path $dir 'launcher-version.txt'
    if (Test-Path -LiteralPath $f) {
        $lines = @(Get-Content -LiteralPath $f)
        $variant = ''
        if ($lines.Count -gt 1) { $variant = $lines[1] }
        return [pscustomobject]@{ Tag = $lines[0]; Variant = $variant }
    }
    if (Test-Path -LiteralPath (Join-Path $dir $ExeName)) { return [pscustomobject]@{ Tag = 'unknown build'; Variant = '' } }
    return $null
}

function Get-ModInfo([string]$dir) {
    $info = [pscustomobject]@{ Folder = (Split-Path $dir -Leaf); Name = ''; Id = '' }
    $f = Join-Path $dir 'modinfo.json'
    if (Test-Path -LiteralPath $f) {
        try {
            $j = Get-Content -LiteralPath $f -Raw -Encoding UTF8 | ConvertFrom-Json
            foreach ($o in @($j)) {
                if ($o.type -eq 'MOD_INFO') {
                    $info.Id = [string]$o.id
                    if ($o.name -is [string]) { $info.Name = $o.name } elseif ($o.name.str) { $info.Name = [string]$o.name.str }
                    break
                }
            }
        } catch { }
    }
    if (-not $info.Name) { $info.Name = $info.Folder }
    return $info
}

function ConvertFrom-GitHubUrl([string]$u) {
    $u = $u.Trim()
    if ($u -match '^(?:https?://)?(?:www\.)?github\.com/([^/\s]+)/([^/\s#?]+?)(?:\.git)?(?:/tree/([^/\s?#]+))?(?:[/?#].*)?$') {
        return [pscustomobject]@{ Owner = $Matches[1]; Repo = $Matches[2]; Ref = $Matches[3] }
    }
    if ($u -match '^([\w.-]+)/([\w.-]+)$') {
        return [pscustomobject]@{ Owner = $Matches[1]; Repo = $Matches[2]; Ref = $null }
    }
    return $null
}

function Format-ReleaseNotes($r) {
    $body = [string]$r.Body
    # "... by @someone in https://github.com/.../pull/123" -> "... by @someone (#123)"
    $body = [regex]::Replace($body, ' in https://github\.com/[^/\s]+/[^/\s]+/pull/(\d+)', ' (#$1)')
    $body = $body -replace '\*\*', ''
    $body = $body -replace '(?m)^## ', ''
    $body = $body -replace '(?m)^\* ', '  - '
    $body = ($body -replace "`r`n", "`n").Trim() -replace "`n", "`r`n"
    $sizes = "Graphics: $(Format-Size $r.GfxSize)"
    if ($r.SndSize) { $sizes += "    Graphics + sounds: $(Format-Size $r.SndSize)" }
    $head = "$($r.Name)`r`nPublished $($r.Date.ToString('dddd d MMMM yyyy, h:mm tt'))`r`n$sizes`r`n`r`n"
    if (-not $body) { $body = '(no notes for this build)' }
    return $head + $body
}

# ---------------------------------------------------------------- background work
# These run in a separate PowerShell runspace so the window never freezes.
# Each gets $sync (status shared with the UI) and $p (a hashtable of inputs).
$script:WorkHelpers = @'
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

function Remove-Dir([string]$path) {
    if (-not (Test-Path -LiteralPath $path)) { return }
    for ($i = 0; $i -lt 5; $i++) {
        try { [System.IO.Directory]::Delete($path, $true); return } catch { Start-Sleep -Milliseconds 600 }
    }
    [System.IO.Directory]::Delete($path, $true)
}

function Get-File([string]$url, [string]$dest, [long]$expected, $sync, [string]$label) {
    $part = "$dest.part"
    $req = [System.Net.HttpWebRequest]::Create($url)
    $req.UserAgent = 'cdda-fork-launcher'
    $req.Timeout = 30000
    $req.ReadWriteTimeout = 60000
    $resp = $req.GetResponse()
    try {
        $total = $resp.ContentLength
        if ($total -le 0) { $total = $expected }
        $in = $resp.GetResponseStream()
        $out = [System.IO.File]::Create($part)
        try {
            $buf = New-Object byte[] 262144
            $done = 0L
            $all = [System.Diagnostics.Stopwatch]::StartNew()
            $tick = [System.Diagnostics.Stopwatch]::StartNew()
            while (($n = $in.Read($buf, 0, $buf.Length)) -gt 0) {
                $out.Write($buf, 0, $n)
                $done += $n
                if ($tick.ElapsedMilliseconds -ge 250) {
                    $tick.Restart()
                    $mbps = ($done / 1MB) / [Math]::Max(0.5, $all.Elapsed.TotalSeconds)
                    if ($total -gt 0) {
                        $sync.Percent = [int](100 * $done / $total)
                        $left = (($total - $done) / 1MB) / [Math]::Max(0.1, $mbps)
                        $sync.Status = ('{0}  {1:N0} / {2:N0} MB  ({3:N1} MB/s, about {4:N0}s left)' -f $label, ($done / 1MB), ($total / 1MB), $mbps, $left)
                    } else {
                        $sync.Percent = -1
                        $sync.Status = ('{0}  {1:N1} MB  ({2:N1} MB/s)' -f $label, ($done / 1MB), $mbps)
                    }
                }
            }
        } finally { $out.Dispose(); $in.Dispose() }
    } finally { $resp.Dispose() }
    if ($total -gt 0 -and (Get-Item -LiteralPath $part).Length -ne $total) {
        Remove-Item -LiteralPath $part -Force
        throw "The download stopped early. Please try again."
    }
    Move-Item -LiteralPath $part -Destination $dest -Force
}

function Expand-ZipSafe([string]$zip, [string]$dest, $sync, [string]$label, [string]$skipPattern = '') {
    Remove-Dir $dest
    [void][System.IO.Directory]::CreateDirectory($dest)
    $destFull = [System.IO.Path]::GetFullPath($dest.TrimEnd('\') + '\')
    $archive = [System.IO.Compression.ZipFile]::OpenRead($zip)
    try {
        $count = $archive.Entries.Count
        $i = 0
        foreach ($e in $archive.Entries) {
            $i++
            if ($skipPattern -and $e.FullName -match $skipPattern) { continue }
            $target = [System.IO.Path]::GetFullPath([System.IO.Path]::Combine($dest, $e.FullName))
            if ($target.Length -ge 259) {
                throw "A file path in the download is too long for Windows here. Move the launcher to a shorter folder (for example C:\Games\CDDA) and try again."
            }
            if (-not $target.StartsWith($destFull, [System.StringComparison]::OrdinalIgnoreCase)) {
                throw "The zip contains an unsafe path: $($e.FullName)"
            }
            if ($e.FullName.EndsWith('/') -or $e.FullName.EndsWith('\')) {
                [void][System.IO.Directory]::CreateDirectory($target)
                continue
            }
            [void][System.IO.Directory]::CreateDirectory([System.IO.Path]::GetDirectoryName($target))
            [System.IO.Compression.ZipFileExtensions]::ExtractToFile($e, $target, $true)
            if ($i % 150 -eq 0 -or $i -eq $count) {
                $sync.Percent = [int](100 * $i / $count)
                $sync.Status = ('{0}  {1:N0} / {2:N0} files' -f $label, $i, $count)
            }
        }
    } finally { $archive.Dispose() }
}

function Find-ModDirs([string]$root) {
    $dirs = @(Get-ChildItem -LiteralPath $root -Recurse -File -Filter 'modinfo.json' | ForEach-Object { $_.DirectoryName })
    $picked = New-Object System.Collections.Generic.List[string]
    foreach ($d in ($dirs | Sort-Object { $_.Length })) {
        $nested = $false
        foreach ($q in $picked) {
            if ($d -eq $q -or $d.StartsWith($q.TrimEnd('\') + '\', [System.StringComparison]::OrdinalIgnoreCase)) { $nested = $true; break }
        }
        if (-not $nested) { $picked.Add($d) }
    }
    return ,$picked
}
'@

$script:FetchWork = {
    $sync.Status = "Checking GitHub ($($p.Repo)) for builds..."
    $sync.Percent = -1
    $rels = Invoke-RestMethod -UseBasicParsing -Uri "https://api.github.com/repos/$($p.Repo)/releases?per_page=40" `
        -Headers @{ 'User-Agent' = 'cdda-fork-launcher'; 'Accept' = 'application/vnd.github+json' }
    $list = foreach ($r in @($rels)) {
        if ($r.draft -or $r.tag_name -notlike $p.TagLike) { continue }
        $gfx = $r.assets | Where-Object { $_.name -match $p.GfxPattern } | Select-Object -First 1
        $snd = if ($p.SndPattern) { $r.assets | Where-Object { $_.name -match $p.SndPattern } | Select-Object -First 1 } else { $null }
        if (-not $gfx -and -not $snd) { continue }
        $date = $r.published_at
        if ($date -isnot [datetime]) {
            $date = [datetime]::Parse([string]$date, [System.Globalization.CultureInfo]::InvariantCulture, [System.Globalization.DateTimeStyles]::AdjustToUniversal)
        }
        [pscustomobject]@{
            Tag     = [string]$r.tag_name
            Name    = [string]$r.name
            Date    = $date.ToLocalTime()
            Body    = [string]$r.body
            HtmlUrl = [string]$r.html_url
            GfxName = $(if ($gfx) { [string]$gfx.name } else { '' })
            GfxUrl  = $(if ($gfx) { [string]$gfx.browser_download_url } else { '' })
            GfxSize = $(if ($gfx) { [long]$gfx.size } else { 0L })
            SndName = $(if ($snd) { [string]$snd.name } else { '' })
            SndUrl  = $(if ($snd) { [string]$snd.browser_download_url } else { '' })
            SndSize = $(if ($snd) { [long]$snd.size } else { 0L })
        }
    }
    # Newest first. Fork tags are dated (testing-build-YYYY-MM-DD, maybe with -2 for a
    # second build that day), so sort by publish date rather than trusting the API order.
    return @($list | Sort-Object Date -Descending)
}

# Looks for a newer launcher on the fork's testing branch. Returns $null when there's nothing to do;
# otherwise Version/Text (Text empty if only missing asset files were fetched) and Assets (name + bytes).
$script:LauncherCheckWork = {
    $sync.Status = 'Checking for a launcher update...'
    $sync.Percent = -1
    $hdr = @{ 'User-Agent' = 'cdda-fork-launcher' }
    $text = (Invoke-WebRequest -UseBasicParsing -Uri $p.Url -Headers $hdr -TimeoutSec 20).Content
    if ($text -is [byte[]]) { $text = [System.Text.Encoding]::UTF8.GetString($text) }
    $m = [regex]::Match([string]$text, "(?m)^\`$script:LauncherVersion = '([^']+)'")
    if (-not $m.Success) { return $null }
    $remote = $m.Groups[1].Value
    $newer = [string]::CompareOrdinal($remote, $p.Current) -gt 0
    $want = if ($newer) { @($p.Assets) } else { @($p.Missing) }
    if (-not $newer -and $want.Count -eq 0) { return $null }
    # Asset names are relative to tools/launcher/ (the folder above Launcher/).
    $base = $p.Url.Substring(0, $p.Url.LastIndexOf('/Launcher/') + 1)
    $assets = foreach ($n in $want) {
        try {
            $rel = (($n -split '/') | ForEach-Object { [uri]::EscapeDataString($_) }) -join '/'
            $r = Invoke-WebRequest -UseBasicParsing -Uri ($base + $rel) -Headers $hdr -TimeoutSec 30
            $bytes = if ($r.Content -is [byte[]]) { $r.Content } else { $r.RawContentStream.ToArray() }
            [pscustomobject]@{ Name = $n; Bytes = $bytes }
        } catch { }
    }
    return [pscustomobject]@{ Version = $remote; Text = $(if ($newer) { [string]$text } else { '' }); Assets = @($assets) }
}

$script:InstallWork = {
    $label = "Downloading $($p.Short)"
    $zip = Join-Path $p.DownloadDir $p.ZipName
    [void][System.IO.Directory]::CreateDirectory($p.DownloadDir)
    $have = (Test-Path -LiteralPath $zip) -and ((Get-Item -LiteralPath $zip).Length -eq $p.Size)
    if (-not $have) { Get-File $p.Url $zip $p.Size $sync $label }

    # data/cache is rebuilt by the game on first start; skipping it keeps paths short.
    Expand-ZipSafe $zip $p.StagingDir $sync "Unpacking $($p.Short)" '(^|/)data/cache/'

    $sync.Percent = -1
    $sync.Status = 'Putting the new build in place...'
    $newRoot = $p.StagingDir
    if (-not (Test-Path -LiteralPath (Join-Path $newRoot $p.ExeName))) {
        $sub = Get-ChildItem -LiteralPath $newRoot -Directory | Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName $p.ExeName) } | Select-Object -First 1
        if (-not $sub) { throw "Couldn't find $($p.ExeName) inside the download." }
        $newRoot = $sub.FullName
    }
    Set-Content -LiteralPath (Join-Path $newRoot 'launcher-version.txt') -Value @($p.Tag, $p.Variant) -Encoding ASCII

    if (Test-Path -LiteralPath $p.GameDir) {
        Remove-Dir $p.PrevDir
        Move-Item -LiteralPath $p.GameDir -Destination $p.PrevDir
    }
    Move-Item -LiteralPath $newRoot -Destination $p.GameDir
    Remove-Dir $p.StagingDir

    # keep only the zip we just used
    Get-ChildItem -LiteralPath $p.DownloadDir -Filter 'cdda-*.zip' | Where-Object { $_.Name -ne $p.ZipName } | Remove-Item -Force
    return [pscustomobject]@{ Tag = $p.Tag; Short = $p.Short; Variant = $p.Variant }
}

$script:ModInstallWork = {
    $work = Join-Path $p.Root '_modstaging'
    try {
        if ($p.Url) {
            $zip = Join-Path $p.DownloadDir $p.ZipName
            Get-File $p.Url $zip 0 $sync "Downloading $($p.Label)"
        } else {
            $zip = $p.ZipPath
        }
        Expand-ZipSafe $zip $work $sync "Unpacking $($p.Label)"
        $dirs = Find-ModDirs $work
        if ($dirs.Count -eq 0) { throw "No mod found in $($p.Label) (there is no modinfo.json in it)." }

        [void][System.IO.Directory]::CreateDirectory($p.UserModsDir)
        $names = New-Object System.Collections.Generic.List[string]
        foreach ($d in $dirs) {
            $name = Split-Path $d -Leaf
            $parent = Split-Path $d -Parent
            if ($d.TrimEnd('\') -eq $work.TrimEnd('\') -or ($p.IsGitHub -and $parent.TrimEnd('\') -eq $work.TrimEnd('\'))) {
                $name = $p.FallbackName
            }
            foreach ($c in [System.IO.Path]::GetInvalidFileNameChars()) { $name = $name.Replace([string]$c, '_') }
            $sync.Status = "Installing mod $name..."
            $target = Join-Path $p.UserModsDir $name
            Remove-Dir $target
            Copy-Item -LiteralPath $d -Destination $target -Recurse
            $names.Add($name)
        }
        return [pscustomobject]@{ Names = @($names); Origin = $p.Origin }
    } finally {
        Remove-Dir $work
        if ($p.Url -and $zip -and (Test-Path -LiteralPath $zip)) { Remove-Item -LiteralPath $zip -Force }
    }
}

$script:SyncWork = {
    [void][System.IO.Directory]::CreateDirectory($p.UserModsDir)
    $i = 0
    foreach ($name in $p.Names) {
        $i++
        $sync.Percent = [int](100 * ($i - 1) / $p.Names.Count)
        $sync.Status = "Copying $name from your fork..."
        $target = Join-Path $p.UserModsDir $name
        Remove-Dir $target
        Copy-Item -LiteralPath (Join-Path $p.Source $name) -Destination $target -Recurse
    }
    return [pscustomobject]@{ Names = @($p.Names) }
}

$script:BackupWork = {
    $sync.Status = 'Zipping your saves...'
    $sync.Percent = -1
    [void][System.IO.Directory]::CreateDirectory($p.BackupDir)
    if (Test-Path -LiteralPath $p.Dest) { Remove-Item -LiteralPath $p.Dest -Force }
    [System.IO.Compression.ZipFile]::CreateFromDirectory($p.Save, $p.Dest, [System.IO.Compression.CompressionLevel]::Optimal, $true)
    return [pscustomobject]@{ Path = $p.Dest; Size = (Get-Item -LiteralPath $p.Dest).Length }
}

$script:CompareWork = {
    $sync.Status = 'Asking GitHub what changed...'
    $sync.Percent = -1
    $c = Invoke-RestMethod -UseBasicParsing -Uri "https://api.github.com/repos/$($p.Repo)/compare/$($p.Base)...$($p.Head)" `
        -Headers @{ 'User-Agent' = 'cdda-fork-launcher'; 'Accept' = 'application/vnd.github+json' }
    $lines = New-Object System.Collections.Generic.List[string]
    $commits = @($c.commits)
    [array]::Reverse($commits)
    foreach ($cm in $commits) {
        $first = ([string]$cm.commit.message -split "`n")[0].Trim()
        if ($first -match '^Merge (branch|remote-tracking)') { continue }
        $who = if ($cm.author.login) { $cm.author.login } else { $cm.commit.author.name }
        $lines.Add("  - $first  ($who)")
    }
    return [pscustomobject]@{ Total = [int]$c.total_commits; Shown = $commits.Count; Lines = @($lines) }
}

function Invoke-WorkNow([scriptblock]$Work, [hashtable]$Params) {
    # Synchronous version, used by -SelfTest.
    $sync = [hashtable]::Synchronized(@{ Status = ''; Percent = -1 })
    $code = "param(`$sync, `$p)`n" + $script:WorkHelpers + "`n" + $Work.ToString()
    return & ([scriptblock]::Create($code)) $sync $Params
}

# ---------------------------------------------------------------- self test (no window)
if ($SelfTest) {
    "Root: $Root"
    "Launcher version: $LauncherVersion"
    "Mod source: $($Settings.ModSource)"
    foreach ($k in @($Sources.Keys)) {
        $script:Settings.Source = $k
        $rels = @(Invoke-WorkNow $FetchWork (Get-FetchParams))
        "--- $($Sources[$k].Label): $($rels.Count) builds"
        $rels | Select-Object -First 3 | ForEach-Object { "  $(Get-ShortTag $_.Tag)  $($_.Date)  gfx=$(Format-Size $_.GfxSize)  snd=$(Format-Size $_.SndSize)  source=$(Get-TagSource $_.Tag)" }
        if ($rels.Count) { (Format-ReleaseNotes $rels[0]).Split("`n") | Select-Object -First 4 }
    }
    "--- launcher update check (pretending to be version 2000-01-01) ---"
    try { $u = Invoke-WorkNow $LauncherCheckWork @{ Url = $LauncherUpdateUrl; Current = '2000-01-01'; Assets = $LauncherAssets; Missing = @() } }
    catch { $u = $null; "  couldn't fetch $LauncherUpdateUrl ($($_.Exception.Message))" }
    if ($u) {
        "  newer launcher found: $($u.Version), $($u.Text.Length) chars, parses: $(Test-LauncherScript $u.Text)"
        foreach ($a in $u.Assets) { "  asset $($a.Name): $($a.Bytes.Length) bytes" }
    } else { '  none found (fine until the launcher is on the testing branch)' }
    "--- themes: $(@($Themes.Keys) -join ', ')   missing assets here: $(@(Get-MissingAssets) -join ', ')"
    "--- url parsing ---"
    foreach ($u in @('https://github.com/TGWeaver/Backrooms-CDDA-Mod', 'github.com/a/b.git', 'https://github.com/a/b/tree/dev/sub/dir', 'a/b', 'not a url')) {
        $g = ConvertFrom-GitHubUrl $u
        if ($g) { "  $u -> $($g.Owner) / $($g.Repo) / ref=$($g.Ref)" } else { "  $u -> (rejected)" }
    }
    return
}

# ---------------------------------------------------------------- command line modes (no window)
if ($InstallNewest) {
    $r = @(Invoke-WorkNow $FetchWork (Get-FetchParams)) | Select-Object -First 1
    if (-not $r) { throw "No $((Get-Source).Label) found on GitHub." }
    $variant = $Settings.Variant
    if ($variant -eq 'sounds' -and $r.SndUrl) { $url = $r.SndUrl; $size = $r.SndSize; $zipName = $r.SndName }
    else { $variant = 'graphics'; $url = $r.GfxUrl; $size = $r.GfxSize; $zipName = $r.GfxName }
    $inst = Get-InstalledInfo $GameDir
    if ($inst -and $inst.Tag -eq $r.Tag -and $inst.Variant -eq $variant) { "Already up to date: $(Get-ShortTag $r.Tag)"; return }
    if (Get-Process -Name $ProcName -ErrorAction SilentlyContinue) { throw 'Close the game first.' }
    "Installing $(Get-ShortTag $r.Tag) ($variant) into $GameDir ..."
    $res = Invoke-WorkNow $InstallWork @{
        Url = $url; Size = $size; ZipName = $zipName; Tag = $r.Tag; Short = (Get-ShortTag $r.Tag); Variant = $variant
        DownloadDir = $DownloadDir; StagingDir = $StagingDir; GameDir = $GameDir; PrevDir = $PrevDir; ExeName = $ExeName
    }
    "Installed $($res.Short)."
    return
}
if ($AddMod) {
    $g = ConvertFrom-GitHubUrl $AddMod
    if (-not $g) { throw "Not a GitHub link: $AddMod" }
    $url = "https://api.github.com/repos/$($g.Owner)/$($g.Repo)/zipball"
    if ($g.Ref) { $url += "/$($g.Ref)" }
    $res = Invoke-WorkNow $ModInstallWork @{
        Url = $url; ZipName = "mod-$($g.Owner)-$($g.Repo).zip"; Label = "$($g.Owner)/$($g.Repo)"; IsGitHub = $true
        FallbackName = $g.Repo; Origin = "GitHub: $($g.Owner)/$($g.Repo)"; Root = $Root; DownloadDir = $DownloadDir; UserModsDir = $UserModsDir
    }
    foreach ($n in $res.Names) { $Settings.ModOrigins[$n] = $res.Origin }
    Save-Settings
    "Installed mod(s): $($res.Names -join ', ') into $UserModsDir"
    return
}

# ================================================================ USER INTERFACE
[System.Windows.Forms.Application]::EnableVisualStyles()

# Dark title bar and dark scrollbars (Windows 10 1809+ / 11). Harmless if unavailable.
try {
    Add-Type -Namespace CddaLauncher -Name Native -MemberDefinition @'
[System.Runtime.InteropServices.DllImport("uxtheme.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
public static extern int SetWindowTheme(System.IntPtr hWnd, string appName, string idList);
[System.Runtime.InteropServices.DllImport("dwmapi.dll")]
public static extern int DwmSetWindowAttribute(System.IntPtr hwnd, int attr, ref int value, int size);
'@
    $script:HasNative = $true
} catch { $script:HasNative = $false }

function New-Point($x, $y) { New-Object System.Drawing.Point($x, $y) }
function New-Size($w, $h)  { New-Object System.Drawing.Size($w, $h) }
function New-Pad($l, $t, $r, $b) { New-Object System.Windows.Forms.Padding($l, $t, $r, $b) }
function New-Font([single]$size, [string]$style = 'Regular', [string]$family = 'Segoe UI') {
    New-Object System.Drawing.Font($family, $size, [System.Drawing.FontStyle]$style)
}
function New-Button([string]$text, [string]$role = '') {
    $b = New-Object System.Windows.Forms.Button
    $b.Text = $text
    $b.AutoSize = $true
    $b.Padding = New-Pad 10 4 10 4
    $b.Margin = New-Pad 0 4 8 4
    $b.FlatStyle = 'Flat'
    $b.Cursor = [System.Windows.Forms.Cursors]::Hand
    if ($role) { $b.Tag = $role }
    return $b
}
function New-Label([string]$text, [string]$role = '') {
    $l = New-Object System.Windows.Forms.Label
    $l.Text = $text; $l.AutoSize = $true; $l.UseMnemonic = $false
    if ($role) { $l.Tag = $role }
    return $l
}
function Show-Info([string]$msg)  { [void][System.Windows.Forms.MessageBox]::Show($form, $msg, 'CDDA Launcher', 'OK', 'Information') }
function Show-Error([string]$msg) { [void][System.Windows.Forms.MessageBox]::Show($form, $msg, 'CDDA Launcher', 'OK', 'Error') }
function Confirm-Action([string]$msg) {
    return ([System.Windows.Forms.MessageBox]::Show($form, $msg, 'CDDA Launcher', 'YesNo', 'Question') -eq 'Yes')
}
function Invoke-Safe([scriptblock]$block) {
    try { & $block } catch { Show-Error $_.Exception.Message }
}
function Open-Folder([string]$path) {
    if (-not (Test-Path -LiteralPath $path)) { New-Item -ItemType Directory -Force -Path $path | Out-Null }
    Start-Process explorer.exe -ArgumentList "`"$path`""
}
# Images are read into memory so self-update can replace the files while the window is open.
function Read-LauncherImage([string]$name) {
    $f = Join-Path $LauncherDir $name
    if (-not (Test-Path -LiteralPath $f)) { return $null }
    try { return [System.Drawing.Image]::FromStream((New-Object System.IO.MemoryStream(, [System.IO.File]::ReadAllBytes($f)))) } catch { return $null }
}
function Read-LauncherIcon {
    $f = Join-Path $LauncherDir 'launcher.ico'
    if (Test-Path -LiteralPath $f) {
        try { return New-Object System.Drawing.Icon((New-Object System.IO.MemoryStream(, [System.IO.File]::ReadAllBytes($f)))) } catch { }
    }
    $exe = Join-Path $GameDir $ExeName
    if (Test-Path -LiteralPath $exe) { try { return [System.Drawing.Icon]::ExtractAssociatedIcon($exe) } catch { } }
    return $null
}

$script:ActionButtons = New-Object System.Collections.Generic.List[System.Windows.Forms.Control]
$script:Busy = $false
$script:Job = $null
$script:Releases = @()
$script:VersionsWarn = $false

# ---------------------------------------------------------------- theme
$script:ThemeKey = if ($Theme -and $Themes.Contains($Theme)) { $Theme } else { $Settings.Theme }
function Get-ThemeColors {
    $t = @{}
    $src = $Themes[$script:ThemeKey]
    foreach ($k in $src.Keys) {
        if ($src[$k] -is [string] -and $src[$k].StartsWith('#')) { $t[$k] = [System.Drawing.ColorTranslator]::FromHtml($src[$k]) } else { $t[$k] = $src[$k] }
    }
    return $t
}
$script:T = Get-ThemeColors

function Set-NativeTheme([System.Windows.Forms.Control]$c) {
    if (-not $script:HasNative -or -not $c.IsHandleCreated) { return }
    $name = if ($T.Dark) { 'DarkMode_Explorer' } else { 'Explorer' }
    try { [void][CddaLauncher.Native]::SetWindowTheme($c.Handle, $name, $null) } catch { }
}
function Set-TitleBarTheme {
    if (-not $script:HasNative -or -not $form.IsHandleCreated) { return }
    $v = if ($T.Dark) { 1 } else { 0 }
    try {
        [void][CddaLauncher.Native]::DwmSetWindowAttribute($form.Handle, 20, [ref]$v, 4)   # Windows 11 / 10 20H1+
        [void][CddaLauncher.Native]::DwmSetWindowAttribute($form.Handle, 19, [ref]$v, 4)   # older Windows 10
    } catch { }
}

function Set-ButtonStyle([System.Windows.Forms.Button]$b, [string]$role) {
    $b.FlatStyle = 'Flat'
    $b.UseVisualStyleBackColor = $false
    switch ($role) {
        'accent' {
            if ($b.Enabled) { $b.BackColor = $T.Accent; $b.ForeColor = $T.AccentText } else { $b.BackColor = $T.Raised; $b.ForeColor = $T.Muted }
            $b.FlatAppearance.BorderColor = $b.BackColor; $b.FlatAppearance.BorderSize = 0
            $b.FlatAppearance.MouseOverBackColor = [System.Windows.Forms.ControlPaint]::Light($T.Accent, 0.15)
            $b.FlatAppearance.MouseDownBackColor = [System.Windows.Forms.ControlPaint]::Dark($T.Accent, 0.05)
        }
        'nav' {
            $active = ($script:CurrentPage -eq $b.Name)
            $b.BackColor = if ($active) { $T.Raised } else { $T.Side }
            $b.ForeColor = if ($active) { $T.Accent } else { $T.Text }
            $b.FlatAppearance.BorderSize = 0
            $b.FlatAppearance.MouseOverBackColor = $T.Hover
            $b.FlatAppearance.MouseDownBackColor = $T.Hover
        }
        default {
            $b.BackColor = $T.Raised; $b.ForeColor = $T.Text
            $b.FlatAppearance.BorderSize = 1; $b.FlatAppearance.BorderColor = $T.Border
            $b.FlatAppearance.MouseOverBackColor = $T.Hover
            $b.FlatAppearance.MouseDownBackColor = $T.Border
        }
    }
}

function Set-ControlTheme([System.Windows.Forms.Control]$c) {
    $role = if ($c.Tag -is [string]) { [string]$c.Tag } else { '' }
    switch -Regex ($c.GetType().Name) {
        '^Button$' { Set-ButtonStyle $c $role; break }
        '^Label$' {
            $c.BackColor = [System.Drawing.Color]::Transparent
            $c.ForeColor = switch ($role) {
                'muted'    { $T.Muted }
                'section'  { $T.Accent }
                'versions' { if ($script:VersionsWarn) { $T.Warn } else { $T.Muted } }
                default    { $T.Text }
            }
            break
        }
        '^(TextBox|ComboBox)$' {
            $c.BackColor = if ($role -eq 'notes') { $T.Surface } else { $T.Raised }
            $c.ForeColor = $T.Text
            if ($c -is [System.Windows.Forms.ComboBox]) { $c.FlatStyle = 'Flat' }
            Set-NativeTheme $c
            break
        }
        '^(ListView|CheckedListBox)$' { $c.BackColor = $T.Surface; $c.ForeColor = $T.Text; Set-NativeTheme $c; break }
        '^(CheckBox|RadioButton)$' { $c.BackColor = [System.Drawing.Color]::Transparent; $c.ForeColor = $T.Text; break }
        '^SplitContainer$' { $c.BackColor = $T.Border; break }
        '^PictureBox$' { $c.BackColor = [System.Drawing.Color]::Transparent; break }
        default {
            $c.BackColor = switch ($role) {
                'side'    { $T.Side }
                'surface' { $T.Surface }
                'raised'  { $T.Raised }
                default   { $T.Back }
            }
            $c.ForeColor = $T.Text
            if ($c -is [System.Windows.Forms.ScrollableControl] -and $c.AutoScroll) { Set-NativeTheme $c }
        }
    }
    foreach ($child in $c.Controls) { Set-ControlTheme $child }
}

function Set-Theme([string]$key) {
    $script:ThemeKey = $key
    $script:T = Get-ThemeColors
    $form.BackColor = $T.Back
    Set-ControlTheme $form
    Set-TitleBarTheme
    $progressBar.Invalidate()
    $lvReleases.Invalidate(); $lvMods.Invalidate()
}

# ListView drawing: themed header and rows (WinForms can't colour headers otherwise).
$script:DrawHeader = {
    param($s, $e)
    $g = $e.Graphics
    $bg = New-Object System.Drawing.SolidBrush($T.Raised)
    $g.FillRectangle($bg, $e.Bounds); $bg.Dispose()
    $pen = New-Object System.Drawing.Pen($T.Border)
    $g.DrawLine($pen, $e.Bounds.Left, $e.Bounds.Bottom - 1, $e.Bounds.Right, $e.Bounds.Bottom - 1)
    $g.DrawLine($pen, $e.Bounds.Right - 1, $e.Bounds.Top + 4, $e.Bounds.Right - 1, $e.Bounds.Bottom - 5); $pen.Dispose()
    $r = New-Object System.Drawing.Rectangle(($e.Bounds.X + 6), $e.Bounds.Y, ($e.Bounds.Width - 8), $e.Bounds.Height)
    [System.Windows.Forms.TextRenderer]::DrawText($g, $e.Header.Text, $s.Font, $r, $T.Muted,
        ([System.Windows.Forms.TextFormatFlags]::VerticalCenter -bor [System.Windows.Forms.TextFormatFlags]::EndEllipsis))
}
$script:DrawSubItem = {
    param($s, $e)
    $g = $e.Graphics
    $sel = $e.Item.Selected
    $bg = New-Object System.Drawing.SolidBrush($(if ($sel) { $T.Hover } else { $T.Surface }))
    $g.FillRectangle($bg, $e.Bounds); $bg.Dispose()
    if ($sel -and $e.ColumnIndex -eq 0) {
        $acc = New-Object System.Drawing.SolidBrush($T.Accent)
        $g.FillRectangle($acc, $e.Bounds.X, $e.Bounds.Y, 3, $e.Bounds.Height); $acc.Dispose()
    }
    $font = if ($e.Item.Font) { $e.Item.Font } else { $s.Font }
    $color = if ($e.ColumnIndex -eq 0 -or $sel) { $T.Text } else { $T.Muted }
    $r = New-Object System.Drawing.Rectangle(($e.Bounds.X + 8), $e.Bounds.Y, ($e.Bounds.Width - 10), $e.Bounds.Height)
    [System.Windows.Forms.TextRenderer]::DrawText($g, $e.SubItem.Text, $font, $r, $color,
        ([System.Windows.Forms.TextFormatFlags]::VerticalCenter -bor [System.Windows.Forms.TextFormatFlags]::EndEllipsis))
}
function New-ThemedListView {
    $lv = New-Object System.Windows.Forms.ListView
    $lv.View = 'Details'; $lv.FullRowSelect = $true; $lv.HideSelection = $false; $lv.Dock = 'Fill'
    $lv.BorderStyle = 'None'; $lv.OwnerDraw = $true
    $lv.Font = New-Font 9.5
    $lv.add_DrawColumnHeader($script:DrawHeader)
    $lv.add_DrawSubItem($script:DrawSubItem)
    $lv.add_DrawItem({ param($s, $e) $e.DrawDefault = $false })
    # Owner-drawn rows don't repaint on selection by themselves.
    $lv.add_SelectedIndexChanged({ param($s, $e) $s.Invalidate() })
    $lv.add_HandleCreated({ param($s, $e) Set-NativeTheme $s; Set-LastColumnFill $s })
    # The header strip after the last column can't be themed, so the last column always fills the width.
    $lv.add_Resize({ param($s, $e) Set-LastColumnFill $s })
    return $lv
}
function Set-LastColumnFill([System.Windows.Forms.ListView]$lv) {
    $n = $lv.Columns.Count
    if ($n -lt 1) { return }
    $used = 0
    for ($i = 0; $i -lt $n - 1; $i++) { $used += $lv.Columns[$i].Width }
    $w = $lv.ClientSize.Width - $used
    if ($w -gt 60 -and $lv.Columns[$n - 1].Width -ne $w) { $lv.Columns[$n - 1].Width = $w }
}

# Drop-down lists drawn in theme colours (a plain ComboBox stays white on Windows).
$script:DrawComboItem = {
    param($s, $e)
    if ($e.Index -lt 0) { return }
    $sel = ($e.State -band [System.Windows.Forms.DrawItemState]::Selected) -ne 0
    $inEdit = ($e.State -band [System.Windows.Forms.DrawItemState]::ComboBoxEdit) -ne 0
    $bgc = if ($sel -and -not $inEdit) { $T.Hover } else { $T.Raised }
    $bg = New-Object System.Drawing.SolidBrush($bgc)
    $e.Graphics.FillRectangle($bg, $e.Bounds); $bg.Dispose()
    $r = New-Object System.Drawing.Rectangle(($e.Bounds.X + 6), $e.Bounds.Y, ($e.Bounds.Width - 6), $e.Bounds.Height)
    [System.Windows.Forms.TextRenderer]::DrawText($e.Graphics, [string]$s.Items[$e.Index], $s.Font, $r, $T.Text,
        ([System.Windows.Forms.TextFormatFlags]::VerticalCenter -bor [System.Windows.Forms.TextFormatFlags]::EndEllipsis))
}
function New-ThemedCombo([int]$width) {
    $cb = New-Object System.Windows.Forms.ComboBox
    $cb.DropDownStyle = 'DropDownList'; $cb.Width = $width
    $cb.DrawMode = 'OwnerDrawFixed'; $cb.ItemHeight = 22; $cb.FlatStyle = 'Flat'
    $cb.add_DrawItem($script:DrawComboItem)
    return $cb
}

# ---------------------------------------------------------------- progress bar (themed, with marquee)
$script:ProgressValue = 0
$script:Marquee = $false
$script:MarqueePos = 0
$progressBar = New-Object System.Windows.Forms.Panel
$progressBar.Height = 6; $progressBar.Dock = 'Top'; $progressBar.Tag = 'raised'
$progressBar.Margin = New-Pad 0 6 0 6
$progressBar.GetType().GetProperty('DoubleBuffered', [System.Reflection.BindingFlags]'Instance,NonPublic').SetValue($progressBar, $true, $null)
$progressBar.add_Paint({
    param($s, $e)
    $g = $e.Graphics
    $w = $s.ClientSize.Width; $h = $s.ClientSize.Height
    $g.Clear($T.Raised)
    $b = New-Object System.Drawing.SolidBrush($T.Accent)
    if ($script:Marquee) {
        $seg = [int]($w / 4)
        $x = ($script:MarqueePos % ($w + $seg)) - $seg
        $g.FillRectangle($b, $x, 0, $seg, $h)
    } elseif ($script:ProgressValue -gt 0) {
        $g.FillRectangle($b, 0, 0, [int]($w * [Math]::Min(100, $script:ProgressValue) / 100), $h)
    }
    $b.Dispose()
})
$marqueeTimer = New-Object System.Windows.Forms.Timer
$marqueeTimer.Interval = 30
$marqueeTimer.add_Tick({ $script:MarqueePos += 8; $progressBar.Invalidate() })
function Set-Progress([int]$pct) {
    if ($pct -lt 0) {
        if (-not $script:Marquee) { $script:Marquee = $true; $marqueeTimer.Start() }
    } else {
        if ($script:Marquee) { $script:Marquee = $false; $marqueeTimer.Stop() }
        $script:ProgressValue = $pct
    }
    $progressBar.Invalidate()
}

# ---------------------------------------------------------------- work runner
function Set-ActionsEnabled([bool]$on) {
    foreach ($b in $script:ActionButtons) { $b.Enabled = $on }
    # Tester builds come in one download (graphics, no sound pack).
    if ($on -and -not (Get-Source).SndPattern) { $rbSnd.Enabled = $false }
}
function Get-ChosenVariant {
    if ($rbSnd.Checked -and (Get-Source).SndPattern) { return 'sounds' }
    return 'graphics'
}

function Start-Work([string]$Title, [scriptblock]$Work, [hashtable]$Params, [scriptblock]$OnDone, [bool]$QuietErrors = $false) {
    if ($script:Busy) { Show-Info 'Please wait - another task is still running.'; return }
    $script:Busy = $true
    Set-ActionsEnabled $false
    $sync = [hashtable]::Synchronized(@{ Status = $Title; Percent = -1 })
    $ps = [powershell]::Create()
    $code = "param(`$sync, `$p)`n" + $script:WorkHelpers + "`n" + $Work.ToString()
    [void]$ps.AddScript($code).AddArgument($sync).AddArgument($Params)
    $script:Job = @{ PS = $ps; Handle = $ps.BeginInvoke(); Sync = $sync; OnDone = $OnDone; QuietErrors = $QuietErrors }
    $lblStatus.Text = $Title
    $jobTimer.Start()
}

# ---------------------------------------------------------------- form
$form = New-Object System.Windows.Forms.Form
$form.Text = 'CDDA Fork Launcher'
$form.Size = New-Size 1080 700
$form.MinimumSize = New-Size 900 600
$form.StartPosition = 'CenterScreen'
$form.Font = New-Font 9.5
$launcherIcon = Read-LauncherIcon
if ($launcherIcon) { $form.Icon = $launcherIcon }

# ---------------------------------------------------------------- sidebar
$side = New-Object System.Windows.Forms.Panel
$side.Dock = 'Left'; $side.Width = 210; $side.Tag = 'side'

$picLogo = New-Object System.Windows.Forms.PictureBox
$picLogo.Size = New-Size 76 76; $picLogo.Location = New-Point 67 22; $picLogo.SizeMode = 'Zoom'
$picLogo.Image = Read-LauncherImage 'launcher-icon.png'
$lblBrand = New-Label 'Cataclysm: DDA'
$lblBrand.Font = New-Font 13 'Bold'; $lblBrand.AutoSize = $false; $lblBrand.TextAlign = 'MiddleCenter'
$lblBrand.Location = New-Point 0 106; $lblBrand.Size = New-Size 210 26
$lblBrandSub = New-Label 'Xbro10 fork launcher' 'muted'
$lblBrandSub.AutoSize = $false; $lblBrandSub.TextAlign = 'MiddleCenter'
$lblBrandSub.Location = New-Point 0 132; $lblBrandSub.Size = New-Size 210 20
$side.Controls.AddRange(@($picLogo, $lblBrand, $lblBrandSub))

$script:NavButtons = @{}
$navY = 176
foreach ($n in @(@('game', 'Game'), @('mods', 'Mods'), @('saves', 'Saves & backups'), @('settings', 'Settings'))) {
    $b = New-Object System.Windows.Forms.Button
    $b.Name = $n[0]; $b.Text = '    ' + $n[1]; $b.Tag = 'nav'
    $b.TextAlign = 'MiddleLeft'; $b.Font = New-Font 10.5; $b.UseMnemonic = $false
    $b.Location = New-Point 0 $navY; $b.Size = New-Size 210 42
    $b.FlatStyle = 'Flat'; $b.Cursor = [System.Windows.Forms.Cursors]::Hand
    $b.add_Click({ param($s, $e) Show-Page $s.Name })
    $side.Controls.Add($b)
    $script:NavButtons[$n[0]] = $b
    $navY += 44
}
$lblSideVersion = New-Label "Launcher $LauncherVersion" 'muted'
$lblSideVersion.Dock = 'Bottom'; $lblSideVersion.AutoSize = $false; $lblSideVersion.Height = 30
$lblSideVersion.TextAlign = 'MiddleCenter'; $lblSideVersion.Font = New-Font 8.5
$side.Controls.Add($lblSideVersion)

# ---------------------------------------------------------------- main area
$main = New-Object System.Windows.Forms.Panel
$main.Dock = 'Fill'; $main.Padding = New-Pad 22 16 22 12

$header = New-Object System.Windows.Forms.Panel
$header.Dock = 'Top'; $header.Height = 66
$lblTitle = New-Label ''
$lblTitle.Font = New-Font 17 'Bold'; $lblTitle.Location = New-Point 0 0
$lblVersions = New-Label '' 'versions'
$lblVersions.Font = New-Font 10; $lblVersions.Location = New-Point 2 38
$header.Controls.AddRange(@($lblTitle, $lblVersions))

$footer = New-Object System.Windows.Forms.Panel
$footer.Dock = 'Bottom'; $footer.Height = 78; $footer.Padding = New-Pad 0 12 0 0
$btnPlay = New-Object System.Windows.Forms.Button
$btnPlay.Text = 'PLAY'; $btnPlay.Tag = 'accent'
$btnPlay.Font = New-Font 17 'Bold'
$btnPlay.Dock = 'Right'; $btnPlay.Width = 190
$btnPlay.FlatStyle = 'Flat'; $btnPlay.Cursor = [System.Windows.Forms.Cursors]::Hand
$btnPlay.add_EnabledChanged({ param($s, $e) Set-ButtonStyle $s 'accent' })
$footerLeft = New-Object System.Windows.Forms.Panel
$footerLeft.Dock = 'Fill'; $footerLeft.Padding = New-Pad 0 6 22 0
$lblStatus = New-Label 'Starting...'
$lblStatus.Dock = 'Top'; $lblStatus.AutoSize = $false; $lblStatus.Height = 24; $lblStatus.AutoEllipsis = $true
$lblStatus.TextAlign = 'MiddleLeft'
$footerLeft.Controls.Add($progressBar)
$footerLeft.Controls.Add($lblStatus)
$footer.Controls.Add($footerLeft)
$footer.Controls.Add($btnPlay)
$script:ActionButtons.Add($btnPlay)

$content = New-Object System.Windows.Forms.Panel
$content.Dock = 'Fill'

function New-Page { $p = New-Object System.Windows.Forms.Panel; $p.Dock = 'Fill'; $p.Visible = $false; $content.Controls.Add($p); return $p }
function New-Bar([string]$dock = 'Bottom') {
    $f = New-Object System.Windows.Forms.FlowLayoutPanel
    $f.Dock = $dock; $f.AutoSize = $true; $f.WrapContents = $true
    $f.Padding = New-Pad 0 8 0 0
    return $f
}
function New-SurfaceBox([System.Windows.Forms.Control]$inner, [int]$pad = 0) {
    $p = New-Object System.Windows.Forms.Panel
    $p.Dock = 'Fill'; $p.Tag = 'surface'; $p.Padding = New-Pad $pad $pad $pad $pad
    $p.Controls.Add($inner)
    return $p
}

# ---------------------------------------------------------------- page: game
$pageGame = New-Page
$sourceBar = New-Bar 'Top'
$sourceBar.Padding = New-Pad 0 0 0 10
$lblSource = New-Label 'Builds from'
$lblSource.Margin = New-Pad 0 8 8 0
$cbSource = New-ThemedCombo 230; $cbSource.Margin = New-Pad 0 4 16 0
foreach ($k in $Sources.Keys) { [void]$cbSource.Items.Add($Sources[$k].Label) }
$cbSource.SelectedIndex = [array]::IndexOf(@($Sources.Keys), $Settings.Source)
$sourceBar.Controls.AddRange(@($lblSource, $cbSource))

$split = New-Object System.Windows.Forms.SplitContainer
$split.Dock = 'Fill'; $split.SplitterWidth = 8
$lvReleases = New-ThemedListView
$lvReleases.MultiSelect = $false
[void]$lvReleases.Columns.Add('Build', 125)
[void]$lvReleases.Columns.Add('Published', 135)
[void]$lvReleases.Columns.Add('Size', 65)
[void]$lvReleases.Columns.Add('', 115)
$split.Panel1.Controls.Add((New-SurfaceBox $lvReleases))
$txtNotes = New-Object System.Windows.Forms.TextBox
$txtNotes.Multiline = $true; $txtNotes.ReadOnly = $true; $txtNotes.ScrollBars = 'Vertical'
$txtNotes.Dock = 'Fill'; $txtNotes.BorderStyle = 'None'; $txtNotes.Tag = 'notes'
$txtNotes.Font = New-Font 9.5
$split.Panel2.Controls.Add((New-SurfaceBox $txtNotes 10))

$gameBar = New-Bar
$btnInstall = New-Button 'Install / update to selected build' 'accent'
$btnInstall.Font = New-Font 9.5 'Bold'
$btnRefresh = New-Button 'Refresh list'
$btnWhatsNew = New-Button 'What changed since my build?'
$btnGitHub = New-Button 'Open on GitHub'
$gameBar.Controls.AddRange(@($btnInstall, $btnRefresh, $btnWhatsNew, $btnGitHub))
foreach ($c in @($cbSource, $btnInstall, $btnRefresh, $btnWhatsNew)) { $script:ActionButtons.Add($c) }
$pageGame.Controls.Add($split)
$pageGame.Controls.Add($gameBar)
$pageGame.Controls.Add($sourceBar)

# ---------------------------------------------------------------- page: mods
$pageMods = New-Page
$lblModsInfo = New-Label ("These mods live in UserData\mods, so game updates never remove them. Turn them on in-game when you " +
    "create a world (Mod manager). Tester builds already include Backrooms Expanded and Survivor Minds.") 'muted'
$lblModsInfo.Dock = 'Top'; $lblModsInfo.AutoSize = $false; $lblModsInfo.Height = 44
$lvMods = New-ThemedListView
$lvMods.MultiSelect = $true
[void]$lvMods.Columns.Add('Mod', 210)
[void]$lvMods.Columns.Add('ID', 150)
[void]$lvMods.Columns.Add('Folder', 150)
[void]$lvMods.Columns.Add('Came from', 230)
$modsBar = New-Bar
$btnSync = New-Button 'Sync my mods from my fork...'
$btnAddZip = New-Button 'Add mod from a zip...'
$btnRemoveMod = New-Button 'Remove selected'
$btnOpenMods = New-Button 'Open mods folder'
$lblGh = New-Label 'GitHub link'
$lblGh.Margin = New-Pad 0 10 8 0
$txtGh = New-Object System.Windows.Forms.TextBox
$txtGh.Width = 380; $txtGh.Margin = New-Pad 0 7 8 4
$btnAddGh = New-Button 'Add mod from GitHub'
$modsBar.Controls.AddRange(@($btnAddZip, $btnRemoveMod, $btnOpenMods, $btnSync, $lblGh, $txtGh, $btnAddGh))
$modsBar.SetFlowBreak($btnSync, $true)
foreach ($c in @($btnSync, $btnAddZip, $btnRemoveMod, $btnAddGh)) { $script:ActionButtons.Add($c) }
$pageMods.Controls.Add((New-SurfaceBox $lvMods))
$pageMods.Controls.Add($modsBar)
$pageMods.Controls.Add($lblModsInfo)

# ---------------------------------------------------------------- shared helpers for form-style pages
function New-FormGrid {
    $g = New-Object System.Windows.Forms.TableLayoutPanel
    $g.Dock = 'Top'; $g.AutoSize = $true; $g.ColumnCount = 3
    [void]$g.ColumnStyles.Add((New-Object System.Windows.Forms.ColumnStyle([System.Windows.Forms.SizeType]::Absolute, 210)))
    [void]$g.ColumnStyles.Add((New-Object System.Windows.Forms.ColumnStyle([System.Windows.Forms.SizeType]::Percent, 100)))
    [void]$g.ColumnStyles.Add((New-Object System.Windows.Forms.ColumnStyle([System.Windows.Forms.SizeType]::AutoSize)))
    return $g
}
function Add-Section($grid, [string]$text) {
    $l = New-Label $text.ToUpper() 'section'
    $l.Font = New-Font 9 'Bold'; $l.Margin = New-Pad 0 12 0 2
    $row = $grid.RowCount; $grid.RowCount = $row + 1
    [void]$grid.RowStyles.Add((New-Object System.Windows.Forms.RowStyle([System.Windows.Forms.SizeType]::AutoSize)))
    $grid.Controls.Add($l, 0, $row); $grid.SetColumnSpan($l, 3)
}
function Add-Row($grid, [string]$label, [System.Windows.Forms.Control]$middle, [System.Windows.Forms.Control]$right) {
    $row = $grid.RowCount; $grid.RowCount = $row + 1
    [void]$grid.RowStyles.Add((New-Object System.Windows.Forms.RowStyle([System.Windows.Forms.SizeType]::AutoSize)))
    if ($label) {
        $l = New-Label $label 'muted'; $l.Margin = New-Pad 0 8 12 2
        $grid.Controls.Add($l, 0, $row)
    }
    if ($middle) {
        if ($middle -is [System.Windows.Forms.TextBox] -or $middle -is [System.Windows.Forms.Label]) { $middle.Dock = 'Fill' }
        if ($middle.Margin.Top -lt 6) { $middle.Margin = New-Pad 0 5 8 2 }
        $grid.Controls.Add($middle, 1, $row)
        if (-not $label) { $grid.SetColumn($middle, 0); $grid.SetColumnSpan($middle, 2) }
    }
    if ($right) { $grid.Controls.Add($right, 2, $row) }
}
function New-PathLabel([string]$text) {
    $l = New-Label $text
    $l.AutoEllipsis = $true; $l.AutoSize = $false; $l.Height = 22
    return $l
}

# ---------------------------------------------------------------- page: saves & backups
$pageSaves = New-Page
$pageSaves.AutoScroll = $true
$savesGrid = New-FormGrid
$btnOpenGame = New-Button 'Open'
$btnOpenUser = New-Button 'Open'
$btnOpenBackups = New-Button 'Open'
$btnBackup = New-Button 'Back up my saves now' 'accent'
$lblPrev = New-PathLabel ''
$btnRollback = New-Button 'Swap back to it'
Add-Section $savesGrid 'Your saves'
Add-Row $savesGrid 'Saves, settings, mods' (New-PathLabel $UserDir) $btnOpenUser
Add-Row $savesGrid 'Save backups' (New-PathLabel $BackupDir) $btnOpenBackups
Add-Row $savesGrid '' $btnBackup $null
Add-Section $savesGrid 'Game builds'
Add-Row $savesGrid 'Installed game' (New-PathLabel $GameDir) $btnOpenGame
Add-Row $savesGrid 'Previous build' $lblPrev $btnRollback
$lblSavesHelp = New-Label ("Updates only replace the Game folder. Your saves, settings and mods stay in UserData. " +
    "Saves played in a newer build may not load in an older one, so back up first before swapping back.") 'muted'
$lblSavesHelp.Dock = 'Top'; $lblSavesHelp.AutoSize = $false; $lblSavesHelp.Height = 60; $lblSavesHelp.Padding = New-Pad 0 18 0 0
$script:ActionButtons.Add($btnBackup); $script:ActionButtons.Add($btnRollback)
$pageSaves.Controls.Add($lblSavesHelp)
$pageSaves.Controls.Add($savesGrid)

# ---------------------------------------------------------------- page: settings
$pageSettings = New-Page
$pageSettings.AutoScroll = $true
$setGrid = New-FormGrid

$cbTheme = New-ThemedCombo 230
foreach ($k in $Themes.Keys) { [void]$cbTheme.Items.Add($Themes[$k].Label) }
$cbTheme.SelectedIndex = [array]::IndexOf(@($Themes.Keys), $script:ThemeKey)

$variantBox = New-Object System.Windows.Forms.FlowLayoutPanel
$variantBox.AutoSize = $true; $variantBox.WrapContents = $false; $variantBox.Margin = New-Pad 0 4 8 0
$rbGfx = New-Object System.Windows.Forms.RadioButton
$rbGfx.Text = 'Graphics'; $rbGfx.AutoSize = $true; $rbGfx.Margin = New-Pad 0 4 16 0
$rbSnd = New-Object System.Windows.Forms.RadioButton
$rbSnd.Text = 'Graphics + sounds (official builds only)'; $rbSnd.AutoSize = $true; $rbSnd.Margin = New-Pad 0 4 0 0
$variantBox.Controls.AddRange(@($rbGfx, $rbSnd))
if ($Settings.Variant -eq 'sounds') { $rbSnd.Checked = $true } else { $rbGfx.Checked = $true }

$chkClose = New-Object System.Windows.Forms.CheckBox
$chkClose.Text = 'Close the launcher when the game starts'; $chkClose.AutoSize = $true
$chkClose.Checked = [bool]$Settings.CloseOnPlay
$chkCheckBuilds = New-Object System.Windows.Forms.CheckBox
$chkCheckBuilds.Text = 'Look for new game builds when the launcher opens'; $chkCheckBuilds.AutoSize = $true
$chkCheckBuilds.Checked = [bool]$Settings.CheckBuildsOnStart
$chkCheckLauncher = New-Object System.Windows.Forms.CheckBox
$chkCheckLauncher.Text = 'Look for launcher updates when the launcher opens'; $chkCheckLauncher.AutoSize = $true
$chkCheckLauncher.Checked = [bool]$Settings.CheckLauncherOnStart
$btnLauncherUpdate = New-Button 'Check now'
$lblLauncherVer = New-Label "Version $LauncherVersion"
$txtModSource = New-Object System.Windows.Forms.TextBox
$txtModSource.Text = $Settings.ModSource
$btnBrowseSource = New-Button 'Browse...'
$btnShortcut = New-Button 'Create desktop shortcut'

Add-Section $setGrid 'Appearance'
Add-Row $setGrid 'Theme' $cbTheme $null
Add-Section $setGrid 'Game'
Add-Row $setGrid 'Download' $variantBox $null
Add-Row $setGrid '' $chkClose $null
Add-Section $setGrid 'Updates'
Add-Row $setGrid '' $chkCheckBuilds $null
Add-Row $setGrid '' $chkCheckLauncher $null
Add-Row $setGrid 'Launcher' $lblLauncherVer $btnLauncherUpdate
Add-Section $setGrid 'Mods'
Add-Row $setGrid 'My mods come from' $txtModSource $btnBrowseSource
Add-Section $setGrid 'Shortcut'
Add-Row $setGrid '' $btnShortcut $null
$lblSetHelp = New-Label ("'My mods come from' is only for people with a copy of the fork's source; testers can leave it empty. " +
    "Settings are saved in Launcher\settings.json.") 'muted'
$lblSetHelp.Dock = 'Top'; $lblSetHelp.AutoSize = $false; $lblSetHelp.Height = 60; $lblSetHelp.Padding = New-Pad 0 18 0 0
foreach ($c in @($rbGfx, $rbSnd, $btnLauncherUpdate)) { $script:ActionButtons.Add($c) }
$pageSettings.Controls.Add($lblSetHelp)
$pageSettings.Controls.Add($setGrid)

$main.Controls.Add($content)
$main.Controls.Add($footer)
$main.Controls.Add($header)
$form.Controls.Add($main)
$form.Controls.Add($side)
$main.BringToFront()

$script:Pages = @{ game = $pageGame; mods = $pageMods; saves = $pageSaves; settings = $pageSettings }
$script:PageTitles = @{ game = ''; mods = 'Mods'; saves = 'Saves & backups'; settings = 'Settings' }
$script:CurrentPage = 'game'
function Show-Page([string]$name) {
    if (-not $script:Pages.ContainsKey($name)) { $name = 'game' }
    $script:CurrentPage = $name
    foreach ($k in $script:Pages.Keys) { $script:Pages[$k].Visible = ($k -eq $name) }
    Set-ControlTheme $script:Pages[$name]   # pages shown for the first time get their native (scrollbar) theme now
    $form.ActiveControl = $null   # no focus border on the sidebar button
    foreach ($b in $script:NavButtons.Values) { Set-ButtonStyle $b 'nav' }
    $lblTitle.Text = if ($name -eq 'game') { (Get-Source).Label } else { $script:PageTitles[$name] }
}

# ---------------------------------------------------------------- job timer
$jobTimer = New-Object System.Windows.Forms.Timer
$jobTimer.Interval = 200
$jobTimer.add_Tick({
    $job = $script:Job
    if (-not $job) { $jobTimer.Stop(); return }
    $lblStatus.Text = [string]$job.Sync.Status
    Set-Progress ([int]$job.Sync.Percent)
    if (-not $job.Handle.IsCompleted) { return }

    $jobTimer.Stop()
    $script:Job = $null
    $result = $null; $err = $null
    try {
        $out = $job.PS.EndInvoke($job.Handle)
        $result = $out | Select-Object -First 1
        if ($out.Count -gt 1) { $result = @($out) }
    } catch {
        $ex = $_.Exception
        while ($ex.InnerException) { $ex = $ex.InnerException }
        $err = $ex.Message
    }
    if (-not $err -and $job.PS.Streams.Error.Count -gt 0) { $err = $job.PS.Streams.Error[0].ToString() }
    $job.PS.Dispose()
    $script:Busy = $false
    Set-ActionsEnabled $true
    Set-Progress 0
    if ($err -and $job.QuietErrors) {
        $lblStatus.Text = 'Ready.'
    } elseif ($err) {
        $lblStatus.Text = "Something went wrong: $err"
        Show-Error "Something went wrong:`r`n`r`n$err"
    } else {
        $lblStatus.Text = 'Ready.'
    }
    if ($job.OnDone) { Invoke-Safe { & $job.OnDone $result $err } }
})

# ---------------------------------------------------------------- UI logic
function Update-VersionLabels {
    $inst = Get-InstalledInfo $GameDir
    $newest = $script:Releases | Select-Object -First 1
    $text = if ($inst) {
        $v = if ($inst.Variant -eq 'sounds') { 'graphics + sounds' } elseif ($inst.Variant) { 'graphics' } else { '' }
        "Installed: $(Get-ShortTag $inst.Tag)" + $(if ($v) { "  ($v)" } else { '' })
    } else { 'Installed: nothing yet - pick a build and press Install' }
    $sameSource = $inst -and ((Get-TagSource $inst.Tag) -eq $script:Settings.Source)
    $behind = $false
    if ($newest) {
        $text += "      Newest: $(Get-ShortTag $newest.Tag)"
        if ($inst -and $inst.Tag -eq $newest.Tag) { $text += '   - you are up to date' }
        elseif ($sameSource -and [string]::CompareOrdinal($inst.Tag, $newest.Tag) -lt 0) { $text += '   - UPDATE AVAILABLE'; $behind = $true }
        elseif ($inst -and -not $sameSource) { $text += "   (yours isn't from this list)" }
    }
    $lblVersions.Text = $text
    $script:VersionsWarn = $behind
    $lblVersions.ForeColor = if ($behind) { $T.Warn } else { $T.Muted }

    $prev = Get-InstalledInfo $PrevDir
    $lblPrev.Text = if ($prev) { "$(Get-ShortTag $prev.Tag)  (kept in $PrevDir)" } else { '(none kept yet - one is kept after your first update)' }
    $btnPlay.Enabled = (-not $script:Busy) -and [bool]$inst
}

function Show-Releases {
    $inst = Get-InstalledInfo $GameDir
    $bold = New-Object System.Drawing.Font($lvReleases.Font, [System.Drawing.FontStyle]::Bold)
    $lvReleases.BeginUpdate()
    $lvReleases.Items.Clear()
    $first = $true
    foreach ($r in $script:Releases) {
        $item = New-Object System.Windows.Forms.ListViewItem((Get-ShortTag $r.Tag))
        [void]$item.SubItems.Add($r.Date.ToString('d MMM, h:mm tt'))
        $size = if ((Get-ChosenVariant) -eq 'sounds' -and $r.SndSize) { $r.SndSize } else { $r.GfxSize }
        [void]$item.SubItems.Add((Format-Size $size))
        $notes = @()
        if ($first) { $notes += 'newest' }
        if ($inst -and $inst.Tag -eq $r.Tag) { $notes += 'INSTALLED'; $item.Font = $bold }
        [void]$item.SubItems.Add(($notes -join ', '))
        $item.Tag = $r
        [void]$lvReleases.Items.Add($item)
        $first = $false
    }
    $lvReleases.EndUpdate()
    if ($lvReleases.Items.Count -gt 0) { $lvReleases.Items[0].Selected = $true }
}

function Get-SelectedRelease {
    if ($lvReleases.SelectedItems.Count -gt 0) { return $lvReleases.SelectedItems[0].Tag }
    return ($script:Releases | Select-Object -First 1)
}

function Update-Releases {
    Start-Work "Checking GitHub for $((Get-Source).Label)..." $FetchWork (Get-FetchParams) {
        param($res, $err)
        if ($err) { return }
        $script:Releases = @($res)
        Show-Releases
        Update-VersionLabels
        $inst = Get-InstalledInfo $GameDir
        $newest = $script:Releases | Select-Object -First 1
        $sameSource = $inst -and ((Get-TagSource $inst.Tag) -eq $script:Settings.Source)
        if (-not $newest) { $lblStatus.Text = "No $((Get-Source).Label) found on GitHub." }
        elseif (-not $inst) { $lblStatus.Text = "Found $($script:Releases.Count) builds. Pick one (the top one is newest) and press Install." }
        elseif ($inst.Tag -eq $newest.Tag) { $lblStatus.Text = 'You have the newest build.' }
        elseif (-not $sameSource) { $lblStatus.Text = "Your installed build is from the other list. Press 'Install / update' to switch to $(Get-ShortTag $newest.Tag)." }
        else {
            $lblStatus.Text = "Update available: $(Get-ShortTag $newest.Tag). Press 'Install / update' to get it."
            if (-not $script:UpdateNoticeShown -and -not $Screenshot) {
                $script:UpdateNoticeShown = $true
                $msg = "A new build is out: $(Get-ShortTag $newest.Tag) (you have $(Get-ShortTag $inst.Tag)).`r`n`r`n" +
                       "Install it now? Your saves, settings and mods are kept."
                if (Confirm-Action $msg) { Install-Selected; return }
            }
        }
        if (-not $script:LauncherChecked -and ($script:Settings.CheckLauncherOnStart -or (Get-MissingAssets).Count)) { $script:LauncherChecked = $true; Test-LauncherUpdate -Quiet }
    }
}

function Test-LauncherUpdate([switch]$Quiet) {
    if (Test-InRepo) {
        if (-not $Quiet) { Show-Info "This launcher is running from the fork's source folder, so it doesn't update itself. Pull the fork instead." }
        return
    }
    $script:LauncherCheckQuiet = [bool]$Quiet
    $params = @{ Url = $LauncherUpdateUrl; Current = $LauncherVersion; Assets = $LauncherAssets; Missing = (Get-MissingAssets) }
    Start-Work 'Checking for a launcher update...' $LauncherCheckWork $params {
        param($res, $err)
        if ($err) { return }
        if ($res -and -not $res.Text) {
            # Same version, but icon files were missing: put them back and show them.
            Save-LauncherAssets $res.Assets
            $ico = Read-LauncherIcon; if ($ico) { $form.Icon = $ico }
            $img = Read-LauncherImage 'launcher-icon.png'; if ($img) { $picLogo.Image = $img }
            $res = $null
        }
        if (-not $res) { if (-not $script:LauncherCheckQuiet) { Show-Info "The launcher is up to date (version $LauncherVersion)." }; $lblStatus.Text = 'Ready.'; return }
        if (-not (Test-LauncherScript $res.Text)) { Show-Error 'A launcher update was found but it looks broken, so it was not installed.'; return }
        if (-not (Confirm-Action "A new launcher is available (version $($res.Version), you have $LauncherVersion).`r`n`r`nUpdate and restart the launcher now?")) { return }
        Install-LauncherUpdate $res.Text
        Save-LauncherAssets $res.Assets
        Start-Process powershell.exe -ArgumentList @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-STA', '-WindowStyle', 'Hidden', '-File', "`"$(Join-Path $LauncherDir 'cdda-launcher.ps1')`"")
        $form.Close()
    } -QuietErrors $script:LauncherCheckQuiet
}

function Test-GameRunning {
    if (Get-Process -Name $ProcName -ErrorAction SilentlyContinue) {
        Show-Info 'The game is running. Close it first, then try again.'
        return $true
    }
    return $false
}

function Install-Selected {
    $r = Get-SelectedRelease
    if (-not $r) { Show-Info 'No builds loaded yet. Press "Refresh list".'; return }
    $variant = Get-ChosenVariant
    if ($variant -eq 'sounds') { $url = $r.SndUrl; $size = $r.SndSize; $zipName = $r.SndName }
    else { $url = $r.GfxUrl; $size = $r.GfxSize; $zipName = $r.GfxName }
    if (-not $url) {
        Show-Info "Build $(Get-ShortTag $r.Tag) has no '$(if ($variant -eq 'sounds') { 'Graphics + sounds' } else { 'Graphics' })' download. Change the download type in Settings, or pick another build."
        return
    }
    if (Test-GameRunning) { return }
    $inst = Get-InstalledInfo $GameDir
    if ($inst -and $inst.Tag -eq $r.Tag -and $inst.Variant -eq $variant) {
        if (-not (Confirm-Action "$(Get-ShortTag $r.Tag) is already installed. Reinstall it?")) { return }
    }
    $params = @{
        Url = $url; Size = $size; ZipName = $zipName; Tag = $r.Tag; Short = (Get-ShortTag $r.Tag); Variant = $variant
        DownloadDir = $DownloadDir; StagingDir = $StagingDir; GameDir = $GameDir; PrevDir = $PrevDir; ExeName = $ExeName
    }
    Start-Work "Downloading $(Get-ShortTag $r.Tag)..." $InstallWork $params {
        param($res, $err)
        if ($err) { return }
        Show-Releases
        Update-VersionLabels
        $lblStatus.Text = "Installed $($res.Short). Press PLAY!"
    }
}

function Show-WhatsNew {
    $inst = Get-InstalledInfo $GameDir
    $r = Get-SelectedRelease
    if (-not $inst -or -not (Get-TagSource $inst.Tag)) { Show-Info 'Install a build first - then this shows every change between your build and the selected one.'; return }
    if ((Get-TagSource $inst.Tag) -ne $script:Settings.Source) { Show-Info "Your installed build isn't from this list, so there's nothing to compare. Switch 'Builds from' back, or install one from this list."; return }
    if (-not $r) { return }
    if ($inst.Tag -eq $r.Tag) { Show-Info 'The selected build is the one you have. Select a newer one in the list.'; return }
    $base = $inst.Tag; $head = $r.Tag
    if ([string]::CompareOrdinal($base, $head) -gt 0) { $base = $r.Tag; $head = $inst.Tag }
    $script:CompareTitle = "Changes from $(Get-ShortTag $base) to $(Get-ShortTag $head)"
    Start-Work 'Asking GitHub what changed...' $CompareWork @{ Repo = (Get-Source).Repo; Base = $base; Head = $head } {
        param($res, $err)
        if ($err) { return }
        $txt = "$($script:CompareTitle): $($res.Total) commits"
        if ($res.Total -gt $res.Shown) { $txt += " (showing the newest $($res.Shown))" }
        $txtNotes.Text = $txt + "`r`n`r`n" + ($res.Lines -join "`r`n")
        $lblStatus.Text = 'Ready.'
    }
}

function Update-ModList {
    $lvMods.BeginUpdate()
    $lvMods.Items.Clear()
    if (Test-Path -LiteralPath $UserModsDir) {
        foreach ($d in (Get-ChildItem -LiteralPath $UserModsDir -Directory | Sort-Object Name)) {
            $i = Get-ModInfo $d.FullName
            $origin = $script:Settings.ModOrigins[$d.Name]
            if (-not $origin) { $origin = 'added by hand' }
            $item = New-Object System.Windows.Forms.ListViewItem($i.Name)
            [void]$item.SubItems.Add($i.Id)
            [void]$item.SubItems.Add($i.Folder)
            [void]$item.SubItems.Add($origin)
            $item.Tag = $d.FullName
            [void]$lvMods.Items.Add($item)
        }
    }
    $lvMods.EndUpdate()
    $script:NavButtons['mods'].Text = "    Mods ($($lvMods.Items.Count))"
}

# Copying mods from a fork checkout only makes sense on a developer's PC.
function Update-SyncButton {
    $src = $txtModSource.Text.Trim()
    $btnSync.Visible = [bool]($src -and (Test-Path -LiteralPath $src))
}

function Get-VanillaModFolders {
    $gm = Join-Path $GameDir 'data\mods'
    if (-not (Test-Path -LiteralPath $gm)) { return $null }
    return @(Get-ChildItem -LiteralPath $gm -Directory | ForEach-Object { $_.Name })
}

function Show-SyncDialog {
    $src = $txtModSource.Text.Trim()
    if (-not $src -or -not (Test-Path -LiteralPath $src)) {
        Show-Info "Set 'My mods come from' in Settings first (your fork's data\mods folder)."
        return
    }
    $vanilla = Get-VanillaModFolders
    if ($null -eq $vanilla) { Show-Info 'Install a game build first, so the launcher can tell which mods are your own and which come with the game.'; return }

    $custom = @(Get-ChildItem -LiteralPath $src -Directory | Where-Object { $vanilla -notcontains $_.Name } | Sort-Object Name)
    if ($custom.Count -eq 0) {
        Show-Info "Every mod folder in`r`n$src`r`nalso ships with the game, so there is nothing of your own to copy."
        return
    }

    $dlg = New-Object System.Windows.Forms.Form
    $dlg.Text = 'Sync my mods from my fork'
    $dlg.Size = New-Size 560 460
    $dlg.StartPosition = 'CenterParent'
    $dlg.Font = $form.Font
    $dlg.MinimizeBox = $false; $dlg.MaximizeBox = $false
    $dlg.Padding = New-Pad 12 8 12 8
    $info = New-Label ("These folders are in your fork but not in the installed game. Ticked ones get copied " +
                 "(replacing older copies) into UserData\mods.`r`nFrom: $src") 'muted'
    $info.Dock = 'Top'; $info.AutoSize = $false; $info.Height = 58
    $list = New-Object System.Windows.Forms.CheckedListBox
    $list.Dock = 'Fill'; $list.CheckOnClick = $true; $list.IntegralHeight = $false; $list.BorderStyle = 'None'
    foreach ($d in $custom) {
        $mi = Get-ModInfo $d.FullName
        $label = if ($mi.Name -ne $d.Name) { "$($d.Name)   -   $($mi.Name)" } else { $d.Name }
        $tick = ($d.Name -notmatch '^(TEST_DATA|Standard_Combat_Tests)$')
        [void]$list.Items.Add([pscustomobject]@{ Folder = $d.Name; Label = $label }, $tick)
    }
    $list.DisplayMember = 'Label'
    $bar = New-Object System.Windows.Forms.FlowLayoutPanel
    $bar.Dock = 'Bottom'; $bar.AutoSize = $true; $bar.FlowDirection = 'RightToLeft'
    $ok = New-Button 'Copy ticked mods' 'accent'; $ok.DialogResult = 'OK'
    $cancel = New-Button 'Cancel'; $cancel.DialogResult = 'Cancel'
    $bar.Controls.Add($cancel); $bar.Controls.Add($ok)
    $dlg.AcceptButton = $ok; $dlg.CancelButton = $cancel
    $dlg.Controls.Add($list); $dlg.Controls.Add($bar); $dlg.Controls.Add($info)
    $list.BringToFront()
    $dlg.BackColor = $T.Back
    Set-ControlTheme $dlg
    $dlg.add_Shown({ param($s, $e) if ($script:HasNative) { $v = if ($T.Dark) { 1 } else { 0 }; try { [void][CddaLauncher.Native]::DwmSetWindowAttribute($s.Handle, 20, [ref]$v, 4) } catch { } } })

    if ($dlg.ShowDialog($form) -ne 'OK') { $dlg.Dispose(); return }
    $names = @($list.CheckedItems | ForEach-Object { $_.Folder })
    $dlg.Dispose()
    if ($names.Count -eq 0) { return }

    Start-Work 'Copying your mods...' $SyncWork @{ Source = $src; Names = $names; UserModsDir = $UserModsDir } {
        param($res, $err)
        if ($err) { return }
        foreach ($n in $res.Names) { $script:Settings.ModOrigins[$n] = 'your fork' }
        Save-Settings
        Update-ModList
        $lblStatus.Text = "Copied $($res.Names.Count) mod(s) from your fork: $($res.Names -join ', ')"
    }
}

function Add-ModFromGitHub {
    $g = ConvertFrom-GitHubUrl $txtGh.Text
    if (-not $g) { Show-Info 'Paste a GitHub link like https://github.com/owner/repo (a link to a branch also works).'; return }
    $url = "https://api.github.com/repos/$($g.Owner)/$($g.Repo)/zipball"
    if ($g.Ref) { $url += "/$($g.Ref)" }
    $origin = "GitHub: $($g.Owner)/$($g.Repo)" + $(if ($g.Ref) { " ($($g.Ref))" } else { '' })
    $params = @{
        Url = $url; ZipName = "mod-$($g.Owner)-$($g.Repo).zip"; Label = "$($g.Owner)/$($g.Repo)"; IsGitHub = $true
        FallbackName = $g.Repo; Origin = $origin; Root = $Root; DownloadDir = $DownloadDir; UserModsDir = $UserModsDir
    }
    Start-Work "Downloading $($g.Owner)/$($g.Repo)..." $ModInstallWork $params ${function:Complete-ModInstall}
}

function Add-ModFromZip {
    $ofd = New-Object System.Windows.Forms.OpenFileDialog
    $ofd.Filter = 'Zip files (*.zip)|*.zip'
    $ofd.Title = 'Pick a mod zip'
    if ($ofd.ShowDialog($form) -ne 'OK') { return }
    $file = $ofd.FileName
    $params = @{
        Url = ''; ZipPath = $file; Label = (Split-Path $file -Leaf); IsGitHub = $false
        FallbackName = [System.IO.Path]::GetFileNameWithoutExtension($file); Origin = "zip: $(Split-Path $file -Leaf)"
        Root = $Root; DownloadDir = $DownloadDir; UserModsDir = $UserModsDir
    }
    Start-Work 'Installing mod from zip...' $ModInstallWork $params ${function:Complete-ModInstall}
}

function Complete-ModInstall($res, $err) {
    if ($err) { return }
    foreach ($n in $res.Names) { $script:Settings.ModOrigins[$n] = $res.Origin }
    Save-Settings
    Update-ModList
    $txtGh.Text = ''
    $vanilla = Get-VanillaModFolders
    $clash = @($res.Names | Where-Object { $vanilla -and $vanilla -contains $_ })
    $lblStatus.Text = "Installed mod(s): $($res.Names -join ', ')"
    if ($clash.Count) {
        Show-Info "Heads up: the game already ships a mod folder called $($clash -join ', '). If the mod IDs match, the game will complain about duplicates - remove one of them."
    }
}

function Remove-SelectedMods {
    $sel = @($lvMods.SelectedItems)
    if ($sel.Count -eq 0) { Show-Info 'Select one or more mods in the list first.'; return }
    $names = ($sel | ForEach-Object { $_.Text }) -join ', '
    if (-not (Confirm-Action "Remove $names?`r`n`r`nThe folders go to the Recycle Bin, so you can get them back.")) { return }
    foreach ($it in $sel) {
        [Microsoft.VisualBasic.FileIO.FileSystem]::DeleteDirectory([string]$it.Tag, 'OnlyErrorDialogs', 'SendToRecycleBin')
        $script:Settings.ModOrigins.Remove((Split-Path ([string]$it.Tag) -Leaf))
    }
    Save-Settings
    Update-ModList
    $lblStatus.Text = "Removed: $names"
}

function Start-Game {
    $exe = Join-Path $GameDir $ExeName
    if (-not (Test-Path -LiteralPath $exe)) { Show-Info 'No game installed yet. Pick a build on the Game page and press Install.'; return }
    if (-not (Test-Path -LiteralPath $UserDir)) { New-Item -ItemType Directory -Force -Path $UserDir | Out-Null }
    # forward slashes so the trailing slash can't escape the closing quote
    $ud = ($UserDir -replace '\\', '/').TrimEnd('/') + '/'
    Start-Process -FilePath $exe -WorkingDirectory $GameDir -ArgumentList "--userdir `"$ud`""
    $lblStatus.Text = 'Game started. Have fun!'
    if ($chkClose.Checked) { $form.Close() }
}

function Backup-Saves {
    $save = Join-Path $UserDir 'save'
    if (-not (Test-Path -LiteralPath $save) -or -not (Get-ChildItem -LiteralPath $save -Force | Select-Object -First 1)) {
        Show-Info 'There are no saves to back up yet.'
        return
    }
    $dest = Join-Path $BackupDir ('saves-{0:yyyy-MM-dd_HHmm}.zip' -f (Get-Date))
    Start-Work 'Zipping your saves...' $BackupWork @{ Save = $save; Dest = $dest; BackupDir = $BackupDir } {
        param($res, $err)
        if ($err) { return }
        $lblStatus.Text = "Saves backed up to $($res.Path) ($(Format-Size $res.Size))"
    }
}

function Restore-Previous {
    $prev = Get-InstalledInfo $PrevDir
    if (-not $prev) { Show-Info 'No previous build is kept yet.'; return }
    if (Test-GameRunning) { return }
    $msg = "Swap back to $(Get-ShortTag $prev.Tag)?`r`n`r`nThe current build becomes the previous one, so you can swap again later.`r`n" +
           "Saves played in a newer build may not load in an older one, so back up your saves first if unsure."
    if (-not (Confirm-Action $msg)) { return }
    $tmp = Join-Path $Root '_swap'
    Move-Item -LiteralPath $GameDir -Destination $tmp
    Move-Item -LiteralPath $PrevDir -Destination $GameDir
    Move-Item -LiteralPath $tmp -Destination $PrevDir
    Show-Releases
    Update-VersionLabels
    $lblStatus.Text = "Now on $(Get-ShortTag $prev.Tag)."
}

function New-DesktopShortcut {
    # Prefer the .exe (it carries the icon); older folders may only have the .bat.
    $target = Join-Path $LauncherHome $LauncherExeName
    if (-not (Test-Path -LiteralPath $target)) { $target = Join-Path $LauncherHome 'Start CDDA Launcher.bat' }
    $lnk = Join-Path ([Environment]::GetFolderPath('Desktop')) 'CDDA Fork Launcher.lnk'
    $sh = New-Object -ComObject WScript.Shell
    $s = $sh.CreateShortcut($lnk)
    $s.TargetPath = $target
    $s.WorkingDirectory = $LauncherHome
    $s.Description = 'CDDA Fork Launcher'
    $ico = Join-Path $LauncherDir 'launcher.ico'
    if ($target -like '*.exe') { $s.IconLocation = "$target,0" } elseif (Test-Path -LiteralPath $ico) { $s.IconLocation = "$ico,0" }
    $s.Save()
    $lblStatus.Text = "Shortcut created: $lnk"
}

# ---------------------------------------------------------------- events
$lvReleases.add_SelectedIndexChanged({ Invoke-Safe { if ($lvReleases.SelectedItems.Count) { $txtNotes.Text = Format-ReleaseNotes $lvReleases.SelectedItems[0].Tag } } })
$lvReleases.add_DoubleClick({ Invoke-Safe { Install-Selected } })
$btnRefresh.add_Click({ Invoke-Safe { Update-Releases } })
$btnInstall.add_Click({ Invoke-Safe { Install-Selected } })
$btnWhatsNew.add_Click({ Invoke-Safe { Show-WhatsNew } })
$btnGitHub.add_Click({ Invoke-Safe {
    $r = Get-SelectedRelease
    $url = if ($r) { $r.HtmlUrl } else { "https://github.com/$((Get-Source).Repo)/releases" }
    Start-Process $url
} })
$cbSource.add_SelectedIndexChanged({ Invoke-Safe {
    $k = @($Sources.Keys)[$cbSource.SelectedIndex]
    if ($k -eq $script:Settings.Source) { return }
    $script:Settings.Source = $k
    Save-Settings
    if ($script:CurrentPage -eq 'game') { $lblTitle.Text = (Get-Source).Label }
    $rbSnd.Enabled = [bool](Get-Source).SndPattern
    $script:Releases = @()
    $txtNotes.Text = ''
    Show-Releases
    Update-Releases
} })
$btnPlay.add_Click({ Invoke-Safe { Start-Game } })

$btnSync.add_Click({ Invoke-Safe { Show-SyncDialog } })
$btnAddZip.add_Click({ Invoke-Safe { Add-ModFromZip } })
$btnAddGh.add_Click({ Invoke-Safe { Add-ModFromGitHub } })
$txtGh.add_KeyDown({ param($s, $e) if ($e.KeyCode -eq 'Enter') { $e.SuppressKeyPress = $true; Invoke-Safe { Add-ModFromGitHub } } })
$btnRemoveMod.add_Click({ Invoke-Safe { Remove-SelectedMods } })
$btnOpenMods.add_Click({ Invoke-Safe { Open-Folder $UserModsDir } })

$btnOpenGame.add_Click({ Invoke-Safe { Open-Folder $GameDir } })
$btnOpenUser.add_Click({ Invoke-Safe { Open-Folder $UserDir } })
$btnOpenBackups.add_Click({ Invoke-Safe { Open-Folder $BackupDir } })
$btnRollback.add_Click({ Invoke-Safe { Restore-Previous } })
$btnBackup.add_Click({ Invoke-Safe { Backup-Saves } })

$cbTheme.add_SelectedIndexChanged({ Invoke-Safe {
    $k = @($Themes.Keys)[$cbTheme.SelectedIndex]
    if ($k -eq $script:ThemeKey) { return }
    $script:Settings.Theme = $k
    Save-Settings
    Set-Theme $k
} })
$rbGfx.add_CheckedChanged({ if ($rbGfx.Checked) { $script:Settings.Variant = 'graphics'; Save-Settings; Show-Releases } })
$rbSnd.add_CheckedChanged({ if ($rbSnd.Checked) { $script:Settings.Variant = 'sounds'; Save-Settings; Show-Releases } })
$chkClose.add_CheckedChanged({ $script:Settings.CloseOnPlay = $chkClose.Checked; Save-Settings })
$chkCheckBuilds.add_CheckedChanged({ $script:Settings.CheckBuildsOnStart = $chkCheckBuilds.Checked; Save-Settings })
$chkCheckLauncher.add_CheckedChanged({ $script:Settings.CheckLauncherOnStart = $chkCheckLauncher.Checked; Save-Settings })
$btnLauncherUpdate.add_Click({ Invoke-Safe { Test-LauncherUpdate } })
$btnBrowseSource.add_Click({ Invoke-Safe {
    $fbd = New-Object System.Windows.Forms.FolderBrowserDialog
    $fbd.Description = "Pick your fork's data\mods folder"
    if ($txtModSource.Text -and (Test-Path -LiteralPath $txtModSource.Text)) { $fbd.SelectedPath = $txtModSource.Text }
    if ($fbd.ShowDialog($form) -eq 'OK') { $txtModSource.Text = $fbd.SelectedPath }
} })
$txtModSource.add_TextChanged({ $script:Settings.ModSource = $txtModSource.Text.Trim(); Save-Settings; Update-SyncButton })
$btnShortcut.add_Click({ Invoke-Safe { New-DesktopShortcut } })

$form.add_Shown({
  try {
    $split.SplitterDistance = [int]($split.Width * 0.55)
    $rbSnd.Enabled = [bool](Get-Source).SndPattern
    Set-Theme $script:ThemeKey
    Show-Page $(if ($Page) { $Page } else { 'game' })
    Update-SyncButton
    Update-VersionLabels
    Update-ModList
    if ($script:Settings.CheckBuildsOnStart -or $Screenshot) {
        Update-Releases
    } else {
        $lblStatus.Text = "Press 'Refresh list' to look for new builds."
        if ($script:Settings.CheckLauncherOnStart -or (Get-MissingAssets).Count) { $script:LauncherChecked = $true; Test-LauncherUpdate -Quiet }
    }
  } catch {
    $msg = "$($_.Exception.Message)`r`n$($_.InvocationInfo.PositionMessage)"
    if ($Screenshot) { Set-Content -LiteralPath "$Screenshot.error.txt" -Value $msg; $form.Close(); return }
    Show-Error "The launcher hit a problem while starting:`r`n`r`n$msg"
  }
    if ($Screenshot) {
        $script:ShotTimer = New-Object System.Windows.Forms.Timer
        $script:ShotTimer.Interval = 500
        $script:ShotTimer.add_Tick({
            if ($script:Busy) { return }
            $script:ShotTimer.Stop()
            $bmp = New-Object System.Drawing.Bitmap($form.Width, $form.Height)
            $form.DrawToBitmap($bmp, (New-Object System.Drawing.Rectangle(0, 0, $form.Width, $form.Height)))
            $bmp.Save($Screenshot, [System.Drawing.Imaging.ImageFormat]::Png)
            $bmp.Dispose()
            $form.Close()
        })
        $script:ShotTimer.Start()
    }
})

$form.add_FormClosing({
    param($s, $e)
    if ($script:Busy -and -not $Screenshot) {
        if (-not (Confirm-Action 'A task is still running (for example a download). Quit anyway?')) { $e.Cancel = $true; return }
        try { $script:Job.PS.BeginStop($null, $null) | Out-Null } catch { }
    }
})

[void]$form.ShowDialog()
$form.Dispose()
