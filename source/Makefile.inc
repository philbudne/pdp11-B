BASE=../..
AS=APOUT_ROOT=$(BASE)/fs/root $(BASE)/tools/apout/apout $(BASE)/fs/root/bin/as

.s.o:
	$(AS) $< && mv a.out $@

clean:
	rm -f $(ALL) *~
