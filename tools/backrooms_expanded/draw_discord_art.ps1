param([string]$OutDir)
# Xbro10 fork: draws the Discord "Playing" card pictures (512x512 PNGs) from the mod's own sprite sheets
# and the launcher icon. Upload them to the Discord application (Rich Presence > Art Assets) under these
# names, which data/discord_presence.json uses:
#   level0, level1, level2, cdda_world, cdda_menu, fork_logo
# cdda_world needs the UndeadPeople tileset (gfx/MSX++UnDeadPeopleEdition) for the overworld tiles.
Add-Type -AssemblyName System.Drawing
$root = Resolve-Path "$PSScriptRoot\..\.."
$gfx = "$root\data\mods\Backrooms_Expanded\gfx"
New-Item -ItemType Directory -Force $OutDir | Out-Null

function New-Card { $b = New-Object System.Drawing.Bitmap 512, 512; $g = [System.Drawing.Graphics]::FromImage($b); $g.InterpolationMode = 'NearestNeighbor'; $g.PixelOffsetMode = 'Half'; return @($b, $g) }
function Blit($g, $sheet, $index, $x, $y, $size, $sw = 32) {
  $src = New-Object System.Drawing.Rectangle ($index * $sw), 0, $sw, $sw
  $g.DrawImage($sheet, (New-Object System.Drawing.Rectangle $x, $y, $size, $size), $src, [System.Drawing.GraphicsUnit]::Pixel)
}
function Vignette($g, $alpha) {
  $path = New-Object System.Drawing.Drawing2D.GraphicsPath; $path.AddEllipse(-160, -160, 832, 832)
  $br = New-Object System.Drawing.Drawing2D.PathGradientBrush $path
  $br.CenterColor = [System.Drawing.Color]::FromArgb(0, 0, 0, 0); $br.SurroundColors = @([System.Drawing.Color]::FromArgb($alpha, 0, 0, 0))
  $g.FillRectangle($br, 0, 0, 512, 512)
}
function Save($b, $name) { $b.Save("$OutDir\$name.png", [System.Drawing.Imaging.ImageFormat]::Png); $b.Dispose(); "saved $name.png" }

# --- Level 0: a lit yellow room from our own tiles, a Smiler waiting in the dark corner ---
$tiles = [System.Drawing.Image]::FromFile("$gfx\br0_tiles.png")
$ents = [System.Drawing.Image]::FromFile("$gfx\br_entities.png")
$room = @(
  '########',
  '#.L..L.#',
  '#......#',
  '#..##..#',
  '#..##..#',
  '#......#',
  '#.L....#',
  '#......#')
$c = New-Card; $b = $c[0]; $g = $c[1]
$rng = New-Object System.Random 7
function Cell($x, $y) { if ($x -lt 0 -or $y -lt 0 -or $x -ge 8 -or $y -ge 8) { return '#' }; return $room[$y][$x] }
for ($y = 0; $y -lt 8; $y++) { for ($x = 0; $x -lt 8; $x++) {
  $ch = Cell $x $y
  if ($ch -eq '#') {
    $m = 0; if ((Cell $x ($y + 1)) -eq '#') { $m += 1 }; if ((Cell ($x + 1) $y) -eq '#') { $m += 2 }; if ((Cell ($x - 1) $y) -eq '#') { $m += 4 }; if ((Cell $x ($y - 1)) -eq '#') { $m += 8 }
    Blit $g $tiles $m ($x * 64) ($y * 64) 64
  } elseif ($ch -eq 'L') { Blit $g $tiles 35 ($x * 64) ($y * 64) 64 }
  else { Blit $g $tiles (32 + $rng.Next(0, 3)) ($x * 64) ($y * 64) 64 }
} }
# darken the bottom-right corner and put the Smiler there
$dark = New-Object System.Drawing.Drawing2D.GraphicsPath; $dark.AddEllipse(250, 250, 400, 400)
$db = New-Object System.Drawing.Drawing2D.PathGradientBrush $dark
$db.CenterColor = [System.Drawing.Color]::FromArgb(215, 8, 6, 2); $db.SurroundColors = @([System.Drawing.Color]::FromArgb(0, 8, 6, 2))
$g.FillPath($db, $dark)
Blit $g $ents 0 384 384 96
Vignette $g 120
Save $b 'level0'

