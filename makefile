########################################################################
# Makefile for the SketchPad vector drawing package.
# Copyright (c) 1999-2025 Oliver Kreylos
#
# This file is part of the WhyTools Build Environment.
# 
# The WhyTools Build Environment is free software; you can redistribute
# it and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
# 
# The WhyTools Build Environment is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with the WhyTools Build Environment; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
########################################################################

# Set directory containing Vrui's build system:
VRUI_MAKEDIR := /usr/local/share/Vrui-13.0/make

# Base installation directory for SketchPad. If this is set to the
# default of $(PWD), SketchPad does not have to be installed to be
# run.
# Important note: Do not use ~ as an abbreviation for the user's home
# directory here; use $(HOME) instead.
INSTALLDIR := $(shell pwd)

########################################################################
# Everything below here should not have to be changed
########################################################################

# Name of the package
PROJECT_NAME = SketchPad

# Version number for installation subdirectories. This is used to keep
# subsequent release versions of SketchPad from clobbering each
# other. The value should be identical to the major.minor version
# number found in VERSION in the root package directory.
PROJECT_MAJOR = 1
PROJECT_MINOR = 0

# Include definitions for the system environment and system-provided
# packages
include $(VRUI_MAKEDIR)/SystemDefinitions
include $(VRUI_MAKEDIR)/Packages.System
include $(VRUI_MAKEDIR)/Configuration.Vrui
include $(VRUI_MAKEDIR)/Packages.Vrui

# Set up installation directories: */
SHADERINSTALLDIR = $(SHAREINSTALLDIR)/Shaders

########################################################################
# Specify additional compiler and linker flags
########################################################################

CFLAGS += -Wall -pedantic

########################################################################
# List common packages used by all components of this project
# (Supported packages can be found in $(VRUI_MAKEDIR)/Packages.*)
########################################################################

PACKAGES = MYVRUI MYGLMOTIF MYIMAGES MYGLGEOMETRY MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH MYIO MYMISC GL

########################################################################
# Specify all final targets
# Use $(EXEDIR)/ before executable names
########################################################################

CONFIGS = 
EXECUTABLES = 

CONFIGS += Config.h

EXECUTABLES += $(EXEDIR)/SketchPad
ALL = $(EXECUTABLES)

.PHONY: all
all: $(CONFIGS) $(ALL)

########################################################################
# Specify other actions to be performed on a `make clean'
########################################################################

.PHONY: extraclean
extraclean:

.PHONY: extrasqueakyclean
extrasqueakyclean:

# Include basic makefile
include $(VRUI_MAKEDIR)/BasicMakefile

########################################################################
# Pseudo-target to print configuration options and configure the package
########################################################################

.PHONY: config config-invalidate
config: config-invalidate $(DEPDIR)/config

config-invalidate:
	@mkdir -p $(DEPDIR)
	@touch $(DEPDIR)/Configure-Begin

$(DEPDIR)/Configure-Begin:
	@mkdir -p $(DEPDIR)
	@echo "---- $(PROJECT_FULLDISPLAYNAME) configuration options: ----"
	@touch $(DEPDIR)/Configure-Begin

$(DEPDIR)/Configure-Project: $(DEPDIR)/Configure-Begin
	@cp Config.h.template Config.h.temp
	@$(call CONFIG_SETSTRINGVAR,Config.h.temp,SKETCHPAD_SHADERDIR,$(SHADERINSTALLDIR))
	@if ! diff -qN Config.h.temp Config.h > /dev/null ; then cp Config.h.temp Config.h ; fi
	@rm Config.h.temp
	@touch $(DEPDIR)/Configure-Project

$(DEPDIR)/Configure-Install: $(DEPDIR)/Configure-Project
	@echo "---- $(PROJECT_FULLDISPLAYNAME) installation configuration ----"
	@echo "Installation directory : $(INSTALLDIR)"
	@echo "Executable directory   : $(EXECUTABLEINSTALLDIR)"
	@echo "Resource directory     : $(SHAREINSTALLDIR)"
	@echo "Shader directory       : $(SHADERINSTALLDIR)"
	@touch $(DEPDIR)/Configure-Install

$(DEPDIR)/Configure-End: $(DEPDIR)/Configure-Install
	@echo "---- End of $(PROJECT_FULLDISPLAYNAME) configuration options ----"
	@touch $(DEPDIR)/Configure-End

$(DEPDIR)/config: $(DEPDIR)/Configure-End
	@touch $(DEPDIR)/config

########################################################################
# Specify build rules for executables
########################################################################

SKETCHPAD_SOURCES = SketchObject.cpp \
                    SketchObjectList.cpp \
                    SketchObjectContainer.cpp \
                    SketchSettings.cpp \
                    Curve.cpp \
                    Group.cpp \
                    Image.cpp \
                    SketchObjectCreator.cpp \
                    PaintBucket.cpp \
                    SketchPad.cpp

$(SKETCHPAD_SOURCES:%.cpp=$(OBJDIR)/%.o): | $(DEPDIR)/config

$(EXEDIR)/SketchPad: $(SKETCHPAD_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: SketchPad
SketchPad: $(EXEDIR)/SketchPad
