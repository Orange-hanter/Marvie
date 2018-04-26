#Generated by VisualGDB project wizard. 
#Note: VisualGDB will automatically update this file when you add new sources to the project.
#All other changes you make in this file will be preserved.
#Visit http://visualgdb.com/makefiles for more details

#VisualGDB: AutoSourceFiles		#<--- remove this line to disable auto-updating of SOURCEFILES and EXTERNAL_LIBS

TARGETNAME := ChibiOSF4.elf
#TARGETTYPE can be APP, STATIC or SHARED
TARGETTYPE := APP

to_lowercase = $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))

CONFIG ?= DEBUG

CONFIGURATION_FLAGS_FILE := $(call to_lowercase,$(CONFIG)).mak

include $(CONFIGURATION_FLAGS_FILE)

#LINKER_SCRIPT defined inside the configuration file (e.g. debug.mak) should override any linker scripts defined in shared .mak files
CONFIGURATION_LINKER_SCRIPT := $(LINKER_SCRIPT)

include $(ADDITIONAL_MAKE_FILES)

ifneq ($(CONFIGURATION_LINKER_SCRIPT),)
LINKER_SCRIPT := $(CONFIGURATION_LINKER_SCRIPT)
endif

ifneq ($(LINKER_SCRIPT),)
LDFLAGS += -T$(LINKER_SCRIPT)
endif

ifeq ($(BINARYDIR),)
error:
	$(error Invalid configuration, please check your inputs)
endif

