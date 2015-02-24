/***********************************************************************************
Copyright (c) Nordic Semiconductor ASA
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

  3. Neither the name of Nordic Semiconductor ASA nor the names of other
  contributors to this software may be used to endorse or promote products
  derived from this software without specific prior written permission.

  4. This software must only be used in a processor manufactured by Nordic
  Semiconductor ASA, or in a processor manufactured by a third party that
  is used in combination with a processor manufactured by Nordic Semiconductor.


THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************************************************/

#include <SPI.h>
#include "lib_aci.h"

#include "mesh_interface.h"


static aci_pins_t pins;

static uint8_t         uart_buffer[20];
static uint8_t         uart_buffer_len = 0;
static uint8_t         dummychar = 0;

void print(char* str){
	Serial.println(str);

}

void __ble_assert(const char *file, uint16_t line)
{
  Serial.print("ERROR ");
  Serial.print(file);
  Serial.print(": ");
  Serial.print(line);
  Serial.print("\n");
  while(1);
}


void setup(void)
{
  Serial.begin(115200);
  //Wait until the serial port is available (useful only for the Leonardo)
  //As the Leonardo board is not reseted every time you open the Serial Monitor
  #if defined (__AVR_ATmega32U4__)
    while(!Serial)
    {}
    delay(5000);  //5 seconds delay for enabling to see the start up comments on the serial board
  #elif defined(__PIC32MX__)
    delay(1000);
  #endif
  
  pins.board_name = BOARD_DEFAULT; //See board.h for details REDBEARLAB_SHIELD_V1_1 or BOARD_DEFAULT
  pins.reqn_pin   = SS; //SS for Nordic board, 9 for REDBEARLAB_SHIELD_V1_1
  pins.rdyn_pin   = 8; //3 for Nordic board, 8 for REDBEARLAB_SHIELD_V1_1
  pins.mosi_pin   = MOSI;
  pins.miso_pin   = MISO;
  pins.sck_pin    = SCK;

  pins.spi_clock_divider      = SPI_CLOCK_DIV64; //SPI_CLOCK_DIV8  = 2MHz SPI speed
                                                 //SPI_CLOCK_DIV16 = 1MHz SPI speed
  
  pins.reset_pin              = 4; //4 for Nordic board, UNUSED for REDBEARLAB_SHIELD_V1_1
  pins.active_pin             = UNUSED;
  pins.optional_chip_sel_pin  = UNUSED;

  pins.interface_is_interrupt = false; //Interrupts still not available in Chipkit
  pins.interrupt_number       = 1;

  mesh_interface_hw_init(&pins);
  
  Serial.println(SS);
  Serial.println(MOSI);
  Serial.println(MISO);
  Serial.println(SCK);
  Serial.println(UNUSED);

  Serial.println("Arduino setup done");
  return;

}

bool stringComplete = false;  // whether the string is complete
uint8_t stringIndex = 0;      //Initialize the index to store incoming chars

int num = -1;

uint8_t accAddr[4] = {0x8f, 0xa6, 0x41,0xa5 };

void loop() {

  //Process any ACI commands or events
  mesh_interface_loop();

  if (stringComplete) 
  {
    uart_buffer_len = stringIndex + 1; 
    num++;
    
    if(num == 0){
       if (!mesh_interface_send_echo(uart_buffer, uart_buffer_len)){
        Serial.println(F("Serial input dropped"));
      }
      else{
       Serial.println(F("echo done"));
     }
    }
    if(num == 1){
      if(!mesh_interface_send_init(accAddr, (uint8_t) 38, (uint8_t) 2)){
       Serial.println(F("init dropped"));
     }
     else{
       Serial.println(F("init done"));
     }
    }
    if(num == 2){
      if(!mesh_interface_send_value_enable((uint8_t) 1)){
       Serial.println(F("enalbe 1 dropped"));
     }
     else{
       Serial.println(F("enable 1 done"));
     }
    }
    if(num == 3){
      if(!mesh_interface_send_value_enable((uint8_t) 2)){
       Serial.println(F("enalbe 2 dropped"));
     }
     else{
       Serial.println(F("enable 2 done"));
     }
    }
    /*
    if(num == 4){
      if(!mesh_interface_send_value_get((uint8_t) 2)){
       Serial.println(F("get 2 dropped"));
     }
     else{
       Serial.println(F("get 2 done"));
     }
	mesh_interface_wait_for_answer(0x00, 0x00);
    }//*/
    if(num > 3){
      uint8_t on = (num/2) % 2;
      uint8_t handle = (num % 2) + 1;
      if(handle == 1) {
		mesh_interface_send_value_set((uint8_t)2, &on, (uint8_t) 1);
       		Serial.println(F("set done"));
	}
      if(handle == 2){
		mesh_interface_send_value_get((uint8_t) 2);
       		Serial.println(F("get done"));
		uint8_t value = 7;
    		mesh_interface_wait_for_answer(&value, sizeof(uint8_t));
       		Serial.print("answer = ");
       		Serial.println(value);
	}
      //if(!mesh_interface_send_value_set(handle, &on, (uint8_t) 1)){
       //Serial.println(F("set dropped"));
     //}
     //else{
       //Serial.println(F("set done"));
     //}
    }

    // clear the uart_buffer:
    for (stringIndex = 0; stringIndex < 20; stringIndex++)
    {
      uart_buffer[stringIndex] = ' ';
    }

    // reset the flag and the index in order to receive more data
    stringIndex    = 0;
    stringComplete = false;
  }
  //For ChipKit you have to call the function that reads from Serial
  #if defined (__PIC32MX__)
    if (Serial.available())
    {
      serialEvent();
    }
  #endif
}

void serialEvent() {

  while(Serial.available() > 0){
    // get the new byte:
    dummychar = (uint8_t)Serial.read();
    if(!stringComplete)
    {
      if (dummychar == '\n')
      {
        // if the incoming character is a newline, set a flag
        // so the main loop can do something about it
        stringIndex--;
        stringComplete = true;
      }
      else
      {
        if(stringIndex > 19)
        {
          Serial.println("Serial input truncated");
          stringIndex--;
          stringComplete = true;
        }
        else
        {
          // add it to the uart_buffer
          uart_buffer[stringIndex] = dummychar;
          stringIndex++;
        }
      }
    }
  }
}