SRCS += main.c
SRCS += shell.c
SRCS += ipctest.c
SRCS += top.c
SRCS += uname.c
SRCS += rd_test.c
SRCS += getchar.c
SRCS += shared_deq_test.c
SRCS += list_test.c
SRCS += system_ctl.c
SRCS += device_lookup.c
SRCS += uart.c
SRCS += char_dev.c

SRCS_$(CONFIG_ACCELEROMETERS) += accel.c
SRCS_$(CONFIG_BAROMETERS) += baro.c
SRCS_$(CONFIG_GYROSCOPES) += gyro.c
SRCS_$(CONFIG_MAGNETOMETERS) += mag.c
SRCS_$(CONFIG_ROTARY_ENCODERS) += rotary_encoder.c
SRCS_$(CONFIG_HAVE_LED) 	+= blink.c
SRCS_$(CONFIG_MM_PROFILING) += mem_perf.c
SRCS_$(CONFIG_PWM_CLASS) += pwm.c
SRCS_$(CONFIG_ADC_CLASS) += adc.c

# Date and rev for uname
DATE := "$(shell date -u)"
SHA := $(shell git log HEAD -n 1 --format='%h')
DIRTY := $(strip $(shell git status --porcelain | wc -c))
CFLAGS += -D BUILD_TIME='$(DATE)' -D BUILD_SHA='"$(SHA)"' -D BUILD_DIRTY=$(DIRTY)

include $(BASE)/tools/submake.mk