SOURCEFILES := board.c Core/ByteRingBuffer.cpp Core/DataTimeService.cpp Core/NewDelete.cpp Drivers/Interfaces/Usart.cpp Drivers/LogicOutput.cpp Drivers/Network/SimGsm/SimGsm.cpp Drivers/Network/SimGsm/SimGsmATResponseParsers.cpp Drivers/Network/SimGsm/SimGsmSocketBase.cpp Drivers/Network/SimGsm/SimGsmTcpServer.cpp Drivers/Network/SimGsm/SimGsmTcpSocket.cpp Drivers/Network/SimGsm/SimGsmUdpSocket.cpp Drivers/Sensors/AbstractSensor.cpp Drivers/Sensors/CE301.cpp ext/fatfs/src/ff.c ext/fatfs/src/ffunicode.c Network/AbstractTcpSocket.cpp Network/AbstractUdpSocket.cpp Network/IpAddress.cpp os/common/oslib/src/chfactory.c os/common/oslib/src/chheap.c os/common/oslib/src/chmboxes.c os/common/oslib/src/chmemcore.c os/common/oslib/src/chmempools.c os/common/ports/ARMCMx/chcore.c os/common/ports/ARMCMx/chcore_v7m.c os/common/ports/ARMCMx/compilers/GCC/chcoreasm_v7m.S os/common/startup/ARMCMx/compilers/GCC/crt0_v7m.S os/common/startup/ARMCMx/compilers/GCC/crt1.c os/common/startup/ARMCMx/compilers/GCC/vectors.S os/hal/lib/streams/chprintf.c os/hal/lib/streams/memstreams.c os/hal/lib/streams/nullstreams.c os/hal/osal/rt/osal.c os/hal/ports/common/ARMCMx/nvic.c os/hal/ports/STM32/LLD/ADCv2/hal_adc_lld.c os/hal/ports/STM32/LLD/CANv1/hal_can_lld.c os/hal/ports/STM32/LLD/DACv1/hal_dac_lld.c os/hal/ports/STM32/LLD/DMAv2/stm32_dma.c os/hal/ports/STM32/LLD/EXTIv1/hal_ext_lld.c os/hal/ports/STM32/LLD/GPIOv2/hal_pal_lld.c os/hal/ports/STM32/LLD/I2Cv1/hal_i2c_lld.c os/hal/ports/STM32/LLD/MACv1/hal_mac_lld.c os/hal/ports/STM32/LLD/OTGv1/hal_usb_lld.c os/hal/ports/STM32/LLD/OTGv1/hal_usb_lld_alt.c os/hal/ports/STM32/LLD/QUADSPIv1/hal_qspi_lld.c os/hal/ports/STM32/LLD/RTCv2/hal_rtc_lld.c os/hal/ports/STM32/LLD/SDIOv1/hal_sdc_lld.c os/hal/ports/STM32/LLD/SPIv1/hal_i2s_lld.c os/hal/ports/STM32/LLD/SPIv1/hal_spi_lld.c os/hal/ports/STM32/LLD/TIMv1/hal_gpt_lld.c os/hal/ports/STM32/LLD/TIMv1/hal_icu_lld.c os/hal/ports/STM32/LLD/TIMv1/hal_pwm_lld.c os/hal/ports/STM32/LLD/TIMv1/hal_st_lld.c os/hal/ports/STM32/LLD/USARTv1/hal_serial_lld.c os/hal/ports/STM32/LLD/USARTv1/hal_uart_lld.c os/hal/ports/STM32/LLD/xWDGv1/hal_wdg_lld.c os/hal/ports/STM32/STM32F4xx/hal_lld.c os/hal/ports/STM32/STM32F4xx/stm32_isr.c os/hal/src/hal.c os/hal/src/hal_adc.c os/hal/src/hal_buffers.c os/hal/src/hal_can.c os/hal/src/hal_crypto.c os/hal/src/hal_dac.c os/hal/src/hal_ext.c os/hal/src/hal_gpt.c os/hal/src/hal_i2c.c os/hal/src/hal_i2s.c os/hal/src/hal_icu.c os/hal/src/hal_mac.c os/hal/src/hal_mmcsd.c os/hal/src/hal_mmc_spi.c os/hal/src/hal_pal.c os/hal/src/hal_pwm.c os/hal/src/hal_qspi.c os/hal/src/hal_queues.c os/hal/src/hal_rtc.c os/hal/src/hal_sdc.c os/hal/src/hal_serial.c os/hal/src/hal_serial_usb.c os/hal/src/hal_spi.c os/hal/src/hal_st.c os/hal/src/hal_uart.c os/hal/src/hal_usb.c os/hal/src/hal_wdg.c os/rt/src/chcond.c os/rt/src/chdebug.c os/rt/src/chdynamic.c os/rt/src/chevents.c os/rt/src/chmsg.c os/rt/src/chmtx.c os/rt/src/chregistry.c os/rt/src/chschd.c os/rt/src/chsem.c os/rt/src/chstats.c os/rt/src/chsys.c os/rt/src/chthreads.c os/rt/src/chtm.c os/rt/src/chtrace.c os/rt/src/chvt.c os/test/lib/ch_test.c os/test/oslib/source/test/oslib_test_root.c os/test/oslib/source/test/oslib_test_sequence_001.c os/test/oslib/source/test/oslib_test_sequence_002.c os/test/oslib/source/test/oslib_test_sequence_003.c os/test/oslib/source/test/oslib_test_sequence_004.c os/test/rt/source/test/rt_test_root.c os/test/rt/source/test/rt_test_sequence_001.c os/test/rt/source/test/rt_test_sequence_002.c os/test/rt/source/test/rt_test_sequence_003.c os/test/rt/source/test/rt_test_sequence_004.c os/test/rt/source/test/rt_test_sequence_005.c os/test/rt/source/test/rt_test_sequence_006.c os/test/rt/source/test/rt_test_sequence_007.c os/test/rt/source/test/rt_test_sequence_008.c os/test/rt/source/test/rt_test_sequence_009.c os/test/rt/source/test/rt_test_sequence_010.c os/various/cpp_wrappers/ch.cpp os/various/evtimer.c os/various/fatfs_bindings/fatfs_diskio.c os/various/fatfs_bindings/fatfs_syscall.c SingleBRSensorReader.cpp BRSensorReader.cpp MultipleBRSensorsReader.cpp Support/LexicalAnalyzer.cpp Support/Utility.cpp Tests/MultipleBRSensorsReaderTest.cpp
EXTERNAL_LIBS := 
EXTERNAL_LIBS_COPIED := $(foreach lib, $(EXTERNAL_LIBS),$(BINARYDIR)/$(notdir $(lib)))

