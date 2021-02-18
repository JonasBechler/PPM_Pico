add_executable(ppm
	ppm.c)

pico_generate_pio_header(ppm ${CMAKE_CURRENT_LIST_DIR}/ppm.pio)


# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(ppm PRIVATE
	pico_stdlib
	pico_multicore
	hardware_pio
	hardware_irq
	hardware_pwm
)

# enable usb output, disable uart output
pico_enable_stdio_usb(ppm 1)
pico_enable_stdio_uart(ppm 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(ppm)

# add url via pico_set_program_url
example_auto_set_url(ppm)
