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
#include "ATCommands.h"

ATCommands::ATCommands()
{
}

void ATCommands::begin(Stream *stream, const at_command_t *commands, uint32_t size, const uint16_t bufferSize, const char *terminator = "\r\n")
{
    this->serial = stream;
    this->term = terminator;
    this->bufferString.reserve(bufferSize);
    this->bufferSize = bufferSize;

    registerCommands(commands, size);
    clearBuffer();
}

/**
 * @brief parseCommand
 * Checks the incoming buffer to ensure it begins with AT and then seeks to
 * determine if the command is of type RUN, TEST, READ or WRITE.  For WRITE
 * commands the buffer is furthered parsed to extract parameters.  Once
 * all actions are complete and the command type is determined the command
 * is compared against the delcared array (atCommands) to find a matching
 * command name.  If a match is found the function is passed to the handler
 * for later execution.
 * @return true 
 * @return false 
 */
bool ATCommands::parseCommand()
{
    uint16_t pos = 2;
    uint8_t type;

    // validate input so that we act only when we have to
    if (this->bufferPos == 0)
    {
        // fall through
        setDefaultHandler(NULL);
        return true;
    }

    if (!this->bufferString.startsWith("AT"))
    {
        return false;
    }

    for (uint16_t i = 2; i < this->bufferSize; i++)
    {
        // if we reach the null terminator and have not yet reached a state then we
        // assume this to be a RUN command
        if (this->bufferString[i] == '\0')
        {
            type = AT_COMMAND_RUN;
            break;
        }

        // eliminate shenanigans
        if (isValidCmdChar(this->bufferString[i]) == 0)
        {
            return false;
        }

        // determine command type
        if (this->bufferString[i] == '=')
        {
            // Is this a TEST or a WRITE command?
            if (this->bufferString[i + 1] == '?')
            {
                type = AT_COMMAND_TEST;
                break;
            }
            else
            {
                type = AT_COMMAND_WRITE;
                break;
            }
        }
        if (this->bufferString[i] == '?')
        {
            type = AT_COMMAND_READ;
            break;
        }

        pos++;
    }
    this->command = this->bufferString.substring(2, pos);
    this->AT_COMMAND_TYPE = type;
    int8_t cmdNumber = -1;

    // search for matching command in array
    for (uint8_t i = 0; i < this->numberOfCommands; i++)
    {
        if (command.equals(atCommands[i].at_cmdName))
        {
            cmdNumber = i;
            break;
        }
    }

    // if we did not find a match there's no point in continuing
    if (cmdNumber == -1)
    {
        clearBuffer();
        return false;
    }

    // handle the different commands
    switch (type)
    {
    case AT_COMMAND_RUN:
        setDefaultHandler(this->atCommands[cmdNumber].at_runCmd);
        goto process;
    case AT_COMMAND_READ:
        setDefaultHandler(this->atCommands[cmdNumber].at_readCmd);
        goto process;
    case AT_COMMAND_TEST:
        setDefaultHandler(this->atCommands[cmdNumber].at_testCmd);
        goto process;
    case AT_COMMAND_WRITE:
        if (parseParameters(pos))
        {
            setDefaultHandler(this->atCommands[cmdNumber].at_writeCmd);
            goto process;
        }
        return false;

    process:
        // future placeholder
        return true;
    }

    return true;
}

/**
 * @brief parseParameters
 * Called mainly by parseCommand as an extention to tokenize parameters
 * usually supplied in WRITE (eg AT+COMMAND=param1,param2) commands.  It makes
 * use of malloc so check above in parseCommand where we free it to keep things
 * neat.
 * @param pos 
 * @return true 
 * @return false 
 */
bool ATCommands::parseParameters(uint16_t pos)
{

    this->bufferString = this->bufferString.substring(pos + 1, this->bufferSize - pos + 1);

    return true;
}

