param([string]$Out, [string]$Preview)
# Draws the Backrooms Expanded boss sprites, one PNG per boss, facing right (the game mirrors them).
# Original art in UndeadPeople style: near-black outline, 3-4 shades per colour, light from the top left.
# Redeemer: 128x96 (four map tiles wide, three tall). A hunched, shaggy three-headed beast with a raised
# bushy tail, a mane of tangled cable round its necks, and three leaning wooden power poles growing out of
# its back with sagging lines between them and current crawling over everything.
Add-Type -AssemblyName System.Drawing
$W = 128; $H = 96
$bmp = New-Object System.Drawing.Bitmap $W, $H, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$rng = New-Object System.Random 128

function Hex([string]$hex) { [System.Drawing.ColorTranslator]::FromHtml($hex) }
$INK = Hex '#0b0c14'
function Px($x, $y, $c) { $x = [int]$x; $y = [int]$y; if ($x -ge 0 -and $x -lt $W -and $y -ge 0 -and $y -lt $H) { $bmp.SetPixel($x, $y, $c) } }
function Fill($x, $y, $fw, $fh, $c) { for ($i = $x; $i -lt $x + $fw; $i++) { for ($j = $y; $j -lt $y + $fh; $j++) { Px $i $j $c } } }
function Line($x0, $y0, $x1, $y1, $c, $lw = 1) {
  $x0 = [int]$x0; $y0 = [int]$y0; $x1 = [int]$x1; $y1 = [int]$y1
  $dx = [Math]::Abs($x1 - $x0); $dy = -[Math]::Abs($y1 - $y0)
  $sx = if ($x0 -lt $x1) { 1 } else { -1 }; $sy = if ($y0 -lt $y1) { 1 } else { -1 }
  $err = $dx + $dy; $x = $x0; $y = $y0
  while ($true) {
    Fill ($x - [int](($lw - 1) / 2)) ($y - [int](($lw - 1) / 2)) $lw $lw $c
    if ($x -eq $x1 -and $y -eq $y1) { break }
    $e2 = 2 * $err
    if ($e2 -ge $dy) { $err += $dy; $x += $sx }
    if ($e2 -le $dx) { $err += $dx; $y += $sy }
  }
}
function Opaque($x, $y) { $x -ge 0 -and $x -lt $W -and $y -ge 0 -and $y -lt $H -and $bmp.GetPixel($x, $y).A -eq 255 }

# Fur palette (blue-black, lit from the top left) and the rest.
$f0 = Hex '#0f1428'; $f1 = Hex '#18203e'; $f2 = Hex '#243159'; $f3 = Hex '#34467a'; $f4 = Hex '#4c64a0'
$h0 = Hex '#2c3140'; $h1 = Hex '#454b5e'; $h2 = Hex '#646b80'; $h3 = Hex '#8d94a8'; $h4 = Hex '#b6bccb'
$wd0 = Hex '#3a2616'; $wd1 = Hex '#5a3c22'; $wd2 = Hex '#7a5432'; $wd3 = Hex '#9a7046'
$ins = Hex '#e2e4de'; $insD = Hex '#9aa09a'; $cab = Hex '#15151c'; $cabL = Hex '#3c3f4c'
$arc = Hex '#7fe8ff'; $arcW = Hex '#f2ffff'; $eye = Hex '#d8ffff'; $claw = Hex '#ddd8c8'; $tooth = Hex '#efece0'; $gum = Hex '#4a1a24'

