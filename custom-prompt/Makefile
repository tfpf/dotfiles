CPPFLAGS = $(shell pkg-config --cflags libgit2)
CXXFLAGS = -fstrict-aliasing -std=c++17 -Wall -Wextra -Wno-unused-parameter
LDLIBS = -lstdc++ $(shell pkg-config --libs libgit2)

MainSource = custom-prompt.cc
MainBashObject = custom-bash-prompt.o
MainBashExecutable = bin/$(MainBashObject:.o=)
MainZshObject = custom-zsh-prompt.o
MainZshExecutable = bin/$(MainZshObject:.o=)
OtherObjects = focus_utils.o json_logger.o

UNAME = $(shell uname)
ifeq "$(UNAME)" "Linux"
    CPPFLAGS += $(shell pkg-config --cflags libnotify)
    LDLIBS += $(shell pkg-config --libs libnotify)
endif

.PHONY: debug release

debug: $(MainBashExecutable) $(MainZshExecutable)

release: CPPFLAGS += -DNDEBUG
release: CXXFLAGS += -flto -O2
release: LDFLAGS += -flto -O2
release: debug

$(MainBashObject): CPPFLAGS += -DBASH
$(MainBashObject): $(MainSource)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

$(MainBashExecutable): $(OtherObjects) $(MainBashObject)
	$(LINK.o) $^ $(LDLIBS) $(OUTPUT_OPTION)

$(MainZshObject): CPPFLAGS += -DZSH
$(MainZshObject): $(MainSource)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

$(MainZshExecutable): $(OtherObjects) $(MainZshObject)
	$(LINK.o) $^ $(LDLIBS) $(OUTPUT_OPTION)
