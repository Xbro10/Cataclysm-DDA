# The Flamero-Mado spear's shape, shared by draw_br_weapons.ps1 (ground sprites, schematic) and draw_br_held.ps1
# (in-hand overlays), so every sprite shows the same spear. Dot-source AFTER sprite_common.ps1, with $bmp, $TW and
# $TH (tile width and height) set.
#
# The spear is described along its own axis: u runs 0 (butt) to 100 (tip), v is pixels across it. A layout
# ($S) says where the butt is, which way the spear points and how long it is, so the same parts draw corner to
# corner on a 32x32 tile or upright on a 32x64 one.
#   u   0-5   pommel knob, gold ring   u  5-48  segmented lacquer grip, the fuel tank segment at u 20-32
#   u  48-57  black bulb collar with gold bands (the jets)
#   u  57-100 broad notched blade with white bevels and a crimson sheen

function SPix($tile, $x, $y, $col) { if ($x -ge 0 -and $x -lt $TW -and $y -ge 0 -and $y -lt $TH) { $bmp.SetPixel($tile * $TW + $x, $y, $col) } }
function SGet($tile, $x, $y) { return $bmp.GetPixel($tile * $TW + $x, $y) }

# A layout: butt at (bx, by), pointing along (dx, dy) (unit vector), length L pixels from butt to tip.
function Layout([double]$bx, [double]$by, [double]$dx, [double]$dy, [double]$L, [switch]$Flip, [double]$Split = 57, [double]$Width = 1.0) {
  $len = [Math]::Sqrt($dx * $dx + $dy * $dy); $dx /= $len; $dy /= $len
  # The normal points to the spear's right (down-right when the spear points up-right).
  # v < 0 is the lit side. Flip swaps sides so the light can come from the upper left either way.
  $f = if ($Flip) { -1.0 } else { 1.0 }
  # Width: scales every part's thickness (1 = as drawn; the held overlays are thinner to match UltiCa's pikes).
  # Split: where the blade starts along the real spear (the parts list above uses 57). A larger Split gives
  # the shaft more of the length and makes the blade shorter.
  return @{ bx = $bx; by = $by; dx = $dx; dy = $dy; nx = - $dy * $f; ny = $dx * $f; k = $L / 100.0; split = $Split; ws = $Width }
}
function AxisXY($S, $u, $v) {
  $u = if ($u -le 57) { $u * $S.split / 57.0 } else { $S.split + ($u - 57) * (100 - $S.split) / 43.0 }
  return @(($S.bx + $u * $S.k * $S.dx + $v * $S.ws * $S.nx), ($S.by + $u * $S.k * $S.dy + $v * $S.ws * $S.ny)) }

# Fill a polygon given as (u, v) pairs, hard-edged (no anti-aliasing), clipped to the tile.
function Poly($tile, $S, $pts, $col) {
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
  $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
  $g.SetClip((New-Object System.Drawing.Rectangle ($tile * $TW), 0, $TW, $TH))
  $arr = New-Object 'System.Drawing.PointF[]' $pts.Count
  for ($i = 0; $i -lt $pts.Count; $i++) {
    $xy = AxisXY $S $pts[$i][0] $pts[$i][1]
    $arr[$i] = New-Object System.Drawing.PointF ([single]($xy[0] + $tile * $TW)), ([single]$xy[1])
  }
  $brush = New-Object System.Drawing.SolidBrush $col
  $g.FillPolygon($brush, $arr)
  $brush.Dispose(); $g.Dispose()
}
# A band across the spear from u0 to u1, half-width w.
function Band($tile, $S, $u0, $u1, $w, $col) { Poly $tile $S @(@($u0, (-$w)), @($u1, (-$w)), @($u1, $w), @($u0, $w)) $col }
# Plot one pixel at an axis position.
function AxPx($tile, $S, $u, $v, $col) { $xy = AxisXY $S $u $v; SPix $tile ([int][Math]::Round($xy[0])) ([int][Math]::Round($xy[1])) $col }