# A shaggy fur mass: an ellipse shaded top-light to bottom-dark with streaky noise, and tufts sticking out
# of its edge (more on top and at the back, fewer underneath).
function FurMass($cx, $cy, $rx, $ry, $tufts = 1.0, $dark = 0) {
  for ($j = -$ry; $j -le $ry; $j++) { for ($i = -$rx; $i -le $rx; $i++) {
    $d = ($i * $i) / [double]($rx * $rx) + ($j * $j) / [double]($ry * $ry)
    if ($d -gt 1.0) { continue }
    $t = (($cy + $j) - 34) / 52.0 - 0.18 * (($cx + $i) - 64) / 64.0 + 0.22 * $d * $d + ($rng.NextDouble() - 0.5) * 0.1 + 0.15 * $dark
    $c = if ($t -lt 0.22) { $f4 } elseif ($t -lt 0.42) { $f3 } elseif ($t -lt 0.66) { $f2 } elseif ($t -lt 0.86) { $f1 } else { $f0 }
    if ($dark -gt 0 -and $c -eq $f4) { $c = $f3 }
    Px ($cx + $i) ($cy + $j) $c
  } }
  $n = [int](($rx + $ry) * 1.6 * $tufts)
  for ($k = 0; $k -lt $n; $k++) {
    $a = $rng.NextDouble() * 2 * [Math]::PI
    $ex = $cx + [Math]::Cos($a) * $rx; $ey = $cy + [Math]::Sin($a) * $ry
    $len = 2 + $rng.Next(0, 4); if ([Math]::Sin($a) -gt 0.5) { $len = 1 + $rng.Next(0, 2) }
    $ox = [Math]::Cos($a) * $len + ($rng.NextDouble() - 0.5) * 2; $oy = [Math]::Sin($a) * $len - 0.6
    $c = if ([Math]::Sin($a) -lt -0.3 -and [Math]::Cos($a) -lt 0.3) { $f3 } elseif ([Math]::Sin($a) -lt 0.2) { $f2 } else { $f1 }
    Line $ex $ey ($ex + $ox) ($ey + $oy) $c
  }
  # Fur streaks inside the mass: short dark and light strands following the coat.
  for ($k = 0; $k -lt [int]($rx * $ry / 16); $k++) {
    $sx = $cx + $rng.Next(-$rx + 2, $rx - 1); $sy = $cy + $rng.Next(-$ry + 2, $ry - 1)
    if (-not (Opaque $sx $sy)) { continue }
    $c = if ($sy -lt $cy - $ry / 3) { $f4 } else { $f0 }
    Line $sx $sy ($sx - 2) ($sy + 2) $c
  }
}

# A leg: a column of fur tapering to a paw with pale claws.
function Leg($x0, $y0, $x1, $y1, $legw, $far) {
  $steps = 10
  for ($s = 0; $s -le $steps; $s++) {
    $x = $x0 + ($x1 - $x0) * $s / $steps; $y = $y0 + ($y1 - $y0) * $s / $steps
    $r = [int]([Math]::Max(2, $legw * (1 - 0.45 * $s / $steps)))
    FurMass ([int]$x) ([int]$y) $r ([Math]::Max(2, [int]($r * 0.8))) 0.15 ($(if ($far) { 1 } else { 0 }))
  }
  $px = [int]$x1; $py = [int]$y1 + 2
  Fill ($px - 4) ($py - 1) 9 3 $(if ($far) { $f0 } else { $f1 })
  foreach ($c in -4, -1, 2, 4) { Px ($px + $c) ($py + 2) $claw; Px ($px + $c + 1) ($py + 2) $claw }
}

# A wolf head, steel grey, facing right. $open: jaws open with teeth.
function Head($cx, $cy, $s, $open) {
  # skull
  for ($j = -5 * $s; $j -le 5 * $s; $j++) { for ($i = -6 * $s; $i -le 6 * $s; $i++) {
    if (($i * $i) / (36.0 * $s * $s) + ($j * $j) / (25.0 * $s * $s) -le 1) {
      $t = ($j + 5 * $s) / (10.0 * $s) - 0.2 * $i / (6.0 * $s) + ($rng.NextDouble() - 0.5) * 0.15
      $c = if ($t -lt 0.25) { $h4 } elseif ($t -lt 0.45) { $h3 } elseif ($t -lt 0.7) { $h2 } else { $h1 }
      Px ($cx + $i) ($cy + $j) $c } } }
  # snout, upper jaw
  $mx = $cx + 4 * $s; $my = $cy - 1
  for ($i = 0; $i -lt 9 * $s; $i++) { $th = [int](3 * $s - $i / (4.0)); if ($th -lt 2) { $th = 2 }
    for ($j = 0; $j -lt $th; $j++) { Px ($mx + $i) ($my + $j) $(if ($j -eq 0) { $h3 } else { $h2 }) } }
  Px ($mx + 9 * $s - 1) $my $INK; Px ($mx + 9 * $s) ($my + 1) $INK
  if ($open) {
    $jy = $my + [int](2.5 * $s)
    Fill $mx $jy ([int](8 * $s)) ([int](2 * $s)) $gum
    Line $mx ($jy + [int](2 * $s)) ($mx + [int](7 * $s)) ($jy + [int](2 * $s) + 1) $h1 2
    for ($i = 1; $i -lt 8 * $s; $i += 2) { Px ($mx + $i) $jy $tooth; Px ($mx + $i) ($jy + [int](2 * $s) - 1) $tooth }
  } else { Line $mx ($my + [int](2.5 * $s)) ($mx + [int](8 * $s)) ($my + [int](2.5 * $s)) $INK }
  # brow, eye, ears
  Line ($cx - 1) ($cy - 2 * $s) ($cx + 4 * $s) ($cy - 2 * $s + 1) $h0
  Fill ($cx + 2 * $s) ($cy - 2 * $s + 1) 3 2 $INK; Fill ($cx + 2 * $s) ($cy - 2 * $s + 1) 2 1 $eye; Px ($cx + 2 * $s + 2) ($cy - 2 * $s + 1) $arc
  Line ($cx - 2 * $s) ($cy - 4 * $s) ($cx - 5 * $s) ($cy - 9 * $s) $h1 2; Line ($cx - 1 * $s) ($cy - 4 * $s) ($cx - 2 * $s) ($cy - 8 * $s) $h2 2
}

