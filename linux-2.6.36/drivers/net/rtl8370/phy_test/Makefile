all: phy_reg_rw 

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

phy_reg_rw: phy_reg_rw.o 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf *.o phy_reg_rw
