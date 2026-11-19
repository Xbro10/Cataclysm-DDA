# The shock lance's shape (Additions, xa_shock_lance_*), shared by draw_xa_lance.ps1 (ground sprites, schematic) and
# draw_xa_lance_held.ps1 / draw_xa_lance_held_udp.ps1 (in-hand overlays). Dot-source AFTER spear_shape.ps1: it reuses
# its Layout, Band, Poly, AxPx, AxisXY, SPix, SGet and OutlineT, so the lance sits exactly where the Flamero-Mado does,
# a sibling with its own parts:
#   u   0-4   steel end cap            u  4-48  pale enamelled shaft with copper coil bands
#   u  18-30  battery pack with a green light   u  48-57  three white ceramic insulators
#   u  57-100 slim lance point with two short copper electrode prongs beside it

$L = @{
  enamel = (Hex '#8aa2b8'); enamelL = (Hex '#c4d4e2'); enamelD = (Hex '#5a7086')
  copper = (Hex '#c0682e'); copperL = (Hex '#f2a066'); copperD = (Hex '#7e3e18')
  pack = (Hex '#2c3036'); packL = (Hex '#4c535c'); led = (Hex '#5cff7a'); ledOff = (Hex '#2a6a36')
  cer = (Hex '#e8e4da'); cerL = (Hex '#ffffff'); cerD = (Hex '#a8a49a')
  steel = (Hex '#5c6470'); steelL = (Hex '#b4bcc8'); steelD = (Hex '#2e333a'); tip = (Hex '#ffffff')
  arcCore = (Hex '#ffffff'); arcGlow = (Hex '#8ee8ff'); arcDeep = (Hex '#3a8cff')
}

function DrawLance($tile, $S, $on, $coilEvery = 6.0) {
  # End cap.
  Band $tile $S 0 3.5 1.6 $L.steel; AxPx $tile $S 1.5 (-0.7) $L.steelL
  # Enamelled shaft, light along the upper side, with copper coil bands.
  Band $tile $S 3.5 48 1.15 $L.enamel
  Poly $tile $S @(@(3.5, -1.15), @(48, -1.15), @(48, -0.45), @(3.5, -0.45)) $L.enamelL
  Poly $tile $S @(@(3.5, 0.6), @(48, 0.6), @(48, 1.15), @(3.5, 1.15)) $L.enamelD
  for ($u = 6.0; $u -lt 47; $u += $coilEvery) {
    if ($u -gt 17 -and $u -lt 31) { continue }
    Band $tile $S $u ($u + 1.6) 1.45 $L.copper
    AxPx $tile $S ($u + 0.8) (-0.9) $L.copperL
  }
  # Battery pack: a dark box with a green light (lit when on).
  Band $tile $S 18 30 1.9 $L.pack; Band $tile $S 19 29 0.6 $L.packL
  AxPx $tile $S 26 (-1.4) ($(if ($on) { $L.led } else { $L.ledOff }))
  # Two ceramic insulator discs on a dark steel neck, so each disc reads on its own even on a 32-pixel tile.
  Band $tile $S 48 57 1.2 $L.steelD
  foreach ($c in 48.5, 53.5) {
    Band $tile $S $c ($c + 3.0) 2.6 $L.cer
    Poly $tile $S @(@($c, -2.6), @(($c + 3.0), -2.6), @(($c + 3.0), -1.6), @($c, -1.6)) $L.cerL
    Poly $tile $S @(@($c, 1.7), @(($c + 3.0), 1.7), @(($c + 3.0), 2.6), @($c, 2.6)) $L.cerD
  }
  # Electrode prongs: thin copper rods beside the point, from the last insulator to tips at u 74.
  foreach ($side in -1, 1) {
    Poly $tile $S @(@(57, ($side * 2.0)), @(74, ($side * 2.9)), @(74, ($side * 3.6)), @(57, ($side * 2.7))) $L.copper
    AxPx $tile $S 74.5 ($side * 3.3) ($(if ($on) { $L.arcGlow } else { $L.copperL }))
  }
  # Slim lance point: steel, bright upper bevel, dark lower face, white tip.
  Poly $tile $S @(@(56.5, -1.3), @(72, -1.9), @(100, 0), @(72, 1.9), @(56.5, 1.3)) $L.steel
  Poly $tile $S @(@(58, -1.2), @(72, -1.8), @(97, -0.2), @(72, -1.0)) $L.steelL
  Poly $tile $S @(@(58, 0.6), @(72, 1.0), @(97, 0.2), @(72, 1.8)) $L.steelD
  AxPx $tile $S 98 0 $L.tip
  if ($on) {
    # Current glows along the point's ridge.
    Poly $tile $S @(@(60, -0.3), @(95, -0.1), @(95, 0.1), @(60, 0.3)) $L.arcGlow
  }
}