boolean ATCommands::hasNext()
{
    if (tokenPos < bufferSize)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief next
 * This is called by user functions to iterate through the tokenized parameters.
 * Returns NULL when there is nothing more.  Subsequent calls pretty much ensure
 * this goes in a loop but it is expected the user knows their own parameters so there
 * would be no need to exceed boundaries.
 * @return char* 
 */
String ATCommands::next()
{
    // if we have reached the boundaries return null so
    // that the caller knows not to expect anything
    if (tokenPos >= this->bufferSize)
    {
        tokenPos = bufferSize;
        return "";
    }

    String result = "";
    int delimiterIndex = this->bufferString.indexOf(",", tokenPos);

    if (delimiterIndex == -1)
    {
        result = this->bufferString.substring(tokenPos);
        tokenPos = this->bufferSize;
        return result;
    }
    else
    {
        result = this->bufferString.substring(tokenPos, delimiterIndex);
        tokenPos = delimiterIndex + 1;
        return result;
    }
}

/**
 * @brief update
 * Main function called by the loop.  Reads in available charactrers and writes
 * to the buffer.  When the line terminator is found continues to parse and eventually
 * process the command.
 * @return AT_COMMANDS_ERRORS 
 */
AT_COMMANDS_ERRORS ATCommands::update()
{
    if (serial == NULL)
    {
        return AT_COMMANDS_ERROR_NO_SERIAL;
    }

    while (serial->available() > 0)
    {
        int ch = serial->read();

#ifdef AT_COMMANDS_DEBUG
        Serial.print(F("Read: bufferSize="));
        Serial.print(this->bufferSize);
        Serial.print(F(" bufferPos="));
        Serial.print(bufferPos);
        Serial.print(F(" termPos="));
        Serial.print(termPos);
        if (ch < 32)
        {
            Serial.print(F(" ch=#"));
            Serial.print(ch);
        }
        else
        {
            Serial.print(" ch=[");
            Serial.print((char)ch);
            Serial.print(F("]"));
        }
        Serial.println();
#endif
        if (ch <= 0)
        {
            continue;
        }

        if (bufferPos < this->bufferSize)
        {
            writeToBuffer(ch);
        }
        else
        {
#ifdef AT_COMMANDS_DEBUG
            Serial.println(F("--BUFFER OVERFLOW--"));
#endif
            clearBuffer();
            return AT_COMMANDS_ERROR_BUFFER_FULL;
        }

        if (term[termPos] != ch)
        {
            termPos = 0;
            continue;
        }

        if (term[++termPos] == 0)
        {

#ifdef AT_COMMANDS_DEBUG
            Serial.print(F("Received: ["));
            for (uint32_t n = 0; n < this->bufferSize; n++)
            {
                Serial.print(this->bufferString[n]);
            }
            Serial.println(F("]"));
#endif

            if (!parseCommand())
            {
                this->error();
                clearBuffer();
                return;
            }

            // process the command
            processCommand();

            // finally clear the buffer
            clearBuffer();
        }
    }
}

/**
 * @brief writeToBuffer
 * writes the input to the buffer excluding line terminators
 * @param data 
 */
void ATCommands::writeToBuffer(int data)
{
    // we don't write EOL to the buffer
    if ((char)data != 13 && (char)data != 10)
    {
        this->bufferString += (char)data;
        bufferPos++;
    }
}

/**
 * @brief setDefaultHandler
 * Sets the function handler (callback) on the user's side.
 * @param function 
 */
void ATCommands::setDefaultHandler(bool (*function)(ATCommands *))
{
    this->defaultHandler = function;
}

/**
 * @brief processCommand
 * Invokes the defined handler to process (callback) the command on the user's side.
 */
void ATCommands::processCommand()
{
    if (defaultHandler != NULL)
        if ((*defaultHandler)(this))
        {
            this->ok();
        }
        else
        {
            this->error();
        }
}

/**
 * @brief registerCommands
 * Registers the user-supplied command array for use later in parseCommand
 * @param commands 
 * @param size 
 * @return true 
 * @return false 
 */
bool ATCommands::registerCommands(const at_command_t *commands, uint32_t size)
{
    atCommands = commands;
    numberOfCommands = (uint16_t)(size / sizeof(at_command_t));
}

/**
 * @brief clearBuffer
 * resets the buffer and other buffer-related variables
 */
void ATCommands::clearBuffer()
{
    //for (uint16_t i = 0; i < this->buffer->size; i++)
    this->bufferString = "";
    termPos = 0;
    bufferPos = 0;
    tokenPos = 0;
}

/**
 * @brief ok
 * prints OK to terminal
 * 
 */
void ATCommands::ok()
{
    this->serial->println("OK");
}

/**
 * @brief error
 * prints ERROR to terminal
 * 
 */
void ATCommands::error()
{
    this->serial->println("ERROR");
}

/**
 * @brief isValidCmdChar
 * Hackish attempt at validating input commands
 * @param ch 
 * @return int 
 */
int ATCommands::isValidCmdChar(const char ch)
{
    return (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || (ch == '+') || (ch == '#') || (ch == '$') || (ch == '@') || (ch == '_') || (ch == '=') || (ch == '?');
}
