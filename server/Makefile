CXX = g++
CXXFLAGS = -g -Wall -I. -MMD -MP
LDFLAGS = -lssl -lcrypto

TARGET = server

OBJDIR = build

SOURCES = $(wildcard *.cpp)
OBJECTS = $(addprefix $(OBJDIR)/, $(notdir $(patsubst %.cpp,%.o,$(SOURCES))))

DEPS = $(addprefix $(OBJDIR)/, $(notdir $(patsubst %.cpp,%.d,$(SOURCES))))

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<


.PHONY: clean
clean:
	rm -f $(TARGET)
	rm -rf $(OBJDIR)

-include $(DEPS)