$C = @{
  # Glossy black lacquer with a cold blue sheen.
  grip = (Hex '#16171e'); gripL = (Hex '#2e3346'); gripH = (Hex '#8e9cc8')
  steel = (Hex '#1e1f26'); steelL = (Hex '#4a5068'); steelD = (Hex '#0e0e12')
  brass = (Hex '#c8962e'); brassL = (Hex '#f4cc6a'); brassD = (Hex '#7e5a18')
  # Blade: near-black steel, a deep crimson sheen on one face, white bevels.
  blade = (Hex '#141418'); face = (Hex '#3a3848'); faceL = (Hex '#5a5870'); low = (Hex '#2c1218'); sheen = (Hex '#7a2030'); sheenL = (Hex '#c43a3a'); edgeL = (Hex '#9aa0ae')
  ridge = (Hex '#6a6e7c'); edge = (Hex '#d6dae4'); tip = (Hex '#ffffff')
  glowD = (Hex '#7a1e16'); glow = (Hex '#d0482a'); glowL = (Hex '#ff9a4a')
}

# Draws the spear. $lit tints the blade's lower bevel and tip with the fire's light.
function DrawSpear($tile, $S, $lit, $ridgeEvery = 4.2) {
  # Pommel: a small black knob with a gold ring under it.
  Band $tile $S 0 3.5 1.7 $C.steel; AxPx $tile $S 1.5 (-0.8) $C.gripH
  Band $tile $S 3.5 5 1.5 $C.brass
  # Grip: glossy black lacquer in rounded segments; each segment has a blue sheen fleck on the lit side.
  Band $tile $S 5 48 1.2 $C.grip
  for ($u = 6.5; $u -lt 47; $u += $ridgeEvery) {
    Band $tile $S $u ($u + 1.0) 1.35 $C.steelD
    Band $tile $S ($u + 1.2) ($u + 3.2) 0.45 $C.gripL
    AxPx $tile $S ($u + 2.2) (-0.7) $C.gripH
  }
  # The fuel tank hides in the shaft: a slightly fatter segment between two gold rings, a gold valve stud.
  Band $tile $S 20 32 1.6 $C.grip; Band $tile $S 21 31 0.5 $C.gripL; AxPx $tile $S 24 (-0.9) $C.gripH
  Band $tile $S 19.5 21 1.8 $C.brass; Band $tile $S 31 32.5 1.8 $C.brass
  Poly $tile $S @(@(25, -1.6), @(27.5, -1.6), @(27, -2.6), @(25.5, -2.6)) $C.brass
  # Collar: a black bulb that swells toward the blade, gold bands at both ends (the jets sit in its rim).
  Poly $tile $S @(@(48, -1.4), @(52, -2.6), @(55.5, -2.8), @(57.5, -1.6), @(57.5, 1.6), @(55.5, 2.8), @(52, 2.6), @(48, 1.4)) $C.steel
  Poly $tile $S @(@(50, -1.6), @(54, -2.4), @(55, -1.4), @(50.5, -0.9)) $C.gripL
  AxPx $tile $S 53 (-1.9) $C.gripH
  Band $tile $S 47.5 49 1.6 $C.brass; Band $tile $S 56 57.5 2.4 $C.brass; AxPx $tile $S 56.6 (-2.0) $C.brassL
  # Blade: a broad shield-shaped head, notched at the shoulders, tapering to a long point.
  $blade = @(@(57.5, -1.6), @(59, -4.4), @(62, -4.6), @(63, -3.7), @(66, -4.6), @(100, 0),
             @(66, 4.6), @(63, 3.7), @(62, 4.6), @(59, 4.4), @(57.5, 1.6))
  Poly $tile $S $blade $C.blade
  # Upper face: slate catching the light, with a lighter streak; lower face black-red with a crimson sheen.
  Poly $tile $S @(@(58.5, -1.4), @(60, -3.9), @(66.5, -3.9), @(97, -0.2), @(60, -0.2)) $C.face
  Poly $tile $S @(@(64, -1.8), @(68, -3.0), @(86, -1.2), @(82, -0.8)) $C.faceL
  Poly $tile $S @(@(58.5, 1.4), @(60, 3.9), @(66.5, 3.9), @(97, 0.2), @(60, 0.2)) $C.low
  Poly $tile $S @(@(64, 1.4), @(70, 3.0), @(84, 1.6), @(78, 0.8)) $C.sheen
  Poly $tile $S @(@(68, 1.8), @(76, 2.0), @(76, 1.3), @(68, 1.2)) $C.sheenL
  # Bevels: a bright white upper edge and shoulder, a grey lower edge; a centre ridge; a white tip.
  Poly $tile $S @(@(66, -4.7), @(100, 0), @(95, -0.3), @(66.5, -3.5)) $C.edge
  Poly $tile $S @(@(58.6, -4.6), @(62.2, -4.8), @(62, -3.6), @(59, -3.4)) $C.edge
  Poly $tile $S @(@(59, -0.35), @(95, -0.1), @(95, 0.1), @(59, 0.35)) $C.ridge
  AxPx $tile $S 98 0 $C.tip
  if ($lit) {
    # Firelight: the lower bevel glows orange-red, the shoulders and the crimson sheen brighten.
    Poly $tile $S @(@(66, 4.7), @(100, 0), @(95, 0.3), @(66.5, 3.5)) $C.glow
    Poly $tile $S @(@(66.5, 4.0), @(82, 2.0), @(81, 1.6), @(66.5, 3.4)) $C.glowL
    Poly $tile $S @(@(59, 4.4), @(62, 4.6), @(61.5, 4.0), @(59.5, 3.8)) $C.glowL
    Poly $tile $S @(@(68, 1.8), @(76, 2.0), @(76, 1.3), @(68, 1.2)) $C.glowL
  } else {
    Poly $tile $S @(@(66, 4.7), @(100, 0), @(95, 0.3), @(66.5, 3.5)) $C.edgeL
    Poly $tile $S @(@(58.6, 4.6), @(62.2, 4.8), @(62, 3.6), @(59, 3.4)) $C.edgeL
  }
}

