################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src2/ADS1115.c \
../Core/Src2/FilterMiddle.c \
../Core/Src2/Mipex_command.c \
../Core/Src2/device.c \
../Core/Src2/modbus.c \
../Core/Src2/msi.c 

OBJS += \
./Core/Src2/ADS1115.o \
./Core/Src2/FilterMiddle.o \
./Core/Src2/Mipex_command.o \
./Core/Src2/device.o \
./Core/Src2/modbus.o \
./Core/Src2/msi.o 

C_DEPS += \
./Core/Src2/ADS1115.d \
./Core/Src2/FilterMiddle.d \
./Core/Src2/Mipex_command.d \
./Core/Src2/device.d \
./Core/Src2/modbus.d \
./Core/Src2/msi.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src2/%.o Core/Src2/%.su: ../Core/Src2/%.c Core/Src2/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DSTM32L031xx -DUSE_FULL_LL_DRIVER -DHSE_VALUE=8000000 -DHSE_STARTUP_TIMEOUT=100 -DLSE_STARTUP_TIMEOUT=5000 -DLSE_VALUE=32768 -DMSI_VALUE=2097000 -DHSI_VALUE=16000000 -DLSI_VALUE=37000 -DVDD_VALUE=3300 -DPREFETCH_ENABLE=0 -DINSTRUCTION_CACHE_ENABLE=1 -DDATA_CACHE_ENABLE=1 -c -I../Core/Inc -I"D:/ProjectSTM/STM32CubeIDE/workspace/Smart-Pwr/Core/Inc1" -I"D:/ProjectSTM/STM32CubeIDE/workspace/Smart-Pwr/Core/Inc2" -IC:/Users/Dmitriy/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.1/Drivers/STM32L0xx_HAL_Driver/Inc -IC:/Users/Dmitriy/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.1/Drivers/CMSIS/Device/ST/STM32L0xx/Include -IC:/Users/Dmitriy/STM32Cube/Repository/STM32Cube_FW_L0_V1.12.1/Drivers/CMSIS/Include -Og -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src2

clean-Core-2f-Src2:
	-$(RM) ./Core/Src2/ADS1115.d ./Core/Src2/ADS1115.o ./Core/Src2/ADS1115.su ./Core/Src2/FilterMiddle.d ./Core/Src2/FilterMiddle.o ./Core/Src2/FilterMiddle.su ./Core/Src2/Mipex_command.d ./Core/Src2/Mipex_command.o ./Core/Src2/Mipex_command.su ./Core/Src2/device.d ./Core/Src2/device.o ./Core/Src2/device.su ./Core/Src2/modbus.d ./Core/Src2/modbus.o ./Core/Src2/modbus.su ./Core/Src2/msi.d ./Core/Src2/msi.o ./Core/Src2/msi.su

.PHONY: clean-Core-2f-Src2

