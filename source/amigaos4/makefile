SDL_BASE =
LDFLAGS = 

CXX = g++
STRIP = strip

CXXFLAGS = -O2 -Wall -fsigned-char -I/SDK/local/newlib/include/SDL2 -I../
LIBS = -use-dynld -lSDL2 -lmikmod -lpthread

# Remove the following two lines to disable sound
METHANE_SND_FLAGS = -DMETHANE_MIKMOD
METHANE_SND_LIBS =
all: message methane

# Methane brothers with sound
message:
	@echo "Compiling Super Methane Brothers."
	@echo "================================="

OBJS=\
MainSource/gfxoff.o MainSource/baddie.o MainSource/bitdraw.o MainSource/bitgroup.o MainSource/bititem.o \
	MainSource/boss.o MainSource/game.o MainSource/gasobj.o MainSource/global.o MainSource/goodie.o MainSource/maps.o \
	MainSource/misc.o MainSource/objlist.o MainSource/player.o MainSource/power.o MainSource/target.o \
	MainSource/suck.o MainSource/weapon.o MainSource/sound.o MainData/snddata.o MainData/gfxdata.o MainData/gfxdata2.o \
	MainData/gfxdata3.o MainData/mapdata.o MainMikMod/audiodrv.o MainAmiga/doc.o

# Basic methane brothers compilation
methane:  $(OBJS)
	${CXX} ${CXXFLAGS} ${OBJS} -o methane ${METHANE_LIBS} ${METHANE_SND_LIBS} ${LIBS} ${LDFLAGS}
	$(STRIP) methane

clean:
	@rm -Rf MainSource
	@rm -Rf MainData
	@rm -Rf MainAmiga
	@rm -Rf MainMikMod
	@rm -f methane

distclean: clean


# The main source code
MainSource/%.o : ../%.cpp
	@echo "  Compiling $<..."
	@if [ ! -d MainSource ]; then mkdir MainSource; fi
	${CXX} ${CXXFLAGS} ${METHANE_FLAGS} ${METHANE_SND_FLAGS} -I ../linux -c $< -o $@

# The main data
MainData/%.o : ../data/%.cpp
	@echo "  Compiling $<..."
	@if [ ! -d MainData ]; then mkdir MainData; fi
	${CXX} ${CXXFLAGS} ${METHANE_FLAGS} ${METHANE_SND_FLAGS} -I ../ -c $< -o $@

# The AmigaOS source code
MainAmiga/%.o : ../amigaos4/%.cpp
	@echo "  Compiling $<..."
	@if [ ! -d MainAmiga ]; then mkdir MainAmiga; fi
	${CXX} ${CXXFLAGS} ${METHANE_FLAGS} ${METHANE_SND_FLAGS} -I ../ -c $< -o $@

# The sound driver source code
MainMikMod/%.o : ../mikmod/%.cpp
	@echo "  Compiling $<..."
	@if [ ! -d MainMikMod ]; then mkdir MainMikMod; fi
	${CXX} ${CXXFLAGS} ${METHANE_FLAGS} ${METHANE_SND_FLAGS} -I ../ -c $< -o $@
