TARGET=ifnl
SRCS=main.c

all: $(TARGET)

$(TARGET): $(SRCS:.c=.o)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -c -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(TARGET) $(SRCS:.c=.o)


