MAKEFILE_ABS_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR := $(dir $(MAKEFILE_ABS_PATH))
BUILD_DIR := $(addprefix $(MAKEFILE_DIR),build)
SRC_DIR := $(addprefix $(MAKEFILE_DIR),/)

TARGET_NAME := zdwm
TARGET := $(addprefix $(MAKEFILE_DIR),$(TARGET_NAME))


$(TARGET_NAME): build

clean:
	${RM} $(TARGET)
	${RM} -r $(BUILD_DIR)

run:
	Xephyr :3 -screen 1920x1080 -dpi `xrdb -get Xft.dpi`

run_multiple_screen:
	Xephyr :3 -screen 1920x1080 -screen 1920x1080 +xinerama -dpi `xrdb -get Xft.dpi`

wm: $(TARGET_NAME)
	DISPLAY=:3 $(TARGET)

debug: $(TARGET_NAME)
	DISPLAY=:3 valgrind --leak-check=full $(TARGET)

prepare:
	@cmake -S $(SRC_DIR) -B $(BUILD_DIR)
	@cp -lf $(BUILD_DIR)/compile_commands.json $(MAKEFILE_DIR)

build: prepare
	${RM} $(TARGET)
	@cmake --build $(BUILD_DIR)
	@cp -lf $(BUILD_DIR)/$(TARGET_NAME) $(TARGET)

install-hooks:
	chmod +x $(MAKEFILE_DIR).githooks/pre-commit
	git -C $(MAKEFILE_DIR) config core.hooksPath $(MAKEFILE_DIR).githooks

.PHONY: clean run wm prepare build install uninstall reinstall install-hooks
