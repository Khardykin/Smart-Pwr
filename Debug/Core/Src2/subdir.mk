################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src2/ADS1115.c \
../Core/Src2/debug.c \
../Core/Src2/device.c 

OBJS += \
./Core/Src2/ADS1115.o \
./Core/Src2/debug.o \
./Core/Src2/device.o 

C_DEPS += \
./Core/Src2/ADS1115.d \
./Core/Src2/debug.d \
./Core/Src2/device.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src2/%.o: ../Core/Src2/%.c Core/Src2/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DSTM32L031xx -DUSE_FULL_LL_DRIVER -DHSE_VALUE=8000000 -DHSE_STARTUP_TIMEOUT=100 -DLSE_STARTUP_TIMEOUT=5000 -DLSE_VALUE=32768 -DMSI_VALUE=524288 -DHSI_VALUE=16000000 -DLSI_VALUE=37000 -DVDD_VALUE=3300 -DPREFETCH_ENABLE=0 -DINSTRUCTION_CACHE_ENABLE=1 -DDATA_CACHE_ENABLE=1 -c -I../Core/Inc -I../Drivers/STM32L0xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32L0xx/Include -I../Drivers/CMSIS/Include -I"D:/ProjectSTM/STM32CubeIDE/workspace/Smart-Pwr/Core/Inc1" -I"D:/ProjectSTM/STM32CubeIDE/workspace/Smart-Pwr/Core/Inc2" -Og -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

