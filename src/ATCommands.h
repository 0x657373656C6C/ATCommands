/*
 This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <Arduino.h>

//#define AT_COMMANDS_DEBUG

#ifndef __AT_COMMANDS_H__
#define __AT_COMMANDS_H__

#define AT_ERROR "ERROR"
#define AT_SUCCESS "OK"

typedef class ATCommands ATCommands;

typedef enum ternary
{
    AT_COMMANDS_SUCCESS = 0,
    AT_COMMANDS_ERROR_NO_SERIAL,
    AT_COMMANDS_ERROR_BUFFER_FULL,
    AT_COMMANDS_ERROR_SYNTAX
} AT_COMMANDS_ERRORS;

typedef enum
{
    AT_COMMAND_READ,
    AT_COMMAND_WRITE,
    AT_COMMAND_TEST,
    AT_COMMAND_RUN
} AT_COMMAND_TYPE;

/**
 * @brief at_parse_t
 * 
 * used to hold a parsed AT command
 * 
 */
typedef struct
{
    char *prefix; // the 'AT' prefix

} at_parse_t;

/**
 * @brief at_command_t struct
 *  borrowed verbatim from esp-at and is used to define an AT command 
*/
typedef struct
{
    char *at_cmdName;                  // command name
    bool (*at_runCmd)(ATCommands *);   // RUN command function pointer
    bool (*at_testCmd)(ATCommands *);  // TEST command function pointer
    bool (*at_readCmd)(ATCommands *);  // READ command function pointer
    bool (*at_writeCmd)(ATCommands *); // WRITE command function pointer

} at_command_t;

typedef struct
{
    String buffer; // the working buffer
    size_t size;   // the buffer size
} at_buffer_t;

class ATCommands
{
private:
    // placeholder for number of commands in the array
    uint16_t numberOfCommands;
    const at_command_t *atCommands;

    // input buffers
    String bufferString = "";
    uint16_t bufferSize = 0;
    uint16_t bufferPos = 0; // keeps track of the buffer so that we don't overflow based on bufferSize above
    const char *term;
    void writeToBuffer(int data);

    // input validation
    static int isValidCmdChar(const char c);
    static uint8_t ATCommands::hexToChar(const char ch);

    // registers command array
    bool registerCommands(const at_command_t *commands, uint32_t size);

    // command parsing
    uint8_t AT_COMMAND_TYPE; // the type of command (see enum declaration)
    //char *params;            // command parameters in the case of a WRITE
    uint16_t tokenPos = 0; // position of token used when splitting parameters
    uint16_t termPos = 0;
    bool parseCommand();                // determines the command and command type
    bool parseParameters(uint16_t pos); // parses parameters in the case of a WRITE command
    void processCommand();              // invokes handler to process command

    // callbacks
    bool (*defaultHandler)(ATCommands *);
    void setDefaultHandler(bool (*function)(ATCommands *));

public:
    Stream *serial;

    // initialize
    ATCommands();

    // register serial port, commands and buffers
    void begin(Stream *serial, const at_command_t *commands, uint32_t size, const uint16_t bufferSize, const char *terminator = "\r\n");

    // command parsing
    String command; // the command (eg: +TEST in AT+TEST)

    /**
	 * @brief Checks the Serial port, reads the input buffer and calls a matching command handler.
	 * @return SERIAL_COMMANDS_SUCCESS when successful or SERIAL_COMMANDS_ERROR_XXXX on error.
	 */
    AT_COMMANDS_ERRORS update();

    /**
	 * @brief Clears the buffer, and resets the indexes.
	 */
    void clearBuffer();

    /**
     * @brief retrieves next token
     * 
     * @return String 
     */
    String next();

    /**
     * @brief indicates if there are more tokens in the buffer
     * 
     * @return true 
     * @return false 
     */
    bool hasNext();

    void ok();
    void error();
};

#endif