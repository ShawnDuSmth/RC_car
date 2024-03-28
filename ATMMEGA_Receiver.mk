SHELL=cmd
OBJS=ATMMEGA_Receiver.o usart.o Software_UART.o
PORTN=$(shell type COMPORT.inc)

ATMMEGA_Receiver.elf: $(OBJS)
	avr-gcc $(CPU) $(OBJS) -Wl,-u,vfprintf -lprintf_flt -lm -o ATMMEGA_Receiver.elf && \
    avr-gcc -mmcu=atmega328 -Wl,-Map,ATMMEGA_Receiver.map $(OBJS) -o ATMMEGA_Receiver.elf
	avr-objcopy -j .text -j .data -O ihex ATMMEGA_Receiver.elf ATMMEGA_Receiver.hex
	@echo Hell Yeah!
	
ATMMEGA_Receiver.o: ATMMEGA_Receiver.c usart.h Software_UART.h
	avr-gcc -Wl,-u,vfprintf -lprintf_flt -lm -g -Os -mmcu=atmega328 -c ATMMEGA_Receiver.c

usart.o: usart.c usart.h
	avr-gcc -Wl,-u,vfprintf -lprintf_flt -lm -g -Os -Wall -mmcu=atmega328p -c usart.c

Software_UART.o: Software_UART.c Software_UART.h
	avr-gcc -Wl,-u,vfprintf -lprintf_flt -lm -g -Os -Wall -mmcu=atmega328p -c Software_UART.c

clean:
	@del *.hex *.elf *.o 2>nul

FlashLoad:
	@taskkill /f /im putty.exe /t /fi "status eq running" > NUL
	spi_atmega -CRYSTAL -p -v ATMMEGA_Receiver.hex
	@cmd /c c:\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N

putty:
	@taskkill /f /im putty.exe /t /fi "status eq running" > NUL
	@cmd /c c:\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N

dummy: ATMMEGA_Receiver.hex ATMMEGA_Receiver.map
	@echo Hello from dummy!
	