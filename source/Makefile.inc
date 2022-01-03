BASE=../..
APOUT=APOUT_ROOT=$(BASE)/fs/root $(BASE)/tools/apout/apout
BIN=$(BASE)/fs/root/bin

AS=$(APOUT) $(BIN)/as
AR=$(APOUT) $(BIN)/ar

.s.o:
	$(AS) $< && mv a.out $@

clean:
	rm -f $(ALL) *~
