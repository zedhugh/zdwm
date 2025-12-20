MAKEFILE_ABS_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR := $(dir $(MAKEFILE_ABS_PATH))
PWD_DIR := $(CURDIR)/
BUILD_DIR := $(addprefix $(MAKEFILE_DIR),build)
SRC_DIR := $(addprefix $(MAKEFILE_DIR),/)

TARGET_NAME := zdwm
TARGET := $(addprefix $(MAKEFILE_DIR),$(TARGET_NAME))

ifneq ($(PWD_DIR),$(MAKEFILE_DIR))
	TARGET := $(addprefix $(MAKEFILE_DIR),$(TARGET_NAME))
endif


$(TARGET_NAME): build

clean:
	${RM} $(TARGET)
	${RM} -r $(BUILD_DIR)

run:
	-$(shell Xephyr :3 -screen 1920x1080 -dpi `xrdb -get Xft.dpi` &)

run_multiple_screen:
	-$(shell Xephyr :3 -screen 1920x1080 -screen 1920x1080 +xinerama -dpi `xrdb -get Xft.dpi` &)

wm: $(TARGET_NAME)
	DISPLAY=:3 $(TARGET)

debug: $(TARGET_NAME)
	DISPLAY=:3 valgrind --leak-check=full $(TARGET)

prepare:
	@cmake -S $(SRC_DIR) -B $(BUILD_DIR)
	@cp $(BUILD_DIR)/compile_commands.json $(MAKEFILE_DIR)

build: prepare
	${RM} $(TARGET)
	@cmake --build $(BUILD_DIR)
	@cp $(BUILD_DIR)/$(TARGET_NAME) $(TARGET)

.PHONY: clean run wm prepare build install uninstall reinstall
