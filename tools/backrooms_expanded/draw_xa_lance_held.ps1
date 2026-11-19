param([string]$Out, [string]$Preview)
# Draws the shock lance in your hands for UltiCa (Additions): 32x64 tiles like UltiCa's tall overlays
# (sprite_offset_y -32). 0 overlay_wielded_xa_shock_lance_off, 1-8 overlay_wielded_xa_shock_lance_on (eight
# animation frames). Upright and slim at the character's side, like the Flamero-Mado; shape in lance_shape.ps1.
Add-Type -AssemblyName System.Drawing
$T = 32; $TW = 32; $TH = 64
$Count = 9
$bmp = New-Object System.Drawing.Bitmap ($Count * $TW), $TH, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"
. "$PSScriptRoot\spear_shape.ps1"
. "$PSScriptRoot\lance_shape.ps1"

$held = Layout 11.5 63 0 -1 62 -Split 66 -Width 0.55

DrawLance 0 $held $false 4.5
OutlineT 0 $INK
foreach ($fr in 0..7) {
  DrawLance (1 + $fr) $held $true 4.5
  OutlineT (1 + $fr) $INK
  LanceArcs (1 + $fr) $held (700 + 41 * $fr) 1.0
}

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
