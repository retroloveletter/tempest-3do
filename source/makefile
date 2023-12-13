3DODEV	= C:/3DODev/
# ARMDEV	= C:/ARM251/

# Project specific settings
NAME	= LaunchMe
STACKSIZE = 4096

CC		= armcc
AS 		= armasm
LD		= armlink
RM		= rm
MODBIN	= modbin
CCFLAGS = -O2 -Otime -bi -za1 -d DEBUG=0 -cpu ARM60
INCPATH	= -I$(3DODEV)includes -Iincludes -Igame/includes
ASFLAGS = -bi
LDFLAGS = -reloc -nodebug -remove -ro-base 0x80
LIBPATH	= $(3DODEV)libs/
STARTUP = $(LIBPATH)cstartup.o

LIBS 	=	$(LIBPATH)Lib3DO.lib \
			$(LIBPATH)operamath.lib \
			$(LIBPATH)graphics.lib \
			$(LIBPATH)audio.lib \
			$(LIBPATH)music.lib \
			$(LIBPATH)filesystem.lib \
			$(LIBPATH)input.lib \
			$(LIBPATH)swi.lib \
			$(LIBPATH)clib.lib

SRC_C		= $(wildcard *.c) $(wildcard */*.c)
SRC_S		= $(wildcard *.s) $(wildcard */*.s)

OBJ	+= $(SRC_S:.s=.o)
OBJ	+= $(SRC_C:.c=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(LD) -dupok -o $(NAME). $(LDFLAGS) $(STARTUP) $(LIBS) $(OBJ)
	$(MODBIN) $(STACKSIZE) $(NAME)

%.o: %.c
	$(CC) $(INCPATH) $(CCFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(INCPATH) $(ASFLAGS) $< -o $@

clean:
	RM -f $(OBJ)
	RM -f $(NAME)
	RM -f ../CD/$(NAME)
	RM -f ../tempest3do.iso
