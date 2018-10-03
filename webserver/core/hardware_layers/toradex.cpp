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
#include <unistd.h>
#include <pthread.h>

#include "custom_layer.h"
#include "libsoc_gpio.h"
#include "libsoc_board.h"
#include "ladder.h"

//inBufferPinMask: pin mask for each input, which
//means what pin is mapped to that OpenPLC input
int inBufferPinMask[MAX_INPUT] = {};

//outBufferPinMask: pin mask for each output, which
//means what pin is mapped to that OpenPLC output
int outBufferPinMask[MAX_OUTPUT] =	{};

//analogOutBufferPinMask: pin mask for the analog PWM
//output of the RaspberryPi
int analogOutBufferPinMask[MAX_ANALOG_OUT] = {};

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

	config = libsoc_board_init();

	printf("Iniciou \n");
	input_gpio = libsoc_gpio_request(libsoc_board_gpio_id(config, "SODIMM_103"), LS_GPIO_SHARED);
	if (input_gpio == NULL) {
		perror("input request failed");
		goto exit;
	}
	printf("Exportou \n");
	output_gpio = libsoc_gpio_request(libsoc_board_gpio_id(config, "SODIMM_101"), LS_GPIO_SHARED);
	if (output_gpio == NULL) {
		perror("output gpio request failed");
		goto exit;
	}
	ret = libsoc_gpio_set_direction(input_gpio, INPUT);
	if (ret == EXIT_FAILURE) {
		perror("Failed to set input gpio direction");
		goto exit;
	}
	ret = libsoc_gpio_set_direction(output_gpio, OUTPUT);
	if (ret == EXIT_FAILURE) {
		perror("Failed to set output gpio direction");
		goto exit;
	}
exit:
	if (input_gpio) {
		ret = libsoc_gpio_free(input_gpio);
		if (ret == EXIT_FAILURE)
			perror("Could not free gpio");
	}

	if (output_gpio) {
		ret = libsoc_gpio_free(output_gpio);
		if (ret == EXIT_FAILURE)
			perror("Could not free led gpio");
	}
	return ret;
}

//-----------------------------------------------------------------------------
// This function is called by the OpenPLC in a loop. Here the internal buffers
// must be updated to reflect the actual state of the input pins. The mutex buffer_lock
// must be used to protect access to the buffers on a threaded environment.
//-----------------------------------------------------------------------------
void updateBuffersIn()
{
	pthread_mutex_lock(&bufferLock); //lock mutex

	if (bool_input[0][0] != NULL) *bool_input[0][0] = libsoc_gpio_get_level(input_gpio);

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
	printf(bool_output[0][0]);
	if (bool_output[0][0] != NULL) libsoc_gpio_set_level(output_gpio, *bool_output[0][0]);

	pthread_mutex_unlock(&bufferLock); //unlock mutex
}