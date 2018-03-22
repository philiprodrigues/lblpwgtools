CXX = g++ -std=c++17

CPPFLAGS = $(SRT_MAKEFLAGS) $(shell root-config --cflags) -I $(IFDHC_FQ_DIR)/inc -I $(BOOST_INC) -I $(CLHEP_INC)

CFLAGS = $(CPPFLAGS) -fPIC

LDFLAGS = -g -L $(SRT_PRIVATE_CONTEXT)/lib/$(SRT_SUBDIR)/ -L $(SRT_PUBLIC_CONTEXT)/lib/$(SRT_SUBDIR)/ -L $(IFDHC_LIB)

LDLIBS = -lifdh # -lOscLibFunc -lStandardRecord

TMPDIR = $(SRT_PRIVATE_CONTEXT)/tmp/$(SRT_SUBDIR)/CAFAna/

VPATH = $(TMPDIR)

SRCS = $(wildcard CAFAna/**/*.cxx OscLib/func/*.cxx Utilities/func/*.cxx) # searches all subdirs

OBJS = $(addprefix $(TMPDIR)/,$(SRCS:.cxx=.o))

FINAL = $(SRT_PRIVATE_CONTEXT)/lib/$(SRT_SUBDIR)/libCAFAna.so

DEPENDS = $(addprefix $(TMPDIR)/,$(SRCS:.cxx=.d))

.PHONY: all

all: $(FINAL)

$(FINAL): $(OBJS)
	@echo '[ link $(notdir $(OBJS)) -> $(notdir $(FINAL)) ]'
	@$(CXX) -o $(FINAL) -shared $(LDFLAGS) $(LDLIBS) $(OBJS)

.PHONY: clean

clean:
	@echo '[ clean ]'
	@rm -f $(OBJS) $(FINAL) $(DEPENDS)

# Object files (in the tmpdir) depend on the correspond dependencies
%.o: %.d

# Object files in the tmpdir depend on the matching cxx files here
$(TMPDIR)/%.o: %.cxx
	@echo '[ compile $< -> $(notdir $@) ]'
	@mkdir -p $(dir $@)
	@$(CXX) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Based on the make manual
# https://www.gnu.org/software/make/manual/html_node/Automatic-Prerequisites.html
$(TMPDIR)/%.d: %.cxx

	@echo '[ depend $< -> $(notdir $@) ]'

	@mkdir -p $(dir $@)
	@rm -f $@
	@$(CXX) -MM $(CPPFLAGS) $< > $@
	@sed -i 's/\(.*\).o:/\1.o \1.d:/' $@

include $(DEPENDS)

