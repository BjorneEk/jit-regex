
SRC_DIR:=src

UNAME:=$(shell uname -s)

P_TARGET:=bin/$(UNAME)/release/target
DEBUG_MAIN:=dmain.c
GEN_MAKEFILE:=build/$(UNAME)/generated.Makefile


all: $(GEN_MAKEFILE) $(P_TARGET) Makefile

build/$(UNAME)/:
	@mkdir -p $@
	@echo  $@

$(GEN_MAKEFILE): build/$(UNAME)/
	@bash build.sh "$(DEBUG_MAIN)" "$(SRC_DIR)" "$(GEN_MAKEFILE)" "buildconfig"
	@echo " $@"

include $(GEN_MAKEFILE)

clean:
	$(RM) -rf build
	$(RM) -rf bin



