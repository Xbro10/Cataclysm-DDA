param([string]$Out, [string]$Preview)
# Draws the tally token (br_tally_token), UndeadPeople-style, one 32x32 tile:
# three brass washers, the top one stamped with five tally marks.
Add-Type -AssemblyName System.Drawing
$T = 32
$Count = 1
$bmp = New-Object System.Drawing.Bitmap ($Count * $T), $T, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
. "$PSScriptRoot\sprite_common.ps1"

$br = Hex '#c89a3a'; $brD = Hex '#8a6420'; $brL = Hex '#f0cc6a'; $brH = Hex '#fff0b0'; $hole = Hex '#3a2a10'
# Two tokens lying flat beside it, overlapping.
Ell 0 9 22 6 4 $brD; Ell 0 9 21 6 4 $br; Ell 0 8 20 4 2 $brL; Ell 0 9 21 4 3 $br; Ell 0 9 21 1 1 $hole
Ell 0 23 24 6 4 $brD; Ell 0 23 23 6 4 $br; Ell 0 22 22 4 2 $brL; Ell 0 23 23 4 3 $br; Ell 0 23 23 1 1 $hole
# The top token, face up and tilted: a rim, a raised face, a centre hole.
Ell 0 16 15 10 6 $brD; Ell 0 16 14 10 6 $br; Ell 0 15 13 8 4 $brL; Ell 0 16 14 8 4 $br
Ell 0 16 14 1 1 $hole
$ink = Hex '#5a3a0a'
# Five tally marks on the face: four strokes and a slash.
foreach ($x in 9, 11, 21, 23) { Line 0 $x 12 $x 16 $ink }
Line 0 8 16 24 12 $ink
Px 0 12 10 $brH; Px 0 13 10 $brH; Px 0 14 10 $brH; Px 0 7 19 $brH; Px 0 21 21 $brH
Outline 0; Shadow 0 16 27 10

SaveSheet $Out $Preview $Count '#3a3634'
