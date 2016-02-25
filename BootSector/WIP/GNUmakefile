ifeq ($(DESTDIR),)
$(error Variable DESTDIR must be defined !)
endif

INCSRCS= \
	 Include/bootxdefs.nasm \
	 Include/fatstruct.nasm \
	 Include/gptstruct.nasm \
	 Include/mbrstruct.nasm \


BOOTSECTOR_BINS= \
	$(DESTDIR)/boot0af \
	$(DESTDIR)/boot0ss \
	$(DESTDIR)/boot1f32 \
	$(DESTDIR)/boot1f32alt

all: $(BOOTSECTOR_BINS)

$(DESTDIR)/boot0af:	boot0.nasm $(INCSRCS)
	nasm -DACTIVEFIRST=1 -DDEBUG=0 -DVERBOSE=0 -iInclude/ -o $@ boot0.nasm

$(DESTDIR)/boot0ss:	boot0.nasm $(INCSRCS)
	nasm -DACTIVEFIRST=0 -DDEBUG=0 -DVERBOSE=0 -iInclude/ -o $@ boot0.nasm

$(DESTDIR)/boot1f32alt:	boot1f32.nasm $(INCSRCS)
	nasm -DBOOTSEL=1 -DDEBUG=0 -DVERBOSE=0 -DUNUSED=0 -DUSESIDL=1 -iInclude/ -o $@ boot1f32.nasm

$(DESTDIR)/boot1f32:	boot1f32.nasm $(INCSRCS)
	nasm -DBOOTSEL=0 -DDEBUG=0 -DVERBOSE=0 -DUNUSED=0 -DUSESIDL=1 -iInclude/ -o $@ boot1f32.nasm

clean:
	rm -f $(BOOTSECTOR_BINS)
