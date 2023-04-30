conf_data = configuration_data()
conf_data.set('TAG', 'spi_nand')
conf_data.set('DEBUG_OPT', 'CONFIG_DEBUG_SPI_NAND')
conf_data.set('PREFIX', 'spi_nand')
conf_data.set('FUNC', 'ui_printf')

configure_file(input : log_macros_tmpl,
               output : 'spi_nand_log.h',
               configuration : conf_data)

src = [
       'main.c',
       'timer.c',
       'spi_flash.c',
       'spi_controller.c',
       'spi_nand_flash.c',
       'spi_nor_flash.c',
       'spi_nor_ids.c',
       'ui.c',
       'file.c'
       ]

argtable2 = dependency('argtable2')

deps = [ argtable2, libusrio_dep, libdpgc_dep ]

need_i2c = false

if get_option('ch341a')
	deps += [ libch341a_dep ]
	add_project_arguments('-DCONFIG_CH341A', language : 'c')
endif

if get_option('mstar_ddc')
	src += ['mstarddc_spi.c']
	need_i2c = true
	add_project_arguments('-DCONFIG_MSTAR_DDC', language : 'c')
	if get_option('debug_mstar_ddc')
	add_project_arguments('-DCONFIG_DEBUG_MSTAR_DDC', language : 'c')
endif
endif

if get_option('eeprom')
	src += ['i2c_eeprom.c']
	need_i2c = true
	add_project_arguments('-DCONFIG_EEPROM', language : 'c')
endif

if need_i2c	
	src += ['i2c_controller.c']
	add_project_arguments('-DCONFIG_NEED_I2C', language : 'c')
endif

executable('snander_electricboogaloo', src, dependencies: deps, install : true)
