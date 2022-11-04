B2C = bin2c
CD = cd
SED = sed


.PHONY: all build_exports pack_usermodule pack_image clean user user_clean kernel kernel_clean

all: clean build_exports pack_image user  pack_usermodule kernel

clean: user_clean kernel_clean

build_exports:
	$(CD) ./user
	psp-build-exports -s ../kernel/exports.exp

pack_image:
	$(B2C) genesis.png ./user/genesis.h genesis

pack_usermodule:
	$(B2C) ./user/localizeruser.prx ./kernel/localize.h localize
	$(SED) -i -u "s#((aligned(16)))#((aligned(64)))#g" ./kernel/localize.h
	$(SED) -i -u "s#static##g" ./kernel/localize.h
	$(RM) -f ./kernel/sed*

user:
	make -C user -f Makefile all
kernel:
	make -C kernel -f Makefile all
	
user_clean:
	make -C user -f Makefile clean
	rm -f ./user/genesis.h
	
kernel_clean:
	make -C kernel -f Makefile clean