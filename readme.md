# Opallios
This project is to make a decorative RGB LED display. This will use a HUB75 interface 64x64 RGB matrix. There will be an FPGA to drive the display, with an embedded linux device in order to render and send data to the FPGA.

## Required features
- Drive the RGB LED display at 30Hz
- Support 18 bit color
- Display gifs on loop
- Wall mounting

## Additional features
- Drive the display at 60Hz or greater
- Support 24 bit color
- Render arbitrary data to framebuffer to draw, such as a 3d rotating object
- Include an accelerometer/gyro to allow for interactive elements, for example, a water simulation
- Support daisy chained displays
- Button(s) to switch between different displays
- Clock?
- Battery backup for real time clock
- Swivelable wall mount
- Global dimming

For a detailed description, see the [design description document](/Documentation/Description_Document.md).