# Outline a tile: every transparent pixel next to an opaque one becomes $col.
function OutlineT($tile, $col) {
  $hits = New-Object System.Collections.Generic.List[object]
  for ($y = 0; $y -lt $TH; $y++) { for ($x = 0; $x -lt $TW; $x++) {
    if ((SGet $tile $x $y).A -ne 0) { continue }
    foreach ($d in @(@(1, 0), @(-1, 0), @(0, 1), @(0, -1))) {
      $nx = $x + $d[0]; $ny = $y + $d[1]
      if ($nx -ge 0 -and $nx -lt $TW -and $ny -ge 0 -and $ny -lt $TH -and (SGet $tile $nx $ny).A -eq 255) { $hits.Add(@($x, $y)); break }
    }
  } }
  foreach ($p in $hits) { SPix $tile $p[0] $p[1] $col }
}

# Flame ribbons. Each ribbon is a list of pixel points (a path); it is painted as overlapping discs that shrink
# toward its end: dark red rim, orange body, yellow core. Behind = only on empty pixels; front = over the spear too.
$F = @{ rim = (Hex '#8c1e10'); red = (Hex '#e0482a'); orange = (Hex '#ff9a32'); yellow = (Hex '#ffe07a'); white = (Hex '#fff6d6') }
# Flame layers, outside in: (share of the radius, colour). Set $script:flameLayers = $FlameBright for a hotter
# flame with a white core (the UndeadPeople held sprite, where the flames are small on screen).
$FlameNormal = @(@(1.0, $F.rim), @(0.72, $F.red), @(0.48, $F.orange), @(0.24, $F.yellow))
$FlameBright = @(@(1.0, $F.red), @(0.8, $F.orange), @(0.56, $F.yellow), @(0.28, $F.white))
$script:flameLayers = $FlameNormal
# Animation: set $script:FlameFrame (0 to $script:FlameFrames - 1) before drawing a frame, and call FlameFrameStart
# at the start of each tile so every ribbon gets its own phase. Over one loop each ribbon sways sideways in a smooth
# wave (more toward its tip), lifts and settles at twice that rate, and pulses a little in size; embers rise a pixel
# a frame. Offsets are not rounded: the ribbons are painted from overlapping discs, so sub-pixel shifts change their
# edges a little each frame, which reads smoother than whole-pixel jumps.
$script:FlameFrame = 0
$script:FlameFrames = 8
$script:ribbonNo = 0
function FlameFrameStart { $script:ribbonNo = 0 }
function FlamePhase { return 2 * [Math]::PI * $script:FlameFrame / $script:FlameFrames }
function SwayPath($path) {
  $ph = FlamePhase; $k = $script:ribbonNo
  $n = $path.Count; $out = New-Object System.Collections.Generic.List[object]
  for ($j = 0; $j -lt $n; $j++) {
    $t = if ($n -gt 1) { $j / ($n - 1) } else { 0 }
    $dx = 1.3 * $t * [Math]::Sin($ph + $k * 1.7)
    $dy = - 1.1 * $t * (0.5 + 0.5 * [Math]::Sin(2 * $ph + $k * 2.1))
    $out.Add(@(($path[$j][0] + $dx), ($path[$j][1] + $dy)))
  }
  return $out
}
function Ribbon($tile, $path, $r0, $front) {
  $path = SwayPath $path
  $r0 = $r0 * (1.0 + 0.1 * [Math]::Sin((FlamePhase) + $script:ribbonNo * 2.3))
  $script:ribbonNo++
  $layers = $script:flameLayers
  foreach ($layer in $layers) {
    $scale = $layer[0]; $col = $layer[1]
    $n = $path.Count
    for ($i = 0; $i -lt $n - 1; $i++) {
      $a = $path[$i]; $b = $path[$i + 1]
      for ($s = 0.0; $s -lt 1.0; $s += 0.25) {
        $t = ($i + $s) / ($n - 1)
        $r = $r0 * (1.0 - 0.8 * $t) * $scale
        $cx = $a[0] + ($b[0] - $a[0]) * $s; $cy = $a[1] + ($b[1] - $a[1]) * $s
        $ri = [int][Math]::Ceiling($r)
        for ($dy = -$ri; $dy -le $ri; $dy++) { for ($dx = -$ri; $dx -le $ri; $dx++) {
          if ($dx * $dx + $dy * $dy -gt $r * $r + 0.25) { continue }
          $px = [int][Math]::Round($cx + $dx); $py = [int][Math]::Round($cy + $dy)
          if ($px -lt 0 -or $px -ge $TW -or $py -lt 0 -or $py -ge $TH) { continue }
          $cur = SGet $tile $px $py
          $isFlame = $script:flamePx.ContainsKey("$tile,$px,$py")
          if ($front -or $cur.A -eq 0 -or $isFlame) { SPix $tile $px $py $col; $script:flamePx["$tile,$px,$py"] = $true }
        } }
      }
    }
  }
}
$script:flamePx = @{}
# Embers: single bright pixels drifting off the flames.
function Embers($tile, $pts) {
  $i = 0
  foreach ($p in $pts) {
    # Each frame an ember rises a pixel and drifts gently sideways; after a loop it is back where it started.
    $x = $p[0] + [int][Math]::Round(0.9 * [Math]::Sin((FlamePhase) + $i)); $y = $p[1] - $script:FlameFrame
    $i++
    if ($x -lt 0 -or $x -ge $TW -or $y -lt 0 -or $y -ge $TH) { continue }
    if ((SGet $tile $x $y).A -eq 0) { SPix $tile $x $y ($(if (($script:FlameFrame + $i) % 3 -eq 0) { $F.yellow } else { $F.orange })) }
  }
}
