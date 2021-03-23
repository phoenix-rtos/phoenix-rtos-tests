UNITY_OBJS := $(addprefix $(PREFIX_O)unity/, unity_fixture.o unity.o)
UNITY_HDRS := $(addprefix unity/, unity_config.h unity_fixture.h unity.h \
				  unity_fixture_internals.h unity_internals.h)

CFLAGS := -Iunity ${CFLAGS}

all: $(PREFIX_A)unity.a $(addprefix $(PREFIX_H), $(UNITY_HDRS))

$(PREFIX_A)unity.a: $(UNITY_OBJS)
	$(ARCH)

$(addprefix $(PREFIX_H), $(UNITY_HDRS)): $(PREFIX_H)%.h : %.h
	$(HEADER)