# A lightning arc between two pixel points: a jagged line that wanders up to $jit pixels off the straight path,
# drawn as a cyan glow with a white core. $rng decides the wander, so each animation frame gets a new bolt.
# Behind = only on empty pixels or other arcs; front = over the lance too.
$script:arcPx = @{}
function Arc($tile, $x0, $y0, $x1, $y1, $rng, $jit = 1.6, $front = $false, $glow = $true) {
  $len = [Math]::Sqrt(($x1 - $x0) * ($x1 - $x0) + ($y1 - $y0) * ($y1 - $y0))
  $steps = [Math]::Max(3, [int]($len / 2.5))
  $nx = -($y1 - $y0) / [Math]::Max(0.001, $len); $ny = ($x1 - $x0) / [Math]::Max(0.001, $len)
  $pts = New-Object System.Collections.Generic.List[object]
  for ($i = 0; $i -le $steps; $i++) {
    $t = $i / $steps
    $off = if ($i -eq 0 -or $i -eq $steps) { 0 } else { ($rng.NextDouble() * 2 - 1) * $jit }
    $pts.Add(@(($x0 + ($x1 - $x0) * $t + $nx * $off), ($y0 + ($y1 - $y0) * $t + $ny * $off)))
  }
  # A 1-pixel cyan line; with $glow, white-hot flecks along about half of it. Thin reads as lightning on a small tile.
  for ($i = 0; $i -lt $pts.Count - 1; $i++) {
    $a = $pts[$i]; $b = $pts[$i + 1]
    $n = [Math]::Max(1, [int]([Math]::Ceiling([Math]::Max([Math]::Abs($b[0] - $a[0]), [Math]::Abs($b[1] - $a[1])))))
    $hot = $glow -and ($rng.Next(2) -eq 0)
    for ($s = 0; $s -le $n; $s++) {
      $px = [int][Math]::Round($a[0] + ($b[0] - $a[0]) * $s / $n); $py = [int][Math]::Round($a[1] + ($b[1] - $a[1]) * $s / $n)
      if ($px -lt 0 -or $px -ge $TW -or $py -lt 0 -or $py -ge $TH) { continue }
      $isArc = $script:arcPx.ContainsKey("$tile,$px,$py")
      if ($front -or (SGet $tile $px $py).A -eq 0 -or $isArc) {
        SPix $tile $px $py ($(if ($hot) { $L.arcCore } else { $L.arcGlow })); $script:arcPx["$tile,$px,$py"] = $true
      }
    }
  }
}
# A pixel point on the lance, from its axis position.
function LancePt($S, $u, $v) { $xy = AxisXY $S $u $v; return @([Math]::Round($xy[0]), [Math]::Round($xy[1])) }
# The switched-on lance's arcs for one animation frame: the prong tips arc to each other and to the point, a short
# arc crawls over the insulators, and a few sparks fly. $seed picks the frame's bolts.
function LanceArcs($tile, $S, $seed, $scale = 1.0) {
  $rng = New-Object System.Random $seed
  $pa = LancePt $S 74.5 (-3.3); $pb = LancePt $S 74.5 3.3; $tipP = LancePt $S 99 0
  # Each prong arcs forward to the point, outside the blade; one side or both, a new bolt every frame.
  $both = $rng.Next(3) -ne 0
  $first = $rng.Next(2)
  foreach ($k in 0, 1) {
    if (-not $both -and $k -ne $first) { continue }
    $p = if ($k -eq 0) { $pa } else { $pb }
    Arc $tile $p[0] $p[1] $tipP[0] $tipP[1] $rng (1.6 * $scale) $true
  }
  # A short spark jumping off a prong, away from the lance.
  $p = if ($rng.Next(2) -eq 0) { $pa } else { $pb }
  $away = LancePt $S (70 + $rng.Next(12)) ((($(if ($p -eq $pa) { -1 } else { 1 }))) * (6 + $rng.Next(3)))
  Arc $tile $p[0] $p[1] $away[0] $away[1] $rng (1.0 * $scale) $false
  $ia = LancePt $S 49 ((-2.6) * ($(if ($rng.Next(2) -eq 0) { 1 } else { -1 }))); $ib = LancePt $S 56.5 ((2.6) * ($(if ($rng.Next(2) -eq 0) { 1 } else { -1 })))
  if ($rng.Next(2) -eq 0) { Arc $tile $ia[0] $ia[1] $ib[0] $ib[1] $rng (1.0 * $scale) $false $false }
  # Sparks: single bright pixels near the head.
  for ($k = 0; $k -lt 4; $k++) {
    $u = 70 + $rng.Next(32); $v = ($rng.NextDouble() * 2 - 1) * 6
    $p = LancePt $S $u $v
    if ($p[0] -ge 0 -and $p[0] -lt $TW -and $p[1] -ge 0 -and $p[1] -lt $TH -and (SGet $tile $p[0] $p[1]).A -eq 0) { SPix $tile $p[0] $p[1] ($(if ($k % 2) { $L.arcCore } else { $L.arcGlow })) }
  }
}