# A wooden pole with cross-arms and insulators; returns the insulator tips for stringing wires.
function Pole($bx, $by, $tx, $ty, $arms) {
  Line $bx $by $tx $ty $wd1 4
  Line ($bx - 1) $by ($tx - 1) $ty $wd2 1
  Line ($bx + 2) $by ($tx + 2) $ty $wd0 1
  for ($k = 0; $k -lt 6; $k++) { $f = $rng.NextDouble(); Px ($bx + ($tx - $bx) * $f) ($by + ($ty - $by) * $f) $wd0 }
  $tips = @()
  for ($a = 0; $a -lt $arms; $a++) {
    $f = 0.08 + 0.13 * $a; $ax = [int]($tx + ($bx - $tx) * $f); $ay = [int]($ty + ($by - $ty) * $f)
    $half = 9 - 2 * $a
    Line ($ax - $half) ($ay + 1) ($ax + $half) ($ay - 1) $wd1 2; Line ($ax - $half) $ay ($ax + $half) ($ay - 2) $wd3 1
    $offs = @((-$half + 1), ($half - 1), (-[int]($half / 2)), ([int]($half / 2)))
    foreach ($o in $offs) {
      $ix = $ax + $o; $iy = $ay - 2 - [int]($o / ($half * 2.0))
      Fill $ix ($iy - 2) 2 2 $ins; Px ($ix + 1) ($iy - 1) $insD; $tips += , @($ix, ($iy - 2))
    }
  }
  # brace
  $f = 0.3; Line ($tx + ($bx - $tx) * 0.08 - 4) ($ty + ($by - $ty) * 0.08 + 2) ($tx + ($bx - $tx) * $f) ($ty + ($by - $ty) * $f) $wd0
  return , $tips
}
# A sagging cable between two points.
function Cable($x0, $y0, $x1, $y1, $sag) {
  $n = [int]([Math]::Abs($x1 - $x0) + [Math]::Abs($y1 - $y0)); if ($n -lt 2) { $n = 2 }
  $px = $x0; $py = $y0
  for ($i = 1; $i -le $n; $i++) { $t = $i / [double]$n
    $x = $x0 + ($x1 - $x0) * $t; $y = $y0 + ($y1 - $y0) * $t + $sag * 4 * $t * (1 - $t)
    Line $px $py $x $y $cab; $px = $x; $py = $y }
}

# ---------------- composition ----------------
$ground = 92

# Poles first (behind the body); bases sink into the back.
$p1 = Pole 44 46 35 9 2
$p2 = Pole 60 42 62 5 2
$p3 = Pole 76 42 89 16 1
# Lines slung between the poles, and a few dangling loose.
Cable $p1[0][0] $p1[0][1] $p2[0][0] $p2[0][1] 7
Cable $p1[1][0] $p1[1][1] $p2[1][0] $p2[1][1] 9
Cable $p2[1][0] $p2[1][1] $p3[1][0] $p3[1][1] 8
Cable $p2[3][0] $p2[3][1] $p3[3][0] $p3[3][1] 10
Cable $p1[4][0] $p1[4][1] 26 34 6
Cable $p3[0][0] $p3[0][1] 104 26 6

# Far legs.
Leg 40 58 35 87 6 $true
Leg 84 56 88 87 6 $true
# Tail: raised, bushy, curling back over.
FurMass 30 50 6 5 1.4
FurMass 23 43 5 5 1.6
FurMass 18 34 5 6 1.8
FurMass 18 25 5 5 2.0
FurMass 22 17 3 4 2.2
# Body: hips, belly, hunched shoulders.
FurMass 46 56 19 11 1.2
FurMass 62 52 18 13 1.2
FurMass 78 46 16 15 1.4
FurMass 60 64 22 5 0.6 1
# Near legs.
Leg 36 60 29 87 7 $false
Leg 52 64 53 87 5 $false
Leg 86 58 94 87 7 $false
Leg 74 62 75 87 5 $false
# Belly shag hanging down.
for ($x = 30; $x -lt 92; $x += 2) { Line $x (67 + $rng.Next(-1, 2)) ($x - 1) (71 + $rng.Next(0, 3)) $f0 }

