all: sw_reg_rw 

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

sw_reg_rw: sw_reg_rw.o 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf *.o sw_reg_rw
