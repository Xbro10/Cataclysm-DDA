param([string]$Out, [string]$Preview)
# Draws Backrooms Expanded's light tools, UndeadPeople-style, 32x32 tiles in a row.
# 0 work lamp (br_work_lamp)          1 work lamp, on (br_work_lamp_on)
# 2 lightstick bundle               3-5 lightstick bundle, lit (3 frames, animated)
# 6 UV torch (br_uv_torch)          7-9 UV torch, on (3 frames, animated)
Add-Type -AssemblyName System.Drawing
$T = 32
$Count = 10
$bmp = New-Object System.Drawing.Bitmap ($Count * $T), $T, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"

# Soft light: blends a colour over what is there (or paints it see-through on empty pixels), fading with distance.
function Glow($tile, [double]$cx, [double]$cy, [double]$r, $col, [double]$peak) {
  for ($y = [int]($cy - $r); $y -le [int]($cy + $r); $y++) {
    for ($x = [int]($cx - $r); $x -le [int]($cx + $r); $x++) {
      if ($x -lt 0 -or $x -ge $T -or $y -lt 0 -or $y -ge $T) { continue }
      $d = [Math]::Sqrt(($x - $cx) * ($x - $cx) + ($y - $cy) * ($y - $cy))
      if ($d -gt $r) { continue }
      $a = $peak * (1 - $d / $r) * (1 - $d / $r)
      $old = $bmp.GetPixel($tile * $T + $x, $y)
      if ($old.A -eq 0) {
        $bmp.SetPixel($tile * $T + $x, $y, [System.Drawing.Color]::FromArgb([int](255 * $a), $col.R, $col.G, $col.B))
      } else {
        $nr = [int]($old.R + ($col.R - $old.R) * $a); $ng = [int]($old.G + ($col.G - $old.G) * $a); $nb = [int]($old.B + ($col.B - $old.B) * $a)
        $na = [Math]::Max([int]$old.A, [int](255 * $a))
        $bmp.SetPixel($tile * $T + $x, $y, [System.Drawing.Color]::FromArgb($na, $nr, $ng, $nb))
      }
    }
  }
}

# ---- 0, 1: work lamp: a yellow boxy floodlight head on a folding steel stand.
$yel = Hex '#e0b02a'; $yelD = Hex '#a87e16'; $yelL = Hex '#f4d466'; $steel = Hex '#8a9098'; $steelD = Hex '#5a6068'
$lensOff = Hex '#9aa4a8'; $lensOffL = Hex '#c8d0d2'; $lensOn = Hex '#fffbe8'; $lensOnL = Hex '#ffffff'
foreach ($tt in 0, 1) {
  # Legs: an A-frame.
  Line $tt 16 18 9 28 $steelD; Line $tt 16 18 23 28 $steelD; Line $tt 16 18 16 28 $steel
  Line $tt 11 25 21 25 $steel
  # Yoke.
  Fill $tt 8 9 2 9 $steel; Fill $tt 22 9 2 9 $steel; Fill $tt 8 17 16 2 $steel
  # Head.
  Fill $tt 10 6 12 11 $yel; Fill $tt 10 6 12 2 $yelL; Fill $tt 10 15 12 2 $yelD
  for ($x = 11; $x -lt 21; $x += 2) { Px $tt $x 5 $yelD }
  $lens = if ($tt -eq 0) { $lensOff } else { $lensOn }
  $lensL = if ($tt -eq 0) { $lensOffL } else { $lensOnL }
  Fill $tt 12 8 8 6 $lens; Fill $tt 12 8 3 2 $lensL
  for ($y = 9; $y -lt 14; $y += 2) { Fill $tt 12 $y 8 1 $(if ($tt -eq 0) { Hex '#86909a' } else { Hex '#fff2c0' }) }
  Fill $tt 12 8 3 1 $lensL
  Outline $tt; Shadow $tt 16 29 9
}
Glow 1 16 11 15 (Hex '#fff6d0') 0.55
Fill 1 12 8 8 6 $lensOn; Fill 1 13 9 4 2 $lensOnL