# Necks: three thick fur columns from the shoulders to the heads.
FurMass 92 40 9 8 1.3
FurMass 96 53 9 8 1.0
FurMass 92 64 8 7 0.8
# Heads: high and alert, middle snarling (the lead), low snarling.
Head 102 31 1.2 $false
Head 108 49 1.4 $true
Head 102 65 1.2 $true

# Mane of tangled cable round the necks and over the shoulders: random-walk scribbles, grey and black.
for ($k = 0; $k -lt 26; $k++) {
  $x = 84 + $rng.Next(0, 22); $y = 30 + $rng.Next(0, 38)
  $c = if ($rng.Next(0, 3) -eq 0) { $cabL } else { $cab }
  for ($s = 0; $s -lt 7; $s++) {
    $nx = $x + $rng.Next(-3, 4); $ny = $y + $rng.Next(-3, 3)
    if ((Opaque $nx $ny) -and ($bmp.GetPixel([int]$nx, [int]$ny) -eq $h2 -or $bmp.GetPixel([int]$nx, [int]$ny) -eq $h3)) { break }
    Line $x $y $nx $ny $c; $x = $nx; $y = $ny
  }
}
# A few cable ends spraying out past the heads (the frayed "beard").
foreach ($e in @(@(112, 39, 120, 35), @(114, 57, 123, 61), @(108, 73, 116, 79), @(88, 26, 82, 18), @(110, 25, 116, 17))) {
  Line $e[0] $e[1] $e[2] $e[3] $cab; Px $e[2] $e[3] $cabL
}

# Outline everything with ink (4-way neighbours of opaque pixels).
$hits = New-Object System.Collections.Generic.List[object]
for ($y = 0; $y -lt $H; $y++) { for ($x = 0; $x -lt $W; $x++) {
  if ($bmp.GetPixel($x, $y).A -ne 0) { continue }
  if ((Opaque ($x + 1) $y) -or (Opaque ($x - 1) $y) -or (Opaque $x ($y + 1)) -or (Opaque $x ($y - 1))) { $hits.Add(@($x, $y)) }
} }
foreach ($p in $hits) { Px $p[0] $p[1] $INK }

# Electricity over the outline: arcs between insulators and along the cables, crackling round the heads.
function Zig($x0, $y0, $x1, $y1, $n) {
  $px = $x0; $py = $y0
  for ($i = 1; $i -le $n; $i++) {
    $nx = [int]($x0 + ($x1 - $x0) * $i / $n); $ny = [int]($y0 + ($y1 - $y0) * $i / $n)
    if ($i -lt $n) { $ny += $rng.Next(-3, 4); $nx += $rng.Next(-2, 3) }
    Line $px $py $nx $ny $arc; $px = $nx; $py = $ny
  }
  Px $x1 $y1 $arcW; Px $x0 $y0 $arcW
}
Zig $p1[0][0] $p1[0][1] $p1[1][0] $p1[1][1] 3
Zig $p2[0][0] $p2[0][1] $p2[2][0] $p2[2][1] 3
Zig $p2[1][0] ($p2[1][1] + 2) $p3[1][0] ($p3[1][1] + 4) 6
Zig $p3[0][0] $p3[0][1] $p3[1][0] $p3[1][1] 2
Zig 104 21 116 17 3; Zig 118 45 124 39 2; Zig 112 71 120 77 2; Zig 86 30 92 24 2
Zig 89 62 93 84 4
foreach ($k in 1..14) { Px $rng.Next(4, $W - 4) $rng.Next(2, 60) $arc }

# Ground shadow.
$sh = [System.Drawing.Color]::FromArgb(80, 0, 0, 0)
for ($j = -3; $j -le 3; $j++) { for ($i = -46; $i -le 46; $i++) {
  if (($i * $i) / 2116.0 + ($j * $j) / 9.0 -le 1) { $x = 62 + $i; $y = $ground + 1 + $j; if ($y -lt $H -and $bmp.GetPixel($x, $y).A -eq 0) { Px $x $y $sh } } } }

$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
if ($Preview) {
  $sc = 6
  $pv = New-Object System.Drawing.Bitmap ($W * $sc), ($H * $sc)
  $g = [System.Drawing.Graphics]::FromImage($pv); $g.Clear((Hex '#4a4e48'))
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
  $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
  $g.DrawImage($bmp, (New-Object System.Drawing.Rectangle 0, 0, ($W * $sc), ($H * $sc)), (New-Object System.Drawing.Rectangle 0, 0, $W, $H), [System.Drawing.GraphicsUnit]::Pixel)
  $pv.Save($Preview, [System.Drawing.Imaging.ImageFormat]::Png)
}