# --- Level 1 / Level 2: map tiles with the place's sign large in the middle ---
function SignCard($sheetFile, $plain, $sign, $name) {
  $sheet = [System.Drawing.Image]::FromFile("$gfx\$sheetFile")
  $c = New-Card; $b = $c[0]; $g = $c[1]
  $r = New-Object System.Random 3
  for ($y = 0; $y -lt 4; $y++) { for ($x = 0; $x -lt 4; $x++) { Blit $g $sheet ($plain[$r.Next(0, $plain.Count)]) ($x * 128) ($y * 128) 128 } }
  Vignette $g 150
  Blit $g $sheet $sign 96 96 320
  Save $b $name
  $sheet.Dispose()
}
SignCard 'br1_overmap.png' @(0, 1, 2) 4 'level1'
SignCard 'br2_overmap.png' @(0, 1) 3 'level2'

# --- Menu picture and the small logo: the launcher's icon on a dark field ---
$icon = [System.Drawing.Image]::FromFile("$root\tools\launcher\Launcher\launcher-icon.png")
foreach ($n in 'cdda_menu', 'fork_logo') {
  $c = New-Card; $b = $c[0]; $g = $c[1]
  $g.Clear([System.Drawing.Color]::FromArgb(255, 20, 19, 12))
  $g.InterpolationMode = 'HighQualityBicubic'
  $pad = if ($n -eq 'fork_logo') { 16 } else { 64 }
  $g.DrawImage($icon, $pad, $pad, 512 - 2 * $pad, 512 - 2 * $pad)
  Save $b $n
}

# --- The normal world: forest, fields and a town edge from UndeadPeople's overmap tiles ---
$udp = "$root\gfx\MSX++UnDeadPeopleEdition"
if (Test-Path "$udp\tile_config.json") {
  Add-Type -AssemblyName System.Web.Extensions
  $ser = New-Object System.Web.Script.Serialization.JavaScriptSerializer; $ser.MaxJsonLength = [int]::MaxValue
  $cfg = $ser.DeserializeObject([IO.File]::ReadAllText("$udp\tile_config.json"))
  $want = @{ 'forest' = $null; 'forest_thick' = $null; 'field' = $null; 'house_01' = $null; 'lake_surface' = $null }
  $start = 0; $where = @{}
  foreach ($s in $cfg['tiles-new']) {
    $w = 32; $h = 32; if ($s.ContainsKey('sprite_width')) { $w = $s['sprite_width']; $h = $s['sprite_height'] }
    $img = [System.Drawing.Image]::FromFile("$udp\" + $s['file']); $cols = [int]($img.Width / $w); $count = $cols * [int]($img.Height / $h)
    foreach ($t in $s['tiles']) { foreach ($id in @($t['id'])) { if ($want.ContainsKey($id) -and $null -eq $want[$id]) {
      $fg = $t['fg']; while ($fg -isnot [int]) { $fg = @($fg)[0]; if ($fg -is [System.Collections.IDictionary]) { $fg = $fg['sprite'] } }
      $want[$id] = @($img, ($fg - $start), $cols, $w) } } }
    $start += $count
  }
  $map = @('ttffhh', 'tfffhh', 'tffflf', 'ttfffl', 'tttffl', 'ttttff')
  $key = @{ 't' = 'forest'; 'd' = 'forest_thick'; 'f' = 'field'; 'h' = 'house_01'; 'l' = 'lake_surface' }
  $c = New-Card; $b = $c[0]; $g = $c[1]
  $cell = 512 / 6
  for ($y = 0; $y -lt 6; $y++) { for ($x = 0; $x -lt 6; $x++) {
    $e = $want[$key[[string]$map[$y][$x]]]; if ($null -eq $e) { continue }
    $src = New-Object System.Drawing.Rectangle (($e[1] % $e[2]) * $e[3]), ([Math]::Floor($e[1] / $e[2]) * $e[3]), $e[3], $e[3]
    $g.DrawImage($e[0], (New-Object System.Drawing.RectangleF ($x * $cell), ($y * $cell), ($cell + 1), ($cell + 1)), $src, [System.Drawing.GraphicsUnit]::Pixel)
  } }
  Vignette $g 140
  Save $b 'cdda_world'
} else { "UndeadPeople tileset not found: cdda_world skipped" }
