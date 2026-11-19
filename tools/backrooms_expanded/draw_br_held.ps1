param([string]$Out, [string]$Preview)
# Draws held-weapon overlays (what your character holds), 32x64 tiles like UltiCa's tall overlays
# (sprite_offset_y -32: the bottom 32 rows line up with the character's tile).
# 0 overlay_wielded_br_flamero_mado_off   1-4 overlay_wielded_br_flamero_mado_on (four animation frames)
# The spear stands upright at the character's side, blade up, like UltiCa's own pikes and spears.
# Its shape lives in spear_shape.ps1, shared with the ground sprites.
Add-Type -AssemblyName System.Drawing
$T = 32; $TW = 32; $TH = 64
$Count = 5
$bmp = New-Object System.Drawing.Bitmap ($Count * $TW), $TH, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"
. "$PSScriptRoot\spear_shape.ps1"

$held = Layout 11.5 63 0 -1 62 -Split 66 -Width 0.55

DrawSpear 0 $held $false 4.0
OutlineT 0 $INK

# Lit, four animation frames on tiles 1-4 (see FlameFrame in spear_shape.ps1).
function DrawLit($tile, $frame) {
  $script:FlameFrame = $frame; FlameFrameStart
  DrawSpear $tile $held $true 4.0
  OutlineT $tile $INK
  # Flames curl out of the collar (about y 24) and climb the blade.
  Ribbon $tile @(@(10, 24), @(7, 23), @(5, 20), @(5, 17), @(6, 14), @(5, 11)) 2.4 $false
  Ribbon $tile @(@(13, 25), @(16, 24), @(18, 21), @(18, 18), @(17, 15), @(18, 12)) 2.4 $false
  Ribbon $tile @(@(9, 22), @(8, 18), @(9, 14), @(8, 10)) 1.6 $false
  Ribbon $tile @(@(14, 22), @(15, 18), @(14, 14), @(15, 10)) 1.6 $false
  Ribbon $tile @(@(11, 26), @(8, 27), @(6, 26)) 1.3 $false
  # One thin strand wraps in front of the collar.
  Ribbon $tile @(@(9, 26), @(11, 24), @(13, 23), @(14, 21)) 1.0 $true
  Embers $tile @(@(4, 8), @(19, 7), @(7, 5), @(17, 3), @(3, 15), @(20, 16))
}
foreach ($fr in 0..3) { DrawLit (1 + $fr) $fr }
$script:FlameFrame = 0

# Save, plus a 6x preview with a gap between tiles.
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
if ($Preview) {
  $sc = 6
  $pv = New-Object System.Drawing.Bitmap ($bmp.Width * $sc + ($Count - 1) * $sc), ($TH * $sc)
  $g = [System.Drawing.Graphics]::FromImage($pv)
  $g.Clear((Hex '#5c5650'))
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
  $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
  for ($ti = 0; $ti -lt $Count; $ti++) {
    $src = New-Object System.Drawing.Rectangle ($ti * $TW), 0, $TW, $TH
    $dst = New-Object System.Drawing.Rectangle ($ti * ($TW * $sc + $sc)), 0, ($TW * $sc), ($TH * $sc)
    $g.DrawImage($bmp, $dst, $src, [System.Drawing.GraphicsUnit]::Pixel)
  }
  $pv.Save($Preview, [System.Drawing.Imaging.ImageFormat]::Png)
}
