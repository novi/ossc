# List of user extra sources
USEREXTRASRC = ./os/hal/extra/hal_i2c_extra.c

# Required include directories
USEREXTRAINC = ./os/hal/extra

# Shared variables
ALLCSRC += $(USEREXTRASRC)
ALLINC  += $(USEREXTRAINC)