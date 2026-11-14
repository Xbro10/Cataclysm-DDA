# Shared helpers for the Backrooms Expanded sprite scripts. Dot-source this AFTER setting $bmp (the sheet)
# and $T (tile size, 32). Every drawing call takes a tile index and tile-local coordinates.
# Style (UndeadPeople-like): 1 px near-black outlines, 2-3 shades per colour, grainy ground,
# landmarks drawn as bevelled signboards with a pictogram and a coloured band.
Add-Type -AssemblyName System.Drawing

function Hex([string]$hex) { [System.Drawing.ColorTranslator]::FromHtml($hex) }
# $INK, not $OUT: PowerShell names are case-insensitive and $Out is the scripts' output path.
$INK = Hex '#161412'

function Px($tile, $x, $y, $col) { if ($x -ge 0 -and $x -lt $T -and $y -ge 0 -and $y -lt $T) { $bmp.SetPixel($tile * $T + $x, $y, $col) } }
function Fill($tile, $x, $y, $w, $h, $col) { for ($i = $x; $i -lt $x + $w; $i++) { for ($j = $y; $j -lt $y + $h; $j++) { Px $tile $i $j $col } } }
function Box($tile, $x, $y, $w, $h, $col) { Fill $tile $x $y $w 1 $col; Fill $tile $x ($y + $h - 1) $w 1 $col; Fill $tile $x $y 1 $h $col; Fill $tile ($x + $w - 1) $y 1 $h $col }
function Ell($tile, $cx, $cy, $rx, $ry, $col) {
  for ($j = -$ry; $j -le $ry; $j++) { for ($i = -$rx; $i -le $rx; $i++) {
    if ((($i * $i) / [double]($rx * $rx + 0.5)) + (($j * $j) / [double]($ry * $ry + 0.5)) -le 1.0) { Px $tile ($cx + $i) ($cy + $j) $col } } }
}
function Line($tile, $x0, $y0, $x1, $y1, $col, $w = 1) {
  $dx = [Math]::Abs($x1 - $x0); $dy = -[Math]::Abs($y1 - $y0)
  $sx = if ($x0 -lt $x1) { 1 } else { -1 }; $sy = if ($y0 -lt $y1) { 1 } else { -1 }
  $err = $dx + $dy; $x = $x0; $y = $y0
  while ($true) {
    Fill $tile $x $y $w $w $col
    if ($x -eq $x1 -and $y -eq $y1) { break }
    $e2 = 2 * $err
    if ($e2 -ge $dy) { $err += $dy; $x += $sx }
    if ($e2 -le $dx) { $err += $dx; $y += $sy }
  }
}
# Outlined shapes: the outline is the same shape one pixel bigger, drawn first.
function ORect($tile, $x, $y, $w, $h, $col) { Fill $tile ($x - 1) ($y - 1) ($w + 2) ($h + 2) $INK; Fill $tile $x $y $w $h $col }
function OEll($tile, $cx, $cy, $rx, $ry, $col) { Ell $tile $cx $cy ($rx + 1) ($ry + 1) $INK; Ell $tile $cx $cy $rx $ry $col }
# Grain: scatter darker and lighter specks over a region.
function Grain($tile, $x, $y, $w, $h, $dark, $light, $n, $rng) {
  for ($k = 0; $k -lt $n; $k++) { Px $tile ($x + $rng.Next(0, $w)) ($y + $rng.Next(0, $h)) $dark }
  for ($k = 0; $k -lt [int]($n / 2); $k++) { Px $tile ($x + $rng.Next(0, $w)) ($y + $rng.Next(0, $h)) $light }
}
# A signboard on posts: bevelled grey face with an icon panel (x 6-25, y 4-14) and a coloured band with windows.
# Draw the ground first, then Sign, then the icon.
function Sign($tile, $band, $bandDark) {
  $post = Hex '#7e7a74'; $face = Hex '#c6c2b8'; $hi = Hex '#e2dfd6'; $lo = Hex '#96928a'; $panel = Hex '#b2aea4'; $win = Hex '#e8e4da'
  ORect $tile 9 22 2 6 $post; ORect $tile 21 22 2 6 $post
  ORect $tile 3 2 26 21 $face
  Fill $tile 3 2 26 1 $hi; Fill $tile 3 2 1 21 $hi; Fill $tile 3 22 26 1 $lo; Fill $tile 28 2 1 21 $lo
  Fill $tile 5 4 22 11 $panel; Fill $tile 5 4 22 1 $lo
  Fill $tile 5 16 22 5 $band; Fill $tile 5 20 22 1 $bandDark; Fill $tile 5 16 22 1 $INK
  foreach ($wx in 7, 12, 17, 22) { Fill $tile $wx 17 3 2 $win; Px $tile ($wx + 2) 18 $bandDark }
}
# Save a sheet and, optionally, an 8x preview with gaps between tiles.
function SaveSheet($out, $preview, $count, $bg = '#2a2826') {
  $bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
  if (-not $preview) { return }
  $sc = 8
  $pv = New-Object System.Drawing.Bitmap ($bmp.Width * $sc + ($count - 1) * $sc), ($T * $sc)
  $g = [System.Drawing.Graphics]::FromImage($pv)
  $g.Clear((Hex $bg))
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
  $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
  # Not $n or $t: PowerShell names are case-insensitive.
  for ($ti = 0; $ti -lt $count; $ti++) {
    $src = New-Object System.Drawing.Rectangle ($ti * $T), 0, $T, $T
    $dst = New-Object System.Drawing.Rectangle ($ti * ($T * $sc + $sc)), 0, ($T * $sc), ($T * $sc)
    $g.DrawImage($bmp, $dst, $src, [System.Drawing.GraphicsUnit]::Pixel)
  }
  $pv.Save($preview, [System.Drawing.Imaging.ImageFormat]::Png)
}

# Outline a whole figure: every transparent pixel touching (4-way) an opaque pixel becomes $INK.
# Use it for furniture, items and monsters drawn on a transparent tile.
function Outline($tile) {
  $hits = New-Object System.Collections.Generic.List[object]
  for ($y = 0; $y -lt $T; $y++) { for ($x = 0; $x -lt $T; $x++) {
    if ($bmp.GetPixel($tile * $T + $x, $y).A -ne 0) { continue }
    foreach ($d in @(@(1, 0), @(-1, 0), @(0, 1), @(0, -1))) {
      $nx = $x + $d[0]; $ny = $y + $d[1]
      if ($nx -ge 0 -and $nx -lt $T -and $ny -ge 0 -and $ny -lt $T -and $bmp.GetPixel($tile * $T + $nx, $ny).A -eq 255) { $hits.Add(@($x, $y)); break }
    }
  } }
  foreach ($p in $hits) { Px $tile $p[0] $p[1] $INK }
}
# A soft ground shadow under a figure, drawn after Outline so it isn't outlined.
function Shadow($tile, $cx, $cy, $rx) {
  $sh = [System.Drawing.Color]::FromArgb(70, 0, 0, 0)
  for ($i = -$rx; $i -le $rx; $i++) { if ($bmp.GetPixel($tile * $T + $cx + $i, $cy).A -eq 0) { Px $tile ($cx + $i) $cy $sh } }
}