CFLAGS += $(COMMONFLAGS)
CXXFLAGS += $(COMMONFLAGS)
ASFLAGS += $(COMMONFLAGS)
LDFLAGS += $(COMMONFLAGS)

CFLAGS += $(addprefix -I,$(INCLUDE_DIRS))
CXXFLAGS += $(addprefix -I,$(INCLUDE_DIRS))

CFLAGS += $(addprefix -D,$(PREPROCESSOR_MACROS))
CXXFLAGS += $(addprefix -D,$(PREPROCESSOR_MACROS))
ASFLAGS += $(addprefix -D,$(PREPROCESSOR_MACROS))

CXXFLAGS += $(addprefix -framework ,$(MACOS_FRAMEWORKS))
CFLAGS += $(addprefix -framework ,$(MACOS_FRAMEWORKS))
LDFLAGS += $(addprefix -framework ,$(MACOS_FRAMEWORKS))

LDFLAGS += $(addprefix -L,$(LIBRARY_DIRS))

ifeq ($(GENERATE_MAP_FILE),1)
LDFLAGS += -Wl,-Map=$(BINARYDIR)/$(basename $(TARGETNAME)).map
endif

LIBRARY_LDFLAGS = $(addprefix -l,$(LIBRARY_NAMES))

ifeq ($(IS_LINUX_PROJECT),1)
	RPATH_PREFIX := -Wl,--rpath='$$ORIGIN/../
	LIBRARY_LDFLAGS += $(EXTERNAL_LIBS)
	LIBRARY_LDFLAGS += -Wl,--rpath='$$ORIGIN'
	LIBRARY_LDFLAGS += $(addsuffix ',$(addprefix $(RPATH_PREFIX),$(dir $(EXTERNAL_LIBS))))
	
	ifeq ($(TARGETTYPE),SHARED)
		CFLAGS += -fPIC
		CXXFLAGS += -fPIC
		ASFLAGS += -fPIC
		LIBRARY_LDFLAGS += -Wl,-soname,$(TARGETNAME)
	endif
	
	ifneq ($(LINUX_PACKAGES),)
		PACKAGE_CFLAGS := $(foreach pkg,$(LINUX_PACKAGES),$(shell pkg-config --cflags $(pkg)))
		PACKAGE_LDFLAGS := $(foreach pkg,$(LINUX_PACKAGES),$(shell pkg-config --libs $(pkg)))
		CFLAGS += $(PACKAGE_CFLAGS)
		CXXFLAGS += $(PACKAGE_CFLAGS)
		LIBRARY_LDFLAGS += $(PACKAGE_LDFLAGS)
	endif	
else
	LIBRARY_LDFLAGS += $(EXTERNAL_LIBS)
endif

LIBRARY_LDFLAGS += $(ADDITIONAL_LINKER_INPUTS)

all_make_files := $(firstword $(MAKEFILE_LIST)) $(CONFIGURATION_FLAGS_FILE) $(ADDITIONAL_MAKE_FILES)

ifeq ($(STARTUPFILES),)
	all_source_files := $(SOURCEFILES)
else
	all_source_files := $(STARTUPFILES) $(filter-out $(STARTUPFILES),$(SOURCEFILES))
endif

source_obj1 := $(all_source_files:.cpp=.o)
source_obj2 := $(source_obj1:.c=.o)
source_obj3 := $(source_obj2:.s=.o)
source_obj4 := $(source_obj3:.S=.o)
source_obj5 := $(source_obj4:.cc=.o)
source_objs := $(source_obj5:.cxx=.o)

all_objs := $(addprefix $(BINARYDIR)/, $(notdir $(source_objs)))

PRIMARY_OUTPUTS :=

ifeq ($(GENERATE_BIN_FILE),1)
PRIMARY_OUTPUTS += $(BINARYDIR)/$(basename $(TARGETNAME)).bin
endif

ifeq ($(GENERATE_IHEX_FILE),1)
PRIMARY_OUTPUTS += $(BINARYDIR)/$(basename $(TARGETNAME)).ihex
endif

ifeq ($(PRIMARY_OUTPUTS),)
PRIMARY_OUTPUTS := $(BINARYDIR)/$(TARGETNAME)
endif

all: $(PRIMARY_OUTPUTS)

$(BINARYDIR)/$(basename $(TARGETNAME)).bin: $(BINARYDIR)/$(TARGETNAME)
	$(OBJCOPY) -O binary $< $@

$(BINARYDIR)/$(basename $(TARGETNAME)).ihex: $(BINARYDIR)/$(TARGETNAME)
	$(OBJCOPY) -O ihex $< $@
	
ifneq ($(LINKER_SCRIPT),)
$(BINARYDIR)/$(TARGETNAME): $(LINKER_SCRIPT)
endif

ifeq ($(TARGETTYPE),APP)
$(BINARYDIR)/$(TARGETNAME): $(all_objs) $(EXTERNAL_LIBS)
	$(LD) -o $@ $(LDFLAGS) $(START_GROUP) $(all_objs) $(LIBRARY_LDFLAGS) $(END_GROUP)
endif

ifeq ($(TARGETTYPE),SHARED)
$(BINARYDIR)/$(TARGETNAME): $(all_objs) $(EXTERNAL_LIBS)
	$(LD) -shared -o $@ $(LDFLAGS) $(START_GROUP) $(all_objs) $(LIBRARY_LDFLAGS) $(END_GROUP)
endif
	
ifeq ($(TARGETTYPE),STATIC)
$(BINARYDIR)/$(TARGETNAME): $(all_objs)
	$(AR) -r $@ $^
endif

-include $(all_objs:.o=.dep)

clean:
ifeq ($(USE_DEL_TO_CLEAN),1)
	cmd /C del /S /Q $(BINARYDIR)
else
	rm -rf $(BINARYDIR)
endif

$(BINARYDIR):
	mkdir $(BINARYDIR)

#VisualGDB: FileSpecificTemplates		#<--- VisualGDB will use the following lines to define rules for source files in subdirectories
$(BINARYDIR)/%.o : %.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/%.o : %.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/%.o : %.S $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) $(ASFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/%.o : %.s $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) $(ASFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/%.o : %.cc $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/%.o : %.cxx $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

#VisualGDB: GeneratedRules				#<--- All lines below are auto-generated. Remove this line to suppress auto-generation of file rules.


$(BINARYDIR)/ByteRingBuffer.o : Core/ByteRingBuffer.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/DataTimeService.o : Core/DataTimeService.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/NewDelete.o : Core/NewDelete.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/Usart.o : Drivers/Interfaces/Usart.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/LogicOutput.o : Drivers/LogicOutput.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/SimGsm.o : Drivers/Network/SimGsm/SimGsm.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/SimGsmATResponseParsers.o : Drivers/Network/SimGsm/SimGsmATResponseParsers.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/SimGsmSocketBase.o : Drivers/Network/SimGsm/SimGsmSocketBase.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/SimGsmTcpServer.o : Drivers/Network/SimGsm/SimGsmTcpServer.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/SimGsmTcpSocket.o : Drivers/Network/SimGsm/SimGsmTcpSocket.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/SimGsmUdpSocket.o : Drivers/Network/SimGsm/SimGsmUdpSocket.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/AbstractSensor.o : Drivers/Sensors/AbstractSensor.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/CE301.o : Drivers/Sensors/CE301.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/ff.o : ext/fatfs/src/ff.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/ffunicode.o : ext/fatfs/src/ffunicode.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/AbstractTcpSocket.o : Network/AbstractTcpSocket.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/AbstractUdpSocket.o : Network/AbstractUdpSocket.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/IpAddress.o : Network/IpAddress.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chfactory.o : os/common/oslib/src/chfactory.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chheap.o : os/common/oslib/src/chheap.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chmboxes.o : os/common/oslib/src/chmboxes.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chmemcore.o : os/common/oslib/src/chmemcore.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chmempools.o : os/common/oslib/src/chmempools.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chcore.o : os/common/ports/ARMCMx/chcore.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chcore_v7m.o : os/common/ports/ARMCMx/chcore_v7m.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chcoreasm_v7m.o : os/common/ports/ARMCMx/compilers/GCC/chcoreasm_v7m.S $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) $(ASFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/crt0_v7m.o : os/common/startup/ARMCMx/compilers/GCC/crt0_v7m.S $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) $(ASFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/crt1.o : os/common/startup/ARMCMx/compilers/GCC/crt1.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/vectors.o : os/common/startup/ARMCMx/compilers/GCC/vectors.S $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) $(ASFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chprintf.o : os/hal/lib/streams/chprintf.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/memstreams.o : os/hal/lib/streams/memstreams.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/nullstreams.o : os/hal/lib/streams/nullstreams.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/osal.o : os/hal/osal/rt/osal.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/nvic.o : os/hal/ports/common/ARMCMx/nvic.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_adc_lld.o : os/hal/ports/STM32/LLD/ADCv2/hal_adc_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_can_lld.o : os/hal/ports/STM32/LLD/CANv1/hal_can_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_dac_lld.o : os/hal/ports/STM32/LLD/DACv1/hal_dac_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/stm32_dma.o : os/hal/ports/STM32/LLD/DMAv2/stm32_dma.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_ext_lld.o : os/hal/ports/STM32/LLD/EXTIv1/hal_ext_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_pal_lld.o : os/hal/ports/STM32/LLD/GPIOv2/hal_pal_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_i2c_lld.o : os/hal/ports/STM32/LLD/I2Cv1/hal_i2c_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_mac_lld.o : os/hal/ports/STM32/LLD/MACv1/hal_mac_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_usb_lld.o : os/hal/ports/STM32/LLD/OTGv1/hal_usb_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_usb_lld_alt.o : os/hal/ports/STM32/LLD/OTGv1/hal_usb_lld_alt.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_qspi_lld.o : os/hal/ports/STM32/LLD/QUADSPIv1/hal_qspi_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_rtc_lld.o : os/hal/ports/STM32/LLD/RTCv2/hal_rtc_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_sdc_lld.o : os/hal/ports/STM32/LLD/SDIOv1/hal_sdc_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_i2s_lld.o : os/hal/ports/STM32/LLD/SPIv1/hal_i2s_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_spi_lld.o : os/hal/ports/STM32/LLD/SPIv1/hal_spi_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_gpt_lld.o : os/hal/ports/STM32/LLD/TIMv1/hal_gpt_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_icu_lld.o : os/hal/ports/STM32/LLD/TIMv1/hal_icu_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_pwm_lld.o : os/hal/ports/STM32/LLD/TIMv1/hal_pwm_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_st_lld.o : os/hal/ports/STM32/LLD/TIMv1/hal_st_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_serial_lld.o : os/hal/ports/STM32/LLD/USARTv1/hal_serial_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_uart_lld.o : os/hal/ports/STM32/LLD/USARTv1/hal_uart_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_wdg_lld.o : os/hal/ports/STM32/LLD/xWDGv1/hal_wdg_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_lld.o : os/hal/ports/STM32/STM32F4xx/hal_lld.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/stm32_isr.o : os/hal/ports/STM32/STM32F4xx/stm32_isr.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal.o : os/hal/src/hal.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_adc.o : os/hal/src/hal_adc.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_buffers.o : os/hal/src/hal_buffers.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_can.o : os/hal/src/hal_can.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_crypto.o : os/hal/src/hal_crypto.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_dac.o : os/hal/src/hal_dac.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_ext.o : os/hal/src/hal_ext.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_gpt.o : os/hal/src/hal_gpt.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_i2c.o : os/hal/src/hal_i2c.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_i2s.o : os/hal/src/hal_i2s.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_icu.o : os/hal/src/hal_icu.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_mac.o : os/hal/src/hal_mac.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_mmcsd.o : os/hal/src/hal_mmcsd.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_mmc_spi.o : os/hal/src/hal_mmc_spi.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_pal.o : os/hal/src/hal_pal.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_pwm.o : os/hal/src/hal_pwm.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_qspi.o : os/hal/src/hal_qspi.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_queues.o : os/hal/src/hal_queues.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_rtc.o : os/hal/src/hal_rtc.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_sdc.o : os/hal/src/hal_sdc.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_serial.o : os/hal/src/hal_serial.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_serial_usb.o : os/hal/src/hal_serial_usb.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_spi.o : os/hal/src/hal_spi.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_st.o : os/hal/src/hal_st.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_uart.o : os/hal/src/hal_uart.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_usb.o : os/hal/src/hal_usb.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/hal_wdg.o : os/hal/src/hal_wdg.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chcond.o : os/rt/src/chcond.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chdebug.o : os/rt/src/chdebug.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chdynamic.o : os/rt/src/chdynamic.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chevents.o : os/rt/src/chevents.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chmsg.o : os/rt/src/chmsg.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chmtx.o : os/rt/src/chmtx.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chregistry.o : os/rt/src/chregistry.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chschd.o : os/rt/src/chschd.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chsem.o : os/rt/src/chsem.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chstats.o : os/rt/src/chstats.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chsys.o : os/rt/src/chsys.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chthreads.o : os/rt/src/chthreads.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chtm.o : os/rt/src/chtm.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chtrace.o : os/rt/src/chtrace.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/chvt.o : os/rt/src/chvt.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/ch_test.o : os/test/lib/ch_test.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/oslib_test_root.o : os/test/oslib/source/test/oslib_test_root.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/oslib_test_sequence_001.o : os/test/oslib/source/test/oslib_test_sequence_001.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/oslib_test_sequence_002.o : os/test/oslib/source/test/oslib_test_sequence_002.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/oslib_test_sequence_003.o : os/test/oslib/source/test/oslib_test_sequence_003.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/oslib_test_sequence_004.o : os/test/oslib/source/test/oslib_test_sequence_004.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/rt_test_root.o : os/test/rt/source/test/rt_test_root.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/rt_test_sequence_001.o : os/test/rt/source/test/rt_test_sequence_001.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/rt_test_sequence_002.o : os/test/rt/source/test/rt_test_sequence_002.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/rt_test_sequence_003.o : os/test/rt/source/test/rt_test_sequence_003.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/rt_test_sequence_004.o : os/test/rt/source/test/rt_test_sequence_004.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/rt_test_sequence_005.o : os/test/rt/source/test/rt_test_sequence_005.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/rt_test_sequence_006.o : os/test/rt/source/test/rt_test_sequence_006.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/rt_test_sequence_007.o : os/test/rt/source/test/rt_test_sequence_007.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/rt_test_sequence_008.o : os/test/rt/source/test/rt_test_sequence_008.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/rt_test_sequence_009.o : os/test/rt/source/test/rt_test_sequence_009.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/rt_test_sequence_010.o : os/test/rt/source/test/rt_test_sequence_010.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/ch.o : os/various/cpp_wrappers/ch.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/evtimer.o : os/various/evtimer.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/fatfs_diskio.o : os/various/fatfs_bindings/fatfs_diskio.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/fatfs_syscall.o : os/various/fatfs_bindings/fatfs_syscall.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/LexicalAnalyzer.o : Support/LexicalAnalyzer.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/Utility.o : Support/Utility.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)


$(BINARYDIR)/MultipleBRSensorsReaderTest.o : Tests/MultipleBRSensorsReaderTest.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $(@:.o=.dep)