# ---- 2-5: lightstick bundle: five glowsticks taped side by side, two grey tape bands.
$tape = Hex '#9a9aa0'; $tapeL = Hex '#c4c4ca'
$sticksOff = @('#5a9a5a', '#7aa84a', '#4a8a7a', '#6a9a4a', '#5a8a5a')
$sticksOn = @('#a8ff9a', '#d4ff7a', '#8affd8', '#c0ff8a', '#a0ffb0')
foreach ($tt in 2, 3, 4, 5) {
  for ($i = 0; $i -lt 5; $i++) {
    # Diagonal sticks, lower-left to upper-right, each offset a little.
    $c = if ($tt -eq 2) { Hex $sticksOff[$i] } else { Hex $sticksOn[$i] }
    $x0 = 6 + 2 * $i; $y0 = 25 + ($i % 2)
    Line $tt $x0 $y0 ($x0 + 13) ($y0 - 15) $c 2
    Px $tt ($x0 + 13) ($y0 - 16) $(if ($tt -eq 2) { Hex '#3a4a3a' } else { Hex '#ffffff' })
  }
  Line $tt 8 19 20 21 $tape 2; Line $tt 13 13 25 15 $tape 2
  Px $tt 12 19 $tapeL; Px $tt 18 14 $tapeL
}
Outline 2; Shadow 2 16 28 9
# Lit: a big green-white glow that breathes over three frames.
$peaks = @(0.55, 0.7, 0.6); $radii = @(14, 16, 15)
for ($f = 0; $f -lt 3; $f++) {
  $tt = 3 + $f
  Outline $tt; Shadow $tt 16 28 9
  Glow $tt 16 18 $radii[$f] (Hex '#c8ffb8') $peaks[$f]
  Glow $tt 16 18 6 (Hex '#ffffff') (0.35 + 0.1 * $f)
}

# ---- 6-9: UV torch: a chunky black-and-purple torch lying corner to corner, violet lens at the top right.
$body = Hex '#2e2a36'; $bodyL = Hex '#4a4458'; $grip = Hex '#1c1a22'; $ring = Hex '#7a5ab0'
$lensC = Hex '#5a2a8a'; $lensCL = Hex '#8a5ac0'; $beamC = Hex '#b070ff'; $beamCore = Hex '#e8c8ff'
foreach ($tt in 6, 7, 8, 9) {
  Line $tt 7 25 19 13 $body 4
  Line $tt 6 24 17 13 $bodyL 1
  for ($k = 0; $k -lt 4; $k++) { Line $tt (8 + 2 * $k) (26 - 2 * $k) (11 + 2 * $k) (23 - 2 * $k) $grip }
  # Head: wider, with a purple ring.
  Line $tt 18 14 22 10 $body 6
  Line $tt 17 11 21 15 $ring 1
  $lc = if ($tt -eq 6) { $lensC } else { Hex '#c890ff' }
  Line $tt 21 7 25 11 $lc 2
  Px $tt 22 8 $(if ($tt -eq 6) { $lensCL } else { Hex '#ffffff' })
  # Switch.
  Px $tt 13 18 $ring; Px $tt 14 17 $ring
}
Outline 6; Shadow 6 15 28 9
# On: a violet cone up and to the right that shimmers over three frames.
$beamPeak = @(0.75, 0.9, 0.8)
for ($f = 0; $f -lt 3; $f++) {
  $tt = 7 + $f
  Outline $tt; Shadow $tt 15 28 9
  for ($s = 0; $s -lt 6; $s++) { Glow $tt (25 + 1.2 * $s) (7 - 1.2 * $s) (3 + $s) $beamC ($beamPeak[$f] - 0.08 * $s) }
  Glow $tt 24 8 4 $beamCore (0.5 + 0.1 * $f)
  # Glinting motes in the beam, different each frame.
  $motes = @(@(@(28, 4), @(26, 2), @(30, 6)), @(@(27, 3), @(30, 2), @(29, 7)), @(@(29, 4), @(27, 1), @(31, 5)))
  foreach ($m in $motes[$f]) { Px $tt $m[0] $m[1] (Hex '#f4e4ff') }
}

SaveSheet $Out $Preview $Count '#3a3634'
