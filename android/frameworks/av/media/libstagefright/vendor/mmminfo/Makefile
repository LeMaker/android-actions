ALSPDIR := $(shell ../../config.make)

include $(ALSPDIR)/ALSP.mk
include $(ALSPDIR)/Config

#定义当前的程序名
PROG := $(ENGINE12NAME)
PROGCC := $(patsubst %.so,$(DDIR)/%.a,$(PROG))

#中间输出结果目录
DDIR := $(CURDIR)/release

CFLAGS += -I$(ALSPINCLUDE)
CFLAGS += -O2

SUBDIRS :=

SOURCES := $(wildcard *.c)
ASMSOURCES := $(wildcard *.S)
OBJS := $(patsubst %.c,$(DDIR)/%.o,$(SOURCES))
ASMOBJS := $(patsubst %.S,$(DDIR)/%.o,$(ASMSOURCES))

all: makedir clean $(PROG)

#创建DDIR目录
makedir:
	@if ! [ -d $(DDIR) ]; \
	then echo "mkdir "$(DDIR) && mkdir $(DDIR); \
	fi

#创建目标文件, 并拷贝到lib目录下
$(PROG): $(SUBDIRS) $(OBJS) $(ASMOBJS) 
	$(LD) $(ENGINE12LDOPTION) $(DDIR)/*.o $(ENGINE12LINKS) -o $(DDIR)/$@ -Map $(DDIR)/$@.map
	$(OBJDUMP) -D $(DDIR)/$@ >$(DDIR)/$@.lst
	mv $(DDIR)/$@ $(ALSPLIBMMMDIR)
	
$(OBJS) : $(DDIR)/%.o:%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(ASMOBJS):$(DDIR)/%.o:%.S	
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -D_ASSEMBLER_ $< -o $@
	
$(SUBDIRS):
	$(MAKE) -C $@

clean:
	cd $(DDIR) && $(RM)  *.o *.lst *.a $(PROG) $(PROG).exe $(PROG).bin *.so

.PHONY: all clean mkdir 
.PHONY: $(SUBDIRS)
