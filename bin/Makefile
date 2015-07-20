ALLDIRS = test test_server

all:
	for i in $(ALLDIRS); do make -C $$i; done
	
clean:
	for i in $(ALLDIRS); do make clean -C $$i; done

.PHONY: clean