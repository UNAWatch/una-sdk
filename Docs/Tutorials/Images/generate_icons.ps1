Add-Type -AssemblyName System.Drawing

$dir = "Docs/Tutorials/Images/Resources/icons"

mkdir $dir -ErrorAction SilentlyContinue

# Function to create icon

function Create-Icon {

    param ($file, $drawAction)

    $b = New-Object System.Drawing.Bitmap 32,32

    $g = [System.Drawing.Graphics]::FromImage($b)

    $g.Clear([System.Drawing.Color]::Transparent)

    & $drawAction $g

    $b.Save("$dir/$file", [System.Drawing.Imaging.ImageFormat]::Png)

    $g.Dispose()

    $b.Dispose()

}

# Bitmap icon

Create-Icon "bitmap.png" {

    param ($g)

    $blue = [System.Drawing.Color]::Blue

    $g.FillRectangle((New-Object System.Drawing.SolidBrush $blue), 4,4,24,24)

    $yellow = [System.Drawing.Color]::Yellow

    $g.FillEllipse((New-Object System.Drawing.SolidBrush $yellow), 20,6,6,6)

    $green = [System.Drawing.Color]::Green

    $points = @(

        (New-Object System.Drawing.Point 8,20),

        (New-Object System.Drawing.Point 16,12),

        (New-Object System.Drawing.Point 24,20)

    )

    $g.FillPolygon((New-Object System.Drawing.SolidBrush $green), $points)

}

# Position icon: crosshair

Create-Icon "position.png" {

    param ($g)

    $black = [System.Drawing.Color]::Black

    $pen = New-Object System.Drawing.Pen $black, 2

    $g.DrawLine($pen, 0,16,32,16)

    $g.DrawLine($pen, 16,0,16,32)

    $pen.Dispose()

}

# Size icon: rectangle with arrows

Create-Icon "size.png" {

    param ($g)

    $black = [System.Drawing.Color]::Black

    $pen = New-Object System.Drawing.Pen $black, 2

    $g.DrawRectangle($pen, 8,8,16,16)

    # arrows

    $g.DrawLine($pen, 0,16,8,16)

    $g.DrawLine($pen, 24,16,32,16)

    $g.DrawLine($pen, 16,0,16,8)

    $g.DrawLine($pen, 16,24,16,32)

    # arrow heads

    $g.DrawLine($pen, 0,16,4,12)

    $g.DrawLine($pen, 0,16,4,20)

    $g.DrawLine($pen, 32,16,28,12)

    $g.DrawLine($pen, 32,16,28,20)

    $g.DrawLine($pen, 16,0,12,4)

    $g.DrawLine($pen, 16,0,20,4)

    $g.DrawLine($pen, 16,32,12,28)

    $g.DrawLine($pen, 16,32,20,28)

    $pen.Dispose()

}

# Opacity icon: semi transparent square over solid

Create-Icon "opacity.png" {

    param ($g)

    $solid = [System.Drawing.Color]::Blue

    $g.FillRectangle((New-Object System.Drawing.SolidBrush $solid), 4,4,24,24)

    $trans = [System.Drawing.Color]::FromArgb(128, 255, 0, 0)  # semi red

    $g.FillRectangle((New-Object System.Drawing.SolidBrush $trans), 10,10,12,12)

}