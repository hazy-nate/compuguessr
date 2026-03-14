
MODE		?= debug
V			?= 0

ifeq ($(filter $(MODE), debug release),)
    $(error MODE must be 'debug' or 'release'. Mode provided: $(MODE))
endif

ifeq ($(V), 1)
	Q :=
else
	Q := @
endif
