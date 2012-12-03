hardware_modules := gralloc hwcomposer audio nfc sensors
include $(call all-named-subdir-makefiles,$(hardware_modules))
