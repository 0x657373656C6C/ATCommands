# ATCommands
This is an Arduino library for folks who love the simplicity of AT-style commands.

## Inspiration
I do a lot of work with ESP8266, ESP32 and various other radio-type modules for LoRA, GSM and other things.  One thing most of them have in common is that you can either take the easy road and use their 'AT' commands via UART or take the long way and struggle through with SPI and registers.

I have noticed some folks doing amazing things with Arduinos in the areas of robotics and communications right up to the point where integration with another microcontroller or the PC is required.  Here some 'interesting' inventions have emerged with bespoke serial protocols, i2c magick or even more SPI.

This library will present an option to use the UART serial port, make use of existing libraries and neatly control the flow of parameters and communication, your way :-)

## Goals
* **Small-ish** footprint but realistically workable by the end user.  I could spent time malloc and realloc my way through things but Arduino has ways to do Arduino things.  So until that does not work anymore...
* **Uncomplicated**.  I've seen other command parses require some elaborate definitions of structs and arrays for commands and variables which until a use case presents itself is not done here.
* Uses existing **Serial** class.
* Lots of *FREEDOM* to implement own functions.

## Command Structure
The Hayes AT command structure on which this is loosely based has four defined command types.

### TEST Command
```
AT+START=?                                              # TEST command
+START=<MODE:UINT32[WO]>                                # Instruction response
Start scanning after write (0 - wifi, 1 - bluetooth).   # Description response
OK                                                      # Automatic acknowledge
```

### WRITE Command
```
AT+START=0                                              # WRITE command
+SCAN=-10,"wifi1"                                       # Unsolicited read response
+SCAN=-50,"wifi2"                                       # Unsolicited read response
+SCAN=-20,"wifi3"                                       # Unsolicited read response
OK     
```

## READ Command
```
AT+BAUD?                                                # Query baud rate
+BAUD=115200,8,N,1                                      # Response
OK
```

## RUN Command
```
ATZ                                                     # RESET 
OK
```

## The ATCommands Object
Life begins with the inclusion of the header and the declaration of the ATCommands object as follows:
```c++
#include <ATCommands.h>

ATCommands AT
```

## Defining Callback Functions
One of the more interesting features of this library is that commands are registered at the start of the program and during execution callbacks are invoked to run the specific functions. 

All that mumbo-jumbo just means this :-

First, we need to define our RUN, READ, WRITE or TEST commands in a structured and agreed contract.

```c++
bool at_command(ATCommands *sender)
{
    ...
}
```

This definition has some important properties.  The sender pointer contains a number of objects of information as well as commands providing internal access to the library.  Moreover the return type (true/false) determines whether OK or ERROR is eventually printed on the terminal.

## Adding Commands To An Array
Rarely does just one command exist so this library provides a convenient way to register commands in bulk through a uniform structure.

```c++
static at_command_t commands[] = {
    {"+PRINT", at_run_cmd_print, at_test_cmd_print, at_read_cmd_print, at_write_cmd_print},
};
```

The declaration above will:
* bind AT+PRINT to the function at_run_cmd_print
* bind AT+PRINT=? to the function at_test_cmd_print
* bind AT+PRINT? to the function at_read_cmd_print
* bind AT+PRINT=Hello,World to the function at_write_cmd_print

## Registering The Command Array
Once the command array has been defined it can be registered in the setup() section of your sketch along with the serial port and other parameters.

```c++
void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);

    AT.begin(&Serial, commands, sizeof(commands), WORKING_BUFFER_SIZE);
}
```
Here we declare our UART that will be servicing the AT stream as well as the command array.  We also pre-declare the array size since it cannot be read as a pointer once passed to the library.  

Lastly we declare a buffer size for the library which should be as big as the length of the input you are specting to receive.  

If you expect short commands with few parameters then the buffer can be smaller.  This is important because the buffer's memory is reserved by the library.

## Calling Update
In the sketch's loop function call update() to read the serial port and process any incoming data.

```c++
void loop()
{
    // put your main code here, to run repeatedly:
    AT.update();
}
```

# Contributions
This library is fairly basic and does a good job covering a fair amount of use cases but still requires some more real-world use and testing.  If you do find a bug please report it via the Issues Tracker.

If, on the other hand, you are super awesome and find a bug AND fix it, or make an improvement no matter how big or small please consider submitting a pull request.  Your awesome is everyone's awesome!

# Credits
Many people do amazing things of whose tips, techniques and even bits of code made it in here:

* [SerialCommands](https://github.com/ppedro74/Arduino-SerialCommands) by ppedro74.  I liked the way commands were registered and the 'sender' concept.
* [cAT](https://github.com/marcinbor85/cAT) by Marcin Borowicz.  One day I might yet port this library to that of cAT.

# TODO
On the list:
* Unsolicited responses (with threading)
