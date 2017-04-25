TARGET := priv2dump

CXXFLAGS += -O2 -std=c++14 -Wall
CXXFLAGS += -fno-rtti -fno-exceptions

LDLIBS += -lz -lsndfile -lpng

CXXFLAGS += -I/usr/local/include
LDLIBS += -L/usr/local/lib

ifeq ($(V),)
SILENTMSG := @echo
SILENTCMD := @
else
SILENTMSG := @true
SILENTCMD :=
endif

OBJ := $(patsubst src/%.cpp,obj/%.o,$(wildcard src/*.cpp))

$(TARGET): $(OBJ)
	$(SILENTMSG) "LINK  $@"
	$(SILENTCMD)$(CXX) -o $@ $^ $(LDLIBS)

obj/%.o: src/%.cpp
	$(SILENTMSG) "CXX   $@"
	$(SILENTCMD)mkdir -p $(dir $@)
	$(SILENTCMD)$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	$(SILENTMSG) "CLEAN"
	$(SILENTCMD)rm -f $(TARGET) $(OBJ)
	$(SILENTCMD)rm -rf obj
