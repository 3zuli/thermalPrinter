# thermalPrinter
Arduino controller for the CITIZEN LT-286 thermal printer module. Runs on ATmega328.

This is an early un-polished version, feel free to contribute.

Accepts input strings via serial port at 115200Bd, converts them using the font defined in "font.h" and prints
them on the printer.

The printing head is a 384bit (48Byte) shift register, which represents 384 individual horizontal pixels of a single line. 
The head is split into 6 64bit sections, each having a separate Strobe signal. When pulled high, the pixels with value 1 
in the given section heat up and print out. The strobe activation time is described in the datasheet. Peak current for each
section is 2.2A at 5V, this practically limits the number of sections that can be strobed at the same time.
After all 6 sections are printed, the stepper is advanced by 1 step and another line can be printed.

Datasheet of the printer module: http://www.goodson.com.au/download/manual/cbm/user/LT286%20Specifications.pdf

The font is fixed char-width of 16 pixels (bits). Code takes the input string, splits it into 24-character lines 
(48B / 2B = 24), then maps each character in a line to the built-in font and prints it out one pixel-line at a time. 

Font generated with The Dot Factory: http://www.eran.io/the-dot-factory-an-lcd-font-and-image-generator/

This code is an early attempt, is written procedurally (no classes yet) and has several issues.
* The code needs cleanup. Contains lots of legacy/not needed/test stuff.
* Printing is slow, this could be fixed if I had a more powerful PSU, then the printing head could be
strobed faster than 1 section at a time. Also the motor driving could be improved somehow.
* There's no easy way to change the font (requieres manually generating a new font & reflashing the FW). 
* Supports only fixed char-width fonts.
* Supports only ASCII characters. No ( ͡° ͜ʖ ͡°) , sorry.
* Doesn't support direct bitmap printing
* Doesn't support the module's built-in sensors for paper feed, head up detection and thermistor for over-temp protection
* ATmega328's RAM doesn't quite cut it, when it comes to working with bitmap fonts. Bigger MCU would make life easier.

Easter eggs: Sine wave plotter and The slowest Mandelbrot fractal generator in the world :)
