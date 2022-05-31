################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src1/arhiv.c \
../Core/Src1/calculations.c \
../Core/Src1/debug.c \
../Core/Src1/eeprom.c \
../Core/Src1/flash.c \
../Core/Src1/lmp91000.c \
../Core/Src1/modbus_lpuart.c 

OBJS += \
./Core/Src1/arhiv.o \
./Core/Src1/calculations.o \
./Core/Src1/debug.o \
./Core/Src1/eeprom.o \
./Core/Src1/flash.o \
./Core/Src1/lmp91000.o \
./Core/Src1/modbus_lpuart.o 

C_DEPS += \
./Core/Src1/arhiv.d \
./Core/Src1/calculations.d \
./Core/Src1/debug.d \
./Core/Src1/eeprom.d \
./Core/Src1/flash.d \
./Core/Src1/lmp91000.d \
./Core/Src1/modbus_lpuart.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src1/%.o Core/Src1/%.su: ../Core/Src1/%.c Core/Src1/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DSTM32L031xx -DUSE_FULL_LL_DRIVER -DHSE_VALUE=8000000 -DHSE_STARTUP_TIMEOUT=100 -DLSE_STARTUP_TIMEOUT=5000 -DLSE_VALUE=32768 -DMSI_VALUE=2097000 -DHSI_VALUE=16000000 -DLSI_VALUE=37000 -DVDD_VALUE=3300 -DPREFETCH_ENABLE=0 -DINSTRUCTION_CACHE_ENABLE=1 -DDATA_CACHE_ENABLE=1 -c -I../Core/Inc -I"D:/ProjectSTM/STM32CubeIDE/workspace/Smart-Pwr/Core/Inc1" -I"D:/ProjectSTM/STM32CubeIDE/workspace/Smart-Pwr/Core/Inc2" -IC:/Users/Dmitriy/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.1/Drivers/STM32L0xx_HAL_Driver/Inc -IC:/Users/Dmitriy/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.1/Drivers/CMSIS/Device/ST/STM32L0xx/Include -IC:/Users/Dmitriy/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.1/Drivers/CMSIS/Include -Og -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src1

clean-Core-2f-Src1:
	-$(RM) ./Core/Src1/arhiv.d ./Core/Src1/arhiv.o ./Core/Src1/arhiv.su ./Core/Src1/calculations.d ./Core/Src1/calculations.o ./Core/Src1/calculations.su ./Core/Src1/debug.d ./Core/Src1/debug.o ./Core/Src1/debug.su ./Core/Src1/eeprom.d ./Core/Src1/eeprom.o ./Core/Src1/eeprom.su ./Core/Src1/flash.d ./Core/Src1/flash.o ./Core/Src1/flash.su ./Core/Src1/lmp91000.d ./Core/Src1/lmp91000.o ./Core/Src1/lmp91000.su ./Core/Src1/modbus_lpuart.d ./Core/Src1/modbus_lpuart.o ./Core/Src1/modbus_lpuart.su

.PHONY: clean-Core-2f-Src1

