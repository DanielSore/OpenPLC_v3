//-----------------------------------------------------------------------------
// Copyright 2015 Thiago Alves
//
// Based on the LDmicro software by Jonathan Westhues
// This file is part of the OpenPLC Software Stack.
//
// OpenPLC is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenPLC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenPLC.  If not, see <http://www.gnu.org/licenses/>.
//------
//
// This file is the hardware layer for the OpenPLC. If you change the platform
// where it is running, you may only need to change this file. All the I/O
// related stuff is here. Basically it provides functions to read and write
// to the OpenPLC internal buffers in order to update I/O state.
// Thiago Alves, Dec 2015
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "custom_layer.h"
#include "libsoc_gpio.h"
#include "libsoc_board.h"
#include "ladder.h"

#define MAX_INPUT 		3
#define MAX_OUTPUT 		3

//inBufferPinMask: pin mask for each input, which
//means what pin is mapped to that OpenPLC input
const char  *inBufferPinMask[MAX_INPUT] = { "15", "35", "52" };

//outBufferPinMask: pin mask for each output, which
//means what pin is mapped to that OpenPLC output
const char *outBufferPinMask[MAX_OUTPUT] =	{ "53", "50", "166" };

gpio *input_gpio = NULL;
gpio *output_gpio = NULL;
board_config *config;

//-----------------------------------------------------------------------------
// This function is called by the main OpenPLC routine when it is initializing.
// Hardware initialization procedures should be here.
//-----------------------------------------------------------------------------
void initializeHardware()
{
	uint32_t ret = 0;

    int fd;
    int i;
    char input_path[33];
    char output_path[33];

    for (i = 0; i < MAX_INPUT; i++)
    {
        // concatenates the path
        strcpy(input_path, "/sys/class/gpio/gpio");
        strcat(input_path, inBufferPinMask[i]);
        strcat(input_path, "/direction");

        // export GPIO
        fd = open("/sys/class/gpio/export", O_WRONLY);
        write(fd, inBufferPinMask[i], 3);
        close(fd);
    
        // configure as input
        fd = open(input_path, O_WRONLY);
        write(fd, "in", 2);
        close(fd);

        // clear the path
        memset(input_path, 0, sizeof input_path);
    }

    for (i = 0; i < MAX_OUTPUT; i++)
    {
        // concatenates the path
        strcpy(output_path, "/sys/class/gpio/gpio");
        strcat(output_path, outBufferPinMask[i]);
        strcat(output_path, "/direction");

        // export GPIO
        fd = open("/sys/class/gpio/export", O_WRONLY);
        write(fd, outBufferPinMask[i], 3);
        close(fd);
    
        // configure as output
        fd = open(output_path, O_WRONLY);
        write(fd, "out", 3);
        close(fd);

        // clear the path
        memset(output_path, 0, sizeof output_path);
    }

}

//-----------------------------------------------------------------------------
// This function is called by the OpenPLC in a loop. Here the internal buffers
// must be updated to reflect the actual state of the input pins. The mutex buffer_lock
// must be used to protect access to the buffers on a threaded environment.
//-----------------------------------------------------------------------------
void updateBuffersIn()
{
	pthread_mutex_lock(&bufferLock); //lock mutex

    int fd;
    char value;
    char pin_path[28];

    for (int i = 0; i < MAX_OUTPUT; i++)
    {
        // concatenates the path
        strcpy(pin_path, "/sys/class/gpio/gpio");
        strcat(pin_path, inBufferPinMask[i]);
        strcat(pin_path, "/value"); 

        fd = open(pin_path, O_RDONLY);
        read(fd, &value, 1); // read GPIO value
        if (bool_input[i/8][i%8] != NULL) *bool_input[i/8][i%8] = value-48;
        close(fd); //close value file

        // clear the path
        memset(pin_path, 0, sizeof pin_path);
    }

    pthread_mutex_unlock(&bufferLock); //unlock mutex
}

//-----------------------------------------------------------------------------
// This function is called by the OpenPLC in a loop. Here the internal buffers
// must be updated to reflect the actual state of the output pins. The mutex buffer_lock
// must be used to protect access to the buffers on a threaded environment.
//-----------------------------------------------------------------------------
void updateBuffersOut()
{
	pthread_mutex_lock(&bufferLock); //lock mutex

    int fd;
    int asciiValue;
    char pin_path[28];

    for (int i = 0; i < MAX_OUTPUT; i++)
    {
        // concatenates the path
        strcpy(pin_path, "/sys/class/gpio/gpio");
        strcat(pin_path, outBufferPinMask[i]);
        strcat(pin_path, "/value");

        fd = open(pin_path, O_WRONLY | O_SYNC);
        if (bool_output[i/8][i%8] != NULL) {
            asciiValue = *bool_output[i/8][i%8]+48;
            write(fd, &asciiValue, 1);
        }
        close(fd);

        // clear the path
        memset(pin_path, 0, sizeof pin_path);
    }

    pthread_mutex_unlock(&bufferLock); //unlock mutex

}