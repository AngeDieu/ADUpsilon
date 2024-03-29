# You can override those settings on the command line

PLATFORM ?= simulator
DEBUG ?= 0

HOME_DISPLAY_EXTERNALS ?= 1
EPSILON_VERSION ?= 15.5.0
OMEGA_VERSION ?= 2.0.2
UPSILON_VERSION ?= 0.1.0-ad
OMEGA_USERNAME ?= Benoit
OMEGA_STATE ?= stable
EPSILON_APPS ?= calculation solver code graph sequence regression statistics probability rpn atomic reader settings external
SUBMODULES_APPS = #no need for submodules as atomic and rpn are locals
EPSILON_I18N ?= fr en nl pt it de es hu
EPSILON_COUNTRIES ?= WW CA DE ES FR GB IT NL PT US
EPSILON_GETOPT ?= 0
ESCHER_LOG_EVENTS_BINARY ?= 0
THEME_NAME ?= jmj
THEME_REPO ?= local
INCLUDE_ULAB ?= 1
