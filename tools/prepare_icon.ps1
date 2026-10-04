# Convert the approved artwork to the same 320x176 PNG format as the SDK icon.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$projectRoot = Split-Path -Parent $PSScriptRoot
$sourceImage = [System.Drawing.Image]::FromFile((Join-Path $projectRoot 'assets/icon-xmb-transparent.png'))
$iconBitmap = New-Object System.Drawing.Bitmap(320,176)
$canvas = [System.Drawing.Graphics]::FromImage($iconBitmap)
try {
    $canvas.Clear([System.Drawing.Color]::Transparent)
    $canvas.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $canvas.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $ratio = [Math]::Min(320.0 / $sourceImage.Width, 176.0 / $sourceImage.Height)
    $scaledWidth = [int][Math]::Round($sourceImage.Width * $ratio)
    $scaledHeight = [int][Math]::Round($sourceImage.Height * $ratio)
    $targetRect = New-Object System.Drawing.Rectangle(([int]((320-$scaledWidth)/2)),([int]((176-$scaledHeight)/2)),$scaledWidth,$scaledHeight)
    $canvas.DrawImage($sourceImage,$targetRect)
    $iconBitmap.Save((Join-Path $projectRoot 'assets/ICON0.PNG'),[System.Drawing.Imaging.ImageFormat]::Png)
} finally {
    $canvas.Dispose()
    $iconBitmap.Dispose()
    $sourceImage.Dispose()
}
