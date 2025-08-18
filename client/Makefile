CXX = g++
CXXFLAGS = -g -Wall -g -I. -I../include -MMD -MP
LDFLAGS = -lncurses -ltinfo

TARGET = client

OBJDIR = build

SOURCES = $(wildcard *.cpp)
OBJECTS = $(addprefix $(OBJDIR)/, $(notdir $(patsubst %.cpp,%.o,$(SOURCES))))

DEPS = $(addprefix $(OBJDIR)/, $(notdir $(patsubst %.cpp,%.d,$(SOURCES))))

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	@echo " >> Client compiled successfully!"

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<


.PHONY: clean
clean:
	rm -f $(TARGET)
	rm -rf $(OBJDIR)

-include $(DEPS)